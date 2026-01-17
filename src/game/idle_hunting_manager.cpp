#include "stdafx.h"
#include "idle_hunting_manager.h"
#include <fstream>

using json = nlohmann::json;

CIdleHuntingManager::CIdleHuntingManager()
{
}

CIdleHuntingManager::~CIdleHuntingManager()
{
}

bool CIdleHuntingManager::LoadGroups(const char* filename)
{
    try {
        m_configFile.assign(filename);
    } catch (...) {
        sys_err("Exception during config file assignment");
        return false;
    }
    
    m_groups.clear();
    
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            sys_err("Cannot open idle hunting groups config: %s", filename);
            return false;
        }
        
        json j = json::parse(file);
        
        if (!j.contains("groups") || !j["groups"].is_array()) {
            sys_err("Invalid idle hunting groups config format");
            return false;
        }
        
        for (const auto& group_data : j["groups"]) {
            IdleHuntingGroup group;
            
            // Required fields
            group.group_id = group_data["id"].get<DWORD>();
            group.name = group_data["name"].get<std::string>();
            group.min_level = group_data["min_level"].get<int>();
            group.premium_only = group_data["premium_only"].get<bool>();
            group.exp_multiplier = group_data["exp_multiplier"].get<float>();
            group.yang_multiplier = group_data["yang_multiplier"].get<float>();
            
            // Optional fields
            if (group_data.contains("display_name"))
                group.display_name = group_data["display_name"].get<std::string>();
            else
                group.display_name = group.name;
                
            if (group_data.contains("description"))
                group.description = group_data["description"].get<std::string>();
            
            // Load mobs
            if (group_data.contains("mobs") && group_data["mobs"].is_array()) {
                for (const auto& mob_data : group_data["mobs"]) {
                    IdleHuntingGroupMob mob;
                    mob.mob_vnum = mob_data["vnum"].get<DWORD>();
                    mob.weight = mob_data["weight"].get<int>();
                    if (mob_data.contains("name"))
                        mob.name = mob_data["name"].get<std::string>();
                    
                    group.mobs.push_back(mob);
                }
            }
            
            // Load drops
            if (group_data.contains("drops") && group_data["drops"].is_array()) {
                for (const auto& drop_data : group_data["drops"]) {
                    IdleHuntingGroupDrop drop;
                    drop.item_vnum = drop_data["item_vnum"].get<DWORD>();
                    drop.drop_chance = drop_data["chance"].get<float>();
                    drop.min_count = drop_data["min"].get<int>();
                    drop.max_count = drop_data["max"].get<int>();
                    if (drop_data.contains("name"))
                        drop.name = drop_data["name"].get<std::string>();
                    
                    group.drops.push_back(drop);
                }
            }
            
            m_groups[group.group_id] = group;
            sys_log(0, "Loaded idle hunting group: %u (%s) - %zu mobs, %zu drops", 
                    group.group_id, group.name.c_str(), group.mobs.size(), group.drops.size());
        }
        sys_log(0, "Loaded %zu idle hunting groups from %s", m_groups.size(), filename);
        return true;
        
    } catch (const json::exception& e) {
        sys_err("JSON parsing error in %s: %s", filename, e.what());
        return false;
    } catch (const std::exception& e) {
        sys_err("Error loading idle hunting groups: %s", e.what());
        return false;
    }
}

void CIdleHuntingManager::ReloadGroups()
{
    if (!m_configFile.empty()) {
        sys_log(0, "Reloading idle hunting groups...");
        LoadGroups(m_configFile.c_str());
    }
}

const IdleHuntingGroup* CIdleHuntingManager::GetGroup(DWORD group_id) const
{
    auto it = m_groups.find(group_id);
    if (it != m_groups.end())
        return &it->second;
    return nullptr;
}

std::vector<const IdleHuntingGroup*> CIdleHuntingManager::GetAvailableGroups(int player_level, bool is_premium) const
{
    std::vector<const IdleHuntingGroup*> available;
    
    for (const auto& pair : m_groups) {
        const IdleHuntingGroup& group = pair.second;
        
        // Check level requirement
        if (player_level < group.min_level)
            continue;
            
        // Check premium requirement
        if (group.premium_only && !is_premium)
            continue;
            
        available.push_back(&group);
    }
    
    return available;
}

bool CIdleHuntingManager::IsGroupAvailable(DWORD group_id, int player_level, bool is_premium) const
{
    const IdleHuntingGroup* group = GetGroup(group_id);
    if (!group)
        return false;
        
    if (player_level < group->min_level)
        return false;
        
    if (group->premium_only && !is_premium)
        return false;
        
    return true;
}