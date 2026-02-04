#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "desc_client.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "fishing.h"
#include "TrafficProfiler.h"
#include "priv_manager.h"
#include "castle.h"

extern time_t get_global_time();

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

std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

int CInputHandshake::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{
	if (bHeader == 10) // 엔터는 무시
		return 0;

	if (bHeader == HEADER_CG_MARK_LOGIN)
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


