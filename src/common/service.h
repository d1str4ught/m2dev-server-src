#ifndef __INC_SERVICE_H__
#define __INC_SERVICE_H__

#define ENABLE_AUTODETECT_INTERNAL_IP
#define ENABLE_PROXY_IP
#define _IMPROVED_PACKET_ENCRYPTION_ // 패킷 암호화 개선
#define __PET_SYSTEM__
#define __UDP_BLOCK__
#endif

/*
This fixes the POS_FIGHTING state, when a character will enter POS_FIGHTING state,
after 10s of no damage dealt, should've been switched to POS_STANDING state.
Files affected: char.cpp, char.h, char_battle.cpp, char_skill.cpp
*/
#ifdef FIX_BATTLE_INACTIVITY_TIMEOUT