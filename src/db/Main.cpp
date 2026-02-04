#include "stdafx.h"
#include "Config.h"
#include "Peer.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "GuildManager.h"
#include "ItemAwardManager.h"
#include "PrivManager.h"
#include "MoneyLog.h"
#include "Marriage.h"
#include "Monarch.h"
#include "ItemIDRangeManager.h"
#include <signal.h>
#undef OS_FREEBSD
void SetPlayerDBName(const char* c_pszPlayerDBName);
void SetTablePostfix(const char* c_pszTablePostfix);
int Start();

std::string g_stTablePostfix;
std::string g_stLocale = "euckr";
std::string g_stPlayerDBName = "";

BOOL g_test_server = false;

//단위 초
int g_iPlayerCacheFlushSeconds = 60*7;
int g_iItemCacheFlushSeconds = 60*5;

//g_iLogoutSeconds 수치는 g_iPlayerCacheFlushSeconds 와 g_iItemCacheFlushSeconds 보다 길어야 한다.
int g_iLogoutSeconds = 60*10;

int g_log = 1;


// MYSHOP_PRICE_LIST
int g_iItemPriceListTableCacheFlushSeconds = 540;
// END_OF_MYSHOP_PRICE_LIST

#ifdef OS_FREEBSD
extern const char * _malloc_options;
#endif

extern void WriteVersion();

void emergency_sig(int sig)
{
	if (sig == SIGSEGV)
		sys_log(0, "SIGNAL: SIGSEGV");
	else if (sig == SIGUSR1)
		sys_log(0, "SIGNAL: SIGUSR1");

	if (sig == SIGSEGV)
		abort();
}

int main()
{
	log_init();

	WriteVersion();

#ifdef OS_FREEBSD
	_malloc_options = "A";
#endif

	CConfig Config;
	CNetPoller poller;
	CDBManager DBManager; 
	CClientManager ClientManager;
	CGuildManager GuildManager;
	CPrivManager PrivManager;
	CMoneyLog MoneyLog;
	ItemAwardManager ItemAwardManager;
	marriage::CManager MarriageManager;
	CMonarch Monarch;
	CItemIDRangeManager ItemIDRangeManager;

	if (!Start())
		return 1;

	GuildManager.Initialize();
	MarriageManager.Initialize();
	ItemIDRangeManager.Build();
	
	sys_log(0, "Metin2DBCacheServer Start\n");

	CClientManager::instance().MainLoop();

	signal_timer_disable();

	DBManager.Quit();
	int iCount;

	while (1)
	{
		iCount = 0;

		iCount += CDBManager::instance().CountReturnQuery(SQL_PLAYER);
		iCount += CDBManager::instance().CountAsyncQuery(SQL_PLAYER);

		if (iCount == 0)
			break;

		usleep(1000);
		sys_log(0, "WAITING_QUERY_COUNT %d", iCount);
	}

	log_destroy();
	return 1;
}

void emptybeat(LPHEART heart, int pulse)
{
	if (!(pulse % heart->passes_per_sec))	// 1초에 한번
	{
	}
}

//
// @version	05/06/13 Bang2ni - 아이템 가격정보 캐시 flush timeout 설정 추가.
//
int Start()
{
	if (!CConfig::instance().LoadFile("conf/db.txt"))
	{
		fprintf(stderr, "Loading conf/db.txt failed.\n");
		return false;
	}

	if (!CConfig::instance().GetValue("TEST_SERVER", &g_test_server))
	{
		fprintf(stderr, "Real Server\n");
	}
	else
		fprintf(stderr, "Test Server\n");

	if (!CConfig::instance().GetValue("LOG", &g_log))
	{
		fprintf(stderr, "Log Off");
		g_log= 0;
	}
	else
	{
		g_log = 1;
		fprintf(stderr, "Log On");
	}
	
	
	int tmpValue;

	int heart_beat = 50;
	if (!CConfig::instance().GetValue("CLIENT_HEART_FPS", &heart_beat))
	{
		fprintf(stderr, "Cannot find CLIENT_HEART_FPS configuration.\n");
		return false;
	}

	thecore_init(heart_beat, emptybeat);
	signal_timer_enable(60);

	char szBuf[256+1];

	if (CConfig::instance().GetValue("LOCALE", szBuf, 256))
	{
		g_stLocale = szBuf;
		sys_log(0, "LOCALE set to %s", g_stLocale.c_str());
	}

	if (!CConfig::instance().GetValue("TABLE_POSTFIX", szBuf, 256))
	{
		sys_err("TABLE_POSTFIX not configured use default");
		szBuf[0] = '\0';
	}

	SetTablePostfix(szBuf);

	if (CConfig::instance().GetValue("PLAYER_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iPlayerCacheFlushSeconds, szBuf);
		sys_log(0, "PLAYER_CACHE_FLUSH_SECONDS: %d", g_iPlayerCacheFlushSeconds);
	}

	if (CConfig::instance().GetValue("ITEM_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iItemCacheFlushSeconds, szBuf);
		sys_log(0, "ITEM_CACHE_FLUSH_SECONDS: %d", g_iItemCacheFlushSeconds);
	}

	// MYSHOP_PRICE_LIST
	if (CConfig::instance().GetValue("ITEM_PRICELIST_CACHE_FLUSH_SECONDS", szBuf, 256)) 
	{
		str_to_number(g_iItemPriceListTableCacheFlushSeconds, szBuf);
		sys_log(0, "ITEM_PRICELIST_CACHE_FLUSH_SECONDS: %d", g_iItemPriceListTableCacheFlushSeconds);
	}
	// END_OF_MYSHOP_PRICE_LIST
	//
	if (CConfig::instance().GetValue("CACHE_FLUSH_LIMIT_PER_SECOND", szBuf, 256))
	{
		DWORD dwVal = 0; str_to_number(dwVal, szBuf);
		CClientManager::instance().SetCacheFlushCountLimit(dwVal);
	}

	int iIDStart;
	if (!CConfig::instance().GetValue("PLAYER_ID_START", &iIDStart))
	{
		sys_err("PLAYER_ID_START not configured");
		return false;
	}

	CClientManager::instance().SetPlayerIDStart(iIDStart);

	char szAddr[64], szDB[64], szUser[64], szPassword[64];
	int iPort;
	char line[256+1];

	if (CConfig::instance().GetValue("SQL_PLAYER", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (player)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_PLAYER, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		fprintf(stderr, "Success PLAYER\n");
		SetPlayerDBName(szDB);
	}
	else
	{
		sys_err("SQL_PLAYER not configured");
		return false;
	}

	if (CConfig::instance().GetValue("SQL_ACCOUNT", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (account)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_ACCOUNT, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		fprintf(stderr, "Success ACCOUNT\n");
	}
	else
	{
		sys_err("SQL_ACCOUNT not configured");
		return false;
	}

	if (CConfig::instance().GetValue("SQL_COMMON", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (common)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_COMMON, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		fprintf(stderr, "Success COMMON\n");
	}
	else
	{
		sys_err("SQL_COMMON not configured");
		return false;
	}
	
	if (!CNetPoller::instance().Create())
	{
		sys_err("Cannot create network poller");
		return false;
	}

	sys_log(0, "ClientManager initialization.. ");

	if (!CClientManager::instance().Initialize())
	{
		sys_log(0, "   failed"); 
		return false;
	}

	sys_log(0, "   OK");

#ifndef OS_WINDOWS
	signal(SIGUSR1, emergency_sig);
#endif
	signal(SIGSEGV, emergency_sig);
	return true;
}

void SetTablePostfix(const char* c_pszTablePostfix)
{
	if (!c_pszTablePostfix || !*c_pszTablePostfix)
		g_stTablePostfix = "";
	else
		g_stTablePostfix = c_pszTablePostfix;
}

const char * GetTablePostfix()
{
	return g_stTablePostfix.c_str();
}

void SetPlayerDBName(const char* c_pszPlayerDBName)
{
	if (! c_pszPlayerDBName || ! *c_pszPlayerDBName)
		g_stPlayerDBName = "";
	else
	{
		g_stPlayerDBName = c_pszPlayerDBName;
		g_stPlayerDBName += ".";
	}
}

const char * GetPlayerDBName()
{
	return g_stPlayerDBName.c_str();
}

