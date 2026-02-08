#include "stdafx.h" 
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#include "threeway_war.h"
#include "crc32.h"

////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
	RegisterHandlers();
}

void CInputP2P::Login(LPDESC d, const char * c_pData)
{
	P2P_MANAGER::instance().Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char * c_pData)
{
	TPacketGGLogout * p = (TPacketGGLogout *) c_pData;
	P2P_MANAGER::instance().Logout(p->szName);
}

int CInputP2P::Relay(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGRelay * p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	sys_log(0, "InputP2P::Relay : %s size %d", p->szName, p->lSize);

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(p->szName);

	const char* c_pbData = c_pData + sizeof(TPacketGGRelay);

	if (!pkChr)
		return p->lSize;

	if (*(const uint16_t*)c_pbData == GC::WHISPER)
	{
		if (pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			// 귓속말 거부 상태에서 귓속말 거부.
			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;
		// bType 상위 4비트: Empire 번호
		// bType 하위 4비트: EWhisperType
		BYTE bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;
		if(p2->bType == 0x0F) {
			// 시스템 메세지 귓속말은 bType의 상위비트까지 모두 사용함.
			p2->bType = WHISPER_TYPE_SYSTEM;
		} else {
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf + sizeof(TPacketGCWhisper), 
							p2->length - sizeof(TPacketGCWhisper),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));
				}
		}

		pkChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pkChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

int CInputP2P::Notice(LPDESC d, const char * c_pData, size_t uiBytes)
{
	TPacketGGNotice * p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		sys_err("invalid packet length %d", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendNotice(szBuf);
	return (p->lSize);
}

int CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild * p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::instance().FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GuildSub::GG::CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat))
					return -1;

				TPacketGGGuildChat * p = (TPacketGGGuildChat *) c_pData;

				if (g)
					g->P2PChat(p->szText);

				return sizeof(TPacketGGGuildChat);
			}
			
		case GuildSub::GG::SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int))
					return -1;

				int iBonus = *((int *) c_pData);
				CGuild* pGuild = CGuildManager::instance().FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int);
			}
		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}
	return 0;
}


struct FuncShout
{
	const char * m_str;
	BYTE m_bEmpire;

	FuncShout(const char * str, BYTE bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}   

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;

		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char * szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::Shout(const char * c_pData)
{
	TPacketGGShout * p = (TPacketGGShout *) c_pData;
	SendShout(p->szText, p->bEmpire);
}

void CInputP2P::Disconnect(const char * c_pData)
{
	TPacketGGDisconnect * p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char * c_pData)
{
	TPacketGGSetup * p = (TPacketGGSetup *) c_pData;
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerRequestAdd(const char* c_pData)
{
	TPacketGGMessengerRequest* p = (TPacketGGMessengerRequest*)c_pData;
	sys_log(0, "P2P: Messenger: Friend Request from %s to %s", p->account, p->target);

	LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindPC(p->target);
	MessengerManager::Instance().P2PRequestToAdd_Stage2(p->account, tch);
}

void CInputP2P::MessengerResponse(const char* c_pData)
{
    TPacketGGMessengerResponse* p = (TPacketGGMessengerResponse*)c_pData;
    
    LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szRequester);
    
    if (!ch)
        return;
    
    switch (p->bResponseType)
    {
        case 0: // already_sent
            ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You already sent a friend request to %s."), p->szTarget);
            break;
            
        case 1: // already_received_reverse
            ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] %s has already sent you a friend request."), p->szTarget);
            break;
            
        case 2: // quest_running
            ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 친구 추가를 받을 수 없는 상태입니다."));
            break;
			
		case 3: // blocking_requests
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 메신져 추가 거부 상태입니다."));
			break;
    }
}

void CInputP2P::MessengerAdd(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Add %s %s", p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szAccount, p->szCompanion);
	MessengerManager::instance().__AddToList(p->szCompanion, p->szAccount, false);
}

void CInputP2P::MessengerRemove(const char * c_pData)
{
	TPacketGGMessenger * p = (TPacketGGMessenger *) c_pData;
	sys_log(0, "P2P: Messenger Remove %s %s", p->szAccount, p->szCompanion);

    // Send removal packet to the person being removed (deletee)
    LPCHARACTER deletee = CHARACTER_MANAGER::instance().FindPC(p->szCompanion);
	
    if (deletee && deletee->GetDesc())
    {
        TPacketGCMessenger pack;
        pack.header = GC::MESSENGER;
        pack.subheader = MessengerSub::GC::REMOVE_FRIEND;
        pack.length = sizeof(TPacketGCMessenger) + sizeof(BYTE) + strlen(p->szAccount);

        BYTE bLen = strlen(p->szAccount);
        deletee->GetDesc()->BufferedPacket(&pack, sizeof(pack));
        deletee->GetDesc()->BufferedPacket(&bLen, sizeof(BYTE));
        deletee->GetDesc()->Packet(p->szAccount, strlen(p->szAccount));
    }

	// MR-3: Remove from messenger Fix
	MessengerManager::instance().__RemoveFromList(p->szAccount, p->szCompanion);
	// MR-3: -- END OF -- Remove from messenger Fix
}

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwTargetPID);
	if (ch && ch->GetMapIndex() < 10000)
	{
		TPacketGGWarpCharacter pw;
		pw.header = GG::WARP_CHARACTER;
		pw.length = sizeof(pw);
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid);
	if (ch)
	{
		ch->WarpSet(p->x, p->y);
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex * p = (TPacketGGGuildWarMapIndex*) c_pData;
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm.FindGuild(p->dwGuildID1);
	CGuild * g2 = gm.FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char * c_pData)
{
	TPacketGGTransfer * p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::XmasWarpSanta(const char * c_pData)
{
	TPacketGGXmasWarpSanta * p =(TPacketGGXmasWarpSanta *) c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int	iNextSpawnDelay = 60;

		if (LC_IsYMIR())
			iNextSpawnDelay = 20 * 60;
		else
			iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay); // 50분있다가 새로운 산타가 나타남 (한국은 20분)

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.header = GG::XMAS_WARP_SANTA_REPLY;
		pack_reply.length = sizeof(pack_reply);
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::instance().Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*) c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end()) {
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char * c_pData)
{
	TPacketGGLoginPing * p = (TPacketGGLoginPing *) c_pData;

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::instance().Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char * c_pData)
{
	TPacketGGBlockChat * p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->szName);

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->szName, p->lBlockDuration);
	}
}   
// END_OF_BLOCK_CHAT
//

void CInputP2P::IamAwake(LPDESC d, const char * c_pData)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

// Shared function to broadcast mark update to all clients on this core
void BroadcastGuildMarkUpdate(DWORD dwGuildID, WORD wImgIdx)
{
	TPacketGCMarkUpdate packet;
	packet.header = GC::MARK_UPDATE;
	packet.length = sizeof(packet);
	packet.guildID = dwGuildID;
	packet.imgIdx = wImgIdx;

	const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
	for (DESC_MANAGER::DESC_SET::const_iterator it = c_set_desc.begin(); it != c_set_desc.end(); ++it)
	{
		LPDESC pkDesc = *it;
		if (pkDesc && pkDesc->GetCharacter())
		{
			pkDesc->Packet(&packet, sizeof(packet));
		}
	}

	sys_log(0, "BroadcastGuildMarkUpdate: guild %u, imgIdx %u, sent to %zu clients",
		dwGuildID, wImgIdx, c_set_desc.size());
}

void CInputP2P::GuildMarkUpdate(const char * c_pData)
{
	TPacketGGMarkUpdate * p = (TPacketGGMarkUpdate *) c_pData;
	BroadcastGuildMarkUpdate(p->dwGuildID, p->wImgIdx);
}


// Custom adapters for non-standard handler signatures

int CInputP2P::HandleRelay(LPDESC d, const char* c_pData)
{
	return Relay(d, c_pData, m_iBufferLeft);
}

int CInputP2P::HandleNotice(LPDESC d, const char* c_pData)
{
	return Notice(d, c_pData, m_iBufferLeft);
}

int CInputP2P::HandleGuild(LPDESC d, const char* c_pData)
{
	return Guild(d, c_pData, m_iBufferLeft);
}

int CInputP2P::HandleShutdown(LPDESC d, const char*)
{
	sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
	Shutdown(10);
	return 0;
}

int CInputP2P::HandleSiege(LPDESC, const char* c_pData)
{
	TPacketGGSiege* p = (TPacketGGSiege*)c_pData;
	castle_siege(p->bEmpire, p->bTowerCount);
	return 0;
}

int CInputP2P::HandleReloadCRC(LPDESC, const char*)
{
	LoadValidCRCList();
	return 0;
}

int CInputP2P::HandleCheckClientVersion(LPDESC, const char*)
{
	CheckClientVersion();
	return 0;
}

void CInputP2P::RegisterHandlers()
{
	// void(LPDESC, const char*) â€” via DescHandler template
	m_handlers[GG::SETUP]              = &CInputP2P::DescHandler<&CInputP2P::Setup>;
	m_handlers[GG::LOGIN]              = &CInputP2P::DescHandler<&CInputP2P::Login>;
	m_handlers[GG::LOGOUT]             = &CInputP2P::DescHandler<&CInputP2P::Logout>;
	m_handlers[GG::FIND_POSITION]      = &CInputP2P::DescHandler<&CInputP2P::FindPosition>;
	m_handlers[GG::LOGIN_PING]         = &CInputP2P::DescHandler<&CInputP2P::LoginPing>;
	m_handlers[GG::CHECK_AWAKENESS]    = &CInputP2P::DescHandler<&CInputP2P::IamAwake>;

	// void(const char*) â€” via DataHandler template
	m_handlers[GG::SHOUT]              = &CInputP2P::DataHandler<&CInputP2P::Shout>;
	m_handlers[GG::DISCONNECT]         = &CInputP2P::DataHandler<&CInputP2P::Disconnect>;
	m_handlers[GG::MESSENGER_ADD]      = &CInputP2P::DataHandler<&CInputP2P::MessengerAdd>;
	m_handlers[GG::MESSENGER_REMOVE]   = &CInputP2P::DataHandler<&CInputP2P::MessengerRemove>;
	m_handlers[GG::MESSENGER_REQUEST_ADD] = &CInputP2P::DataHandler<&CInputP2P::MessengerRequestAdd>;
	m_handlers[GG::MESSENGER_RESPONSE] = &CInputP2P::DataHandler<&CInputP2P::MessengerResponse>;
	m_handlers[GG::WARP_CHARACTER]     = &CInputP2P::DataHandler<&CInputP2P::WarpCharacter>;
	m_handlers[GG::GUILD_WAR_ZONE_MAP_INDEX] = &CInputP2P::DataHandler<&CInputP2P::GuildWarZoneMapIndex>;
	m_handlers[GG::TRANSFER]           = &CInputP2P::DataHandler<&CInputP2P::Transfer>;
	m_handlers[GG::XMAS_WARP_SANTA]    = &CInputP2P::DataHandler<&CInputP2P::XmasWarpSanta>;
	m_handlers[GG::XMAS_WARP_SANTA_REPLY] = &CInputP2P::DataHandler<&CInputP2P::XmasWarpSantaReply>;
	m_handlers[GG::BLOCK_CHAT]         = &CInputP2P::DataHandler<&CInputP2P::BlockChat>;
	m_handlers[GG::MARK_UPDATE]        = &CInputP2P::DataHandler<&CInputP2P::GuildMarkUpdate>;

	// Custom adapters (variable-length or special signatures)
	m_handlers[GG::RELAY]              = &CInputP2P::HandleRelay;
	m_handlers[GG::NOTICE]             = &CInputP2P::HandleNotice;
	m_handlers[GG::GUILD]              = &CInputP2P::HandleGuild;
	m_handlers[GG::SHUTDOWN]           = &CInputP2P::HandleShutdown;
	m_handlers[GG::SIEGE]              = &CInputP2P::HandleSiege;
	m_handlers[GG::RELOAD_CRC_LIST]    = &CInputP2P::HandleReloadCRC;
	m_handlers[GG::CHECK_CLIENT_VERSION] = &CInputP2P::HandleCheckClientVersion;
}

int CInputP2P::Analyze(LPDESC d, uint16_t wHeader, const char * c_pData)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", wHeader);

	auto it = m_handlers.find(wHeader);
	if (it == m_handlers.end())
	{
		sys_err("CInputP2P::Analyze: unknown header %d (0x%04X) from %s", wHeader, wHeader, d->GetHostName());
		return 0;
	}

	return (this->*(it->second))(d, c_pData);
}
