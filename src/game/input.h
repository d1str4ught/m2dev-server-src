#ifndef __INC_METIN_II_GAME_INPUT_PROCESSOR__
#define __INC_METIN_II_GAME_INPUT_PROCESSOR__

#include <unordered_map>
#include "packet_info.h"

enum
{
	INPROC_CLOSE,
	INPROC_HANDSHAKE,
	INPROC_LOGIN,
	INPROC_MAIN,
	INPROC_DEAD,
	INPROC_DB,
	INPROC_P2P,
	INPROC_AUTH,
};

void LoginFailure(LPDESC d, const char * c_pszStatus);
void RecordLoginFailure(const char* hostName);
void ClearLoginFailure(const char* hostName);
void BroadcastGuildMarkUpdate(DWORD dwGuildID, WORD wImgIdx);


class CInputProcessor
{
	public:
		CInputProcessor();
		virtual ~CInputProcessor() {};

		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
		virtual BYTE GetType() = 0;

		void BindPacketInfo(CPacketInfo * pPacketInfo);
		void Pong(LPDESC d);
		void Version(LPCHARACTER ch, const char* c_pData);

	protected:
		virtual int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) = 0;

		CPacketInfo * m_pPacketInfo;
		int	m_iBufferLeft;

		CPacketInfoCG 	m_packetInfoCG;
};

class CInputClose : public CInputProcessor
{
	public:
		BYTE	GetType() override { return INPROC_CLOSE; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override { return m_iBufferLeft; }
};

class CInputHandshake : public CInputProcessor
{
	public:
		CInputHandshake();
		~CInputHandshake() override;

		BYTE	GetType() override { return INPROC_HANDSHAKE; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

		using HSHandler = int (CInputHandshake::*)(LPDESC, const char*);
		std::unordered_map<uint16_t, HSHandler> m_handlers;
		void RegisterHandlers();

		int HandleText(LPDESC d, const char* c_pData);
		int HandleMarkLogin(LPDESC d, const char* c_pData);
		int HandleStateChecker(LPDESC d, const char* c_pData);
		int HandlePong(LPDESC d, const char* c_pData);
		int HandleKeyResponse(LPDESC d, const char* c_pData);

	protected:
		void		GuildMarkLogin(LPDESC d, const char* c_pData);

		CPacketInfo *	m_pMainPacketInfo;
};

class CInputLogin : public CInputProcessor
{
	public:
		CInputLogin();
		BYTE	GetType() override { return INPROC_LOGIN; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

		using LoginHandler = int (CInputLogin::*)(LPDESC, const char*);
		std::unordered_map<uint16_t, LoginHandler> m_handlers;
		void RegisterHandlers();

		template<void (CInputLogin::*fn)(LPDESC, const char*)>
		int SimpleHandler(LPDESC d, const char* p) { (this->*fn)(d, p); return 0; }

		int HandlePong(LPDESC d, const char*);
		int HandleMarkLogin(LPDESC d, const char*);
		int HandleGuildSymbolUpload(LPDESC d, const char*);
		int HandleVersion(LPDESC d, const char*);

	protected:
		void		LoginByKey(LPDESC d, const char * data);

		void		CharacterSelect(LPDESC d, const char * data);
		void		CharacterCreate(LPDESC d, const char * data);
		void		CharacterDelete(LPDESC d, const char * data);
		void		Entergame(LPDESC d, const char * data);
		void		Empire(LPDESC d, const char * c_pData);
		void		GuildMarkCRCList(LPDESC d, const char* c_pData);
		// MARK_BUG_FIX
		void		GuildMarkIDXList(LPDESC d, const char* c_pData);
		// END_OF_MARK_BUG_FIX
		void		GuildMarkUpload(LPDESC d, const char* c_pData);
		int			GuildSymbolUpload(LPDESC d, const char* c_pData, size_t uiBytes);
		void		GuildSymbolCRC(LPDESC d, const char* c_pData);
		void		ChangeName(LPDESC d, const char * data);
};

class CInputMain : public CInputProcessor
{
	public:
		CInputMain();
		BYTE	GetType() override { return INPROC_MAIN; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

		// Handler dispatch
		using MainHandler = int (CInputMain::*)(LPDESC, const char*);
		struct HandlerEntry {
			MainHandler handler;
			bool blockInObserverMode;
		};
		std::unordered_map<uint16_t, HandlerEntry> m_handlers;
		virtual void RegisterHandlers();

		// Template adapters for common handler patterns (defined in input_main.cpp)
		template<void (CInputMain::*fn)(LPCHARACTER, const char*)>
		int SimpleHandler(LPDESC d, const char* p);

		template<void (CInputMain::*fn)(LPCHARACTER, const void*)>
		int SimpleHandlerV(LPDESC d, const char* p);

		// Custom adapters for non-standard handler signatures
		int HandlePong(LPDESC d, const char* p);
		int HandleChat(LPDESC d, const char* p);
		int HandleWhisper(LPDESC d, const char* p);
		int HandleMove(LPDESC d, const char* p);
		int HandleAttack(LPDESC d, const char* p);
		int HandleShoot(LPDESC d, const char* p);
		int HandleShop(LPDESC d, const char* p);
		int HandleMessenger(LPDESC d, const char* p);
		int HandleSyncPosition(LPDESC d, const char* p);
		int HandleFlyTargeting(LPDESC d, const char* p);
		int HandleAddFlyTargeting(LPDESC d, const char* p);
		int HandleQuestCancel(LPDESC d, const char* p);
		int HandleSafeboxCheckout(LPDESC d, const char* p);
		int HandleMallCheckout(LPDESC d, const char* p);
		int HandleGuild(LPDESC d, const char* p);
		int HandleMyShop(LPDESC d, const char* p);
		int HandleClientVersion(LPDESC d, const char* p);
		int HandleDragonSoulRefine(LPDESC d, const char* p);

	protected:
		void		Attack(LPCHARACTER ch, const uint16_t header, const char* data);

		int			Whisper(LPCHARACTER ch, const char * data, size_t uiBytes);
		int			Chat(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		ItemUse(LPCHARACTER ch, const char * data);
		void		ItemDrop(LPCHARACTER ch, const char * data);
		void		ItemDrop2(LPCHARACTER ch, const char * data);
		void		ItemMove(LPCHARACTER ch, const char * data);
		void		ItemPickup(LPCHARACTER ch, const char * data);
		void		ItemToItem(LPCHARACTER ch, const char * pcData);
		void		QuickslotAdd(LPCHARACTER ch, const char * data);
		void		QuickslotDelete(LPCHARACTER ch, const char * data);
		void		QuickslotSwap(LPCHARACTER ch, const char * data);
		int			Shop(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		OnClick(LPCHARACTER ch, const char * data);
		void		Exchange(LPCHARACTER ch, const char * data);
		void		Position(LPCHARACTER ch, const char * data);
		void		Move(LPCHARACTER ch, const char * data);
		int			SyncPosition(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		FlyTarget(LPCHARACTER ch, const char * pcData, uint16_t wHeader);
		void		UseSkill(LPCHARACTER ch, const char * pcData);

		void		ScriptAnswer(LPCHARACTER ch, const void * pvData);
		void		ScriptButton(LPCHARACTER ch, const void * pvData);
		void		ScriptSelectItem(LPCHARACTER ch, const void * pvData);

		void		QuestInputString(LPCHARACTER ch, const void * pvData);
		void		QuestConfirm(LPCHARACTER ch, const void* pvData);
		void		QuestCancel(LPCHARACTER ch);
		void		Target(LPCHARACTER ch, const char * pcData);
		void		Warp(LPCHARACTER ch, const char * pcData);
		void		SafeboxCheckin(LPCHARACTER ch, const char * c_pData);
		void		SafeboxCheckout(LPCHARACTER ch, const char * c_pData, bool bMall);
		void		SafeboxItemMove(LPCHARACTER ch, const char * data);
		int			Messenger(LPCHARACTER ch, const char* c_pData, size_t uiBytes);

		void 		PartyInvite(LPCHARACTER ch, const char * c_pData);
		void 		PartyInviteAnswer(LPCHARACTER ch, const char * c_pData);
		void		PartyRemove(LPCHARACTER ch, const char * c_pData);
		void		PartySetState(LPCHARACTER ch, const char * c_pData);
		void		PartyUseSkill(LPCHARACTER ch, const char * c_pData);
		void		PartyParameter(LPCHARACTER ch, const char * c_pData);

		int			Guild(LPCHARACTER ch, const char * data, size_t uiBytes);
		void		AnswerMakeGuild(LPCHARACTER ch, const char* c_pData);

		// Guild sub-handlers (dispatched from Guild())
		int			GuildSub_DepositMoney(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_WithdrawMoney(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_AddMember(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_RemoveMember(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_ChangeGradeName(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_ChangeGradeAuthority(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_Offer(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_ChargeGSP(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_PostComment(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_DeleteComment(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_RefreshComment(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_ChangeMemberGrade(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_UseSkill(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_ChangeMemberGeneral(LPCHARACTER ch, const char* data, size_t uiBytes);
		int			GuildSub_InviteAnswer(LPCHARACTER ch, const char* data, size_t uiBytes);

		void		Fishing(LPCHARACTER ch, const char* c_pData);
		void		ItemGive(LPCHARACTER ch, const char* c_pData);
		void		Hack(LPCHARACTER ch, const char * c_pData);
		int			MyShop(LPCHARACTER ch, const char * c_pData, size_t uiBytes);

		void		Refine(LPCHARACTER ch, const char* c_pData);
};

class CInputDead : public CInputMain
{
	public:
		CInputDead();
		BYTE	GetType() override { return INPROC_DEAD; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;
		void RegisterHandlers() override;
};

class CInputDB : public CInputProcessor
{
public:
	CInputDB();
	bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed) override;
	BYTE GetType() override { return INPROC_DB; }

protected:
	int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

	using DBHandler = int (CInputDB::*)(LPDESC, const char*);
	std::unordered_map<uint16_t, DBHandler> m_handlers;
	void RegisterHandlers();
	LPDESC FindByHandle() const;

	// Template: void handler(const char*) — data-only
	template<void (CInputDB::*fn)(const char*)>
	int DataHandler(LPDESC, const char* p) { (this->*fn)(p); return 0; }

	// Template: void handler(LPDESC, const char*) — desc from FindByHandle
	template<void (CInputDB::*fn)(LPDESC, const char*)>
	int DescHandler(LPDESC, const char* p) { (this->*fn)(FindByHandle(), p); return 0; }

	// Template: void handler(T*) — cast c_pData to typed pointer
	template<class T, void (CInputDB::*fn)(T*)>
	int TypedHandler(LPDESC, const char* p) { (this->*fn)((T*)p); return 0; }

	// Custom adapters for special signatures
	int HandleLoginSuccess(LPDESC, const char*);
	int HandleLoginNotExist(LPDESC, const char*);
	int HandleLoginWrongPasswd(LPDESC, const char*);
	int HandlePlayerCreateFailed(LPDESC, const char*);
	int HandlePlayerCreateAlready(LPDESC, const char*);
	int HandlePlayerDeleteFail(LPDESC, const char*);
	int HandlePlayerLoadFailed(LPDESC, const char*);
	int HandleSafeboxWrongPassword(LPDESC, const char*);
	int HandleGuildSkillRecharge(LPDESC, const char*);
	int HandleGuildWarReserveDelete(LPDESC, const char*);
	int HandleSpareItemIDRange(LPDESC, const char*);
	int HandleHorseName(LPDESC, const char*);
	int HandleMyshopPricelistRes(LPDESC, const char*);
	int HandleRespondChannelStatus(LPDESC, const char*);

protected:
	void		MapLocations(const char * c_pData);
	void		LoginSuccess(DWORD dwHandle, const char *data);
	void		PlayerCreateFailure(LPDESC d, BYTE bType);	// 0 = 일반 실패 1 = 이미 있음
	void		PlayerDeleteSuccess(LPDESC d, const char * data);
	void		PlayerDeleteFail(LPDESC d);
	void		PlayerLoad(LPDESC d, const char* data);
	void		PlayerCreateSuccess(LPDESC d, const char * data);
	void		Boot(const char* data);
	void		QuestLoad(LPDESC d, const char * c_pData);
	void		SafeboxLoad(LPDESC d, const char * c_pData);
	void		SafeboxChangeSize(LPDESC d, const char * c_pData);
	void		SafeboxWrongPassword(LPDESC d);
	void		SafeboxChangePasswordAnswer(LPDESC d, const char* c_pData);
	void		MallLoad(LPDESC d, const char * c_pData);
	void		EmpireSelect(LPDESC d, const char * c_pData);
	void		P2P(const char * c_pData);
	void		ItemLoad(LPDESC d, const char * c_pData);
	void		AffectLoad(LPDESC d, const char * c_pData);

	void		GuildLoad(const char * c_pData);
	void		GuildSkillUpdate(const char* c_pData);
	void		GuildSkillRecharge();
	void		GuildExpUpdate(const char* c_pData);
	void		GuildAddMember(const char* c_pData);
	void		GuildRemoveMember(const char* c_pData);
	void		GuildChangeGrade(const char* c_pData);
	void		GuildChangeMemberData(const char* c_pData);
	void		GuildDisband(const char* c_pData);
	void		GuildLadder(const char* c_pData);
	void		GuildWar(const char* c_pData);
	void		GuildWarScore(const char* c_pData);
	void		GuildSkillUsableChange(const char* c_pData);
	void		GuildMoneyChange(const char* c_pData);
	void		GuildWithdrawMoney(const char* c_pData);
	void		GuildWarReserveAdd(TGuildWarReserve * p);
	void		GuildWarReserveUpdate(TGuildWarReserve * p);
	void		GuildWarReserveDelete(DWORD dwID);
	void		GuildWarBet(TPacketGDGuildWarBet * p);
	void		GuildChangeMaster(TPacketChangeGuildMaster* p);

	void		LoginAlready(LPDESC d, const char * c_pData);

	void		PartyCreate(const char* c_pData);
	void		PartyDelete(const char* c_pData);
	void		PartyAdd(const char* c_pData);
	void		PartyRemove(const char* c_pData);
	void		PartyStateChange(const char* c_pData);
	void		PartySetMemberLevel(const char* c_pData);

	void		Time(const char * c_pData);

	void		ReloadProto(const char * c_pData);
	void		ChangeName(LPDESC d, const char * data);

	void		AuthLogin(LPDESC d, const char * c_pData);
	void		ItemAward(const char * c_pData);

	void		ChangeEmpirePriv(const char* c_pData);
	void		ChangeGuildPriv(const char* c_pData);
	void		ChangeCharacterPriv(const char* c_pData);

	void		MoneyLog(const char* c_pData);

	void		SetEventFlag(const char* c_pData);

	void		CreateObject(const char * c_pData);
	void		DeleteObject(const char * c_pData);
	void		UpdateLand(const char * c_pData);

	void		Notice(const char * c_pData);

	void		MarriageAdd(TPacketMarriageAdd * p);
	void		MarriageUpdate(TPacketMarriageUpdate * p);
	void		MarriageRemove(TPacketMarriageRemove * p);

	void		WeddingRequest(TPacketWeddingRequest* p);
	void		WeddingReady(TPacketWeddingReady* p);
	void		WeddingStart(TPacketWeddingStart* p);
	void		WeddingEnd(TPacketWeddingEnd* p);

	// MYSHOP_PRICE_LIST
	void		MyshopPricelistRes( LPDESC d, const TPacketMyshopPricelistHeader* p );
	// END_OF_MYSHOP_PRICE_LIST

	//RELOAD_ADMIN
	void ReloadAdmin( const char * c_pData );
	//END_RELOAD_ADMIN

	void		DetailLog(const TPacketNeedLoginLogInfo* info);
	void		ItemAwardInformer(TPacketItemAwardInfromer* data);

	void		RespondChannelStatus(LPDESC desc, const char* pcData);

	protected:
		DWORD		m_dwHandle;
};

class CInputP2P : public CInputProcessor
{
	public:
		CInputP2P();
		BYTE	GetType() override { return INPROC_P2P; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

		using P2PHandler = int (CInputP2P::*)(LPDESC, const char*);
		std::unordered_map<uint16_t, P2PHandler> m_handlers;
		void RegisterHandlers();

		template<void (CInputP2P::*fn)(LPDESC, const char*)>
		int DescHandler(LPDESC d, const char* p) { (this->*fn)(d, p); return 0; }

		template<void (CInputP2P::*fn)(const char*)>
		int DataHandler(LPDESC, const char* p) { (this->*fn)(p); return 0; }

		int HandleRelay(LPDESC d, const char*);
		int HandleNotice(LPDESC d, const char*);
		int HandleGuild(LPDESC d, const char*);
		int HandleShutdown(LPDESC d, const char*);
		int HandleSiege(LPDESC d, const char*);
		int HandleReloadCRC(LPDESC d, const char*);
		int HandleCheckClientVersion(LPDESC d, const char*);

	public:
		void		Setup(LPDESC d, const char * c_pData);
		void		Login(LPDESC d, const char * c_pData);
		void		Logout(LPDESC d, const char * c_pData);
		int			Relay(LPDESC d, const char * c_pData, size_t uiBytes);
		int			Notice(LPDESC d, const char * c_pData, size_t uiBytes);
		int			Guild(LPDESC d, const char* c_pData, size_t uiBytes);
		void		Shout(const char * c_pData);
		void		Disconnect(const char * c_pData);
		void		MessengerAdd(const char * c_pData);
		void		MessengerRemove(const char * c_pData);
		void		FindPosition(LPDESC d, const char* c_pData);
		void		WarpCharacter(const char* c_pData);
		void		GuildWarZoneMapIndex(const char* c_pData);
		void		Transfer(const char * c_pData);
		void		XmasWarpSanta(const char * c_pData);
		void		XmasWarpSantaReply(const char * c_pData);
		void		LoginPing(LPDESC d, const char * c_pData);
		void		BlockChat(const char * c_pData);
		void		IamAwake(LPDESC d, const char * c_pData);
		void		MessengerRequestAdd(const char* c_pData);
		void		MessengerResponse(const char* c_pData);
		void		GuildMarkUpdate(const char * c_pData);

	protected:
		CPacketInfoGG 	m_packetInfoGG;
};

class CInputAuth : public CInputProcessor
{
	public:
		CInputAuth();
		BYTE GetType() override { return INPROC_AUTH; }

	protected:
		int	Analyze(LPDESC d, uint16_t wHeader, const char * c_pData) override;

		using AuthHandler = int (CInputAuth::*)(LPDESC, const char*);
		std::unordered_map<uint16_t, AuthHandler> m_handlers;
		void RegisterHandlers();

		int HandlePong(LPDESC d, const char*);
		int HandleLogin3(LPDESC d, const char*);

	public:
		void		Login(LPDESC d, const char * c_pData);

};

#endif /* __INC_METIN_II_GAME_INPUT_PROCESSOR__ */
