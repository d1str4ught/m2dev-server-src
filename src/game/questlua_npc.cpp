#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "questmanager.h"
#include "char.h"
#include "party.h"
#include "xmas_event.h"
#include "char_manager.h"
#include "shop_manager.h"
#include "guild.h"
#include "mob_manager.h"

namespace quest
{
	//
	// "npc" lua functions
	//
	int npc_open_shop(lua_State * L)
	{
		int iShopVnum = 0;

		if (lua_gettop(L) == 1)
		{
			if (lua_isnumber(L, 1))
				iShopVnum = (int) lua_tonumber(L, 1);
		}

		if (CQuestManager::instance().GetCurrentNPCCharacterPtr())
			CShopManager::instance().StartShopping(CQuestManager::instance().GetCurrentCharacterPtr(), CQuestManager::instance().GetCurrentNPCCharacterPtr(), iShopVnum);
		return 0;
	}

	int npc_is_pc(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc && npc->IsPC())
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
		return 1;
	}

	int npc_get_empire(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushnumber(L, npc->GetEmpire());
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int npc_get_race(lua_State * L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentNPCRace());
		return 1;
	}

	int npc_get_guild(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		CGuild* pGuild = NULL;
		if (npc)
			pGuild = npc->GetGuild();

		lua_pushnumber(L, pGuild ? pGuild->GetID() : 0);
		return 1;
	}

	int npc_get_remain_skill_book_count(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		lua_pushnumber(L, MAX(0, npc->GetPoint(POINT_ATT_GRADE_BONUS)));
		return 1;
	}

	int npc_dec_remain_skill_book_count(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			return 0;
		}
		npc->SetPoint(POINT_ATT_GRADE_BONUS, MAX(0, npc->GetPoint(POINT_ATT_GRADE_BONUS)-1));
		return 0;
	}

	int npc_get_remain_hairdye_count(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		lua_pushnumber(L, MAX(0, npc->GetPoint(POINT_DEF_GRADE_BONUS)));
		return 1;
	}

	int npc_dec_remain_hairdye_count(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || npc->IsPC() || npc->GetRaceNum() != xmas::MOB_SANTA_VNUM)
		{
			return 0;
		}
		npc->SetPoint(POINT_DEF_GRADE_BONUS, MAX(0, npc->GetPoint(POINT_DEF_GRADE_BONUS)-1));
		return 0;
	}

	int npc_is_quest(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (npc)
		{
			const std::string & r_st = q.GetCurrentQuestName();

			if (q.GetQuestIndexByName(r_st) == npc->GetQuestBy())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int npc_kill(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (npc)
		{
			npc->Dead();
		}
		return 0;
	}

	int npc_purge(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (npc)
		{
			M2_DESTROY_CHARACTER(npc);
		}
		return 0;
	}

	int npc_is_near(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 1))
			dist = lua_tonumber(L, 1);

		if (ch == NULL || npc == NULL)
		{
			lua_pushboolean(L, false);
		}
		else
		{
			lua_pushboolean(L, DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist*100);
		}

		return 1;
	}

	int npc_is_near_vid(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid vid");
			lua_pushboolean(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find((DWORD)lua_tonumber(L, 1));
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 2))
			dist = lua_tonumber(L, 2);

		if (ch == NULL || npc == NULL)
		{
			lua_pushboolean(L, false);
		}
		else
		{
			lua_pushboolean(L, DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist*100);
		}

		return 1;
	}

	int npc_unlock(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if ( npc != NULL )
		{
			if (npc->IsPC())
				return 0;

			if (npc->GetQuestNPCID() == ch->GetPlayerID())
			{
				npc->SetQuestNPCID(0);
			}
		}
		return 0;
	}

	int npc_lock(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (!npc || npc->IsPC())
		{
			lua_pushboolean(L, TRUE);
			return 1;
		}

		if (npc->GetQuestNPCID() == 0 || npc->GetQuestNPCID() == ch->GetPlayerID())
		{
			npc->SetQuestNPCID(ch->GetPlayerID());
			lua_pushboolean(L, TRUE);
		}
		else
		{
			lua_pushboolean(L, FALSE);
		}

		return 1;
	}

	int npc_get_leader_vid(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		LPPARTY party = npc->GetParty();
		LPCHARACTER leader = party->GetLeader();

		if (leader)
			lua_pushnumber(L, leader->GetVID());
		else
			lua_pushnumber(L, 0);


		return 1;
	}

	int npc_get_vid(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		
		lua_pushnumber(L, npc->GetVID());


		return 1;
	}

	int npc_get_vid_attack_mul(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			lua_pushnumber(L, targetChar->GetAttMul());
		else
			lua_pushnumber(L, 0);


		return 1;
	}
	
	int npc_set_vid_attack_mul(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number attack_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetAttMul(attack_mul);

		return 0;
	}

	int npc_get_vid_damage_mul(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			lua_pushnumber(L, targetChar->GetDamMul());
		else
			lua_pushnumber(L, 0);


		return 1;
	}
	
	int npc_set_vid_damage_mul(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number damage_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetDamMul(damage_mul);

		return 0;
	}

	// MR-8: Damage Immunity System - Lua Bindings
	int npc_set_damage_immunity(lua_State* L)
	{
		// npc.set_damage_immunity(vid, immune_bool)
		// Sets basic immunity on/off for a specific VID
		
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		DWORD dwVID = (DWORD)lua_tonumber(L, 1);
		bool bImmune = lua_toboolean(L, 2);
		
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(dwVID);
		if (!ch)
		{
			sys_err("npc.set_damage_immunity: VID %u not found", dwVID);
			lua_pushboolean(L, false);
			return 1;
		}
		
		if (!ch->IsMonster() && !ch->IsStone() && !ch->IsDoor())
		{
			sys_err("npc.set_damage_immunity: VID %u is not a monster/stone/door", dwVID);
			lua_pushboolean(L, false);
			return 1;
		}
		
		ch->SetDamageImmunity(bImmune);
		lua_pushboolean(L, true);
		return 1;
	}

	int npc_is_damage_immune(lua_State* L)
	{
		// npc.is_damage_immune(vid)
		// Checks if a VID has damage immunity enabled
		
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find((DWORD)lua_tonumber(L, 1));
		lua_pushboolean(L, ch ? ch->IsDamageImmune() : false);
		return 1;
	}

	int npc_add_damage_immunity_condition(lua_State* L)
	{
		// npc.add_damage_immunity_condition(vid, condition_type, value, [extra_string])
		// Adds a condition that must be met for attacker to damage this entity
		// Types: 0=affect, 1=level_min, 2=level_max, 3=quest_flag, 4=item_equipped, 5=empire, 6=job
		
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		DWORD dwVID = (DWORD)lua_tonumber(L, 1);
		BYTE bType = (BYTE)lua_tonumber(L, 2);
		DWORD dwValue = (DWORD)lua_tonumber(L, 3);
		std::string strExtra = lua_isstring(L, 4) ? lua_tostring(L, 4) : "";
		
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(dwVID);
		if (!ch)
		{
			sys_err("npc.add_damage_immunity_condition: VID %u not found", dwVID);
			lua_pushboolean(L, false);
			return 1;
		}
		
		if (!ch->IsMonster() && !ch->IsStone() && !ch->IsDoor())
		{
			sys_err("npc.add_damage_immunity_condition: VID %u is not a monster/stone/door", dwVID);
			lua_pushboolean(L, false);
			return 1;
		}
		
		ch->AddDamageImmunityCondition(bType, dwValue, strExtra);
		lua_pushboolean(L, true);
		return 1;
	}

	int npc_clear_damage_immunity_conditions(lua_State* L)
	{
		// npc.clear_damage_immunity_conditions(vid)
		// Removes all damage immunity conditions from a VID
		
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find((DWORD)lua_tonumber(L, 1));
		if (!ch)
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		ch->ClearDamageImmunityConditions();
		lua_pushboolean(L, true);
		return 1;
	}

	int npc_set_damage_immunity_with_conditions(lua_State* L)
	{
		// npc.set_damage_immunity_with_conditions(vid, conditions_table)
		// High-level function that sets immunity and conditions in one call
		// Example: npc.set_damage_immunity_with_conditions(vid, {
		//   {type=0, value=23},           -- Need affect 23
		//   {type=1, value=50},           -- Need level >= 50
		//   {type=3, value=1, extra="dungeon.flag"}  -- Need quest flag
		// })
		
		if (!lua_isnumber(L, 1) || !lua_istable(L, 2))
		{
			lua_pushboolean(L, false);
			return 1;
		}
		
		DWORD dwVID = (DWORD)lua_tonumber(L, 1);
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(dwVID);
		
		if (!ch)
		{
			sys_err("npc.set_damage_immunity_with_conditions: VID %u not found", dwVID);
			lua_pushboolean(L, false);

			return 1;
		}
		
		if (!ch->IsMonster() && !ch->IsStone() && !ch->IsDoor())
		{
			sys_err("npc.set_damage_immunity_with_conditions: VID %u is not a monster/stone/door", dwVID);
			lua_pushboolean(L, false);

			return 1;
		}
		
		// CRITICAL: Set immunity flag FIRST to close the race condition window
		// This ensures the mob is protected immediately, even before conditions are parsed
		ch->SetDamageImmunity(true);
		
		// Clear existing conditions
		ch->ClearDamageImmunityConditions();
		
		// Parse conditions table
		int condCount = 0;

		lua_pushnil(L);

		while (lua_next(L, 2) != 0)
		{
			if (lua_istable(L, -1))
			{
				BYTE bType = 0;
				DWORD dwValue = 0;
				std::string strExtra = "";
				
				// Get 'type' field
				lua_pushstring(L, "type");
				lua_gettable(L, -2);

				if (lua_isnumber(L, -1))
					bType = (BYTE)lua_tonumber(L, -1);

				lua_pop(L, 1);
				
				// Get 'value' field
				lua_pushstring(L, "value");
				lua_gettable(L, -2);

				if (lua_isnumber(L, -1))
					dwValue = (DWORD)lua_tonumber(L, -1);
				lua_pop(L, 1);
				
				// Get 'extra' field (optional)
				lua_pushstring(L, "extra");
				lua_gettable(L, -2);

				if (lua_isstring(L, -1))
					strExtra = lua_tostring(L, -1);

				lua_pop(L, 1);
				
				ch->AddDamageImmunityCondition(bType, dwValue, strExtra);
				condCount++;
			}

			lua_pop(L, 1);
		}
		
		// Note: Immunity flag was already set at the start to minimize race condition
		// If no conditions were added, the mob will block ALL damage (fail-safe)
		
		lua_pushboolean(L, true);
		
		return 1;
	}
	// MR-8: -- END OF -- Damage Immunity System - Lua Bindings

	void RegisterNPCFunctionTable()
	{
		luaL_reg npc_functions[] = 
		{
			{ "getrace",			npc_get_race			},
			{ "get_race",			npc_get_race			},
			{ "open_shop",			npc_open_shop			},
			{ "get_empire",			npc_get_empire			},
			{ "is_pc",				npc_is_pc			},
			{ "is_quest",			npc_is_quest			},
			{ "kill",				npc_kill			},
			{ "purge",				npc_purge			},
			{ "is_near",			npc_is_near			},
			{ "is_near_vid",			npc_is_near_vid			},
			{ "lock",				npc_lock			},
			{ "unlock",				npc_unlock			},
			{ "get_guild",			npc_get_guild			},
			{ "get_leader_vid",		npc_get_leader_vid	},
			{ "get_vid",			npc_get_vid	},
			{ "get_vid_attack_mul",		npc_get_vid_attack_mul	},
			{ "set_vid_attack_mul",		npc_set_vid_attack_mul	},
			{ "get_vid_damage_mul",		npc_get_vid_damage_mul	},
			{ "set_vid_damage_mul",		npc_set_vid_damage_mul	},

			// X-mas santa special
			{ "get_remain_skill_book_count",	npc_get_remain_skill_book_count },
			{ "dec_remain_skill_book_count",	npc_dec_remain_skill_book_count },
			{ "get_remain_hairdye_count",	npc_get_remain_hairdye_count	},
			{ "dec_remain_hairdye_count",	npc_dec_remain_hairdye_count	},

			// MR-8: Damage Immunity System - Lua Bindings
			{ "set_damage_immunity",			npc_set_damage_immunity },
			{ "is_damage_immune",				npc_is_damage_immune },
			{ "add_damage_immunity_condition",	npc_add_damage_immunity_condition },
			{ "clear_damage_immunity_conditions", npc_clear_damage_immunity_conditions },
			{ "set_damage_immunity_with_conditions", npc_set_damage_immunity_with_conditions },
			// MR-8: -- END OF -- Damage Immunity System - Lua Bindings

			{ NULL,				NULL			    	}
		};

		CQuestManager::instance().AddLuaFunctionTable("npc", npc_functions);
	}
};
