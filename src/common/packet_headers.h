#pragma once

#include <cstdint>

//
// Unified packet header constants (uint16_t)
//
// CG/GC framing: [header:2] [length:2] [payload...]
//   - header: one of the CG:: / GC:: constants below
//   - length: total packet size including header+length (minimum 4)
//   - payload: packet-specific data
//
// GG framing: [header:2] [length:2] [payload...]
//   - Same as CG/GC, used for inter-server P2P communication
//
// GD/DG framing: [header:2] [handle:4] [size:4] [payload...]
//   - header: one of the GD:: / DG:: constants below
//   - handle: descriptor handle (identifies the client connection)
//   - size: payload size (NOT including the 10-byte frame)
//   - payload: packet-specific data
//
// Ranges:
//   CG: 0x0006-0x0CFF  (Client -> Game)
//   GC: 0x0007-0x0CFF  (Game -> Client)
//   GG: 0x8000-0x8FFF  (Game <-> Game, P2P)
//   GD: 0x9000-0x90FF  (Game -> DB)
//   DG: 0x9100-0x91FF  (DB -> Game)
//

// Packet header type (used by dispatch/registration systems)
typedef uint16_t TPacketHeader;

// CG/GC minimum packet size: header(2) + length(2)
constexpr uint16_t PACKET_HEADER_SIZE = 4;

// GD/DG frame overhead: header(2) + handle(4) + size(4)
constexpr uint32_t GD_FRAME_HEADER_SIZE = 10;

// ============================================================================
// CG -- Client -> Game
// ============================================================================
namespace CG
{
    // Control
    constexpr uint16_t PONG               = 0x0006;
    constexpr uint16_t KEY_RESPONSE       = 0x000A;
    constexpr uint16_t CLIENT_VERSION     = 0x000D;
    constexpr uint16_t STATE_CHECKER      = 0x000F;
    constexpr uint16_t TEXT               = 0x0011;

    // Authentication
    constexpr uint16_t LOGIN2             = 0x0101;
    constexpr uint16_t LOGIN3             = 0x0102;
    constexpr uint16_t LOGIN_SECURE       = 0x0103;
    constexpr uint16_t EMPIRE             = 0x010A;
    constexpr uint16_t CHANGE_NAME        = 0x010B;

    // Character
    constexpr uint16_t CHARACTER_CREATE   = 0x0201;
    constexpr uint16_t CHARACTER_DELETE   = 0x0202;
    constexpr uint16_t CHARACTER_SELECT   = 0x0203;
    constexpr uint16_t ENTERGAME          = 0x0204;

    // Movement
    constexpr uint16_t MOVE               = 0x0301;
    constexpr uint16_t SYNC_POSITION      = 0x0303;
    constexpr uint16_t WARP               = 0x0305;

    // Combat
    constexpr uint16_t ATTACK             = 0x0401;
    constexpr uint16_t USE_SKILL          = 0x0402;
    constexpr uint16_t SHOOT              = 0x0403;
    constexpr uint16_t FLY_TARGETING      = 0x0404;
    constexpr uint16_t ADD_FLY_TARGETING  = 0x0405;

    // Items
    constexpr uint16_t ITEM_USE           = 0x0501;
    constexpr uint16_t ITEM_DROP          = 0x0502;
    constexpr uint16_t ITEM_DROP2         = 0x0503;
    constexpr uint16_t ITEM_MOVE          = 0x0504;
    constexpr uint16_t ITEM_PICKUP        = 0x0505;
    constexpr uint16_t ITEM_USE_TO_ITEM   = 0x0506;
    constexpr uint16_t ITEM_GIVE          = 0x0507;
    constexpr uint16_t EXCHANGE           = 0x0508;
    constexpr uint16_t QUICKSLOT_ADD      = 0x0509;
    constexpr uint16_t QUICKSLOT_DEL      = 0x050A;
    constexpr uint16_t QUICKSLOT_SWAP     = 0x050B;
    constexpr uint16_t REFINE             = 0x050C;
    constexpr uint16_t DRAGON_SOUL_REFINE = 0x050D;

    // Chat
    constexpr uint16_t CHAT               = 0x0601;
    constexpr uint16_t WHISPER            = 0x0602;
    // Social
    constexpr uint16_t PARTY_INVITE       = 0x0701;
    constexpr uint16_t PARTY_INVITE_ANSWER = 0x0702;
    constexpr uint16_t PARTY_REMOVE       = 0x0703;
    constexpr uint16_t PARTY_SET_STATE    = 0x0704;
    constexpr uint16_t PARTY_USE_SKILL    = 0x0705;
    constexpr uint16_t PARTY_PARAMETER    = 0x0706;
    constexpr uint16_t GUILD              = 0x0720;
    constexpr uint16_t ANSWER_MAKE_GUILD  = 0x0721;
    constexpr uint16_t GUILD_SYMBOL_UPLOAD = 0x0722;
    constexpr uint16_t SYMBOL_CRC         = 0x0723;
    constexpr uint16_t MESSENGER          = 0x0740;

    // Shop / Safebox / Mall
    constexpr uint16_t SHOP               = 0x0801;
    constexpr uint16_t MYSHOP             = 0x0802;
    constexpr uint16_t SAFEBOX_CHECKIN    = 0x0820;
    constexpr uint16_t SAFEBOX_CHECKOUT   = 0x0821;
    constexpr uint16_t SAFEBOX_ITEM_MOVE  = 0x0822;
    constexpr uint16_t MALL_CHECKOUT      = 0x0840;

    // Quest
    constexpr uint16_t SCRIPT_ANSWER      = 0x0901;
    constexpr uint16_t SCRIPT_BUTTON      = 0x0902;
    constexpr uint16_t SCRIPT_SELECT_ITEM = 0x0903;
    constexpr uint16_t QUEST_INPUT_STRING = 0x0904;
    constexpr uint16_t QUEST_CONFIRM      = 0x0905;
    constexpr uint16_t QUEST_CANCEL       = 0x0906;

    // UI / Targeting
    constexpr uint16_t TARGET             = 0x0A01;
    constexpr uint16_t ON_CLICK           = 0x0A02;
    constexpr uint16_t CHARACTER_POSITION = 0x0A60;

    // World
    constexpr uint16_t FISHING            = 0x0B01;
    constexpr uint16_t DUNGEON            = 0x0B02;
    constexpr uint16_t HACK               = 0x0B03;

    // Guild Marks
    constexpr uint16_t MARK_LOGIN         = 0x0C01;
    constexpr uint16_t MARK_CRCLIST       = 0x0C02;
    constexpr uint16_t MARK_UPLOAD        = 0x0C03;
    constexpr uint16_t MARK_IDXLIST       = 0x0C04;
}

// ============================================================================
// GC -- Game -> Client
// ============================================================================
namespace GC
{
    // Control
    constexpr uint16_t PING               = 0x0007;
    constexpr uint16_t PHASE              = 0x0008;
    constexpr uint16_t KEY_CHALLENGE      = 0x000B;
    constexpr uint16_t KEY_COMPLETE       = 0x000C;
    constexpr uint16_t RESPOND_CHANNELSTATUS = 0x0010;

    // Authentication
    constexpr uint16_t LOGIN_SUCCESS3     = 0x0104;
    constexpr uint16_t LOGIN_SUCCESS4     = 0x0105;
    constexpr uint16_t LOGIN_FAILURE      = 0x0106;
    constexpr uint16_t LOGIN_KEY          = 0x0107;
    constexpr uint16_t AUTH_SUCCESS       = 0x0108;
    constexpr uint16_t EMPIRE             = 0x0109;
    constexpr uint16_t CHANGE_NAME        = 0x010C;

    // Character
    constexpr uint16_t CHARACTER_ADD      = 0x0205;
    constexpr uint16_t CHARACTER_ADD2     = 0x0206;
    constexpr uint16_t CHAR_ADDITIONAL_INFO = 0x0207;
    constexpr uint16_t CHARACTER_DEL      = 0x0208;
    constexpr uint16_t CHARACTER_UPDATE   = 0x0209;
    constexpr uint16_t CHARACTER_UPDATE2  = 0x020A;
    constexpr uint16_t CHARACTER_POSITION = 0x020B;
    constexpr uint16_t PLAYER_CREATE_SUCCESS = 0x020C;
    constexpr uint16_t PLAYER_CREATE_FAILURE = 0x020D;
    constexpr uint16_t PLAYER_DELETE_SUCCESS = 0x020E;
    constexpr uint16_t PLAYER_DELETE_WRONG_SOCIAL_ID = 0x020F;
    constexpr uint16_t MAIN_CHARACTER     = 0x0210;
    constexpr uint16_t PLAYER_POINTS      = 0x0214;
    constexpr uint16_t PLAYER_POINT_CHANGE = 0x0215;
    constexpr uint16_t STUN               = 0x0216;
    constexpr uint16_t DEAD               = 0x0217;
    constexpr uint16_t CHANGE_SPEED       = 0x0218;
    constexpr uint16_t WALK_MODE          = 0x0219;
    constexpr uint16_t SKILL_LEVEL        = 0x021A;
    constexpr uint16_t SKILL_LEVEL_NEW    = 0x021B;
    constexpr uint16_t SKILL_COOLTIME_END = 0x021C;
    constexpr uint16_t CHANGE_SKILL_GROUP = 0x021D;
    constexpr uint16_t VIEW_EQUIP         = 0x021E;

    // Movement
    constexpr uint16_t MOVE               = 0x0302;
    constexpr uint16_t SYNC_POSITION      = 0x0304;
    constexpr uint16_t WARP               = 0x0306;
    constexpr uint16_t MOTION             = 0x0307;
    constexpr uint16_t DIG_MOTION         = 0x0308;

    // Combat
    constexpr uint16_t DAMAGE_INFO        = 0x0410;
    constexpr uint16_t FLY_TARGETING      = 0x0411;
    constexpr uint16_t ADD_FLY_TARGETING  = 0x0412;
    constexpr uint16_t CREATE_FLY         = 0x0413;
    constexpr uint16_t PVP                = 0x0414;
    constexpr uint16_t DUEL_START         = 0x0415;

    // Items
    constexpr uint16_t ITEM_DEL           = 0x0510;
    constexpr uint16_t ITEM_SET           = 0x0511;
    constexpr uint16_t ITEM_USE           = 0x0512;
    constexpr uint16_t ITEM_DROP          = 0x0513;
    constexpr uint16_t ITEM_UPDATE        = 0x0514;
    constexpr uint16_t ITEM_GROUND_ADD    = 0x0515;
    constexpr uint16_t ITEM_GROUND_DEL    = 0x0516;
    constexpr uint16_t ITEM_OWNERSHIP     = 0x0517;
    constexpr uint16_t ITEM_GET           = 0x0518;
    constexpr uint16_t QUICKSLOT_ADD      = 0x0519;
    constexpr uint16_t QUICKSLOT_DEL      = 0x051A;
    constexpr uint16_t QUICKSLOT_SWAP     = 0x051B;
    constexpr uint16_t EXCHANGE           = 0x051C;
    constexpr uint16_t REFINE_INFORMATION = 0x051D;
    constexpr uint16_t REFINE_INFORMATION_NEW = 0x051E;
    constexpr uint16_t DRAGON_SOUL_REFINE = 0x051F;

    // Chat
    constexpr uint16_t CHAT               = 0x0603;
    constexpr uint16_t WHISPER            = 0x0604;

    // Social
    constexpr uint16_t PARTY_INVITE       = 0x0710;
    constexpr uint16_t PARTY_ADD          = 0x0711;
    constexpr uint16_t PARTY_UPDATE       = 0x0712;
    constexpr uint16_t PARTY_REMOVE       = 0x0713;
    constexpr uint16_t PARTY_LINK         = 0x0714;
    constexpr uint16_t PARTY_UNLINK       = 0x0715;
    constexpr uint16_t PARTY_PARAMETER    = 0x0716;
    constexpr uint16_t GUILD              = 0x0730;
    constexpr uint16_t REQUEST_MAKE_GUILD = 0x0731;
    constexpr uint16_t SYMBOL_DATA        = 0x0732;
    constexpr uint16_t MESSENGER          = 0x0741;
    constexpr uint16_t LOVER_INFO         = 0x0750;
    constexpr uint16_t LOVE_POINT_UPDATE  = 0x0751;

    // Shop / Safebox / Mall
    constexpr uint16_t SHOP               = 0x0810;
    constexpr uint16_t SHOP_SIGN          = 0x0811;
    constexpr uint16_t SAFEBOX_SET        = 0x0830;
    constexpr uint16_t SAFEBOX_DEL        = 0x0831;
    constexpr uint16_t SAFEBOX_WRONG_PASSWORD = 0x0832;
    constexpr uint16_t SAFEBOX_SIZE       = 0x0833;
    constexpr uint16_t SAFEBOX_MONEY_CHANGE = 0x0834;
    constexpr uint16_t MALL_OPEN          = 0x0841;
    constexpr uint16_t MALL_SET           = 0x0842;
    constexpr uint16_t MALL_DEL           = 0x0843;

    // Quest
    constexpr uint16_t SCRIPT             = 0x0910;
    constexpr uint16_t QUEST_CONFIRM      = 0x0911;
    constexpr uint16_t QUEST_INFO         = 0x0912;

    // UI / Effects / Targeting
    constexpr uint16_t TARGET             = 0x0A10;
    constexpr uint16_t TARGET_UPDATE      = 0x0A11;
    constexpr uint16_t TARGET_DELETE      = 0x0A12;
    constexpr uint16_t TARGET_CREATE_NEW  = 0x0A13;
    constexpr uint16_t AFFECT_ADD         = 0x0A20;
    constexpr uint16_t AFFECT_REMOVE      = 0x0A21;
    constexpr uint16_t SEPCIAL_EFFECT     = 0x0A30;
    constexpr uint16_t SPECIFIC_EFFECT    = 0x0A31;
    constexpr uint16_t MOUNT              = 0x0A40;
    constexpr uint16_t OWNERSHIP          = 0x0A41;
    constexpr uint16_t NPC_POSITION       = 0x0A50;

    // World
    constexpr uint16_t FISHING            = 0x0B10;
    constexpr uint16_t DUNGEON            = 0x0B11;
    constexpr uint16_t LAND_LIST          = 0x0B12;
    constexpr uint16_t TIME               = 0x0B13;
    constexpr uint16_t CHANNEL            = 0x0B14;
    constexpr uint16_t MARK_UPDATE        = 0x0B15;
    constexpr uint16_t OBSERVER_ADD       = 0x0B20;
    constexpr uint16_t OBSERVER_REMOVE    = 0x0B21;
    constexpr uint16_t OBSERVER_MOVE      = 0x0B22;

    // Guild Marks
    constexpr uint16_t MARK_BLOCK         = 0x0C10;
    constexpr uint16_t MARK_IDXLIST       = 0x0C11;
    constexpr uint16_t MARK_DIFF_DATA     = 0x0C12;
}

// ============================================================================
// Phase constants (used by GC::PHASE payload)
// ============================================================================
enum EPhases
{
	PHASE_CLOSE,
	PHASE_HANDSHAKE,
	PHASE_LOGIN,
	PHASE_SELECT,
	PHASE_LOADING,
	PHASE_GAME,
	PHASE_DEAD,

	// Internal / server-side phases
	PHASE_CLIENT_CONNECTING,
	PHASE_DBCLIENT,
	PHASE_P2P,
	PHASE_AUTH,
};

// ============================================================================
// GG -- Game <-> Game (P2P)
// ============================================================================
namespace GG
{
    constexpr uint16_t LOGIN                   = 0x8001;
    constexpr uint16_t LOGOUT                  = 0x8002;
    constexpr uint16_t RELAY                   = 0x8003;
    constexpr uint16_t NOTICE                  = 0x8004;
    constexpr uint16_t SHUTDOWN                = 0x8005;
    constexpr uint16_t GUILD                   = 0x8006;
    constexpr uint16_t DISCONNECT              = 0x8007;
    constexpr uint16_t SHOUT                   = 0x8008;
    constexpr uint16_t SETUP                   = 0x8009;
    constexpr uint16_t MESSENGER_ADD           = 0x800A;
    constexpr uint16_t MESSENGER_REMOVE        = 0x800B;
    constexpr uint16_t FIND_POSITION           = 0x800C;
    constexpr uint16_t WARP_CHARACTER          = 0x800D;
    constexpr uint16_t GUILD_WAR_ZONE_MAP_INDEX = 0x800F;
    constexpr uint16_t TRANSFER                = 0x8010;
    constexpr uint16_t XMAS_WARP_SANTA         = 0x8011;
    constexpr uint16_t XMAS_WARP_SANTA_REPLY   = 0x8012;
    constexpr uint16_t RELOAD_CRC_LIST         = 0x8013;
    constexpr uint16_t LOGIN_PING              = 0x8014;
    constexpr uint16_t CHECK_CLIENT_VERSION    = 0x8015;
    constexpr uint16_t BLOCK_CHAT              = 0x8016;
    constexpr uint16_t MESSENGER_REQUEST_ADD   = 0x8017;
    constexpr uint16_t MESSENGER_RESPONSE      = 0x8018;
    constexpr uint16_t SIEGE                   = 0x8019;
    constexpr uint16_t CHECK_AWAKENESS         = 0x801C;
    constexpr uint16_t MARK_UPDATE             = 0x801D;
}

// ============================================================================
// GD -- Game -> DB
// ============================================================================
namespace GD
{
    constexpr uint16_t LOGOUT               = 0x9001;
    constexpr uint16_t PLAYER_LOAD           = 0x9002;
    constexpr uint16_t PLAYER_SAVE           = 0x9003;
    constexpr uint16_t PLAYER_CREATE         = 0x9004;
    constexpr uint16_t PLAYER_DELETE         = 0x9005;
    constexpr uint16_t LOGIN_KEY             = 0x9006;
    constexpr uint16_t BOOT                  = 0x9007;
    constexpr uint16_t PLAYER_COUNT          = 0x9008;
    constexpr uint16_t QUEST_SAVE            = 0x9009;
    constexpr uint16_t SAFEBOX_LOAD          = 0x900A;
    constexpr uint16_t SAFEBOX_SAVE          = 0x900B;
    constexpr uint16_t SAFEBOX_CHANGE_SIZE   = 0x900C;
    constexpr uint16_t EMPIRE_SELECT         = 0x900D;
    constexpr uint16_t SAFEBOX_CHANGE_PASSWORD       = 0x900E;
    constexpr uint16_t SAFEBOX_CHANGE_PASSWORD_SECOND = 0x900F;
    constexpr uint16_t DIRECT_ENTER          = 0x9010;

    // Guild
    constexpr uint16_t GUILD_SKILL_UPDATE    = 0x9011;
    constexpr uint16_t GUILD_EXP_UPDATE      = 0x9012;
    constexpr uint16_t GUILD_ADD_MEMBER      = 0x9013;
    constexpr uint16_t GUILD_REMOVE_MEMBER   = 0x9014;
    constexpr uint16_t GUILD_CHANGE_GRADE    = 0x9015;
    constexpr uint16_t GUILD_CHANGE_MEMBER_DATA = 0x9016;
    constexpr uint16_t GUILD_DISBAND         = 0x9017;
    constexpr uint16_t GUILD_WAR             = 0x9018;
    constexpr uint16_t GUILD_WAR_SCORE       = 0x9019;
    constexpr uint16_t GUILD_CREATE          = 0x901A;

    // Items
    constexpr uint16_t ITEM_SAVE             = 0x901B;
    constexpr uint16_t ITEM_DESTROY          = 0x901C;

    // Affects
    constexpr uint16_t ADD_AFFECT            = 0x901D;
    constexpr uint16_t REMOVE_AFFECT         = 0x901E;

    // Misc
    constexpr uint16_t HIGHSCORE_REGISTER    = 0x901F;
    constexpr uint16_t ITEM_FLUSH            = 0x9020;

    // Party
    constexpr uint16_t PARTY_CREATE          = 0x9021;
    constexpr uint16_t PARTY_DELETE          = 0x9022;
    constexpr uint16_t PARTY_ADD             = 0x9023;
    constexpr uint16_t PARTY_REMOVE          = 0x9024;
    constexpr uint16_t PARTY_STATE_CHANGE    = 0x9025;
    constexpr uint16_t PARTY_HEAL_USE        = 0x9026;

    constexpr uint16_t FLUSH_CACHE           = 0x9027;
    constexpr uint16_t RELOAD_PROTO          = 0x9028;
    constexpr uint16_t CHANGE_NAME           = 0x9029;

    // Guild Economy
    constexpr uint16_t GUILD_CHANGE_LADDER_POINT = 0x902B;
    constexpr uint16_t GUILD_USE_SKILL       = 0x902C;
    constexpr uint16_t REQUEST_EMPIRE_PRIV   = 0x902D;
    constexpr uint16_t REQUEST_GUILD_PRIV    = 0x902E;
    constexpr uint16_t MONEY_LOG             = 0x902F;
    constexpr uint16_t GUILD_DEPOSIT_MONEY   = 0x9030;
    constexpr uint16_t GUILD_WITHDRAW_MONEY  = 0x9031;
    constexpr uint16_t GUILD_WITHDRAW_MONEY_GIVE_REPLY = 0x9032;
    constexpr uint16_t REQUEST_CHARACTER_PRIV = 0x9033;
    constexpr uint16_t SET_EVENT_FLAG        = 0x9034;
    constexpr uint16_t PARTY_SET_MEMBER_LEVEL = 0x9035;
    constexpr uint16_t GUILD_WAR_BET         = 0x9036;

    // Land/Building
    constexpr uint16_t CREATE_OBJECT         = 0x9037;
    constexpr uint16_t DELETE_OBJECT         = 0x9038;
    constexpr uint16_t UPDATE_LAND           = 0x9039;

    // Marriage
    constexpr uint16_t MARRIAGE_ADD          = 0x903A;
    constexpr uint16_t MARRIAGE_UPDATE       = 0x903B;
    constexpr uint16_t MARRIAGE_REMOVE       = 0x903C;
    constexpr uint16_t WEDDING_REQUEST       = 0x903D;
    constexpr uint16_t WEDDING_READY         = 0x903E;
    constexpr uint16_t WEDDING_END           = 0x903F;

    // Auth
    constexpr uint16_t AUTH_LOGIN            = 0x9040;
    constexpr uint16_t LOGIN_BY_KEY          = 0x9041;
    constexpr uint16_t MALL_LOAD             = 0x9042;

    // MyShop
    constexpr uint16_t MYSHOP_PRICELIST_UPDATE = 0x9043;
    constexpr uint16_t MYSHOP_PRICELIST_REQ = 0x9044;

    constexpr uint16_t BLOCK_CHAT            = 0x9045;
    constexpr uint16_t HAMMER_OF_TOR         = 0x9046;
    constexpr uint16_t RELOAD_ADMIN          = 0x9047;
    constexpr uint16_t BREAK_MARRIAGE        = 0x9048;

    constexpr uint16_t REQ_CHANGE_GUILD_MASTER = 0x9053;
    constexpr uint16_t REQ_SPARE_ITEM_ID_RANGE = 0x9054;
    constexpr uint16_t UPDATE_HORSE_NAME     = 0x9055;
    constexpr uint16_t REQ_HORSE_NAME        = 0x9056;
    constexpr uint16_t DC                    = 0x9057;
    constexpr uint16_t VALID_LOGOUT          = 0x9058;
    constexpr uint16_t REQUEST_CHARGE_CASH   = 0x9059;
    constexpr uint16_t DELETE_AWARDID        = 0x905A;
    constexpr uint16_t UPDATE_CHANNELSTATUS  = 0x905B;
    constexpr uint16_t REQUEST_CHANNELSTATUS = 0x905C;

    constexpr uint16_t SETUP                 = 0x90FF;
}

// ============================================================================
// DG -- DB -> Game
// ============================================================================
namespace DG
{
    constexpr uint16_t NOTICE                = 0x9101;

    constexpr uint16_t LOGIN_SUCCESS         = 0x9102;
    constexpr uint16_t LOGIN_NOT_EXIST       = 0x9103;
    constexpr uint16_t LOGIN_WRONG_PASSWD    = 0x9104;
    constexpr uint16_t LOGIN_ALREADY         = 0x9105;

    constexpr uint16_t PLAYER_LOAD_SUCCESS   = 0x9106;
    constexpr uint16_t PLAYER_LOAD_FAILED    = 0x9107;
    constexpr uint16_t PLAYER_CREATE_SUCCESS = 0x9108;
    constexpr uint16_t PLAYER_CREATE_ALREADY = 0x9109;
    constexpr uint16_t PLAYER_CREATE_FAILED  = 0x910A;
    constexpr uint16_t PLAYER_DELETE_SUCCESS = 0x910B;
    constexpr uint16_t PLAYER_DELETE_FAILED  = 0x910C;

    constexpr uint16_t ITEM_LOAD             = 0x910D;
    constexpr uint16_t BOOT                  = 0x910E;
    constexpr uint16_t QUEST_LOAD            = 0x910F;

    constexpr uint16_t SAFEBOX_LOAD          = 0x9110;
    constexpr uint16_t SAFEBOX_CHANGE_SIZE   = 0x9111;
    constexpr uint16_t SAFEBOX_WRONG_PASSWORD = 0x9112;
    constexpr uint16_t SAFEBOX_CHANGE_PASSWORD_ANSWER = 0x9113;
    constexpr uint16_t EMPIRE_SELECT         = 0x9114;
    constexpr uint16_t AFFECT_LOAD           = 0x9115;
    constexpr uint16_t MALL_LOAD             = 0x9116;

    constexpr uint16_t DIRECT_ENTER          = 0x9117;

    // Guild
    constexpr uint16_t GUILD_SKILL_UPDATE    = 0x9118;
    constexpr uint16_t GUILD_SKILL_RECHARGE  = 0x9119;
    constexpr uint16_t GUILD_EXP_UPDATE      = 0x911A;

    // Party
    constexpr uint16_t PARTY_CREATE          = 0x911B;
    constexpr uint16_t PARTY_DELETE          = 0x911C;
    constexpr uint16_t PARTY_ADD             = 0x911D;
    constexpr uint16_t PARTY_REMOVE          = 0x911E;
    constexpr uint16_t PARTY_STATE_CHANGE    = 0x911F;
    constexpr uint16_t PARTY_HEAL_USE        = 0x9120;
    constexpr uint16_t PARTY_SET_MEMBER_LEVEL = 0x9121;

    constexpr uint16_t TIME                  = 0x9122;
    constexpr uint16_t ITEM_ID_RANGE         = 0x9123;

    // Guild (continued)
    constexpr uint16_t GUILD_ADD_MEMBER      = 0x9124;
    constexpr uint16_t GUILD_REMOVE_MEMBER   = 0x9125;
    constexpr uint16_t GUILD_CHANGE_GRADE    = 0x9126;
    constexpr uint16_t GUILD_CHANGE_MEMBER_DATA = 0x9127;
    constexpr uint16_t GUILD_DISBAND         = 0x9128;
    constexpr uint16_t GUILD_WAR             = 0x9129;
    constexpr uint16_t GUILD_WAR_SCORE       = 0x912A;
    constexpr uint16_t GUILD_TIME_UPDATE     = 0x912B;
    constexpr uint16_t GUILD_LOAD            = 0x912C;
    constexpr uint16_t GUILD_LADDER          = 0x912D;
    constexpr uint16_t GUILD_SKILL_USABLE_CHANGE = 0x912E;
    constexpr uint16_t GUILD_MONEY_CHANGE    = 0x912F;
    constexpr uint16_t GUILD_WITHDRAW_MONEY_GIVE = 0x9130;

    constexpr uint16_t SET_EVENT_FLAG        = 0x9131;
    constexpr uint16_t GUILD_WAR_RESERVE_ADD = 0x9132;
    constexpr uint16_t GUILD_WAR_RESERVE_DEL = 0x9133;
    constexpr uint16_t GUILD_WAR_BET         = 0x9134;

    constexpr uint16_t RELOAD_PROTO          = 0x9135;
    constexpr uint16_t CHANGE_NAME           = 0x9136;
    constexpr uint16_t AUTH_LOGIN            = 0x9137;

    constexpr uint16_t CHANGE_EMPIRE_PRIV    = 0x9138;
    constexpr uint16_t CHANGE_GUILD_PRIV     = 0x9139;
    constexpr uint16_t MONEY_LOG             = 0x913A;
    constexpr uint16_t CHANGE_CHARACTER_PRIV = 0x913B;

    // Land/Building
    constexpr uint16_t CREATE_OBJECT         = 0x913C;
    constexpr uint16_t DELETE_OBJECT         = 0x913D;
    constexpr uint16_t UPDATE_LAND           = 0x913E;

    // Marriage
    constexpr uint16_t MARRIAGE_ADD          = 0x913F;
    constexpr uint16_t MARRIAGE_UPDATE       = 0x9140;
    constexpr uint16_t MARRIAGE_REMOVE       = 0x9141;
    constexpr uint16_t WEDDING_REQUEST       = 0x9142;
    constexpr uint16_t WEDDING_READY         = 0x9143;
    constexpr uint16_t WEDDING_START         = 0x9144;
    constexpr uint16_t WEDDING_END           = 0x9145;

    constexpr uint16_t MYSHOP_PRICELIST_RES  = 0x9146;
    constexpr uint16_t RELOAD_ADMIN          = 0x9147;
    constexpr uint16_t BREAK_MARRIAGE        = 0x9148;

    constexpr uint16_t ACK_CHANGE_GUILD_MASTER = 0x9154;
    constexpr uint16_t ACK_SPARE_ITEM_ID_RANGE = 0x9155;
    constexpr uint16_t UPDATE_HORSE_NAME     = 0x9156;
    constexpr uint16_t ACK_HORSE_NAME        = 0x9157;
    constexpr uint16_t NEED_LOGIN_LOG        = 0x9158;
    constexpr uint16_t RESULT_CHARGE_CASH    = 0x9159;
    constexpr uint16_t ITEMAWARD_INFORMER    = 0x915A;
    constexpr uint16_t RESPOND_CHANNELSTATUS = 0x915B;

    constexpr uint16_t MAP_LOCATIONS         = 0x91FE;
    constexpr uint16_t P2P                   = 0x91FF;
}

// ============================================================================
// Subheader enums — grouped by feature
// ============================================================================

namespace GuildSub {
    namespace GG { enum : uint8_t {
        CHAT,
        SET_MEMBER_COUNT_BONUS,
    }; }
    namespace CG { enum : uint8_t {
        ADD_MEMBER,
        REMOVE_MEMBER,
        CHANGE_GRADE_NAME,
        CHANGE_GRADE_AUTHORITY,
        OFFER,
        POST_COMMENT,
        DELETE_COMMENT,
        REFRESH_COMMENT,
        CHANGE_MEMBER_GRADE,
        USE_SKILL,
        CHANGE_MEMBER_GENERAL,
        GUILD_INVITE_ANSWER,
        CHARGE_GSP,
        DEPOSIT_MONEY,
        WITHDRAW_MONEY,
    }; }
    namespace GC { enum : uint8_t {
        LOGIN,
        LOGOUT,
        LIST,
        GRADE,
        ADD,
        REMOVE,
        GRADE_NAME,
        GRADE_AUTH,
        INFO,
        COMMENTS,
        CHANGE_EXP,
        CHANGE_MEMBER_GRADE,
        SKILL_INFO,
        CHANGE_MEMBER_GENERAL,
        GUILD_INVITE,
        WAR,
        GUILD_NAME,
        GUILD_WAR_LIST,
        GUILD_WAR_END_LIST,
        WAR_SCORE,
        MONEY_CHANGE,
    }; }
}

namespace ShopSub {
    namespace CG { enum : uint8_t {
        END,
        BUY,
        SELL,
        SELL2,
    }; }
    namespace GC { enum : uint8_t {
        START,
        END,
        UPDATE_ITEM,
        UPDATE_PRICE,
        OK,
        NOT_ENOUGH_MONEY,
        SOLDOUT,
        INVENTORY_FULL,
        INVALID_POS,
        SOLD_OUT,
        START_EX,
        NOT_ENOUGH_MONEY_EX,
    }; }
}

namespace ExchangeSub {
    namespace CG { enum : uint8_t {
        START,
        ITEM_ADD,
        ITEM_DEL,
        ELK_ADD,
        ACCEPT,
        CANCEL,
    }; }
    namespace GC { enum : uint8_t {
        START,
        ITEM_ADD,
        ITEM_DEL,
        GOLD_ADD,
        ACCEPT,
        END,
        ALREADY,
        LESS_GOLD,
    }; }
}

namespace MessengerSub {
    namespace CG { enum : uint8_t {
        ADD_BY_VID,
        ADD_BY_NAME,
        REMOVE,
        INVITE_ANSWER,
    }; }
    namespace GC { enum : uint8_t {
        LIST,
        LOGIN,
        LOGOUT,
        INVITE,
        REMOVE_FRIEND,
    }; }
}

namespace FishingSub {
    namespace GC { enum : uint8_t {
        START,
        STOP,
        REACT,
        SUCCESS,
        FAIL,
        FISH,
    }; }
}

namespace DungeonSub {
    namespace GC { enum : uint8_t {
        TIME_ATTACK_START = 0,
        DESTINATION_POSITION = 1,
    }; }
}

namespace PartySub {
    namespace GG { enum : uint8_t {
        CREATE,
        DESTROY,
        JOIN,
        QUIT,
    }; }
}

namespace DragonSoulSub { enum : uint8_t {
    OPEN,
    CLOSE,
    DO_REFINE_GRADE,
    DO_REFINE_STEP,
    DO_REFINE_STRENGTH,
    REFINE_FAIL,
    REFINE_FAIL_MAX_REFINE,
    REFINE_FAIL_INVALID_MATERIAL,
    REFINE_FAIL_NOT_ENOUGH_MONEY,
    REFINE_FAIL_NOT_ENOUGH_MATERIAL,
    REFINE_FAIL_TOO_MUCH_MATERIAL,
    REFINE_SUCCEED,
}; }
