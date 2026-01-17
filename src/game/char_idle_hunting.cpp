#include "stdafx.h"
#include "char.h"
#include "db.h"
#include "config.h"
#include "mob_manager.h"
#include "item_manager.h"
#include "item.h"
#include "desc.h"
#include "log.h"
#include "questmanager.h"
#include "idle_hunting_manager.h"

// Configuration
static const DWORD MAX_DAILY_SECONDS = 28800; // 8 hours
static const int BASE_KILLS_PER_HOUR = 100;
static const float EXP_RATE_MODIFIER = 1.0f; // 300% of normal exp
static const float YANG_RATE_MODIFIER = 1.0f; // 100% of normal yang

void CHARACTER::LoadIdleHunting()
{
    if (!IsPC())
        return;

    std::unique_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(
        "SELECT mob_group_id, start_time, end_time, last_claim_time, total_time_today, last_reset_date, is_active, max_daily_seconds "
        "FROM idle_hunting WHERE pid = %u", 
        GetPlayerID()));
    
    if (pMsg->Get()->uiNumRows > 0)
    {
        MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
        
        m_idleHunting.groupId = strtoul(row[0], nullptr, 10);
        m_idleHunting.startTime = strtoul(row[1], nullptr, 10);
        m_idleHunting.endTime = strtoul(row[2], nullptr, 10);
        m_idleHunting.lastClaimTime = strtoul(row[3], nullptr, 10);
        m_idleHunting.totalTimeToday = strtoul(row[4], nullptr, 10);
        m_idleHunting.maxDailySeconds = row[7] ? strtoul(row[7], nullptr, 10) : MAX_DAILY_SECONDS;
        
        if (row[5])
            strncpy(m_idleHunting.lastResetDate, row[5], sizeof(m_idleHunting.lastResetDate) - 1);

        m_idleHunting.isActive = atoi(row[6]);
        
        // Check for daily reset at login
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        if (timeinfo)
        {
            char currentDate[11];
            strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
            
            // Reset counter if new day
            if (strcmp(currentDate, m_idleHunting.lastResetDate) != 0)
            {
                m_idleHunting.totalTimeToday = 0;
                strncpy(m_idleHunting.lastResetDate, currentDate, sizeof(m_idleHunting.lastResetDate) - 1);
                m_idleHunting.lastResetDate[sizeof(m_idleHunting.lastResetDate) - 1] = '\0';
                SaveIdleHunting();
                sys_log(0, "IDLE_HUNT: Daily reset for player %u at login (was %s, now %s)", GetPlayerID(), row[5], currentDate);
            }
        }

        // If hunt was active (is_active=1), player logged back in, set to "ready to claim" state
        if (m_idleHunting.isActive == 1)
        {
            const IdleHuntingGroup* group = CIdleHuntingManager::instance().GetGroup(m_idleHunting.groupId);
            if (!group)
            {
                sys_err("Idle Hunting: Invalid group ID %u for player %u", m_idleHunting.groupId, GetPlayerID());
                m_idleHunting.groupId = 0;
                m_idleHunting.isActive = 0;
                SaveIdleHunting();
                return;
            }
            
            // Mark as ready to claim (state 2)
            m_idleHunting.isActive = 2;
            m_idleHunting.endTime = static_cast<DWORD>(time(0)); // Freeze hunt end time
            SaveIdleHunting();
            
            sys_log(0, "IDLE_HUNT: Player %u logged in with active hunt, marked as ready to claim", GetPlayerID());
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_COMPLETE_LOGIN"));
        }
        else if (m_idleHunting.isActive == 2)
        {
            // Player already has unclaimed rewards from a previous login
            sys_log(0, "IDLE_HUNT: Player %u logged in with unclaimed rewards", GetPlayerID());
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_UNCLAIMED_REWARDS"));
        }
        
        // Send state update to client
        if (GetDesc())
        {
            extern void SendIdleHuntingUpdateToClient(LPCHARACTER ch);
            SendIdleHuntingUpdateToClient(this);
        }
    }
}

void CHARACTER::StartIdleHunting(DWORD groupId)
{
    sys_log(0, "IDLE_HUNT: StartIdleHunting called for player %u, group %u", GetPlayerID(), groupId);
    
    // Validation
    if (!IsPC())
    {
        sys_log(0, "IDLE_HUNT: Not a PC, returning");
        return;
    }

    const IdleHuntingGroup* group = CIdleHuntingManager::instance().GetGroup(groupId);
    if (!group)
    {
        sys_log(0, "IDLE_HUNT: Group %u not found!", groupId);
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_INVALID_GROUP"));
        sys_err("Idle Hunting: Player %u tried to hunt invalid group %u", GetPlayerID(), groupId);
        return;
    }

    if (GetLevel() < group->min_level)
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_LEVEL_TOO_LOW"), group->min_level);
        return;
    }

    // Check premium requirement
    if (group->premium_only && !IsGM())  // Replace with proper premium check when available
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_PREMIUM_REQUIRED"));
        return;
    }
    
    sys_log(0, "IDLE_HUNT: Group validated: %s (level %d)", group->name.c_str(), group->min_level);

    if (m_idleHunting.groupId > 0)
    {
        sys_log(0, "IDLE_HUNT: Hunt already configured (group=%u, is_active=%d), returning", m_idleHunting.groupId, m_idleHunting.isActive);
        if (m_idleHunting.isActive == 0)
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_ALREADY_CONFIGURED"));
        else if (m_idleHunting.isActive == 1)
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_ALREADY_ACTIVE"));
        else if (m_idleHunting.isActive == 2)
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_CLAIM_FIRST"));
        return;
    }

    // Check daily limit before starting
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    if (!timeinfo)
    {
        sys_err("localtime failed for player %u", GetPlayerID());
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_SYSTEM_ERROR"));
        return;
    }
    
    char currentDate[11];
    strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
    
    // Reset counter if new day
    if (strcmp(currentDate, m_idleHunting.lastResetDate) != 0)
    {
        m_idleHunting.totalTimeToday = 0;
    }
    
    if (m_idleHunting.totalTimeToday >= m_idleHunting.maxDailySeconds)
    {
        int maxHours = m_idleHunting.maxDailySeconds / 3600;
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_DAILY_LIMIT_REACHED"), maxHours);
        return;
    }

    sys_log(0, "IDLE_HUNT: Configuring idle hunt for player %u, group %u (pending logout)", GetPlayerID(), groupId);
    
    // Set to pending state - hunt won't start until player logs out
    m_idleHunting.groupId = groupId;
    m_idleHunting.startTime = 0; // Will be set on logout
    m_idleHunting.endTime = 0;
    m_idleHunting.lastClaimTime = 0;
    m_idleHunting.isActive = 0; // Pending state - not active yet
    strncpy(m_idleHunting.lastResetDate, currentDate, sizeof(m_idleHunting.lastResetDate) - 1);
    m_idleHunting.lastResetDate[sizeof(m_idleHunting.lastResetDate) - 1] = '\0';
    
    sys_log(0, "IDLE_HUNT: Saving pending hunt configuration for player %u", GetPlayerID());
    SaveIdleHunting();
    sys_log(0, "IDLE_HUNT: SaveIdleHunting completed for player %u", GetPlayerID());
    
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_CONFIGURED"), group->display_name.c_str());
    
    char szHint[128];
    snprintf(szHint, sizeof(szHint), "group_id %u level %d (pending)", groupId, group->min_level);
    LogManager::instance().CharLog(this, 0, "IDLE_HUNT_CONFIG", szHint);
}

void CHARACTER::StopIdleHunting()
{
    if (m_idleHunting.groupId == 0)
        return;

    char szHint[128];
    snprintf(szHint, sizeof(szHint), "group_id %u time_used %u state %d", m_idleHunting.groupId, m_idleHunting.totalTimeToday, m_idleHunting.isActive);
    LogManager::instance().CharLog(this, 0, "IDLE_HUNT_STOP", szHint);
    
    m_idleHunting.groupId = 0;
    m_idleHunting.isActive = 0;
    m_idleHunting.startTime = 0;
    m_idleHunting.endTime = 0;
    m_idleHunting.lastClaimTime = 0;
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_CANCELLED"));
    
    SaveIdleHunting();
}

void CHARACTER::CalculateIdleRewards()
{
    // Only calculate rewards if in "ready to claim" state (2)
    if (m_idleHunting.isActive != 2)
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_NO_REWARDS"));
        return;
    }

    // Validate group
    const IdleHuntingGroup* group = CIdleHuntingManager::instance().GetGroup(m_idleHunting.groupId);
    if (!group)
    {
        sys_err("Idle Hunting: Invalid group %u for player %u", m_idleHunting.groupId, GetPlayerID());
        StopIdleHunting();
        return;
    }

    DWORD currentTime = static_cast<DWORD>(time(0));
    
    // Validate time hasn't gone backwards
    if (currentTime < m_idleHunting.startTime)
    {
        sys_err("Idle Hunting: Time went backwards for player %u", GetPlayerID());
        StopIdleHunting();
        return;
    }
    
    DWORD elapsedSeconds = currentTime - m_idleHunting.startTime;
    
    // Max 24 hours offline (sanity check)
    if (elapsedSeconds > 86400)
    {
        sys_err("Idle Hunting: Suspicious elapsed time %u for player %u", elapsedSeconds, GetPlayerID());
        elapsedSeconds = 86400;
    }
    
    // Check daily limit
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char currentDate[11];
    strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
    
    // Reset counter if new day
    if (strcmp(currentDate, m_idleHunting.lastResetDate) != 0)
    {
        m_idleHunting.totalTimeToday = 0;
        strncpy(m_idleHunting.lastResetDate, currentDate, sizeof(m_idleHunting.lastResetDate) - 1);
    }
    
    // Calculate remaining time
    DWORD remainingTime = m_idleHunting.maxDailySeconds - m_idleHunting.totalTimeToday;
    
    if (remainingTime <= 0)
    {
        int maxHours = m_idleHunting.maxDailySeconds / 3600;
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_DAILY_LIMIT_HIT"), maxHours);
        StopIdleHunting();
        return;
    }
    
    // Cap elapsed time to remaining daily limit
    if (elapsedSeconds > remainingTime)
    {
        elapsedSeconds = remainingTime;
        int maxHours = m_idleHunting.maxDailySeconds / 3600;
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_DAILY_LIMIT_PARTIAL"), maxHours);
    }
    
    // Update daily time counter
    m_idleHunting.totalTimeToday += elapsedSeconds;

    // Get player combat stats
    int playerLevel = GetLevel();
    int playerAttGrade = GetPoint(POINT_ATT_GRADE);
    int playerAtkSpeed = GetPoint(POINT_ATT_SPEED);

    // Get equipped weapon for realistic damage calculation
    LPITEM pWeapon = GetWear(WEAR_WEAPON);
    int weaponDamageMin = 0;
    int weaponDamageMax = 0;

    if (pWeapon && pWeapon->GetType() == ITEM_WEAPON)
    {
        weaponDamageMin = pWeapon->GetValue(3);
        weaponDamageMax = pWeapon->GetValue(4);
    }

    // Calculate base weapon damage
    int weaponDamage = ((weaponDamageMin + weaponDamageMax) / 2) * 2;

    // Calculate attack speed factor
    float attackSpeedFactor = playerAtkSpeed / 100.0f;
    if (attackSpeedFactor < 0.5f) attackSpeedFactor = 0.5f;

    // Base time per attack
    float secondsPerHit = 2.0f / attackSpeedFactor;

    // Calculate total weight for distribution
    int totalWeight = 0;
    for (const auto& mob : group->mobs)
        totalWeight += mob.weight;

    if (totalWeight <= 0)
    {
        sys_err("Idle Hunting: Group %u has zero total weight", m_idleHunting.groupId);
        StopIdleHunting();
        return;
    }

    // Calculate weighted averages across all mobs in the group
    float totalKills = 0;
    QWORD totalExp = 0;
    DWORD totalGold = 0;

    for (const auto& groupMob : group->mobs)
    {
        const CMob* mobData = CMobManager::instance().Get(groupMob.mob_vnum);
        if (!mobData)
        {
            sys_err("Idle Hunting: Invalid mob %u in group %u", groupMob.mob_vnum, m_idleHunting.groupId);
            continue;
        }

        // Calculate this mob's share of hunting time based on weight
        float mobTimeFraction = (float)groupMob.weight / (float)totalWeight;
        DWORD mobElapsedSeconds = (DWORD)(elapsedSeconds * mobTimeFraction);

        // Get mob stats
        int mobLevel = mobData->m_table.bLevel;
        DWORD mobHP = mobData->m_table.dwMaxHP;
        int mobDefGrade = mobData->m_table.wDef;

        // Calculate damage and kills for this mob
        float attackRating = 1.0f;
        int baseAttack = playerAttGrade + weaponDamage - (playerLevel * 2);
        int totalAttack = (int)(baseAttack * attackRating) + (playerLevel * 2);
        int totalDefense = mobDefGrade;
        int effectiveDamage = MAX(1, totalAttack - totalDefense);

        float hitsToKill = (float)mobHP / (float)effectiveDamage;
        float combatTime = hitsToKill * secondsPerHit;
        float totalTimePerKill = combatTime + 4.0f;
        float killsPerHour = 3600.0f / totalTimePerKill;

        // Apply level difference efficiency
        int levelDiff = playerLevel - mobLevel;
        float efficiencyRate = 1.0f;

        if (levelDiff > 10)
            efficiencyRate = 0.1f;
        else if (levelDiff > 5)
            efficiencyRate = 0.5f;
        else if (levelDiff < -10)
            efficiencyRate = 0.2f;
        else if (levelDiff < -5)
            efficiencyRate = 0.4f;

        killsPerHour *= efficiencyRate;

        // Sanity caps per mob
        if (killsPerHour < 10.0f)
            killsPerHour = 10.0f;
        if (killsPerHour > 500.0f)
            killsPerHour = 500.0f;

        float mobKills = (mobElapsedSeconds / 3600.0f) * killsPerHour;
        totalKills += mobKills;

        // Calculate exp for this mob with level bonuses
        QWORD expPerKill = mobData->m_table.dwExp;
        float levelExpMultiplier = EXP_RATE_MODIFIER;
        if (playerLevel <= 10)
            levelExpMultiplier *= 5.0f;
        else if (playerLevel <= 30)
            levelExpMultiplier *= 3.0f;
        else if (playerLevel <= 50)
            levelExpMultiplier *= 2.0f;

        QWORD mobExp = static_cast<QWORD>(mobKills * expPerKill * levelExpMultiplier);
        totalExp += mobExp;

        // Calculate gold for this mob
        int goldMin = mobData->m_table.dwGoldMin;
        int goldMax = mobData->m_table.dwGoldMax;
        int mobKillsInt = (int)mobKills;

        for (int i = 0; i < mobKillsInt; i++)
        {
            if (number(1, 100) <= 30) // 30% gold drop chance
            {
                totalGold += number(goldMin, goldMax);
            }
        }
    }

    // Apply group multipliers
    totalExp = static_cast<QWORD>(totalExp * group->exp_multiplier);
    totalGold = static_cast<DWORD>(totalGold * group->yang_multiplier * YANG_RATE_MODIFIER);

    // Sanity caps
    int totalKillsInt = static_cast<int>(totalKills);
    if (totalKillsInt < 0)
        totalKillsInt = 0;
    if (totalKillsInt > 10000)
    {
        sys_err("Idle Hunting: Suspicious kill count %d for player %u", totalKillsInt, GetPlayerID());
        totalKillsInt = 10000;
    }

    if (totalExp > 2000000000)
    {
        sys_err("Idle Hunting: Suspicious exp %llu for player %u", totalExp, GetPlayerID());
        totalExp = 2000000000;
    }

    if (totalGold > 2000000000)
    {
        sys_err("Idle Hunting: Suspicious yang %u for player %u", totalGold, GetPlayerID());
        totalGold = 2000000000;
    }

    // Apply rewards
    if (totalExp > 0)
        PointChange(POINT_EXP, static_cast<int>(totalExp));

    if (totalGold > 0)
    {
        PointChange(POINT_GOLD, totalGold);
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_YANG_EARNED"), totalGold);
        
        char szYangHint[128];
        snprintf(szYangHint, sizeof(szYangHint), "yang_earned %u kills %d group %u", totalGold, totalKillsInt, m_idleHunting.groupId);
        LogManager::instance().CharLog(this, 0, "IDLE_HUNT_YANG", szYangHint);
    }

    // Generate items from group drop table
    GenerateIdleHuntingDrops(m_idleHunting.groupId, totalKillsInt);

    // Log claim
    char szHint[256];
    snprintf(szHint, sizeof(szHint), "group_id %u (%s) kills %d exp %llu time %us", 
        m_idleHunting.groupId, group->name.c_str(), totalKillsInt, totalExp, elapsedSeconds);
    LogManager::instance().CharLog(this, 0, "IDLE_HUNT_CLAIM", szHint);

    // Format time for display
    int hours = elapsedSeconds / 3600;
    int minutes = (elapsedSeconds % 3600) / 60;
    int remainingHours = (m_idleHunting.maxDailySeconds - m_idleHunting.totalTimeToday) / 3600;
    int remainingMinutes = ((m_idleHunting.maxDailySeconds - m_idleHunting.totalTimeToday) % 3600) / 60;

    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_RESULTS_HEADER"));
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_RESULTS_GROUP"), group->name.c_str());
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_RESULTS_TIME"), hours, minutes);
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_RESULTS_KILLS"), totalKillsInt);
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_RESULTS_EXP"), totalExp);
    ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_DAILY_REMAINING"), remainingHours, remainingMinutes);

    // Update last claim time
    m_idleHunting.lastClaimTime = currentTime;

    // Automatically stop idle hunting
    StopIdleHunting();
}

void CHARACTER::GenerateIdleHuntingDrops(DWORD groupId, int killCount)
{
    if (killCount <= 0)
        return;

    const IdleHuntingGroup* group = CIdleHuntingManager::instance().GetGroup(groupId);
    if (!group)
    {
        sys_err("Idle Hunting: Invalid group %u for drops", groupId);
        return;
    }

    int itemsDropped = 0;
    std::map<DWORD, int> collectedItems;

    // Use group drop table
    for (const auto& drop : group->drops)
    {
        for (int i = 0; i < killCount; i++)
        {
            // Adjust drop chance for idle (50% of configured rate)
            int adjustedChance = drop.drop_chance / 2;
            if (adjustedChance > 0 && number(1, 10000) <= adjustedChance)
            {
                int count = number(drop.min_count, drop.max_count);
                collectedItems[drop.item_vnum] += count;
            }
        }
    }

    // Create and give all collected items
    for (const auto& pair : collectedItems)
    {
        DWORD itemVnum = pair.first;
        int totalCount = pair.second;

        // Cap individual item stacks
        if (totalCount > 200)
            totalCount = 200;

        // Give items in stacks
        while (totalCount > 0 && itemsDropped < 100) // Max 100 item stacks
        {
            int giveCount = MIN(totalCount, 200); // Max stack size
            LPITEM item = AutoGiveItem(itemVnum, giveCount);
            if (item)
            {
                ChatPacket(CHAT_TYPE_INFO, LC_TEXT("IDLE_HUNT_ITEM_DROP"), item->GetName(), giveCount);
                itemsDropped++;
                totalCount -= giveCount;
            }
            else
                break; // Inventory full
        }
    }

    if (itemsDropped > 0)
    {
        char szHint[64];
        snprintf(szHint, sizeof(szHint), "%d", itemsDropped);
        LogManager::instance().CharLog(this, 0, "IDLE_HUNT_ITEMS", szHint);
    }
}

void CHARACTER::SaveIdleHunting()
{
    sys_log(0, "IDLE_HUNT_SAVE: Function called, IsPC=%d", IsPC() ? 1 : 0);
    
    if (!IsPC())
    {
        sys_log(0, "IDLE_HUNT_SAVE: Early return - not a PC");
        return;
    }
    
    sys_log(0, "IDLE_HUNT_SAVE: IsPC check passed, building query");

    char szQuery[512];
    snprintf(szQuery, sizeof(szQuery),
        "REPLACE INTO player.idle_hunting "
        "(pid, mob_group_id, start_time, end_time, last_claim_time, total_time_today, last_reset_date, is_active, max_daily_seconds) "
        "VALUES(%u, %u, %u, %u, %u, %u, '%s', %d, %u)", 
        GetPlayerID(), 
        m_idleHunting.groupId, 
        m_idleHunting.startTime,
        m_idleHunting.endTime,
        m_idleHunting.lastClaimTime,
        m_idleHunting.totalTimeToday,
        m_idleHunting.lastResetDate,
        m_idleHunting.isActive,
        m_idleHunting.maxDailySeconds
    );
    
    sys_log(0, "IDLE_HUNT_SAVE player=%u query=%s", GetPlayerID(), szQuery);
    DBManager::instance().Query(szQuery);
}

DWORD CHARACTER::GetIdleHuntingDuration() const
{
    if (!IsPC() || m_idleHunting.isActive != 2)
        return 0;
    
    DWORD endTime = m_idleHunting.endTime;
    
    if (endTime == 0 || endTime < m_idleHunting.startTime)
        return 0;
    
    DWORD elapsedSeconds = endTime - m_idleHunting.startTime;
    
    // Same caps as in CalculateIdleRewards
    if (elapsedSeconds > 86400)
        elapsedSeconds = 86400;
    
    DWORD remainingTime = m_idleHunting.maxDailySeconds - m_idleHunting.totalTimeToday;
    if (elapsedSeconds > remainingTime)
        elapsedSeconds = remainingTime;
    
    return elapsedSeconds;
}