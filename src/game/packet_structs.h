#pragma once
#include "packet_headers.h"

#pragma pack(1)
typedef struct SPacketGGSetup
{
	uint16_t	header;
	uint16_t	length;
	uint16_t	wPort;
	uint8_t	bChannel;
} TPacketGGSetup;

typedef struct SPacketGGLogin
{
	uint16_t	header;
	uint16_t	length;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	uint32_t	dwPID;
	uint8_t	bEmpire;
	int32_t	lMapIndex;
	uint8_t	bChannel;
} TPacketGGLogin;

typedef struct SPacketGGLogout
{
	uint16_t	header;
	uint16_t	length;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGLogout;

typedef struct SPacketGGRelay
{
	uint16_t	header;
	uint16_t	length;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lSize;
} TPacketGGRelay;

typedef struct SPacketGGNotice
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lSize;
} TPacketGGNotice;

//FORKED_ROAD
typedef struct SPacketGGForkedMapInfo
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bPass;
	uint8_t	bSungzi;
} TPacketGGForkedMapInfo;
//END_FORKED_ROAD
typedef struct SPacketGGShutdown
{
	uint16_t	header;
	uint16_t	length;
} TPacketGGShutdown;

typedef struct SPacketGGGuild
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bSubHeader;
	uint32_t	dwGuild;
} TPacketGGGuild;

typedef struct SPacketGGGuildChat
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bSubHeader;
	uint32_t	dwGuild;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGGuildChat;

typedef struct SPacketGGParty
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	subheader;
	uint32_t	pid;
	uint32_t	leaderpid;
} TPacketGGParty;

typedef struct SPacketGGDisconnect
{
	uint16_t	header;
	uint16_t	length;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGDisconnect;

typedef struct SPacketGGShout
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bEmpire;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGShout;

typedef struct SPacketGGXmasWarpSanta
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bChannel;
	int32_t	lMapIndex;
} TPacketGGXmasWarpSanta;

typedef struct SPacketGGXmasWarpSantaReply
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bChannel;
} TPacketGGXmasWarpSantaReply;

typedef struct SPacketGGMessenger
{
	uint16_t       header;
	uint16_t       length;
	char        szAccount[CHARACTER_NAME_MAX_LEN + 1];
	char        szCompanion[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGMessenger;

typedef struct SPacketGGMessengerRequest
{
	uint16_t	header;
	uint16_t	length;
	char	account[CHARACTER_NAME_MAX_LEN + 1];
	char	target[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGMessengerRequest;

typedef struct SPacketGGMessengerResponse
{
    uint16_t header;
    uint16_t length;
    char szRequester[CHARACTER_NAME_MAX_LEN + 1];
    char szTarget[CHARACTER_NAME_MAX_LEN + 1];
    BYTE bResponseType; // 0=already_sent, 1=already_received_reverse, 2=quest_running, 3=blocking_requests
} TPacketGGMessengerResponse;

typedef struct SPacketGGFindPosition
{
	uint16_t header;
	uint16_t length;
	uint32_t dwFromPID; // 저 위치로 워프하려는 사람
	uint32_t dwTargetPID; // 찾는 사람
} TPacketGGFindPosition;

typedef struct SPacketGGWarpCharacter
{
	uint16_t header;
	uint16_t length;
	uint32_t pid;
	int32_t x;
	int32_t y;
} TPacketGGWarpCharacter;

//  GG::GUILD_WAR_ZONE_MAP_INDEX	    = 15,

typedef struct SPacketGGGuildWarMapIndex
{
	uint16_t header;
	uint16_t length;
	uint32_t dwGuildID1;
	uint32_t dwGuildID2;
	int32_t lMapIndex;
} TPacketGGGuildWarMapIndex;

typedef struct SPacketGGTransfer
{
	uint16_t	header;
	uint16_t	length;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lX, lY;
} TPacketGGTransfer;

typedef struct SPacketGGLoginPing
{
	uint16_t	header;
	uint16_t	length;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGLoginPing;

typedef struct SPacketGGBlockChat
{
	uint16_t	header;
	uint16_t	length;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lBlockDuration;
} TPacketGGBlockChat;

typedef struct packet_gg_mark_update
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwGuildID;
	uint16_t	wImgIdx;
} TPacketGGMarkUpdate;

/* 클라이언트 측에서 보내는 패킷 */

typedef struct command_text
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGText;

typedef struct command_login2
{
	uint16_t	header;
	uint16_t	length;
	char	login[LOGIN_MAX_LEN + 1];
	uint32_t	dwLoginKey;
} TPacketCGLogin2;

typedef struct command_login3
{
	uint16_t	header;
	uint16_t	length;
	char	login[LOGIN_MAX_LEN + 1];
	char	passwd[PASSWD_MAX_LEN + 1];
} TPacketCGLogin3;

typedef struct packet_login_key
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwLoginKey;
} TPacketGCLoginKey;

typedef struct command_player_select
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	index;
} TPacketCGPlayerSelect;

typedef struct command_player_delete
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	index;
	char	private_code[8];
} TPacketCGPlayerDelete;

typedef struct command_player_create
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        index;
	char        name[CHARACTER_NAME_MAX_LEN + 1];
	uint16_t        job;
	uint8_t	shape;
	uint8_t	Con;
	uint8_t	Int;
	uint8_t	Str;
	uint8_t	Dex;
} TPacketCGPlayerCreate;

typedef struct command_player_create_success
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		bAccountCharacterIndex;
	TSimplePlayer	player;
} TPacketGCPlayerCreateSuccess;

// 공격
typedef struct command_attack
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;
	uint32_t	dwVID;
	uint8_t	bCRCMagicCubeProcPiece;
	uint8_t	bCRCMagicCubeFilePiece;
} TPacketCGAttack;

enum EMoveFuncType
{
	FUNC_WAIT,
	FUNC_MOVE,
	FUNC_ATTACK,
	FUNC_COMBO,
	FUNC_MOB_SKILL,
	_FUNC_SKILL,
	FUNC_MAX_NUM,
	FUNC_SKILL = 0x80,
};

// 이동
typedef struct command_move
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bFunc;
	uint8_t	bArg;
	uint8_t	bRot;
	int32_t	lX;
	int32_t	lY;
	uint32_t	dwTime;
} TPacketCGMove;

typedef struct command_sync_position_element
{
	uint32_t	dwVID;
	int32_t	lX;
	int32_t	lY;
} TPacketCGSyncPositionElement;

// 위치 동기화
typedef struct command_sync_position	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGSyncPosition;

/* 채팅 (3) */
typedef struct command_chat	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	type;
} TPacketCGChat;

/* 귓속말 */
typedef struct command_whisper
{
	uint16_t	header;
	uint16_t	length;
	char 	szNameTo[CHARACTER_NAME_MAX_LEN + 1];
} TPacketCGWhisper;

typedef struct command_entergame
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGEnterGame;

typedef struct command_item_use
{
	uint16_t	header;
	uint16_t	length;
	TItemPos 	Cell;
} TPacketCGItemUse;

typedef struct command_item_use_to_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	Cell;
	TItemPos	TargetCell;
} TPacketCGItemUseToItem;

typedef struct command_item_drop
{
	uint16_t	header;
	uint16_t	length;
	TItemPos 	Cell;
	uint32_t	gold;
} TPacketCGItemDrop;

typedef struct command_item_drop2
{
	uint16_t	header;
	uint16_t	length;
	TItemPos 	Cell;
	uint32_t	gold;
	uint8_t	count;
} TPacketCGItemDrop2;

typedef struct command_item_move
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	Cell;
	TItemPos	CellTo;
	uint8_t	count;
} TPacketCGItemMove;

typedef struct command_item_pickup
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
} TPacketCGItemPickup;

typedef struct command_quickslot_add
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
	TQuickslot	slot;
} TPacketCGQuickslotAdd;

typedef struct command_quickslot_del
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
} TPacketCGQuickslotDel;

typedef struct command_quickslot_swap
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
	uint8_t	change_pos;
} TPacketCGQuickslotSwap;

typedef struct command_shop_buy
{
	uint8_t	count;
} TPacketCGShopBuy;

typedef struct command_shop_sell
{
	uint8_t	pos;
	uint8_t	count;
} TPacketCGShopSell;

typedef struct command_shop
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	subheader;
} TPacketCGShop;

typedef struct command_on_click
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
} TPacketCGOnClick;

typedef struct command_exchange
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	sub_header;
	uint32_t	arg1;
	uint8_t	arg2;
	TItemPos	Pos;
} TPacketCGExchange;

typedef struct command_position
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	position;
} TPacketCGPosition;

typedef struct command_script_answer
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	answer;
	//char	file[32 + 1];
	//uint8_t	answer[16 + 1];
} TPacketCGScriptAnswer;


typedef struct command_script_button
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	idx;
} TPacketCGScriptButton;

typedef struct command_quest_input_string
{
	uint16_t	header;
	uint16_t	length;
	char msg[64+1];
} TPacketCGQuestInputString;

typedef struct command_quest_confirm
{
	uint16_t	header;
	uint16_t	length;
	uint8_t answer;
	uint32_t requestPID;
} TPacketCGQuestConfirm;

typedef struct command_quest_cancel
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGQuestCancel;

/*
 * 서버 측에서 보내는 패킷 
 */
typedef struct packet_quest_confirm
{
	uint16_t	header;
	uint16_t	length;
	char msg[64+1]; 
	int32_t timeout;
	uint32_t requestPID;
} TPacketGCQuestConfirm;

typedef struct packet_phase
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	phase;
} TPacketGCPhase;

enum
{
	LOGIN_FAILURE_ALREADY	= 1,
	LOGIN_FAILURE_ID_NOT_EXIST	= 2,
	LOGIN_FAILURE_WRONG_PASS	= 3,
	LOGIN_FAILURE_FALSE		= 4,
	LOGIN_FAILURE_NOT_TESTOR	= 5,
	LOGIN_FAILURE_NOT_TEST_TIME	= 6,
	LOGIN_FAILURE_FULL		= 7
};

typedef struct packet_login_success
{
	uint16_t	header;
	uint16_t	length;
	TSimplePlayer	players[PLAYER_PER_ACCOUNT];
	uint32_t		guild_id[PLAYER_PER_ACCOUNT];
	char		guild_name[PLAYER_PER_ACCOUNT][GUILD_NAME_MAX_LEN+1];

	uint32_t		handle;
	uint32_t		random_key;
} TPacketGCLoginSuccess;

typedef struct packet_auth_success
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwLoginKey;
	uint8_t	bResult;
} TPacketGCAuthSuccess;

typedef struct packet_login_failure
{
	uint16_t	header;
	uint16_t	length;
	char	szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
} TPacketGCLoginFailure;

typedef struct packet_create_failure
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;
} TPacketGCCreateFailure;

// Minimal header-only packet (no payload)
typedef struct packet_gc_blank
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCBlank;

// Character delete success (returns the account slot index that was deleted)
typedef struct packet_player_delete_success
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		account_index;
} TPacketGCDestroyCharacterSuccess;

enum
{
	ADD_CHARACTER_STATE_DEAD		= (1 << 0),
	ADD_CHARACTER_STATE_SPAWN		= (1 << 1),
	ADD_CHARACTER_STATE_GUNGON		= (1 << 2),
	ADD_CHARACTER_STATE_KILLER		= (1 << 3),
	ADD_CHARACTER_STATE_PARTY		= (1 << 4),
};

enum ECharacterEquipmentPart
{
	CHR_EQUIPPART_ARMOR,
	CHR_EQUIPPART_WEAPON,
	CHR_EQUIPPART_HEAD,
	CHR_EQUIPPART_HAIR,
	CHR_EQUIPPART_NUM,
};

typedef struct packet_add_char
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;

	float	angle;
	int32_t	x;
	int32_t	y;
	int32_t	z;

	uint8_t	bType;
	uint16_t	wRaceNum;
	uint8_t	bMovingSpeed;
	uint8_t	bAttackSpeed;

	uint8_t	bStateFlag;
	uint32_t	dwAffectFlag[2];	// 효과
} TPacketGCCharacterAdd;

typedef struct packet_char_additional_info
{
	uint16_t	header;
	uint16_t	length;
	uint32_t   dwVID;
	char    name[CHARACTER_NAME_MAX_LEN + 1];
	uint16_t    awPart[CHR_EQUIPPART_NUM];
	uint8_t	bEmpire;
	uint32_t   dwGuildID;
	uint32_t   dwLevel;
	int16_t	sAlignment;
	uint8_t	bPKMode;
	uint32_t	dwMountVnum;
} TPacketGCCharacterAdditionalInfo;

/*
   typedef struct packet_update_char_old
   {
   uint16_t	header;
   uint16_t	length;
   uint32_t	dwVID;

   uint16_t        awPart[CHR_EQUIPPART_NUM];
   uint8_t	bMovingSpeed;
   uint8_t	bAttackSpeed;

   uint8_t	bStateFlag;
   uint32_t	dwAffectFlag[2];

   uint32_t	dwGuildID;
   int16_t	sAlignment;
   uint8_t	bPKMode;
   uint32_t	dwMountVnum;
   } TPacketGCCharacterUpdateOld;
 */

typedef struct packet_update_char
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;

	uint16_t        awPart[CHR_EQUIPPART_NUM];
	uint8_t	bMovingSpeed;
	uint8_t	bAttackSpeed;

	uint8_t	bStateFlag;
	uint32_t	dwAffectFlag[2];

	uint32_t	dwGuildID;
	int16_t	sAlignment;
	uint8_t	bPKMode;
	uint32_t	dwMountVnum;
	//uint16_t	wRaceNum;
} TPacketGCCharacterUpdate;

typedef struct packet_del_char
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	id;
} TPacketGCCharacterDelete;

typedef struct packet_chat	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	type;
	uint32_t	id;
	uint8_t	bEmpire;
} TPacketGCChat;

typedef struct packet_whisper	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;
	char	szNameFrom[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCWhisper;

typedef struct packet_main_character
{
	enum { MUSIC_NAME_LEN = 24 };

	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	uint16_t	wRaceNum;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	char	szBGMName[MUSIC_NAME_LEN + 1];
	float	fBGMVol;
	int32_t	lx, ly, lz;
	uint8_t	empire;
	uint8_t	skill_group;
} TPacketGCMainCharacter;

typedef struct packet_points
{
	uint16_t	header;
	uint16_t	length;
	int32_t		points[POINT_MAX_NUM];
} TPacketGCPoints;

typedef struct packet_skill_level
{
	uint16_t	header;
	uint16_t	length;
	TPlayerSkill	skills[SKILL_MAX_NUM];
} TPacketGCSkillLevel;

typedef struct packet_point_change
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	uint8_t	type;
	int32_t	amount;
	int32_t	value;
} TPacketGCPointChange;

typedef struct packet_stun
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
} TPacketGCStun;

typedef struct packet_dead
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
} TPacketGCDead;

typedef struct packet_del_item
{
	uint16_t	header;
	uint16_t	length;
	TItemPos	pos;
} TPacketGCItemDel;

typedef struct packet_item_set
{
	uint16_t	header;
	uint16_t	length;
	TItemPos				pos;
	uint32_t				vnum;
	uint8_t					count;
	uint32_t				flags;
	uint32_t				anti_flags;
	uint8_t					highlight;
	int32_t					alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute	aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TPacketGCItemSet;

typedef struct packet_item_get
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwItemVnum;
	uint8_t		bCount;
	uint8_t		bArg;		// 0: normal, 1: from party member (need extra handling)
	char		szFromName[CHARACTER_NAME_MAX_LEN + 1];	// party member name when bArg == 1
} TPacketGCItemGet;

struct packet_item_use
{
	uint16_t	header;
	uint16_t	length;
	TItemPos Cell;
	uint32_t	ch_vid;
	uint32_t	victim_vid;
	uint32_t	vnum;
};

struct packet_item_move
{
	uint16_t	header;
	uint16_t	length;
	TItemPos Cell;
	TItemPos CellTo;
};

typedef struct packet_item_update
{
	uint16_t	header;
	uint16_t	length;
	TItemPos Cell;
	uint8_t	count;
	int32_t	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
} TPacketGCItemUpdate;

typedef struct packet_item_ground_add
{
	uint16_t	header;
	uint16_t	length;
	int32_t 	x, y, z;
	uint32_t	dwVID;
	uint32_t	dwVnum;
} TPacketGCItemGroundAdd;

typedef struct packet_item_ownership
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGCItemOwnership;

typedef struct packet_item_ground_del
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
} TPacketGCItemGroundDel;

struct packet_quickslot_add
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
	TQuickslot	slot;
};

struct packet_quickslot_del
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
};

struct packet_quickslot_swap
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
	uint8_t	pos_to;
};

struct packet_motion
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint32_t	victim_vid;
	uint16_t	motion;
};

struct packet_shop_item
{   
	uint32_t       vnum;
	uint32_t       price;
	uint8_t        count;
	uint8_t		display_pos;
	int32_t	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

typedef struct packet_shop_start
{
	uint32_t   owner_vid;
	struct packet_shop_item	items[SHOP_HOST_ITEM_MAX_NUM];
} TPacketGCShopStart;

typedef struct packet_shop_start_ex // 다음에 TSubPacketShopTab* shop_tabs 이 따라옴.
{
	typedef struct sub_packet_shop_tab 
	{
		char name[SHOP_TAB_NAME_MAX];
		uint8_t coin_type;
		packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
	} TSubPacketShopTab;
	uint32_t owner_vid;
	uint8_t shop_tab_count;
} TPacketGCShopStartEx;

typedef struct packet_shop_update_item
{
	uint8_t			pos;
	struct packet_shop_item	item;
} TPacketGCShopUpdateItem;

typedef struct packet_shop_update_price
{
	int32_t				iPrice;
} TPacketGCShopUpdatePrice;

typedef struct packet_shop	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        subheader;
} TPacketGCShop;

struct packet_exchange
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	sub_header;
	uint8_t	is_me;
	uint32_t	arg1;	// vnum
	TItemPos	arg2;	// cell
	uint32_t	arg3;	// count
	int32_t	alSockets[ITEM_SOCKET_MAX_NUM];
	TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
};

struct packet_position
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint8_t	position;
};

typedef struct packet_ping
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	server_time;
} TPacketGCPing;

typedef struct packet_pong
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGPong;

struct packet_script
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	skin;
	uint16_t	src_size;
};

typedef struct packet_change_speed
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		vid;
	uint16_t		moving_speed;
} TPacketGCChangeSpeed;

struct packet_mount
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint32_t	mount_vid;
	uint8_t	pos;
	uint32_t	x, y;
};

typedef struct packet_move
{	
	uint16_t	header;
	uint16_t	length;
	uint8_t		bFunc;
	uint8_t		bArg;
	uint8_t		bRot;
	uint32_t		dwVID;
	int32_t		lX;
	int32_t		lY;
	uint32_t		dwTime;
	uint32_t		dwDuration;
} TPacketGCMove;

// 소유권
typedef struct packet_ownership
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		dwOwnerVID;
	uint32_t		dwVictimVID;
} TPacketGCOwnership;

// 위치 동기화 패킷의 bCount 만큼 붙는 단위
typedef struct packet_sync_position_element
{
	uint32_t	dwVID;
	int32_t	lX;
	int32_t	lY;
} TPacketGCSyncPositionElement;

// 위치 동기화
typedef struct packet_sync_position	// 가변 패킷
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCSyncPosition;

typedef struct packet_fly
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bType;
	uint32_t	dwStartVID;
	uint32_t	dwEndVID;
} TPacketGCCreateFly;

typedef struct command_fly_targeting
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		dwTargetVID;
	int32_t		x, y;
} TPacketCGFlyTargeting;

typedef struct packet_fly_targeting
{
	uint16_t	header;
	uint16_t	length;
	uint32_t		dwShooterVID;
	uint32_t		dwTargetVID;
	int32_t		x, y;
} TPacketGCFlyTargeting;

typedef struct packet_shoot
{
	uint16_t	header;
	uint16_t	length;
	uint8_t		bType;
} TPacketCGShoot;

typedef struct packet_duel_start
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCDuelStart;

enum EPVPModes
{
	PVP_MODE_NONE,
	PVP_MODE_AGREE,
	PVP_MODE_FIGHT,
	PVP_MODE_REVENGE
};

typedef struct packet_pvp
{
	uint16_t	header;
	uint16_t	length;
	uint32_t       dwVIDSrc;
	uint32_t       dwVIDDst;
	uint8_t        bMode;	// 0 이면 끔, 1이면 켬
} TPacketGCPVP;

typedef struct command_use_skill
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVnum;
	uint32_t	dwVID;
} TPacketCGUseSkill;

typedef struct command_target
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
} TPacketCGTarget;

typedef struct packet_target
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	uint8_t	bHPPercent;
} TPacketGCTarget;

typedef struct packet_warp
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lX;
	int32_t	lY;
	int32_t	lAddr;
	uint16_t	wPort;
} TPacketGCWarp;

typedef struct command_warp
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGWarp;

struct packet_quest_info
{
	uint16_t	header;
	uint16_t	length;
	uint16_t index;
	uint8_t flag;
};

typedef struct packet_messenger
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
} TPacketGCMessenger;

typedef struct packet_messenger_guild_list
{
	uint8_t connected;
	uint8_t length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildList;

typedef struct packet_messenger_guild_login
{
	uint8_t length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogin;

typedef struct packet_messenger_guild_logout
{
	uint8_t length;

	//char login[LOGIN_MAX_LEN+1];
} TPacketGCMessengerGuildLogout;

typedef struct packet_messenger_list_offline
{
	uint8_t connected; // always 0
	uint8_t length;
} TPacketGCMessengerListOffline;

typedef struct packet_messenger_list_online
{
	uint8_t connected; // always 1
	uint8_t length;
} TPacketGCMessengerListOnline;

typedef struct command_messenger
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
} TPacketCGMessenger;

typedef struct command_messenger_add_by_vid
{
	uint32_t vid;
} TPacketCGMessengerAddByVID;

typedef struct command_messenger_add_by_name
{
	uint8_t length;
	//char login[LOGIN_MAX_LEN+1];
} TPacketCGMessengerAddByName;

typedef struct command_messenger_remove
{
	char login[LOGIN_MAX_LEN+1];
	//uint32_t account;
} TPacketCGMessengerRemove;

typedef struct command_safebox_checkout
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckout;

typedef struct command_safebox_checkin
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bSafePos;
	TItemPos	ItemPos;
} TPacketCGSafeboxCheckin;

///////////////////////////////////////////////////////////////////////////////////
// Party

typedef struct command_party_parameter
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bDistributeMode;
} TPacketCGPartyParameter;

typedef struct paryt_parameter
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bDistributeMode;
} TPacketGCPartyParameter;

typedef struct packet_party_add
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	pid;
	char	name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCPartyAdd;

typedef struct command_party_invite
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
} TPacketCGPartyInvite;

typedef struct packet_party_invite
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	leader_vid;
} TPacketGCPartyInvite;

typedef struct command_party_invite_answer
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	leader_vid;
	uint8_t	accept;
} TPacketCGPartyInviteAnswer;

typedef struct packet_party_update
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	pid;
	uint8_t	role;
	uint8_t	percent_hp;
	int16_t	affects[7];
} TPacketGCPartyUpdate;

typedef struct packet_party_remove
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
} TPacketGCPartyRemove;

typedef struct packet_party_link
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
	uint32_t vid;
} TPacketGCPartyLink;

typedef struct packet_party_unlink
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
	uint32_t vid;
} TPacketGCPartyUnlink;

typedef struct command_party_remove
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
} TPacketCGPartyRemove;

typedef struct command_party_set_state
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
	uint8_t byRole;
	uint8_t flag;
} TPacketCGPartySetState;

enum 
{
	PARTY_SKILL_HEAL = 1,
	PARTY_SKILL_WARP = 2
};

typedef struct command_party_use_skill
{
	uint16_t	header;
	uint16_t	length;
	uint8_t bySkillIndex;
	uint32_t vid;
} TPacketCGPartyUseSkill;

typedef struct packet_safebox_size
{
	uint16_t	header;
	uint16_t	length;
	uint8_t bSize;
} TPacketCGSafeboxSize;

typedef struct packet_safebox_wrong_password
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGSafeboxWrongPassword;

typedef struct command_empire
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bEmpire;
} TPacketCGEmpire;

typedef struct packet_empire
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bEmpire;
} TPacketGCEmpire;

enum
{
	SAFEBOX_MONEY_STATE_SAVE,
	SAFEBOX_MONEY_STATE_WITHDRAW,
};

typedef struct command_safebox_money
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        bState;
	int32_t	lMoney;
} TPacketCGSafeboxMoney;

typedef struct packet_safebox_money_change
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lMoney;
} TPacketGCSafeboxMoneyChange;

// Guild

typedef struct packet_guild
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
} TPacketGCGuild;

typedef struct packet_guild_name_t
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
	uint32_t	guildID;
	char	guildName[GUILD_NAME_MAX_LEN];
} TPacketGCGuildName;

typedef struct packet_guild_war
{
	uint32_t	dwGuildSelf;
	uint32_t	dwGuildOpp;
	uint8_t	bType;
	uint8_t 	bWarState;
} TPacketGCGuildWar;

typedef struct command_guild
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
} TPacketCGGuild;

typedef struct command_guild_answer_make_guild
{
	uint16_t	header;
	uint16_t	length;
	char guild_name[GUILD_NAME_MAX_LEN+1];
} TPacketCGAnswerMakeGuild;

typedef struct command_guild_use_skill
{
	uint32_t	dwVnum;
	uint32_t	dwPID;
} TPacketCGGuildUseSkill;

// Guild Mark
typedef struct command_mark_login
{
	uint16_t	header;
	uint16_t	length;
	uint32_t   handle;
	uint32_t   random_key;
} TPacketCGMarkLogin;

typedef struct command_mark_upload
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	gid;
	uint8_t	image[16*12*4];
} TPacketCGMarkUpload;

typedef struct command_mark_idxlist
{
	uint16_t	header;
	uint16_t	length;
} TPacketCGMarkIDXList;

typedef struct command_mark_crclist
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	imgIdx;
	uint32_t	crclist[80];
} TPacketCGMarkCRCList;

typedef struct packet_mark_idxlist
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	bufSize;
	uint16_t	count;
	//뒤에 size * (uint16_t + uint16_t)만큼 데이터 붙음
} TPacketGCMarkIDXList;

typedef struct packet_mark_block
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	bufSize;
	uint8_t	imgIdx;
	uint32_t	count;
	// 뒤에 64 x 48 x 픽셀크기(4바이트) = 12288만큼 데이터 붙음
} TPacketGCMarkBlock;

typedef struct packet_mark_update
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	guildID;
	uint16_t	imgIdx;
} TPacketGCMarkUpdate;

typedef struct command_symbol_upload
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	guild_id;
} TPacketCGGuildSymbolUpload;

typedef struct command_symbol_crc
{
	uint16_t	header;
	uint16_t	length;
	uint32_t guild_id;
	uint32_t crc;
	uint32_t size;
} TPacketCGSymbolCRC;

typedef struct packet_symbol_data
{
	uint16_t	header;
	uint16_t	length;
	uint32_t guild_id;
} TPacketGCGuildSymbolData;

// Fishing

typedef struct command_fishing
{
	uint16_t	header;
	uint16_t	length;
	uint8_t dir;
} TPacketCGFishing;

typedef struct packet_fishing
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
	uint32_t info;
	uint8_t dir;
} TPacketGCFishing;

typedef struct command_give_item
{
	uint16_t	header;
	uint16_t	length;
	uint32_t dwTargetVID;
	TItemPos ItemPos;
	uint8_t byItemCount;
} TPacketCGGiveItem;

typedef struct SPacketCGHack
{
	uint16_t	header;
	uint16_t	length;
	char	szBuf[255 + 1];
} TPacketCGHack;

typedef struct packet_dungeon
{
	uint16_t	header;
	uint16_t	length;
	uint8_t subheader;
} TPacketGCDungeon;

typedef struct packet_dungeon_dest_position
{
	int32_t x;
	int32_t y;
} TPacketGCDungeonDestPosition;

typedef struct SPacketGCShopSign
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwVID;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
} TPacketGCShopSign;

typedef struct SPacketCGMyShop
{
	uint16_t	header;
	uint16_t	length;
	char	szSign[SHOP_SIGN_MAX_LEN + 1];
	uint8_t	bCount;
} TPacketCGMyShop;

typedef struct SPacketGCTime
{
	uint16_t	header;
	uint16_t	length;
	time_t	time;
} TPacketGCTime;

enum
{
	WALKMODE_RUN,
	WALKMODE_WALK,
};

typedef struct SPacketGCWalkMode
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	vid;
	uint8_t	mode;
} TPacketGCWalkMode;

typedef struct SPacketGCChangeSkillGroup
{
	uint16_t	header;
	uint16_t	length;
	uint8_t        skill_group;
} TPacketGCChangeSkillGroup;

typedef struct SPacketCGRefine
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
	uint8_t	type;
} TPacketCGRefine;

typedef struct SPacketCGRequestRefineInfo
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	pos;
} TPacketCGRequestRefineInfo;

typedef struct SPacketGCRefineInformaion
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	type;
	uint8_t	pos;
	uint32_t	src_vnum;
	uint32_t	result_vnum;
	uint8_t	material_count;
	int32_t		cost; // 소요 비용
	int32_t		prob; // 확률
	TRefineMaterial materials[REFINE_MATERIAL_MAX_NUM];
} TPacketGCRefineInformation;

struct TNPCPosition
{
	uint8_t bType;
	uint32_t dwVnum;
	char name[CHARACTER_NAME_MAX_LEN+1];
	int32_t x;
	int32_t y;
};

typedef struct SPacketGCNPCPosition
{
	uint16_t	header;
	uint16_t	length;
	uint16_t count;

	// array of TNPCPosition
} TPacketGCNPCPosition;

typedef struct SPacketGCSpecialEffect
{
	uint16_t	header;
	uint16_t	length;
	uint8_t type;
	uint32_t vid;
} TPacketGCSpecialEffect;

typedef struct SPacketCGChangeName
{
	uint16_t	header;
	uint16_t	length;
	uint8_t index;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketCGChangeName;

typedef struct SPacketGCChangeName
{
	uint16_t	header;
	uint16_t	length;
	uint32_t pid;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TPacketGCChangeName;


typedef struct command_client_version
{
	uint16_t	header;
	uint16_t	length;
	char filename[32+1];
	char timestamp[32+1];
} TPacketCGClientVersion;

typedef struct packet_channel
{
	uint16_t	header;
	uint16_t	length;
	uint8_t channel;
} TPacketGCChannel;

typedef struct pakcet_view_equip
{
	uint16_t	header;
	uint16_t	length;
	uint32_t vid;
	struct {
		uint32_t	vnum;
		uint8_t	count;
		int32_t	alSockets[ITEM_SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
	} equips[WEAR_MAX_NUM];
} TPacketViewEquip;

typedef struct 
{
	uint32_t	dwID;
	int32_t	x, y;
	int32_t	width, height;
	uint32_t	dwGuildID;
} TLandPacketElement;

typedef struct packet_land_list
{
	uint16_t	header;
	uint16_t	length;
} TPacketGCLandList;

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lID;
	char	szName[32+1];
	uint32_t	dwVID;
	uint8_t	bType;
} TPacketGCTargetCreate;

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lID;
	int32_t	lX, lY;
} TPacketGCTargetUpdate;

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	int32_t	lID;
} TPacketGCTargetDelete;

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	TPacketAffectElement elem;
} TPacketGCAffectAdd;

typedef struct
{
	uint16_t	header;
	uint16_t	length;
	uint32_t	dwType;
	uint8_t	bApplyOn;
} TPacketGCAffectRemove;

typedef struct packet_lover_info
{
	uint16_t	header;
	uint16_t	length;
	char name[CHARACTER_NAME_MAX_LEN + 1];
	uint8_t love_point;
} TPacketGCLoverInfo;

typedef struct packet_love_point_update
{
	uint16_t	header;
	uint16_t	length;
	uint8_t love_point;
} TPacketGCLovePointUpdate;

// MINING
typedef struct packet_dig_motion
{
	uint16_t	header;
	uint16_t	length;
	uint32_t vid;
	uint32_t target_vid;
	uint8_t count;
} TPacketGCDigMotion;
// END_OF_MINING

// SCRIPT_SELECT_ITEM
typedef struct command_script_select_item
{
	uint16_t	header;
	uint16_t	length;
	uint32_t selection;
} TPacketCGScriptSelectItem;
// END_OF_SCRIPT_SELECT_ITEM

typedef struct packet_damage_info
{
	uint16_t	header;
	uint16_t	length;
	uint32_t dwVID;
	uint8_t flag;
	int32_t damage;
} TPacketGCDamageInfo;

typedef struct tag_GGSiege
{
	uint16_t	header;
	uint16_t	length;
	uint8_t	bEmpire;
	uint8_t	bTowerCount;
} TPacketGGSiege;

typedef struct SPacketGGCheckAwakeness
{
	uint16_t header;
	uint16_t length;
} TPacketGGCheckAwakeness;

// Secure authentication packets (libsodium/XChaCha20-Poly1305)
#pragma pack(push, 1)

// Server -> Client: Key exchange challenge
struct TPacketGCKeyChallenge
{
	uint16_t	header; // GC::KEY_CHALLENGE (0xf8)
	uint16_t	length;
	uint8_t server_pk[32];     // Server's X25519 public key
	uint8_t challenge[32];     // Random challenge bytes
	uint32_t server_time;      // Server's current time for client sync
};

// Client -> Server: Key exchange response
struct TPacketCGKeyResponse
{
	uint16_t	header; // CG::KEY_RESPONSE (0xf9)
	uint16_t	length;
	uint8_t client_pk[32];     // Client's X25519 public key
	uint8_t challenge_response[32]; // HMAC(challenge, rx_key)
};

// Server -> Client: Key exchange complete
struct TPacketGCKeyComplete
{
	uint16_t	header; // GC::KEY_COMPLETE (0xf7)
	uint16_t	length;
	uint8_t encrypted_token[32 + 16]; // Session token + Poly1305 tag
	uint8_t nonce[24];         // XChaCha20 nonce
};

// Client -> Server: Secure login
struct TPacketCGLoginSecure
{
	uint16_t	header; // CG::LOGIN_SECURE (0xf6)
	uint16_t	length;
	char name[LOGIN_MAX_LEN + 1];
	char pwd[PASSWD_MAX_LEN + 1];
	uint8_t session_token[32]; // Session token from KeyComplete
};

#pragma pack(pop)

#define MAX_EFFECT_FILE_NAME 128
typedef struct SPacketGCSpecificEffect
{
	uint16_t	header;
	uint16_t	length;
	uint32_t vid;
	char effect_file[MAX_EFFECT_FILE_NAME];
} TPacketGCSpecificEffect;

// 용혼석
enum EDragonSoulRefineWindowRefineType
{
	DragonSoulRefineWindow_UPGRADE,
	DragonSoulRefineWindow_IMPROVEMENT,
	DragonSoulRefineWindow_REFINE,
};

typedef struct SPacketCGDragonSoulRefine
{
	SPacketCGDragonSoulRefine() : header(CG::DRAGON_SOUL_REFINE), length(sizeof(SPacketCGDragonSoulRefine))
	{}
	uint16_t	header;
	uint16_t	length;
	uint8_t bSubType;
	TItemPos ItemGrid[DRAGON_SOUL_REFINE_GRID_SIZE];
} TPacketCGDragonSoulRefine;

typedef struct SPacketGCDragonSoulRefine
{
	SPacketGCDragonSoulRefine() : header(GC::DRAGON_SOUL_REFINE), length(sizeof(SPacketGCDragonSoulRefine))
	{}
	uint16_t	header;
	uint16_t	length;
	uint8_t bSubType;
	TItemPos Pos;
} TPacketGCDragonSoulRefine;

typedef struct SPacketCGStateCheck
{
	uint16_t	header;
	uint16_t	length;
	uint32_t key;	
	uint32_t index;
} TPacketCGStateCheck;

typedef struct SPacketGCStateCheck
{
	uint16_t	header;
	uint16_t	length;
	uint32_t key;
	uint32_t index;
	unsigned char state;
} TPacketGCStateCheck;

#pragma pack()
