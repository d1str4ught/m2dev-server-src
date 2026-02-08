#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "locale_service.h"
#include "db.h"

#include <unordered_map>

extern time_t get_global_time();

static const int LOGIN_MAX_ATTEMPTS = 5;
static const DWORD LOGIN_BLOCK_DURATION_MS = 60000;

struct LoginAttemptInfo
{
	int failCount;
	DWORD lastAttemptTime;
};

static std::unordered_map<std::string, LoginAttemptInfo> s_loginAttempts;

void RecordLoginFailure(const char* hostName)
{
	DWORD now = get_dword_time();
	auto it = s_loginAttempts.find(hostName);

	if (it == s_loginAttempts.end())
	{
		s_loginAttempts[hostName] = { 1, now };
	}
	else
	{
		if (now - it->second.lastAttemptTime >= LOGIN_BLOCK_DURATION_MS)
			it->second.failCount = 0;

		it->second.failCount++;
		it->second.lastAttemptTime = now;
	}
}

void ClearLoginFailure(const char* hostName)
{
	s_loginAttempts.erase(hostName);
}

static bool IsLoginRateLimited(const char* hostName)
{
	auto it = s_loginAttempts.find(hostName);
	if (it == s_loginAttempts.end())
		return false;

	DWORD now = get_dword_time();
	if (now - it->second.lastAttemptTime >= LOGIN_BLOCK_DURATION_MS)
	{
		s_loginAttempts.erase(it);
		return false;
	}

	return it->second.failCount >= LOGIN_MAX_ATTEMPTS;
}

bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		// 알파벳과 수자만 허용
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		// 캐나다는 몇몇 특수문자 허용
		if (LC_IsCanada())
		{
			switch (*tmp)
			{
				case ' ':
				case '_':
				case '-':
				case '.':
				case '!':
				case '@':
				case '#':
				case '$':
				case '%':
				case '^':
				case '&':
				case '*':
				case '(':
				case ')':
					continue;
			}
		}

		if (LC_IsYMIR() == true || LC_IsKorea() == true)
		{
			switch (*tmp)
			{
				case '-' :
				case '_' :
					continue;
			}
		}

		if (LC_IsBrazil() == true)
		{
			switch (*tmp)
			{
				case '_' :
				case '-' :
				case '=' :
					continue;
			}
		}

		if (LC_IsJapan() == true)
		{
			switch (*tmp)
			{
				case '-' :
				case '_' :
				case '@':
				case '#':
					continue;
			}
		}

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
	RegisterHandlers();
}

int CInputAuth::HandlePong(LPDESC d, const char*)
{
	Pong(d);
	return 0;
}

int CInputAuth::HandleLogin3(LPDESC d, const char* c_pData)
{
	Login(d, c_pData);
	return 0;
}

void CInputAuth::RegisterHandlers()
{
	m_handlers[CG::PONG]   = &CInputAuth::HandlePong;
	m_handlers[CG::LOGIN3] = &CInputAuth::HandleLogin3;
}

void CInputAuth::Login(LPDESC d, const char * c_pData)
{
	TPacketCGLogin3 * pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	// Rate limiting: block IPs with too many failed attempts
	if (IsLoginRateLimited(d->GetHostName()))
	{
		sys_log(0, "InputAuth::Login : RATE LIMITED IP %s", d->GetHostName());
		LoginFailure(d, "BLOCK");
		d->DelayedDisconnect(3);
		return;
	}

	// string 무결성을 위해 복사
	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = GC::LOGIN_FAILURE;
		failurePacket.length = sizeof(failurePacket);
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);

	sys_log(0, "InputAuth::Login : key %u login %s", dwKey, login);

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",

				szPasswd, szLogin);
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p, 
				"SELECT PASSWORD('%s'),password,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd, szLogin);
	}
}

int CInputAuth::Analyze(LPDESC d, uint16_t wHeader, const char * c_pData)
{
	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.",
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	auto it = m_handlers.find(wHeader);
	if (it == m_handlers.end())
	{
		sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", wHeader, wHeader);
		return 0;
	}

	return (this->*(it->second))(d, c_pData);
}
