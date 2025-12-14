#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "messenger_manager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "log.h"
#include "config.h"
#include "p2p.h"
#include "crc32.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"

#ifdef FIX_MESSENGER_ACTION_SYNC
static char __account[CHARACTER_NAME_MAX_LEN * 2 + 1];
static char __companion[CHARACTER_NAME_MAX_LEN * 2 + 1];
#endif

MessengerManager::MessengerManager()
{
}

MessengerManager::~MessengerManager()
{
}

void MessengerManager::Initialize()
{
}

void MessengerManager::Destroy()
{
}

void MessengerManager::P2PLogin(MessengerManager::keyA account)
{
	Login(account);
}

void MessengerManager::P2PLogout(MessengerManager::keyA account)
{
	Logout(account);
}

void MessengerManager::Login(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) != m_set_loginAccount.end())
		return;

#ifdef FIX_MESSENGER_ACTION_SYNC
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());

	if (account.compare(__account))
		return;
#endif

	DBManager::instance().FuncQuery(std::bind(&MessengerManager::LoadList, this, std::placeholders::_1),
			"SELECT account, companion FROM messenger_list%s WHERE account='%s'", get_table_postfix(), account.c_str());

	m_set_loginAccount.insert(account);
}

void MessengerManager::LoadList(SQLMsg * msg)
{
	if (NULL == msg)
		return;

	if (NULL == msg->Get())
		return;

	if (msg->Get()->uiNumRows == 0)
		return;

	std::string account;

	sys_log(1, "Messenger::LoadList");

	for (uint i = 0; i < msg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && row[1])
		{
			if (account.length() == 0)
				account = row[0];

			m_Relation[row[0]].insert(row[1]);
			m_InverseRelation[row[1]].insert(row[0]);
		}
	}

	SendList(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
		SendLogin(*it, account);
}

void MessengerManager::Logout(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) == m_set_loginAccount.end())
		return;

	m_set_loginAccount.erase(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
	{
		SendLogout(*it, account);
	}

	std::map<keyT, std::set<keyT> >::iterator it2 = m_Relation.begin();

	while (it2 != m_Relation.end())
	{
		it2->second.erase(account);
		++it2;
	}

	m_Relation.erase(account);
	//m_map_stMobile.erase(account);

#ifdef FIX_MESSENGER_ACTION_SYNC
	// remove any pending requests from/to this account so they don't get stuck
	EraseRequestsForAccount(account);
#endif
}

#ifdef CROSS_CHANNEL_FRIEND_REQUEST
void MessengerManager::RegisterRequestToAdd(const char* name, const char* targetName)
{
	uint32_t dw1 = GetCRC32(name, strlen(name));
	uint32_t dw2 = GetCRC32(targetName, strlen(targetName));

	char buf[64]{ 0, };
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	buf[63] = '\0';

	uint32_t dwComplex = GetCRC32(buf, strlen(buf));

#ifdef FIX_MESSENGER_ACTION_SYNC
	// Check if this requester already sent the same request
	if (m_set_requestToAdd.find(dwComplex) != m_set_requestToAdd.end())
	{
		// Send P2P response back to requester's core
        TPacketGGMessengerResponse p2pResp{};
        p2pResp.bHeader = HEADER_GG_MESSENGER_RESPONSE;
        strlcpy(p2pResp.szRequester, name, sizeof(p2pResp.szRequester));
        strlcpy(p2pResp.szTarget, targetName, sizeof(p2pResp.szTarget));
        p2pResp.bResponseType = 0; // already_sent
        P2P_MANAGER::Instance().Send(&p2pResp, sizeof(TPacketGGMessengerResponse));
        return;
	}

    // Clear any old incoming requests for this target before adding new one
    EraseIncomingRequestsForTarget(targetName);
	RegisterRequestComplex(dw1, dw2, dwComplex);
#else
	m_set_requestToAdd.insert(dwComplex);
#endif
}

// stage 1: starts on the core where "ch" resides. Validate ch and move to stage 2
void MessengerManager::P2PRequestToAdd_Stage1(LPCHARACTER ch, const char* targetName)
{
	LPCHARACTER pkTarget = CHARACTER_MANAGER::Instance().FindPC(targetName);

	uint32_t dw1 = GetCRC32(ch->GetName(), strlen(ch->GetName()));
	uint32_t dw2 = GetCRC32(targetName, strlen(targetName));

	char buf[64]{ 0, };
	snprintf(buf, sizeof(buf), "%u:%u", dw2, dw1);
	buf[63] = '\0';

	uint32_t dwComplex = GetCRC32(buf, strlen(buf));

	// Check if target already sent a request to requester (reverse)
	if (m_set_requestToAdd.find(dwComplex) != m_set_requestToAdd.end())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] %s has already sent you a friend request."), targetName);
		return;
	}

	if (!pkTarget)
	{
		if (!ch || !ch->IsPC())
			return;

		if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 친구 추가를 받을 수 없는 상태입니다."));
			return;
		}

		TPacketGGMessengerRequest p2pp{};
		p2pp.header = HEADER_GG_MESSENGER_REQUEST_ADD;
		strlcpy(p2pp.account, ch->GetName(), CHARACTER_NAME_MAX_LEN + 1);
		strlcpy(p2pp.target, targetName, CHARACTER_NAME_MAX_LEN + 1);
		P2P_MANAGER::Instance().Send(&p2pp, sizeof(TPacketGGMessengerRequest));
	}
	else // if we have both, just continue normally
		RequestToAdd(ch, pkTarget);
}

// stage 2: ends up on the core where the target resides
void MessengerManager::P2PRequestToAdd_Stage2(const char* characterName, LPCHARACTER target)
{
	LPCHARACTER ch = CHARACTER_MANAGER::Instance().FindPC(characterName);
	
	if (!target || !target->IsPC())
		return;

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning())
	{
		// Send response back to requester's core
        TPacketGGMessengerResponse p2pResp{};
        p2pResp.bHeader = HEADER_GG_MESSENGER_RESPONSE;
        strlcpy(p2pResp.szRequester, characterName, sizeof(p2pResp.szRequester));
        strlcpy(p2pResp.szTarget, target->GetName(), sizeof(p2pResp.szTarget));
        p2pResp.bResponseType = 2; // quest_running
        P2P_MANAGER::Instance().Send(&p2pResp, sizeof(TPacketGGMessengerResponse));
        return;
	}

	if (target->IsBlockMode(BLOCK_MESSENGER_INVITE))
	{
		// Send response back to requester's core
        TPacketGGMessengerResponse p2pResp{};
        p2pResp.bHeader = HEADER_GG_MESSENGER_RESPONSE;
        strlcpy(p2pResp.szRequester, characterName, sizeof(p2pResp.szRequester));
        strlcpy(p2pResp.szTarget, target->GetName(), sizeof(p2pResp.szTarget));
        p2pResp.bResponseType = 3; // blocking_requests
        P2P_MANAGER::Instance().Send(&p2pResp, sizeof(TPacketGGMessengerResponse));
        return;
	}

	MessengerManager::Instance().RegisterRequestToAdd(characterName, target->GetName());
	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", characterName);
}
#endif

void MessengerManager::RequestToAdd(LPCHARACTER ch, LPCHARACTER target)
{
	if (!ch->IsPC() || !target->IsPC())
		return;

	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 친구 추가를 받을 수 없는 상태입니다."));
	    return;
	}

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning() == true)
		return;

	DWORD dw1 = GetCRC32(ch->GetName(), strlen(ch->GetName()));
	DWORD dw2 = GetCRC32(target->GetName(), strlen(target->GetName()));

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

#ifdef FIX_MESSENGER_ACTION_SYNC
	std::string requester = ch->GetName();
	std::string companion = target->GetName();

	char buf2[64];
	snprintf(buf2, sizeof(buf2), "%u:%u", dw2, dw1);
	DWORD dwComplexRev = GetCRC32(buf2, strlen(buf2));

	// In-memory quick check (fast, works if lists are loaded)
	if (IsInList(requester, companion) || IsInList(companion, requester))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You are already friends with %s."), companion.c_str());
		return;
	}

	// Check if this requester already sent the same request
	if (m_set_requestToAdd.find(dwComplex) != m_set_requestToAdd.end())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You already sent a friend request to %s."), companion.c_str());
		return;
	}

	// Check if target already sent a request to requester (reverse)
	if (m_set_requestToAdd.find(dwComplexRev) != m_set_requestToAdd.end())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] %s has already sent you a friend request."), companion.c_str());

		return;
	}

    // Clear any old incoming requests for this target before adding new one
    EraseIncomingRequestsForTarget(target->GetName());
	// register complex indexed mappings so we can erase them on disconnect
	RegisterRequestComplex(dw1, dw2, dwComplex);
#else
	m_set_requestToAdd.insert(dwComplex);
#endif

	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", ch->GetName());
}

// void MessengerManager::AuthToAdd(MessengerManager::keyA account, MessengerManager::keyA companion, bool bDeny)
// {
	// DWORD dw1 = GetCRC32(companion.c_str(), companion.length());
	// DWORD dw2 = GetCRC32(account.c_str(), account.length());

	// char buf[64];
	// snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	// DWORD dwComplex = GetCRC32(buf, strlen(buf));

	// if (m_set_requestToAdd.find(dwComplex) == m_set_requestToAdd.end())
	// {
		// sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", companion.c_str(), account.c_str());
		// return;
	// }

	// m_set_requestToAdd.erase(dwComplex);

	// if (!bDeny)
	// {
		// AddToList(companion, account);
		// AddToList(account, companion);
	// }
// }

bool MessengerManager::AuthToAdd(MessengerManager::keyA account, MessengerManager::keyA companion, bool bDeny)
{
	DWORD dw1 = GetCRC32(companion.c_str(), companion.length());
	DWORD dw2 = GetCRC32(account.c_str(), account.length());

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	if (m_set_requestToAdd.find(dwComplex) == m_set_requestToAdd.end())
	{
		sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", companion.c_str(), account.c_str());
		return false;
	}

#ifndef FIX_MESSENGER_ACTION_SYNC
	m_set_requestToAdd.erase(dwComplex);
#else
	RemoveComplex(dwComplex);

	// In-memory quick check (fast, works if lists are loaded)
	if (IsInList(account, companion) || IsInList(companion, account))
	{
		LPCHARACTER acc_ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

		if (acc_ch)
			acc_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Friends] You are already friends with %s."), companion.c_str());

		return false;
	}
#endif

	if (!bDeny)
	{
		AddToList(companion, account);
		AddToList(account, companion);
	}

	return true;
}

#ifdef FIX_MESSENGER_ACTION_SYNC
void MessengerManager::RegisterRequestComplex(DWORD dw1, DWORD dw2, DWORD dwComplex)
{
	// avoid duplicates
	if (m_set_requestToAdd.find(dwComplex) != m_set_requestToAdd.end())
		return;

	m_set_requestToAdd.insert(dwComplex);
	m_map_requestComplex[dwComplex] = std::make_pair(dw1, dw2);
	m_map_requestsFrom[dw1].insert(dwComplex);
	m_map_requestsTo[dw2].insert(dwComplex);
}

void MessengerManager::RemoveComplex(DWORD dwComplex)
{
	auto it = m_map_requestComplex.find(dwComplex);
	
	if (it == m_map_requestComplex.end())
	{
		m_set_requestToAdd.erase(dwComplex);
		return;
	}

	DWORD dw1 = it->second.first;
	DWORD dw2 = it->second.second;

	// erase complex mapping
	m_map_requestComplex.erase(it);

	// remove from requester index
	auto itFrom = m_map_requestsFrom.find(dw1);
	
	if (itFrom != m_map_requestsFrom.end())
	{
		itFrom->second.erase(dwComplex);
		
		if (itFrom->second.empty())
			m_map_requestsFrom.erase(itFrom);
	}

	// remove from target index
	auto itTo = m_map_requestsTo.find(dw2);
	
	if (itTo != m_map_requestsTo.end())
	{
		itTo->second.erase(dwComplex);
		
		if (itTo->second.empty())
			m_map_requestsTo.erase(itTo);
	}

	// remove from master set
	m_set_requestToAdd.erase(dwComplex);
}

void MessengerManager::EraseRequestsForAccount(keyA account)
{
	DWORD dw = GetCRC32(account.c_str(), account.length());

	std::vector<DWORD> toRemove;

	auto itFrom = m_map_requestsFrom.find(dw);
	
	if (itFrom != m_map_requestsFrom.end())
	{
		for (DWORD c : itFrom->second)
			toRemove.push_back(c);
	}

	auto itTo = m_map_requestsTo.find(dw);
	
	if (itTo != m_map_requestsTo.end())
	{
		for (DWORD c : itTo->second)
			toRemove.push_back(c);
	}

	if (toRemove.empty())
		return;

		// uniqueify to avoid double removals
	std::sort(toRemove.begin(), toRemove.end());
	toRemove.erase(std::unique(toRemove.begin(), toRemove.end()), toRemove.end());

	for (DWORD c : toRemove)
		RemoveComplex(c);
}


void MessengerManager::EraseIncomingRequestsForTarget(const char* targetName)
{
    DWORD dwTarget = GetCRC32(targetName, strlen(targetName));

    std::vector<DWORD> toRemove;

    // Find all requests where this person is the target (incoming requests)
    auto itTo = m_map_requestsTo.find(dwTarget);
    
    if (itTo != m_map_requestsTo.end())
    {
        for (DWORD c : itTo->second)
            toRemove.push_back(c);
    }

    if (toRemove.empty())
        return;

    // Remove all found requests
    for (DWORD c : toRemove)
        RemoveComplex(c);
}

void MessengerManager::__AddToList(MessengerManager::keyA account, MessengerManager::keyA companion, bool isRequester)
#else
void MessengerManager::__AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
#endif
{
	m_Relation[account].insert(companion);
	m_InverseRelation[companion].insert(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

#ifdef FIX_MESSENGER_ACTION_SYNC
	if (d && isRequester)
#else
	if (d)
#endif
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> %s 님을 친구로 추가하였습니다."), companion.c_str());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(companion.c_str());

#ifdef CROSS_CHANNEL_FRIEND_REQUEST
	if (tch || P2P_MANAGER::Instance().Find(companion.c_str()))
#else
	if (tch)
#endif
		SendLogin(account, companion);
	else
		SendLogout(account, companion);
}

void MessengerManager::AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (m_Relation[account].find(companion) != m_Relation[account].end())
		return;

#ifdef FIX_MESSENGER_ACTION_SYNC
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
	DBManager::instance().EscapeString(__companion, sizeof(__companion), companion.c_str(), companion.size());

	if (account.compare(__account) || companion.compare(__companion))
		return;
#endif

	sys_log(0, "Messenger Add %s %s", account.c_str(), companion.c_str());

	DBManager::instance().Query("INSERT INTO messenger_list%s VALUES ('%s', '%s')", 
			get_table_postfix(), account.c_str(), companion.c_str());

	__AddToList(account, companion);

	TPacketGGMessenger p2ppck;

	p2ppck.bHeader = HEADER_GG_MESSENGER_ADD;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

#ifdef FIX_MESSENGER_ACTION_SYNC
void MessengerManager::__RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion, bool isRequester)
#else
void MessengerManager::__RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
#endif
{
	m_Relation[account].erase(companion);
	m_InverseRelation[companion].erase(account);
#ifdef FIX_MESSENGER_ACTION_SYNC
	m_Relation[companion].erase(account);
	m_InverseRelation[account].erase(companion);
#endif

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

#ifdef FIX_MESSENGER_ACTION_SYNC
	if (d && isRequester)
#else
	if (d)
#endif
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> %s 님을 메신저에서 삭제하였습니다."), companion.c_str());

#ifdef FIX_MESSENGER_ACTION_SYNC
	LPCHARACTER tch = CHARACTER_MANAGER::Instance().FindPC(companion.c_str());

	if (tch && tch->GetDesc())
	{
		TPacketGCMessenger p;

		p.header		= HEADER_GC_MESSENGER;
		p.subheader		= MESSENGER_SUBHEADER_GC_REMOVE_FRIEND;
		p.size			= sizeof(TPacketGCMessenger) + sizeof(BYTE) + account.size();

		BYTE bLen		= account.size();
		tch->GetDesc()->BufferedPacket(&p, sizeof(p));
		tch->GetDesc()->BufferedPacket(&bLen, sizeof(BYTE));
		tch->GetDesc()->Packet(account.c_str(), account.size());
	}
#endif
}

bool MessengerManager::IsInList(MessengerManager::keyA account, MessengerManager::keyA companion) // Fix
{
    if (m_Relation.find(account) == m_Relation.end())
        return false;

    if (m_Relation[account].empty())
        return false;

    return m_Relation[account].find(companion) != m_Relation[account].end();
}

void MessengerManager::RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.empty())
		return;
	
	if (companion.size() == 0)
		return;
	
	if (!IsInList(account, companion)) // Fix
		return;
	
#ifdef FIX_MESSENGER_ACTION_SYNC
	DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());
    DBManager::instance().EscapeString(__companion, sizeof(__companion), companion.c_str(), companion.size());
  
    if (account.compare(__account) || companion.compare(__companion))
        return;
#else
	char companionEscaped[CHARACTER_NAME_MAX_LEN * 2 + 1];

	DBManager::instance().EscapeString(companionEscaped, sizeof(companionEscaped), companion.c_str(), companion.length());
#endif

	sys_log(1, "Messenger Remove %s %s", account.c_str(), companion.c_str());
	
	// DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' AND companion = '%s'",
			// get_table_postfix(), account.c_str(), companion.c_str());

	// Fix
#ifdef FIX_MESSENGER_ACTION_SYNC
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE (account='%s' AND companion = '%s') OR (account = '%s' AND companion = '%s')",
			get_table_postfix(), account.c_str(), companion.c_str(), companion.c_str(), account.c_str());
#else
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' AND companion = '%s'",
			get_table_postfix(), account.c_str(), companion.c_str());
#endif

	__RemoveFromList(account, companion);

	TPacketGGMessenger p2ppck;
	
	p2ppck.bHeader = HEADER_GG_MESSENGER_REMOVE;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

void MessengerManager::RemoveAllList(keyA account)
{
	std::set<keyT>	company(m_Relation[account]);

#ifdef FIX_MESSENGER_ACTION_SYNC
    DBManager::instance().EscapeString(__account, sizeof(__account), account.c_str(), account.size());

	if (account.compare(__account))
        	return;
#endif

	/* SQL Data 삭제 */
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' OR companion='%s'",
			get_table_postfix(), account.c_str(), account.c_str());

	/* 내가 가지고있는 리스트 삭제 */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			iter++ )
	{
		this->RemoveFromList(account, *iter);
#ifdef FIX_MESSENGER_ACTION_SYNC
		this->RemoveFromList(*iter, account);
#endif
	}

	/* 복사한 데이타 삭제 */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			)
	{
		company.erase(iter++);
	}

	company.clear();
}

void MessengerManager::SendList(MessengerManager::keyA account)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (m_Relation.find(account) == m_Relation.end())
		return;

	if (m_Relation[account].empty())
		return;

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LIST;
	pack.size		= sizeof(TPacketGCMessenger);

	TPacketGCMessengerListOffline pack_offline;
	TPacketGCMessengerListOnline pack_online;

	TEMP_BUFFER buf(128 * 1024); // 128k

	itertype(m_Relation[account]) it = m_Relation[account].begin(), eit = m_Relation[account].end();

	while (it != eit)
	{
		if (m_set_loginAccount.find(*it) != m_set_loginAccount.end())
		{
			pack_online.connected = 1;

			// Online
			pack_online.length = it->size();

			buf.write(&pack_online, sizeof(TPacketGCMessengerListOnline));
			buf.write(it->c_str(), it->size());
		}
		else
		{
			pack_offline.connected = 0;

			// Offline
			pack_offline.length = it->size();

			buf.write(&pack_offline, sizeof(TPacketGCMessengerListOffline));
			buf.write(it->c_str(), it->size());
		}

		++it;
	}

	pack.size += buf.size();

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->Packet(buf.read_peek(), buf.size());
}

void MessengerManager::SendLogin(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(companion.c_str()) != GM_PLAYER)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header			= HEADER_GC_MESSENGER;
	pack.subheader		= MESSENGER_SUBHEADER_GC_LOGIN;
	pack.size			= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

void MessengerManager::SendLogout(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (!companion.size())
		return;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LOGOUT;
	pack.size		= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

