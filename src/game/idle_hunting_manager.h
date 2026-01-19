#ifndef __INC_IDLE_HUNTING_MANAGER_H__
#define __INC_IDLE_HUNTING_MANAGER_H__

#include <json.hpp>
#include <vector>
#include <map>
#include <string>

struct IdleHuntingGroupMob {
    DWORD mob_vnum;
    int weight;
    std::string name;
};

struct IdleHuntingGroupDrop {
    DWORD item_vnum;
    float drop_chance;
    int min_count;
    int max_count;
    std::string name;
};

struct IdleHuntingGroup {
    DWORD group_id;
    std::string name;
    std::string display_name;
    std::string description;
    int min_level;
    bool premium_only;
    float exp_multiplier;
    float yang_multiplier;
    
    std::vector<IdleHuntingGroupMob> mobs;
    std::vector<IdleHuntingGroupDrop> drops;
};

class CIdleHuntingManager : public singleton<CIdleHuntingManager>
{
public:
    CIdleHuntingManager();
    ~CIdleHuntingManager();
    
    bool LoadGroups(const char* filename);
    void ReloadGroups();
    
    const IdleHuntingGroup* GetGroup(DWORD group_id) const;
    std::vector<const IdleHuntingGroup*> GetAvailableGroups(int player_level, bool is_premium) const;
    
    bool IsGroupAvailable(DWORD group_id, int player_level, bool is_premium) const;
    
private:
    std::map<DWORD, IdleHuntingGroup> m_groups;
    std::string m_configFile;
};

#endif