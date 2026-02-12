#include "stdafx.h"
#include <cinttypes>
#include "config.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "packet_structs.h"
#include "messenger_manager.h"
#include "sectree_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "guild.h"
#include "guild_manager.h"
#include "locale_service.h"
#include "log.h"

extern int max_bytes_written;
extern int current_bytes_written;
extern int total_bytes_written;

DESC::DESC()
{
	Initialize();
}

DESC::~DESC()
{
	Destroy();
}

void DESC::Initialize()
{
	m_bDestroyed = false;

	m_pInputProcessor = NULL;
	m_lpFdw = NULL;
	m_sock = INVALID_SOCKET;
	m_iPhase = PHASE_CLOSE;
	m_dwHandle = 0;

	m_wPort = 0;
	m_LastTryToConnectTime = 0;

	m_inputBuffer.Clear();

	m_dwClientTime = 0;
	m_handshake_time = get_dword_time();

	m_bufferedOutputBuffer.Clear();
	m_hasBufferedOutput = false;
	m_outputBuffer.Clear();

	m_pkPingEvent = NULL;
	m_lpCharacter = NULL;
	memset( &m_accountTable, 0, sizeof(m_accountTable) );

	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );

	m_pLogFile = NULL;

	m_wP2PPort = 0;
	m_bP2PChannel = 0;

	m_bAdminMode = false;
	m_bPong = true;
	m_bChannelStatusRequested = false;

	m_pkLoginKey = NULL;
	m_dwLoginKey = 0;

	m_bCRCMagicCubeIdx = 0;
	m_dwProcCRC = 0;
	m_dwFileCRC = 0;
	m_bHackCRCQuery = 0;

	m_outtime = 0;
	m_playtime = 0;
	m_offtime = 0;

	m_pkDisconnectEvent = NULL;
}

void DESC::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	if (m_pkLoginKey)
		m_pkLoginKey->Expire();

	if (GetAccountTable().id)
		DESC_MANAGER::instance().DisconnectAccount(GetAccountTable().login);

	if (m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	if (m_lpCharacter)
	{
		m_lpCharacter->Disconnect("DESC::~DESC");
		m_lpCharacter = NULL;
	}

	m_outputBuffer.Clear();
	m_inputBuffer.Clear();
	m_bufferedOutputBuffer.Clear();
	m_hasBufferedOutput = false;

	event_cancel(&m_pkPingEvent);
	event_cancel(&m_pkDisconnectEvent);

	if (!g_bAuthServer)
	{
		if (m_accountTable.login[0] && m_accountTable.passwd[0])
		{
			TLogoutPacket pack;

			strlcpy(pack.login, m_accountTable.login, sizeof(pack.login));
			strlcpy(pack.passwd, m_accountTable.passwd, sizeof(pack.passwd));

			db_clientdesc->DBPacket(GD::LOGOUT, m_dwHandle, &pack, sizeof(TLogoutPacket));
		}
	}

	if (m_sock != INVALID_SOCKET)
	{
		sys_log(0, "SYSTEM: closing socket. DESC #%d", m_sock);
		Log("SYSTEM: closing socket. DESC #%d", m_sock);
		fdwatch_del_fd(m_lpFdw, m_sock);

		m_secureCipher.CleanUp();

		socket_close(m_sock);
		m_sock = INVALID_SOCKET;
	}
}

EVENTFUNC(ping_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ping_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC desc = info->desc;

	if (desc->IsAdminMode())
		return (ping_event_second_cycle);

	if (!desc->IsPong())
	{
		sys_log(0, "PING_EVENT: no pong %s", desc->GetHostName());

		desc->SetPhase(PHASE_CLOSE);

		return (ping_event_second_cycle);
	}
	else
	{
		TPacketGCPing p;
		p.header = GC::PING;
		p.length = sizeof(p);
		p.server_time = get_dword_time();
		desc->Packet(&p, sizeof(struct packet_ping));
		desc->SetPong(false);
	}

	return (ping_event_second_cycle);
}

bool DESC::IsPong()
{
	return m_bPong;
}

void DESC::SetPong(bool b)
{
	m_bPong = b;
}

bool DESC::Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in & c_rSockAddr, DWORD _handle)
{
	m_lpFdw		= _fdw;
	m_sock		= _fd;

	m_stHost		= inet_ntoa(c_rSockAddr.sin_addr);
	m_wPort			= c_rSockAddr.sin_port;
	m_dwHandle		= _handle;

	m_outputBuffer.Reserve(65536 * 3);
	m_inputBuffer.Reserve(MAX_INPUT_LEN);

	m_SockAddr = c_rSockAddr;

	fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);

	// Ping Event
	desc_event_info* info = AllocEventInfo<desc_event_info>();

	info->desc = this;
	assert(m_pkPingEvent == NULL);

	m_pkPingEvent = event_create(ping_event, info, ping_event_second_cycle);

	SetPhase(PHASE_HANDSHAKE);
	m_handshake_time = get_dword_time();
	SendKeyChallenge();

	sys_log(0, "[CONN] Setup complete for %s, KeyChallenge sent", GetHostName());

	sys_log(0, "SYSTEM: new connection from [%s] fd: %d input_capacity %zu, ptr %p",
			m_stHost.c_str(), m_sock, m_inputBuffer.Capacity(), this);

	Log("SYSTEM: new connection from [%s] fd: %d ptr %p", m_stHost.c_str(), m_sock, this);
	return true;
}

int DESC::ProcessInput()
{
	m_inputBuffer.EnsureWritable(4096);

	size_t writable = m_inputBuffer.WritableBytes();
	ssize_t bytes_read = socket_read(m_sock, (char*)m_inputBuffer.WritePtr(), writable);

	if (bytes_read < 0)
		return -1;
	else if (bytes_read == 0)
		return 0;

	if (m_secureCipher.IsActivated()) {
		m_secureCipher.DecryptInPlace(m_inputBuffer.WritePtr(), bytes_read);
	}

	m_inputBuffer.CommitWrite(bytes_read);

	if (!m_pInputProcessor)
		sys_err("no input processor");
	else
	{
		int iBytesProceed = 0;

		// false가 리턴 되면 다른 phase로 바뀐 것이므로 다시 프로세스로 돌입한다!
		while (!m_pInputProcessor->Process(this, m_inputBuffer.ReadPtr(), static_cast<int>(m_inputBuffer.ReadableBytes()), iBytesProceed))
		{
			m_inputBuffer.Discard(iBytesProceed);
			iBytesProceed = 0;
		}

		m_inputBuffer.Discard(iBytesProceed);
	}

	return (bytes_read);
}

int DESC::ProcessOutput()
{
	if (m_outputBuffer.ReadableBytes() <= 0)
		return 0;

	int buffer_left = fdwatch_get_buffer_size(m_lpFdw, m_sock);

	if (buffer_left <= 0)
		return 0;

	int bytes_to_write = MIN(buffer_left, static_cast<int>(m_outputBuffer.ReadableBytes()));

	if (bytes_to_write == 0)
		return 0;

	int result = socket_write(m_sock, (const char *) m_outputBuffer.ReadPtr(), bytes_to_write);

	if (result == 0)
	{
		max_bytes_written = MAX(bytes_to_write, max_bytes_written);

		total_bytes_written += bytes_to_write;
		current_bytes_written += bytes_to_write;

		m_outputBuffer.Discard(bytes_to_write);

		if (m_outputBuffer.ReadableBytes() != 0)
			fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
	}

	return (result);
}

void DESC::BufferedPacket(const void * c_pvData, int iSize)
{
	if (m_iPhase == PHASE_CLOSE)
		return;

	if (!m_hasBufferedOutput)
	{
		m_bufferedOutputBuffer.Clear();
		m_hasBufferedOutput = true;
	}

	if (iSize >= (int)sizeof(uint16_t) * 2)
	{
		const uint16_t wHeader = *static_cast<const uint16_t*>(c_pvData);
		LogSentPacket(wHeader, static_cast<uint16_t>(iSize));
	}

	m_bufferedOutputBuffer.Write(c_pvData, iSize);
}

void DESC::Packet(const void * c_pvData, int iSize)
{
	assert(iSize > 0);

	if (m_iPhase == PHASE_CLOSE)
		return;

	if (!m_hasBufferedOutput && iSize >= (int)sizeof(uint16_t) * 2)
	{
		const uint16_t wHeader = *static_cast<const uint16_t*>(c_pvData);
		LogSentPacket(wHeader, static_cast<uint16_t>(iSize));
	}

	if (m_stRelayName.length() != 0)
	{
		// Relay 패킷은 암호화하지 않는다.
		TPacketGGRelay p;

		p.header = GG::RELAY;
		strlcpy(p.szName, m_stRelayName.c_str(), sizeof(p.szName));
		p.lSize = iSize;
		p.length = sizeof(p) + iSize;

		m_outputBuffer.Write(&p, sizeof(p));
		m_stRelayName.clear();
		m_outputBuffer.Write(c_pvData, iSize);
	}
	else
	{
		if (m_hasBufferedOutput)
		{
			m_bufferedOutputBuffer.Write(c_pvData, iSize);

			c_pvData = m_bufferedOutputBuffer.ReadPtr();
			iSize = static_cast<int>(m_bufferedOutputBuffer.ReadableBytes());
		}

		m_outputBuffer.EnsureWritable(iSize);
		std::memcpy(m_outputBuffer.WritePtr(), c_pvData, iSize);

		if (m_secureCipher.IsActivated()) {
			m_secureCipher.EncryptInPlace(m_outputBuffer.WritePtr(), iSize);
		}

		m_outputBuffer.CommitWrite(iSize);

		if (m_hasBufferedOutput)
		{
			m_bufferedOutputBuffer.Clear();
			m_hasBufferedOutput = false;
		}
	}

	if (m_iPhase != PHASE_CLOSE)
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
}

void DESC::LargePacket(const void * c_pvData, int iSize)
{
	m_outputBuffer.EnsureWritable(iSize);
	Packet(c_pvData, iSize);
}

void DESC::SetPhase(int _phase)
{
	m_iPhase = _phase;

	TPacketGCPhase pack;
	pack.header = GC::PHASE;
	pack.length = sizeof(pack);
	pack.phase = _phase;
	Packet(&pack, sizeof(TPacketGCPhase));

	switch (m_iPhase)
	{
		case PHASE_CLOSE:
			m_pInputProcessor = &m_inputClose;
			break;

		case PHASE_HANDSHAKE:
			m_pInputProcessor = &m_inputHandshake;
			break;

		case PHASE_SELECT:
		case PHASE_LOGIN:
		case PHASE_LOADING:
			m_pInputProcessor = &m_inputLogin;
			break;

		case PHASE_GAME:
		case PHASE_DEAD:
			m_pInputProcessor = &m_inputMain;
			break;

		case PHASE_AUTH:
			m_pInputProcessor = &m_inputAuth;
			sys_log(0, "AUTH_PHASE %p", this);
			break;
	}
}

void DESC::BindAccountTable(TAccountTable * pAccountTable)
{
	assert(pAccountTable != NULL);
	thecore_memcpy(&m_accountTable, pAccountTable, sizeof(TAccountTable));
	DESC_MANAGER::instance().ConnectAccount(m_accountTable.login, this);
}

void DESC::Log(const char * format, ...)
{
	if (!m_pLogFile)
		return;

	va_list args;

	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);

	fprintf(m_pLogFile,
			"%02d %02d %02d:%02d:%02d | ",
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	va_start(args, format);
	vfprintf(m_pLogFile, format, args);
	va_end(args);

	fputs("\n", m_pLogFile);

	fflush(m_pLogFile);
}

bool DESC::IsExpiredHandshake() const
{
	if (m_handshake_time == 0)
		return false;

	return (m_handshake_time + (5 * 1000)) < get_dword_time();
}

DWORD DESC::GetClientTime()
{
	return m_dwClientTime;
}

void DESC::SendKeyChallenge()
{
	if (!m_secureCipher.Initialize())
	{
		sys_err("Failed to initialize SecureCipher");
		SetPhase(PHASE_CLOSE);
		return;
	}

	m_secureCipher.GenerateChallenge(m_challenge);

	TPacketGCKeyChallenge packet;
	packet.header = GC::KEY_CHALLENGE;
	packet.length = sizeof(packet);
	m_secureCipher.GetPublicKey(packet.server_pk);
	memcpy(packet.challenge, m_challenge, SecureCipher::CHALLENGE_SIZE);
	packet.server_time = get_dword_time();

	Packet(&packet, sizeof(packet));

	sys_log(0, "[HANDSHAKE] KeyChallenge sent to %s (server_time: %u, pk: %02x%02x%02x%02x)",
		GetHostName(), packet.server_time,
		packet.server_pk[0], packet.server_pk[1], packet.server_pk[2], packet.server_pk[3]);
}

bool DESC::HandleKeyResponse(const uint8_t* client_pk, const uint8_t* challenge_response)
{
	if (!m_secureCipher.ComputeServerKeys(client_pk))
	{
		sys_err("Failed to compute server session keys for %s", GetHostName());
		return false;
	}

	if (!m_secureCipher.VerifyChallengeResponse(m_challenge, challenge_response))
	{
		sys_err("Challenge response verification failed for %s", GetHostName());
		return false;
	}

	sys_log(0, "[HANDSHAKE] KeyResponse verified for %s (tx_nonce: %llu, rx_nonce: %llu)",
		GetHostName(), (unsigned long long)m_secureCipher.GetTxNonce(), (unsigned long long)m_secureCipher.GetRxNonce());
	return true;
}

void DESC::SendKeyComplete()
{
	uint8_t session_token[SecureCipher::SESSION_TOKEN_SIZE];
	randombytes_buf(session_token, sizeof(session_token));
	m_secureCipher.SetSessionToken(session_token);

	TPacketGCKeyComplete packet;
	packet.header = GC::KEY_COMPLETE;
	packet.length = sizeof(packet);

	if (!m_secureCipher.EncryptToken(session_token, sizeof(session_token),
	                                  packet.encrypted_token, packet.nonce))
	{
		sys_err("Failed to encrypt session token for %s", GetHostName());
		SetPhase(PHASE_CLOSE);
		return;
	}

	Packet(&packet, sizeof(packet));

	ProcessOutput();
	m_secureCipher.SetActivated(true);

	sys_log(0, "[HANDSHAKE] Cipher ACTIVATED for %s (tx_nonce: %llu, rx_nonce: %llu)",
		GetHostName(), (unsigned long long)m_secureCipher.GetTxNonce(), (unsigned long long)m_secureCipher.GetRxNonce());
}

void DESC::SetRelay(const char * c_pszName)
{
	m_stRelayName = c_pszName;
}

void DESC::BindCharacter(LPCHARACTER ch)
{
	m_lpCharacter = ch;
}

void DESC::FlushOutput()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	if (m_outputBuffer.ReadableBytes() <= 0)
		return;

	struct timeval sleep_tv, now_tv, start_tv;
	int event_triggered = false;

	gettimeofday(&start_tv, NULL);

	socket_block(m_sock);
	sys_log(0, "FLUSH START %zu", m_outputBuffer.ReadableBytes());

	while (m_outputBuffer.ReadableBytes() > 0)
	{
		gettimeofday(&now_tv, NULL);

		int iSecondsPassed = now_tv.tv_sec - start_tv.tv_sec;

		if (iSecondsPassed > 10)
		{
			if (!event_triggered || iSecondsPassed > 20)
			{
				SetPhase(PHASE_CLOSE);
				break;
			}
		}

		sleep_tv.tv_sec = 0;
		sleep_tv.tv_usec = 10000;

		int num_events = fdwatch(m_lpFdw, &sleep_tv);

		if (num_events < 0)
		{
			sys_err("num_events < 0 : %d", num_events);
			break;
		}

		int event_idx;

		for (event_idx = 0; event_idx < num_events; ++event_idx)
		{
			LPDESC d2 = (LPDESC) fdwatch_get_client_data(m_lpFdw, event_idx);

			if (d2 != this)
				continue;

			switch (fdwatch_check_event(m_lpFdw, m_sock, event_idx))
			{
				case FDW_WRITE:
					event_triggered = true;

					if (ProcessOutput() < 0)
					{
						sys_err("Cannot flush output buffer");
						SetPhase(PHASE_CLOSE);
					}
					break;

				case FDW_EOF:
					SetPhase(PHASE_CLOSE);
					break;
			}
		}

		if (IsPhase(PHASE_CLOSE))
			break;
	}

	if (m_outputBuffer.ReadableBytes() == 0)
		sys_log(0, "FLUSH SUCCESS");
	else
		sys_log(0, "FLUSH FAIL");

	usleep(250000);
}

EVENTFUNC(disconnect_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "disconnect_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC d = info->desc;

	d->m_pkDisconnectEvent = NULL;
	d->SetPhase(PHASE_CLOSE);
	return 0;
}

bool DESC::DelayedDisconnect(int iSec)
{
	if (m_pkDisconnectEvent != NULL) {
		return false;
	}

	desc_event_info* info = AllocEventInfo<desc_event_info>();
	info->desc = this;

	m_pkDisconnectEvent = event_create(disconnect_event, info, PASSES_PER_SEC(iSec));
	return true;
}

void DESC::DisconnectOfSameLogin()
{
	if (GetCharacter())
	{
		if (m_pkDisconnectEvent)
			return;

		GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 컴퓨터에서 로그인 하여 접속을 종료 합니다."));
		DelayedDisconnect(5);
	}
	else
	{
		SetPhase(PHASE_CLOSE);
	}
}

void DESC::SetAdminMode()
{
	m_bAdminMode = true;
}

bool DESC::IsAdminMode()
{
	return m_bAdminMode;
}

void DESC::SendLoginSuccessPacket()
{
	TAccountTable & rTable = GetAccountTable();

	TPacketGCLoginSuccess p;

	p.header    = GC::LOGIN_SUCCESS4;
	p.length = sizeof(p);

	p.handle     = GetHandle();
	p.random_key = DESC_MANAGER::instance().MakeRandomKey(GetHandle()); // FOR MARK
	thecore_memcpy(p.players, rTable.players, sizeof(rTable.players));

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{

#ifdef ENABLE_PROXY_IP
		if (!g_stProxyIP.empty())
			rTable.players[i].lAddr=inet_addr(g_stProxyIP.c_str());
#endif

		CGuild* g = CGuildManager::instance().GetLinkedGuild(rTable.players[i].dwID);

		if (g)
		{
			p.guild_id[i] = g->GetID();
			strlcpy(p.guild_name[i], g->GetName(), sizeof(p.guild_name[i]));
		}
		else
		{
			p.guild_id[i] = 0;
			p.guild_name[i][0] = '\0';
		}
	}

	Packet(&p, sizeof(TPacketGCLoginSuccess));
}

void DESC::SetLoginKey(DWORD dwKey)
{
	m_dwLoginKey = dwKey;
}

void DESC::SetLoginKey(CLoginKey * pkKey)
{
	m_pkLoginKey = pkKey;
	sys_log(0, "SetLoginKey %u", m_pkLoginKey->m_dwKey);
}

DWORD DESC::GetLoginKey()
{
	if (m_pkLoginKey)
		return m_pkLoginKey->m_dwKey;

	return m_dwLoginKey;
}

void DESC::AssembleCRCMagicCube(BYTE bProcPiece, BYTE bFilePiece)
{
	static BYTE abXORTable[32] =
	{
		102,  30, 0, 0, 0, 0, 0, 0,
		188,  44, 0, 0, 0, 0, 0, 0,
		39, 201, 0, 0, 0, 0, 0, 0,
		43,   5, 0, 0, 0, 0, 0, 0,
	};

	bProcPiece = (bProcPiece ^ abXORTable[m_bCRCMagicCubeIdx]);
	bFilePiece = (bFilePiece ^ abXORTable[m_bCRCMagicCubeIdx+1]);

	m_dwProcCRC |= bProcPiece << m_bCRCMagicCubeIdx;
	m_dwFileCRC |= bFilePiece << m_bCRCMagicCubeIdx;

	m_bCRCMagicCubeIdx += 8;

	if (!(m_bCRCMagicCubeIdx & 31))
	{
		m_dwProcCRC = 0;
		m_dwFileCRC = 0;
		m_bCRCMagicCubeIdx = 0;
	}
}

BYTE DESC::GetEmpire()
{
	return m_accountTable.bEmpire;
}

void DESC::RawPacket(const void * c_pvData, int iSize)
{
	if (m_iPhase == PHASE_CLOSE)
		return;

	m_outputBuffer.Write(c_pvData, iSize);

	if (m_iPhase != PHASE_CLOSE)
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
}

void DESC::ChatPacket(BYTE type, const char * format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header    = GC::CHAT;
	pack_chat.length      = sizeof(struct packet_chat) + len;
	pack_chat.type      = type;
	pack_chat.id        = 0;
	pack_chat.bEmpire   = GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	Packet(buf.read_peek(), buf.size());
}

void DESC::LogRecvPacket(uint16_t header, uint16_t length)
{
	auto& e = m_aRecentRecvPackets[m_dwRecvPacketSeq % PACKET_LOG_SIZE];
	e.seq = m_dwRecvPacketSeq;
	e.header = header;
	e.length = length;
	m_dwRecvPacketSeq++;
}

void DESC::LogSentPacket(uint16_t header, uint16_t length)
{
	auto& e = m_aRecentSentPackets[m_dwSentPacketSeq % PACKET_LOG_SIZE];
	e.seq = m_dwSentPacketSeq;
	e.header = header;
	e.length = length;
	m_dwSentPacketSeq++;
}

void DESC::DumpRecentPackets() const
{
	const uint32_t recvCount = std::min(m_dwRecvPacketSeq, (uint32_t)PACKET_LOG_SIZE);
	const uint32_t sentCount = std::min(m_dwSentPacketSeq, (uint32_t)PACKET_LOG_SIZE);

	sys_err("=== Recent RECV packets (last %u of %u total) fd=%d host=%s ===",
		recvCount, m_dwRecvPacketSeq, m_sock, m_stHost.c_str());

	for (uint32_t i = 0; i < recvCount; i++)
	{
		uint32_t idx = (m_dwRecvPacketSeq > PACKET_LOG_SIZE)
			? (m_dwRecvPacketSeq - PACKET_LOG_SIZE + i)
			: i;
		const auto& e = m_aRecentRecvPackets[idx % PACKET_LOG_SIZE];
		sys_err("  RECV #%u: header=0x%04X len=%u", e.seq, e.header, e.length);
	}

	sys_err("=== Recent SENT packets (last %u of %u total) ===", sentCount, m_dwSentPacketSeq);

	for (uint32_t i = 0; i < sentCount; i++)
	{
		uint32_t idx = (m_dwSentPacketSeq > PACKET_LOG_SIZE)
			? (m_dwSentPacketSeq - PACKET_LOG_SIZE + i)
			: i;
		const auto& e = m_aRecentSentPackets[idx % PACKET_LOG_SIZE];
		sys_err("  SENT #%u: header=0x%04X len=%u", e.seq, e.header, e.length);
	}
}
