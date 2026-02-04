#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "login_sim.h"
#include "fishing.h"
#include "TrafficProfiler.h"
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

	BYTE	bLastHeader = 0;
	int		iLastPacketLen = 0;
	int		iPacketLen;

	if (!m_pPacketInfo)
	{
		sys_err("No packet info has been binded to");
		return true;
	}

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
		BYTE bHeader = (BYTE) *(c_pData);
		const char * c_pszName;

		if (bHeader == 0) // 암호화 처리가 있으므로 0번 헤더는 스킵한다.
			iPacketLen = 1;
		else if (!m_pPacketInfo->Get(bHeader, &iPacketLen, &c_pszName))
		{
			LPCHARACTER ch = lpDesc->GetCharacter();

			char szLogBuffer[1024];
			snprintf(szLogBuffer, sizeof(szLogBuffer),
				"UNKNOWN HEADER: %u(0x%X) LAST HEADER: %u(0x%X)[%u], REMAIN BYTES: %d, CHAR: %s, PHASE: %d, fd: %d host: %s",
				bHeader, bHeader, bLastHeader, bLastHeader, iLastPacketLen, m_iBufferLeft,
				ch ? ch->GetName() : "<none>",
				lpDesc->GetPhase(), lpDesc->GetSocket(), lpDesc->GetHostName()
			);

			if (lpDesc->GetPhase() < PHASE_SELECT) // early phase
				sys_log(0, szLogBuffer);
			else
				sys_err(szLogBuffer);

#if defined(_DEBUG) && defined(_WIN32)
			printdata((uint8_t*)c_pvOrig, m_iBufferLeft);
#endif
			lpDesc->SetPhase(PHASE_CLOSE);
			return true;
		}

#ifdef _DEBUG
		const auto ch = lpDesc->GetCharacter();
		const std::string stName = ch ? ch->GetName() : lpDesc->GetHostName();
		sys_log(0, "RECEIVED HEADER : %u(0x%X) to %s  (size %d) ", bHeader, bHeader, stName.c_str(), iBytes);
#endif

		if (m_iBufferLeft < iPacketLen)
			return true;

		if (bHeader)
		{
			if (test_server && bHeader != HEADER_CG_MOVE)
				sys_log(0, "Packet Analyze [Header %d][bufferLeft %d] ", bHeader, m_iBufferLeft);

			m_pPacketInfo->Start();

			int iExtraPacketSize = Analyze(lpDesc, bHeader, c_pData);

			if (iExtraPacketSize < 0)
			{
				LPCHARACTER ch = lpDesc->GetCharacter();
				sys_err("Failed to analyze header(%u) size: %d phase: %d host %s from %s (%u) in: %u",
					bHeader, iPacketLen, lpDesc->GetPhase(), lpDesc->GetHostName(),
					ch ? ch->GetName() : "NO_CHAR", ch ? ch->GetPlayerID() : 0, ch ? ch->GetMapIndex() : 0
				);
				return true;
			}

			iPacketLen += iExtraPacketSize;
			lpDesc->Log("%s %d", c_pszName, iPacketLen);
			m_pPacketInfo->End();
		}

		// TRAFFIC_PROFILER
		if (g_bTrafficProfileOn)
			TrafficProfiler::instance().Report(TrafficProfiler::IODIR_INPUT, bHeader, iPacketLen);
		// END_OF_TRAFFIC_PROFILER

		if (bHeader == HEADER_CG_PONG)
			sys_log(0, "PONG! %u %u", m_pPacketInfo->IsSequence(bHeader), *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE)));

		if (m_pPacketInfo->IsSequence(bHeader))
		{
			BYTE bSeq = lpDesc->GetSequence();
			BYTE bSeqReceived = *(BYTE *) (c_pData + iPacketLen - sizeof(BYTE));

			if (bSeq != bSeqReceived)
			{
				sys_err("SEQUENCE %x mismatch 0x%x != 0x%x header %u", get_pointer(lpDesc), bSeq, bSeqReceived, bHeader);
				lpDesc->SetPhase(PHASE_CLOSE);
				return true;
			}
		}

		c_pData	+= iPacketLen;
		m_iBufferLeft -= iPacketLen;
		r_iBytesProceed += iPacketLen;

		iLastPacketLen = iPacketLen;
		bLastHeader	= bHeader;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, const char * c_pData)
{
	TPacketCGHandshake * p = (TPacketCGHandshake *) c_pData;

	if (d->GetHandshake() != p->dwHandshake)
	{
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->dwTime, p->lDelta, false))
			{
				// Use secure key exchange (libsodium/XChaCha20-Poly1305)
				d->SendKeyChallenge();
			}
		}
		else
			d->HandshakeProcess(p->dwTime, p->lDelta, true);
	}
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

	failurePacket.header = HEADER_GC_LOGIN_FAILURE;
	strlcpy(failurePacket.szStatus, c_pszStatus, sizeof(failurePacket.szStatus));

	d->Packet(&failurePacket, sizeof(failurePacket));
}

CInputHandshake::CInputHandshake()
{
	CPacketInfoCG * pkPacketInfo = M2_NEW CPacketInfoCG;
	pkPacketInfo->SetSequence(HEADER_CG_PONG, false);

	m_pMainPacketInfo = m_pPacketInfo;
	BindPacketInfo(pkPacketInfo);
}

CInputHandshake::~CInputHandshake()
{
	if( NULL != m_pPacketInfo )
	{
		M2_DELETE(m_pPacketInfo);
		m_pPacketInfo = NULL;
	}
}


std::map<DWORD, CLoginSim *> g_sim;
std::map<DWORD, CLoginSim *> g_simByPID;
std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (bHeader == 10) // 엔터는 무시
		return 0;

	if (bHeader == HEADER_CG_TEXT)
	{
		++c_pData;
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
			packet.bHeader = HEADER_GG_CHECK_AWAKENESS;

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
				db_clientdesc->DBPacket(HEADER_GD_DELETE_AWARDID, 0, &p, sizeof(p));
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
					p.bHeader = HEADER_GG_SHUTDOWN;
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

					pgg.bHeader = HEADER_GG_DISCONNECT;
					strlcpy(pgg.szLogin, msg.c_str(), sizeof(pgg.szLogin));

					P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGDisconnect));

					// delete login key
					{
						TPacketDC p;
						strlcpy(p.login, msg.c_str(), sizeof(p.login));
						db_clientdesc->DBPacket(HEADER_GD_DC, 0, &p, sizeof(p));
					}
				}
				else if (!stBuf.compare(0, 10, "RELOAD_CRC"))
				{
					LoadValidCRCList();

					BYTE bHeader = HEADER_GG_RELOAD_CRC_LIST;
					P2P_MANAGER::instance().Send(&bHeader, sizeof(BYTE));
					stResult = "OK";
				}
				else if (!stBuf.compare(0, 20, "CHECK_CLIENT_VERSION"))
				{
					CheckClientVersion();

					BYTE bHeader = HEADER_GG_CHECK_CLIENT_VERSION;
					P2P_MANAGER::instance().Send(&bHeader, sizeof(BYTE));
					stResult = "OK";
				}
				else if (!stBuf.compare(0, 6, "RELOAD"))
				{
					if (stBuf.size() == 6)
					{
						LoadStateUserCount();
						db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
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
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
								break;

							case 'q':
								quest::CQuestManager::instance().Reload();
								break;

							case 'f':
								fishing::Initialize();
								break;

							case 'a':
								db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
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
	else if (bHeader == HEADER_CG_MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			// 끊어버려! - 마크 서버가 아닌데 마크를 요청하려고?
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		// 무조건 인증 --;
		sys_log(0, "MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (bHeader == HEADER_CG_STATE_CHECKER)
	{
		if (d->isChannelStatusRequested()) {
			return 0;
		}
		d->SetChannelStatusRequested(true);
		db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHANNELSTATUS, d->GetHandle(), NULL, 0);
	}
	else if (bHeader == HEADER_CG_PONG)
		Pong(d);
	else if (bHeader == HEADER_CG_HANDSHAKE)
		Handshake(d, c_pData);
	// Secure key exchange (libsodium/XChaCha20-Poly1305)
	else if (bHeader == HEADER_CG_KEY_RESPONSE)
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
			// Key exchange succeeded, send completion
			d->SendKeyComplete();

			// Move to next phase
			if (g_bAuthServer) {
				d->SetPhase(PHASE_AUTH);
			} else {
				d->SetPhase(PHASE_LOGIN);
			}
		}
		else
		{
			sys_err("[CInputHandshake] Secure key response verification failed for %s",
				inet_ntoa(d->GetAddr().sin_addr));
			d->SetPhase(PHASE_CLOSE);
		}
	}
	else
		sys_err("Handshake phase does not handle packet %d (fd %d)", bHeader, d->GetSocket());

	return 0;
}


