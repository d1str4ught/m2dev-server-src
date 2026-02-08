#include "stdafx.h"
#include "common/stl.h"
#include "constants.h"
#include "packet_info.h"

CPacketInfo::CPacketInfo()
	: m_pCurrentPacket(NULL), m_dwStartTime(0)
{
}

CPacketInfo::~CPacketInfo()
{
	// unique_ptr handles cleanup automatically
}

void CPacketInfo::Set(int header, int iSize, const char * c_pszName)
{
	if (m_pPacketMap.find(header) != m_pPacketMap.end())
		return;

	auto element = std::make_unique<TPacketElement>();

	element->iSize = iSize;
	element->stName.assign(c_pszName);
	element->iCalled = 0;
	element->dwLoad = 0;

	m_pPacketMap.emplace(header, std::move(element));
}

bool CPacketInfo::Get(int header, int * size, const char ** c_ppszName)
{
	auto it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return false;

	*size = it->second->iSize;
	*c_ppszName = it->second->stName.c_str();

	m_pCurrentPacket = it->second.get();
	return true;
}

TPacketElement * CPacketInfo::GetElement(int header)
{
	auto it = m_pPacketMap.find(header);

	if (it == m_pPacketMap.end())
		return NULL;

	return it->second.get();
}

void CPacketInfo::Start()
{
	assert(m_pCurrentPacket != NULL);
	m_dwStartTime = get_dword_time();
}

void CPacketInfo::End()
{
	++m_pCurrentPacket->iCalled;
	m_pCurrentPacket->dwLoad += get_dword_time() - m_dwStartTime;
}

void CPacketInfo::Log(const char * c_pszFileName)
{
	FILE * fp;

	fp = fopen(c_pszFileName, "w");

	if (!fp)
		return;

	fprintf(fp, "Name             Called     Load       Ratio\n");

	for (auto it = m_pPacketMap.begin(); it != m_pPacketMap.end(); ++it)
	{
		TPacketElement * p = it->second.get();

		fprintf(fp, "%-16s %-10d %-10u %.2f\n",
				p->stName.c_str(),
				p->iCalled,
				p->dwLoad,
				p->iCalled != 0 ? (float) p->dwLoad / p->iCalled : 0.0f);
	}

	fclose(fp);
}
///---------------------------------------------------------

CPacketInfoCG::CPacketInfoCG()
{
	Set(CG::TEXT, sizeof(TPacketCGText), "Text");
	Set(CG::MARK_LOGIN, sizeof(TPacketCGMarkLogin), "MarkLogin");
	Set(CG::MARK_IDXLIST, sizeof(TPacketCGMarkIDXList), "MarkIdxList");
	Set(CG::MARK_CRCLIST, sizeof(TPacketCGMarkCRCList), "MarkCrcList");
	Set(CG::MARK_UPLOAD, sizeof(TPacketCGMarkUpload), "MarkUpload");
	Set(CG::KEY_RESPONSE, sizeof(TPacketCGKeyResponse), "KeyResponse");

	Set(CG::GUILD_SYMBOL_UPLOAD, sizeof(TPacketCGGuildSymbolUpload), "SymbolUpload");
	Set(CG::SYMBOL_CRC, sizeof(TPacketCGSymbolCRC), "SymbolCRC");
	Set(CG::LOGIN2, sizeof(TPacketCGLogin2), "Login2");
	Set(CG::LOGIN3, sizeof(TPacketCGLogin3), "Login3");
	Set(CG::ATTACK, sizeof(TPacketCGAttack), "Attack");
	Set(CG::CHAT, sizeof(TPacketCGChat), "Chat");
	Set(CG::WHISPER, sizeof(TPacketCGWhisper), "Whisper");

	Set(CG::CHARACTER_SELECT, sizeof(TPacketCGPlayerSelect), "Select");
	Set(CG::CHARACTER_CREATE, sizeof(TPacketCGPlayerCreate), "Create");
	Set(CG::CHARACTER_DELETE, sizeof(TPacketCGPlayerDelete), "Delete");
	Set(CG::ENTERGAME, sizeof(TPacketCGEnterGame), "EnterGame");

	Set(CG::ITEM_USE, sizeof(TPacketCGItemUse), "ItemUse");
	Set(CG::ITEM_DROP, sizeof(TPacketCGItemDrop), "ItemDrop");
	Set(CG::ITEM_DROP2, sizeof(TPacketCGItemDrop2), "ItemDrop2");
	Set(CG::ITEM_MOVE, sizeof(TPacketCGItemMove), "ItemMove");
	Set(CG::ITEM_PICKUP, sizeof(TPacketCGItemPickup), "ItemPickup");

	Set(CG::QUICKSLOT_ADD, sizeof(TPacketCGQuickslotAdd), "QuickslotAdd");
	Set(CG::QUICKSLOT_DEL, sizeof(TPacketCGQuickslotDel), "QuickslotDel");
	Set(CG::QUICKSLOT_SWAP, sizeof(TPacketCGQuickslotSwap), "QuickslotSwap");

	Set(CG::SHOP, sizeof(TPacketCGShop), "Shop");

	Set(CG::ON_CLICK, sizeof(TPacketCGOnClick), "OnClick");
	Set(CG::EXCHANGE, sizeof(TPacketCGExchange), "Exchange");
	Set(CG::CHARACTER_POSITION, sizeof(TPacketCGPosition), "Position");
	Set(CG::SCRIPT_ANSWER, sizeof(TPacketCGScriptAnswer), "ScriptAnswer");
	Set(CG::SCRIPT_BUTTON, sizeof(TPacketCGScriptButton), "ScriptButton");
	Set(CG::QUEST_INPUT_STRING, sizeof(TPacketCGQuestInputString), "QuestInputString");
	Set(CG::QUEST_CONFIRM, sizeof(TPacketCGQuestConfirm), "QuestConfirm");
	Set(CG::QUEST_CANCEL, sizeof(TPacketCGQuestCancel), "QuestCancel");

	Set(CG::MOVE, sizeof(TPacketCGMove), "Move");
	Set(CG::SYNC_POSITION, sizeof(TPacketCGSyncPosition), "SyncPosition");

	Set(CG::FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "FlyTarget");
	Set(CG::ADD_FLY_TARGETING, sizeof(TPacketCGFlyTargeting), "AddFlyTarget");
	Set(CG::SHOOT, sizeof(TPacketCGShoot), "Shoot");

	Set(CG::USE_SKILL, sizeof(TPacketCGUseSkill), "UseSkill");

	Set(CG::ITEM_USE_TO_ITEM, sizeof(TPacketCGItemUseToItem), "UseItemToItem");
	Set(CG::TARGET, sizeof(TPacketCGTarget), "Target");
	Set(CG::WARP, sizeof(TPacketCGWarp), "Warp");
	Set(CG::MESSENGER, sizeof(TPacketCGMessenger), "Messenger");

	Set(CG::PARTY_REMOVE, sizeof(TPacketCGPartyRemove), "PartyRemove");
	Set(CG::PARTY_INVITE, sizeof(TPacketCGPartyInvite), "PartyInvite");
	Set(CG::PARTY_INVITE_ANSWER, sizeof(TPacketCGPartyInviteAnswer), "PartyInviteAnswer");
	Set(CG::PARTY_SET_STATE, sizeof(TPacketCGPartySetState), "PartySetState");
	Set(CG::PARTY_USE_SKILL, sizeof(TPacketCGPartyUseSkill), "PartyUseSkill");
	Set(CG::PARTY_PARAMETER, sizeof(TPacketCGPartyParameter), "PartyParam");

	Set(CG::EMPIRE, sizeof(TPacketCGEmpire), "Empire");
	Set(CG::SAFEBOX_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "SafeboxCheckout");
	Set(CG::SAFEBOX_CHECKIN, sizeof(TPacketCGSafeboxCheckin), "SafeboxCheckin");

	Set(CG::SAFEBOX_ITEM_MOVE, sizeof(TPacketCGItemMove), "SafeboxItemMove");

	Set(CG::GUILD, sizeof(TPacketCGGuild), "Guild");
	Set(CG::ANSWER_MAKE_GUILD, sizeof(TPacketCGAnswerMakeGuild), "AnswerMakeGuild");

	Set(CG::FISHING, sizeof(TPacketCGFishing), "Fishing");
	Set(CG::ITEM_GIVE, sizeof(TPacketCGGiveItem), "ItemGive");
	Set(CG::HACK, sizeof(TPacketCGHack), "Hack");
	Set(CG::MYSHOP, sizeof(TPacketCGMyShop), "MyShop");

	Set(CG::REFINE, sizeof(TPacketCGRefine), "Refine");
	Set(CG::CHANGE_NAME, sizeof(TPacketCGChangeName), "ChangeName");

	Set(CG::CLIENT_VERSION, sizeof(TPacketCGClientVersion), "Version");
	Set(CG::PONG, sizeof(TPacketCGPong), "Pong");
	Set(CG::MALL_CHECKOUT, sizeof(TPacketCGSafeboxCheckout), "MallCheckout");

	Set(CG::SCRIPT_SELECT_ITEM, sizeof(TPacketCGScriptSelectItem), "ScriptSelectItem");

	Set(CG::DRAGON_SOUL_REFINE, sizeof(TPacketCGDragonSoulRefine), "DragonSoulRefine");
	Set(CG::STATE_CHECKER, sizeof(TPacketCGStateCheck), "ServerStateCheck");
	
}

CPacketInfoCG::~CPacketInfoCG()
{
	Log("packet_info.txt");
}

////////////////////////////////////////////////////////////////////////////////
CPacketInfoGG::CPacketInfoGG()
{
	Set(GG::SETUP,		sizeof(TPacketGGSetup),		"Setup");
	Set(GG::LOGIN,		sizeof(TPacketGGLogin),		"Login");
	Set(GG::LOGOUT,		sizeof(TPacketGGLogout),	"Logout");
	Set(GG::RELAY,		sizeof(TPacketGGRelay),		"Relay");
	Set(GG::NOTICE,		sizeof(TPacketGGNotice),	"Notice");
	Set(GG::SHUTDOWN,		sizeof(TPacketGGShutdown),	"Shutdown");
	Set(GG::GUILD,		sizeof(TPacketGGGuild),		"Guild");
	Set(GG::SHOUT,		sizeof(TPacketGGShout),		"Shout");
	Set(GG::DISCONNECT,		sizeof(TPacketGGDisconnect),	"Disconnect");
	Set(GG::MESSENGER_ADD,	sizeof(TPacketGGMessenger),	"MessengerAdd");
	Set(GG::MESSENGER_REQUEST_ADD, sizeof(TPacketGGMessengerRequest), "MessengerRequestAdd");
	Set(GG::MESSENGER_RESPONSE, sizeof(TPacketGGMessengerResponse), "MessengerResponse");
	Set(GG::MESSENGER_REMOVE,	sizeof(TPacketGGMessenger),	"MessengerRemove");
	Set(GG::FIND_POSITION,	sizeof(TPacketGGFindPosition),	"FindPosition");
	Set(GG::WARP_CHARACTER,	sizeof(TPacketGGWarpCharacter),	"WarpCharacter");
	Set(GG::GUILD_WAR_ZONE_MAP_INDEX, sizeof(TPacketGGGuildWarMapIndex), "GuildWarMapIndex");
	Set(GG::TRANSFER,		sizeof(TPacketGGTransfer),	"Transfer");
	Set(GG::XMAS_WARP_SANTA,	sizeof(TPacketGGXmasWarpSanta),	"XmasWarpSanta");
	Set(GG::XMAS_WARP_SANTA_REPLY, sizeof(TPacketGGXmasWarpSantaReply), "XmasWarpSantaReply");
	Set(GG::RELOAD_CRC_LIST,	PACKET_HEADER_SIZE,		"ReloadCRCList");
	Set(GG::CHECK_CLIENT_VERSION, PACKET_HEADER_SIZE,		"CheckClientVersion");
	Set(GG::LOGIN_PING,		sizeof(TPacketGGLoginPing),	"LoginPing");
	Set(GG::BLOCK_CHAT,		sizeof(TPacketGGBlockChat),	"BlockChat");
	Set(GG::SIEGE,		sizeof(TPacketGGSiege),		"Siege");
	Set(GG::CHECK_AWAKENESS,	sizeof(TPacketGGCheckAwakeness),	"CheckAwakeness");
	Set(GG::MARK_UPDATE,		sizeof(TPacketGGMarkUpdate),		"MarkUpdate");
}

CPacketInfoGG::~CPacketInfoGG()
{
	Log("p2p_packet_info.txt");
}

