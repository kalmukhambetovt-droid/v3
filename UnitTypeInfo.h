#pragma once
#include "ResourceStock.h"
#include <optional>
#include <string>

enum class UnitType { Infantry, Scout, Tank, Worker, Medic };
constexpr int kUnitTypeCount = 5;


class UnitTypeInfo {
private:
    UnitType type;
    std::string name;      
    char symbol;          
    int maxHealth;
    int damage;
    int armor;
    int moveRange;        
    int attackRange;
    ResourceStock cost;
    int productionTurns;
    bool heavy;            // не проходит лес, вязнет в болоте
    bool light;            // проходит горы и не замедляется в лесу
    std::string requiredTechnology; 

public:
    UnitTypeInfo(UnitType unitType, std::string typeName, char typeSymbol, int hp, int damageValue,
                 int armorValue, int move, int range, const ResourceStock& price, int turns,
                 bool isHeavy, bool isLight, std::string technology);

    UnitType getType() const { return type; }
    const std::string& getName() const { return name; }
    char getSymbol() const { return symbol; }
    int getMaxHealth() const { return maxHealth; }
    int getDamage() const { return damage; }
    int getArmor() const { return armor; }
    int getMoveRange() const { return moveRange; }
    int getAttackRange() const { return attackRange; }
    const ResourceStock& getCost() const { return cost; }
    int getProductionTurns() const { return productionTurns; }
    bool isHeavy() const { return heavy; }
    bool isLight() const { return light; }
    const std::string& getRequiredTechnology() const { return requiredTechnology; }

    static const UnitTypeInfo& getInfo(UnitType type);
};

std::optional<UnitType> unitTypeFromString(const std::string& name);
const std::string& unitTypeToString(UnitType type);
std::string unitTypeList();
