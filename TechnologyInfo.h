#pragma once
#include "ResourceStock.h"
#include "UnitTypeInfo.h"
#include <string>
#include <vector>

enum class TechnologyEffect { AttackBonus, ArmorBonus, UnlockUnit };

class TechnologyInfo {
private:
    std::string id;        // имя в команде research
    std::string name;
    ResourceStock cost;
    int researchTurns;
    TechnologyEffect effect;
    int value;             // величина бонуса для AttackBonus и ArmorBonus
    UnitType unlockedUnit; // имеет смысл только для UnlockUnit

public:
    TechnologyInfo(std::string technologyId, std::string technologyName, const ResourceStock& price,
                   int turns, TechnologyEffect technologyEffect, int effectValue, UnitType unit);

    const std::string& getId() const { return id; }
    const std::string& getName() const { return name; }
    const ResourceStock& getCost() const { return cost; }
    int getResearchTurns() const { return researchTurns; }
    TechnologyEffect getEffect() const { return effect; }
    int getValue() const { return value; }
    UnitType getUnlockedUnit() const { return unlockedUnit; }

    static const TechnologyInfo* find(const std::string& technologyId);
    static const std::vector<TechnologyInfo>& all();
};

std::string technologyList();
