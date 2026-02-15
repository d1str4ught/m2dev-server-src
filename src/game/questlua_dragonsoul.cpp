#include "stdafx.h"

#include "config.h"
#include "questmanager.h"
#include "char.h"

#undef sys_err
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(std::source_location::current(), fmt __VA_OPT__(, __VA_ARGS__))

namespace quest 
{
	int ds_open_refine_window(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL == ch)
		{
			sys_err("DS_QUEST_OPEN_REFINE_WINDOW:: NULL POINT ERROR");
			return 0;
		}

		if (ch->DragonSoul_IsQualified())
		{
			sys_err("DS_QUEST_OPEN_REFINE_WINDOW:: ALREADY QUALIFIED");
			return 0;
		}

		ch->DragonSoul_RefineWindow_Open(CQuestManager::instance().GetCurrentNPCCharacterPtr());

		return 0;
	}

	int ds_give_qualification(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL == ch)
		{
			sys_err("DS_QUEST_GIVE_QUALIFICATION:: NULL POINT ERROR");
			return 0;
		}

		if (ch->DragonSoul_IsQualified())
		{
			sys_err("DS_QUEST_GIVE_QUALIFICATION:: ALREADY QUALIFIED");
			return 0;
		}

		// MR-12: Check min level for Dragonsoul qualification
		if (ch->GetLevel() <= 30)
		{
			sys_err("DS_QUEST_GIVE_QUALIFICATION:: LEVEL TOO LOW");
			return 0;
		}
		// MR-12: -- END OF -- Check min level for Dragonsoul qualification

		ch->DragonSoul_GiveQualification();

		return 0;
	}

	int ds_is_qualified(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL == ch)
		{
			sys_err("DS_QUEST_IS_QUALIFIED:: NULL POINT ERROR");
			lua_pushnumber(L, 0);
			return 1;
		}

		// MR-12: Check min level for Dragonsoul qualification
		if (ch->GetLevel() <= 30)
		{
			sys_err("DS_QUEST_IS_QUALIFIED:: LEVEL TOO LOW");
			lua_pushnumber(L, 0);
			return 1;
		}
		// MR-12: -- END OF -- Check min level for Dragonsoul qualification

		lua_pushnumber(L, ch->DragonSoul_IsQualified());
		return 1;
	}

	void RegisterDragonSoulFunctionTable()
	{
		luaL_reg ds_functions[] = 
		{
			{ "open_refine_window"	, ds_open_refine_window },
			{ "give_qualification"	, ds_give_qualification },
			{ "is_qualified"		, ds_is_qualified		},
			{ NULL					, NULL					}
		};

		CQuestManager::instance().AddLuaFunctionTable("ds", ds_functions);
	}
};
