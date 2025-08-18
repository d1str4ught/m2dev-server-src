#include "stdafx.h"
#include "char.h"
#include "mob_manager.h"
#include "affect.h"
#include "item.h"
#include "polymorph.h"

CPolymorphUtils::CPolymorphUtils()
{
	m_mapSPDType.insert(std::make_pair(101, 101));
	m_mapSPDType.insert(std::make_pair(1901, 1901));
}

POLYMORPH_BONUS_TYPE CPolymorphUtils::GetBonusType(DWORD dwVnum)
{
	std::unordered_map<DWORD, DWORD>::iterator iter;

	iter = m_mapSPDType.find(dwVnum);

	if (iter != m_mapSPDType.end())
		return POLYMORPH_SPD_BONUS;

	iter = m_mapATKType.find(dwVnum);

	if (iter != m_mapATKType.end())
		return POLYMORPH_ATK_BONUS;

	iter = m_mapDEFType.find(dwVnum);

	if (iter != m_mapDEFType.end())
		return POLYMORPH_DEF_BONUS;

	return POLYMORPH_NO_BONUS;
}

bool CPolymorphUtils::PolymorphCharacter(LPCHARACTER pChar, LPITEM pItem, const CMob* pMob)
{
	BYTE bySkillLevel = pChar->GetSkillLevel(POLYMORPH_SKILL_ID);
	DWORD dwDuration = 0;
	DWORD dwBonusPercent = 0;
	int iPolyPercent = 0;

	switch (pChar->GetSkillMasterType(POLYMORPH_SKILL_ID))
	{
		case SKILL_NORMAL:
			dwDuration = 10;
			break;

		case SKILL_MASTER:
			dwDuration = 15;
			break;

		case SKILL_GRAND_MASTER:
			dwDuration = 20;
			break;

		case SKILL_PERFECT_MASTER:
			dwDuration = 25;
			break;

		default:
			return false;
	}

	// dwDuration *= 60;

	// 변신 확률 = 캐릭터 레벨 - 몹 레벨 + 둔갑서 레벨 + 29 + 둔갑 스킬 레벨
	iPolyPercent = pChar->GetLevel() - pMob->m_table.bLevel + pItem->GetSocket(2) + (29 + bySkillLevel);

	if (iPolyPercent <= 0)
	{
		pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑에 실패 하였습니다"));
		return false;
	}
	else
	{
		if (number(1, 100) > iPolyPercent)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑에 실패 하였습니다"));
			return false;
		}
	}

	pChar->AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, pMob->m_table.dwVnum, AFF_POLYMORPH, dwDuration, 0, true);

	// 변신 보너스 = 둔갑 스킬 레벨 + 둔갑서 레벨
	dwBonusPercent = bySkillLevel + pItem->GetSocket(2);

	switch (GetBonusType(pMob->m_table.dwVnum))
	{
		case POLYMORPH_ATK_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_DEF_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_DEF_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_SPD_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_MOV_SPEED, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		default:
		case POLYMORPH_NO_BONUS:
			break;
	}

	return true;
}

bool CPolymorphUtils::UpdateBookPracticeGrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (pChar == NULL || pItem == NULL)
		return false;

	if (pItem->GetSocket(1) > 0)
		pItem->SetSocket(1, pItem->GetSocket(1) - 1);
	else
		pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑서 수련을 마첬습니다. 신선에게 찾아가세요."));

	return true;
}

bool CPolymorphUtils::GiveBook(LPCHARACTER pChar, DWORD dwMobVnum, DWORD dwPracticeCount, BYTE BookLevel, BYTE LevelLimit)
{
	// 소켓0                소켓1       소켓2
	// 둔갑할 몬스터 번호   수련정도    둔갑서 레벨
	if (pChar == NULL)
		return false;

	LPITEM pItem = pChar->AutoGiveItem(POLYMORPH_BOOK_ID, 1);

	if (pItem == NULL)
		return false;

	if (CMobManager::instance().Get(dwMobVnum) == NULL)
	{
		sys_err("Wrong Polymorph vnum passed: CPolymorphUtils::GiveBook(PID(%d), %d %d %d %d)", 
				pChar->GetPlayerID(), dwMobVnum, dwPracticeCount, BookLevel, LevelLimit);
		return false;
	}

	pItem->SetSocket(0, dwMobVnum);			// 둔갑할 몬스터 번호
	pItem->SetSocket(1, dwPracticeCount);		// 수련해야할 횟수
	pItem->SetSocket(2, BookLevel);			// 수련레벨
	return true;
}

bool CPolymorphUtils::BookUpgrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (pChar == NULL || pItem == NULL)
		return false;

	pItem->SetSocket(1, pItem->GetSocket(2) * 50);
	pItem->SetSocket(2, pItem->GetSocket(2)+1);
	return true;
}
