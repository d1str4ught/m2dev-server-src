#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "fishing.h"
#include "priv_manager.h"
#include "castle.h"

extern time_t get_global_time();

bool IsEmptyAdminPage()
{
	return g_stAdminPageIP.empty();
}

bool IsAdminPage(const char * ip)
{
	for (size_t n = 0; n < g_stAdminPageIP.size(); ++n)
	{
		if (g_stAdminPageIP[n] == ip)
			return 1; 
	}	
	return 0;
}

CInputProcessor::CInputProcessor() : m_pPacketInfo(NULL), m_iBufferLeft(0)
{
	if (!m_pPacketInfo)
		BindPacketInfo(&m_packetInfoCG);
}

void CInputProcessor::BindPacketInfo(CPacketInfo * pPacketInfo)
{
	m_pPacketInfo = pPacketInfo;
}

bool CInputProcessor::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	uint16_t	wLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		uint16_t wHeader;
		const char * c_pszName;
		int iRegisteredSize;

		// 2-byte header + 2-byte wire length
		if (m_iBufferLeft < (int)sizeof(uint16_t))
			return true;

		wHeader = *(uint16_t*)(c_pData);

		if (wHeader == 0)
		{
			// Skip zero-padding (2 bytes)
			c_pData += sizeof(uint16_t);
			m_iBufferLeft -= sizeof(uint16_t);
			r_iBytesProceed += sizeof(uint16_t);
			continue;
		}

		// Need at least 4 bytes (header + length) to read the framing
		if (m_iBufferLeft < (int)PACKET_HEADER_SIZE)
			return true;

		// Read wire length from packet frame
		iPacketLen = *(uint16_t*)(c_pData + 2);

		if (iPacketLen < (int)PACKET_HEADER_SIZE || iPacketLen > MAX_INPUT_LEN)
		{
			LPCHARACTER ch = lpDesc->GetCharacter();
			sys_err("INVALID PACKET LENGTH: header 0x%04X length %d (min %d, max %d), recv_seq %u, CHAR: %s, fd: %d host: %s",
				wHeader, iPacketLen, (int)PACKET_HEADER_SIZE, MAX_INPUT_LEN,
				lpDesc->GetRecvPacketSeq(),
				ch ? ch->GetName() : "<none>",
				lpDesc->GetSocket(), lpDesc->GetHostName()
			);
			lpDesc->DumpRecentPackets();
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		// Validate the header is registered
		if (!m_pPacketInfo->Get(wHeader, &iRegisteredSize, &c_pszName))
		{
			LPCHARACTER ch = lpDesc->GetCharacter();

			sys_err("UNKNOWN HEADER: 0x%04X (recv_seq #%u), LAST: 0x%04X[%u], REMAIN: %d, CHAR: %s, PHASE: %d, fd: %d host: %s",
				wHeader, lpDesc->GetRecvPacketSeq(),
				wLastHeader, iLastPacketLen, m_iBufferLeft,
				ch ? ch->GetName() : "<none>",
				lpDesc->GetPhase(), lpDesc->GetSocket(), lpDesc->GetHostName()
			);

			lpDesc->DumpRecentPackets();

#if defined(_DEBUG) && defined(_WIN32)
			printdata((uint8_t*)c_pvOrig, m_iBufferLeft);
#endif
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

		if (m_iBufferLeft < iPacketLen)
		{
			return true;
		}

		// Log this packet in the sequence tracker
		lpDesc->LogRecvPacket(wHeader, static_cast<uint16_t>(iPacketLen));

		if (wHeader)
		{
			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, wHeader, c_pData);

			if (iExtraPacketSize < 0)
			{
				LPCHARACTER ch = lpDesc->GetCharacter();
				sys_err("Failed to analyze header(0x%04X) recv_seq #%u, size: %d phase: %d host %s from %s (%u) in: %u",
					wHeader, lpDesc->GetRecvPacketSeq() - 1,
					iPacketLen, lpDesc->GetPhase(), lpDesc->GetHostName(),
					ch ? ch->GetName() : "NO_CHAR", ch ? ch->GetPlayerID() : 0, ch ? ch->GetMapIndex() : 0
				);
				lpDesc->DumpRecentPackets();
				return true;
			}

			lpDesc->Log("%s %d", c_pszName, iPacketLen);

			m_pPacketInfo->End();
		}

		c_pData	+= iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		wLastHeader	= wHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Version(LPCHARACTER ch, const char* c_pData)
{
	if (!ch)
		return;

	TPacketCGClientVersion * p = (TPacketCGClientVersion *) c_pData;
	sys_log(0, "VERSION: %s %s %s", ch->GetName(), p->timestamp, p->filename);
	ch->GetDesc()->SetClientVersion(p->timestamp);
}

void LoginFailure(LPDESC d, const char * c_pszStatus)
{
	if (!d)
		return;

	TPacketGCLoginFailure failurePacket;

	failurePacket.header = GC::LOGIN_FAILURE;
	failurePacket.length = sizeof(failurePacket);
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG * pkPacketInfo = M2_NEW CPacketInfoCG;

	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
	RegisterHandlers();
}

CInputHandshake::~CInputHandshake()
{
	if( NULL != m_pPacketInfo )
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}


std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::HandleText(LPDESC d, const char * c_pData)
{
	c_pData += PACKET_HEADER_SIZE; // skip [header:2][length:2]
	const char * c_pSep;

	if (!(c_pSep = strchr(c_pData, '\n')))	// \n을 찾는다.
		return -1;

	if (*(c_pSep - 1) == '\r')
		--c_pSep;

	std::string stResult;
	std::string stBuf;
	stBuf.assign(c_pData, 0, c_pSep - c_pData);

	sys_log(0, "SOCKET_CMD: FROM(%s) CMD(%s)", d->GetHostName(), stBuf.c_str());

	if (!stBuf.compare("IS_SERVER_UP"))
	{
		if (g_bNoMoreClient)
			stResult = "NO";
		else
			stResult = "YES";
	}
	//else if (!stBuf.compare("SHOWMETHEMONEY"))
	else if (stBuf == g_stAdminPagePassword)
	{
		if (!IsEmptyAdminPage())
		{
			if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
			{
				char szTmp[64];
				snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
				stResult += szTmp;
			}
			else
			{
				d->SetAdminMode();
				stResult = "UNKNOWN";
			}
		}
		else
		{
			d->SetAdminMode();
			stResult = "UNKNOWN";
		}
	}
	else if (!stBuf.compare("USER_COUNT"))
	{
		char szTmp[64];

		if (!IsEmptyAdminPage())
		{
			if (!IsAdminPage(inet_ntoa(d->GetAddr().sin_addr)))
			{
				snprintf(szTmp, sizeof(szTmp), "WEBADMIN : Wrong Connector : %s", inet_ntoa(d->GetAddr().sin_addr));
			}
			else
			{
				int iTotal;
				int * paiEmpireUserCount;
				int iLocal;
				DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
				snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
			}
		}
		else
		{
			int iTotal;
			int * paiEmpireUserCount;
			int iLocal;
			DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
			snprintf(szTmp, sizeof(szTmp), "%d %d %d %d %d", iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
		}
		stResult += szTmp;
	}
	else if (!stBuf.compare("CHECK_P2P_CONNECTIONS"))
	{
		std::ostringstream oss(std::ostringstream::out);
		
		oss << "P2P CONNECTION NUMBER : " << P2P_MANAGER::instance().GetDescCount() << "\n";
		std::string hostNames;
		P2P_MANAGER::Instance().GetP2PHostNames(hostNames);
		oss << hostNames;
		stResult = oss.str();
		TPacketGGCheckAwakeness packet;
		packet.header = GG::CHECK_AWAKENESS;
		packet.length = sizeof(packet);

		P2P_MANAGER::instance().Send(&packet, sizeof(packet));
	}
	else if (!stBuf.compare("PACKET_INFO"))
	{
		m_pMainPacketInfo->Log("packet_info.txt");
		stResult = "OK";
	}
	else if (!stBuf.compare("PROFILE"))
	{
		CProfiler::instance().Log("profile.txt");
		stResult = "OK";
	}
	//gift notify delete command
	else if (!stBuf.compare(0,15,"DELETE_AWARDID "))
		{
			char szTmp[64];
			std::string msg = stBuf.substr(15,26);	// item_award의 id범위?
			
			TPacketDeleteAwardID p;
			p.dwID = (DWORD)(atoi(msg.c_str()));
			snprintf(szTmp,sizeof(szTmp),"Sent to DB cache to delete ItemAward, id: %d",p.dwID);
			//sys_log(0,"%d",p.dwID);
			// strlcpy(p.login, msg.c_str(), sizeof(p.login));
			db_clientdesc->DBPacket(GD::DELETE_AWARDID, 0, &p, sizeof(p));
			stResult += szTmp;
		}
	else
	{
		stResult = "UNKNOWN";
		
		if (d->IsAdminMode())
		{
			// 어드민 명령들
			if (!stBuf.compare(0, 7, "NOTICE "))
			{
				std::string msg = stBuf.substr(7, 50);
				LogManager::instance().CharLog(0, 0, 0, 1, "NOTICE", msg.c_str(), d->GetHostName());
				BroadcastNotice(msg.c_str());
			}
			else if (!stBuf.compare("SHUTDOWN"))
			{
				LogManager::instance().CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
				TPacketGGShutdown p;
				p.header = GG::SHUTDOWN; p.length = sizeof(p);
				P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));
				sys_err("Accept shutdown command from %s.", d->GetHostName());
				Shutdown(10);
			}
			else if (!stBuf.compare("SHUTDOWN_ONLY"))
			{
				LogManager::instance().CharLog(0, 0, 0, 2, "SHUTDOWN", "", d->GetHostName());
				sys_err("Accept shutdown only command from %s.", d->GetHostName());
				Shutdown(10);
			}
			else if (!stBuf.compare(0, 3, "DC "))
			{
				std::string msg = stBuf.substr(3, LOGIN_MAX_LEN);

				sys_log(1, "DC : '%s'", msg.c_str());

				TPacketGGDisconnect pgg;

				pgg.header = GG::DISCONNECT; pgg.length = sizeof(pgg);
				strlcpy(pgg.szLogin, msg.c_str(), sizeof(pgg.szLogin));

				P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGDisconnect));

				// delete login key
				{
					TPacketDC p;
					strlcpy(p.login, msg.c_str(), sizeof(p.login));
					db_clientdesc->DBPacket(GD::DC, 0, &p, sizeof(p));
				}
			}
			else if (!stBuf.compare(0, 10, "RELOAD_CRC"))
			{
				LoadValidCRCList();

				struct { uint16_t header; uint16_t length; } pReloadCRC;
				pReloadCRC.header = GG::RELOAD_CRC_LIST;
				pReloadCRC.length = sizeof(pReloadCRC);
				P2P_MANAGER::instance().Send(&pReloadCRC, sizeof(pReloadCRC));
				stResult = "OK";
			}
			else if (!stBuf.compare(0, 20, "CHECK_CLIENT_VERSION"))
			{
				CheckClientVersion();

				struct { uint16_t header; uint16_t length; } pCheckVer;
				pCheckVer.header = GG::CHECK_CLIENT_VERSION;
				pCheckVer.length = sizeof(pCheckVer);
				P2P_MANAGER::instance().Send(&pCheckVer, sizeof(pCheckVer));
				stResult = "OK";
			}
			else if (!stBuf.compare(0, 6, "RELOAD"))
			{
				if (stBuf.size() == 6)
				{
					LoadStateUserCount();
					db_clientdesc->DBPacket(GD::RELOAD_PROTO, 0, NULL, 0);
				}
				else
				{
					char c = stBuf[7];

					switch (LOWER(c))
					{
						case 'u':
							LoadStateUserCount();
							break;

						case 'p':
							db_clientdesc->DBPacket(GD::RELOAD_PROTO, 0, NULL, 0);
							break;

						case 'q':
							quest::CQuestManager::instance().Reload();
							break;

						case 'f':
							fishing::Initialize();
							break;

						case 'a':
							db_clientdesc->DBPacket(GD::RELOAD_ADMIN, 0, NULL, 0);
							sys_log(0, "Reloading admin infomation.");
							break;
					}
				}
			}
			else if (!stBuf.compare(0, 6, "EVENT "))
			{
				std::istringstream is(stBuf);
				std::string strEvent, strFlagName;
				long lValue;
				is >> strEvent >> strFlagName >> lValue;

				if (!is.fail())
				{
					sys_log(0, "EXTERNAL EVENT FLAG name %s value %d", strFlagName.c_str(), lValue);
					quest::CQuestManager::instance().RequestSetEventFlag(strFlagName, lValue);
					stResult = "EVENT FLAG CHANGE ";
					stResult += strFlagName;
				}
				else
				{
					stResult = "EVENT FLAG FAIL";
				}
			}
			// BLOCK_CHAT
			else if (!stBuf.compare(0, 11, "BLOCK_CHAT "))
			{
				std::istringstream is(stBuf);
				std::string strBlockChat, strCharName;
				long lDuration;
				is >> strBlockChat >> strCharName >> lDuration;

				if (!is.fail())
				{
					sys_log(0, "EXTERNAL BLOCK_CHAT name %s duration %d", strCharName.c_str(), lDuration);

					do_block_chat(NULL, const_cast<char*>(stBuf.c_str() + 11), 0, 0);

					stResult = "BLOCK_CHAT ";
					stResult += strCharName;
				}
				else
				{
					stResult = "BLOCK_CHAT FAIL";
				}
			}
			// END_OF_BLOCK_CHAT
			else if (!stBuf.compare(0, 12, "PRIV_EMPIRE "))
			{
				int	empire, type, value, duration;
				std::istringstream is(stBuf);
				std::string strPrivEmpire;
				is >> strPrivEmpire >> empire >> type >> value >> duration;

				// 최대치 10배
				value = MINMAX(0, value, 1000);
				stResult = "PRIV_EMPIRE FAIL";

				if (!is.fail())
				{
					// check parameter
					if (empire < 0 || 3 < empire);
					else if (type < 1 || 4 < type);
					else if (value < 0);
					else if (duration < 0);
					else
					{
						stResult = "PRIV_EMPIRE SUCCEED";

						// 시간 단위로 변경
						duration = duration * (60 * 60);

						sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, duration=%d) by web", 
								empire, type, value, duration);
						CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
					}
				}
			}
		}
	}

	sys_log(1, "TEXT %s RESULT %s", stBuf.c_str(), stResult.c_str());
	stResult += "\n";
	d->Packet(stResult.c_str(), stResult.length());
	return (c_pSep - c_pData) + 1;
}

int CInputHandshake::HandleMarkLogin(LPDESC d, const char*)
{
	if (!guild_mark_server)
	{
		sys_err("Guild Mark login requested but i'm not a mark server!");
		d->SetPhase(PHASE_CLOSE);
		return 0;
	}

	sys_log(0, "MARK_SERVER: Login");
	d->SetPhase(PHASE_LOGIN);
	return 0;
}

int CInputHandshake::HandleStateChecker(LPDESC d, const char*)
{
	if (d->isChannelStatusRequested()) {
		return 0;
	}
	d->SetChannelStatusRequested(true);
	db_clientdesc->DBPacket(GD::REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	return 0;
}

int CInputHandshake::HandlePong(LPDESC d, const char*)
{
	Pong(d);
	return 0;
}

int CInputHandshake::HandleKeyResponse(LPDESC d, const char* c_pData)
{
	TPacketCGKeyResponse* p = (TPacketCGKeyResponse*)c_pData;

	if (!d->GetSecureCipher().IsInitialized())
	{
		sys_err("SecureCipher not initialized. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	if (d->HandleKeyResponse(p->client_pk, p->challenge_response))
	{
		d->SendKeyComplete();

		if (g_bAuthServer) {
			sys_log(0, "[HANDSHAKE] Key exchange complete for %s, transitioning to PHASE_AUTH", d->GetHostName());
			d->SetPhase(PHASE_AUTH);
		} else {
			sys_log(0, "[HANDSHAKE] Key exchange complete for %s, transitioning to PHASE_LOGIN", d->GetHostName());
			d->SetPhase(PHASE_LOGIN);
		}
	}
	else
	{
		sys_err("[HANDSHAKE] Secure key response verification FAILED for %s",
			inet_ntoa(d->GetAddr().sin_addr));
		d->SetPhase(PHASE_CLOSE);
	}
	return 0;
}

void CInputHandshake::RegisterHandlers()
{
	m_handlers[CG::TEXT]          = &CInputHandshake::HandleText;
	m_handlers[CG::MARK_LOGIN]   = &CInputHandshake::HandleMarkLogin;
	m_handlers[CG::STATE_CHECKER] = &CInputHandshake::HandleStateChecker;
	m_handlers[CG::PONG]         = &CInputHandshake::HandlePong;
	m_handlers[CG::KEY_RESPONSE] = &CInputHandshake::HandleKeyResponse;
}

int CInputHandshake::Analyze(LPDESC d, uint16_t wHeader, const char * c_pData)
{
	auto it = m_handlers.find(wHeader);
	if (it == m_handlers.end())
	{
		sys_err("Handshake phase does not handle packet %d (fd %d)", wHeader, d->GetSocket());
		return 0;
	}

	return (this->*(it->second))(d, c_pData);
}
