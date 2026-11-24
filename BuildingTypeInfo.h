#pragma once
#include "ResourceStock.h"
#include <optional>
#include <string>

enum class BuildingType { Barracks, Factory, PowerPlant, Laboratory, Warehouse, Hospital };
constexpr int kBuildingTypeCount = 6;

// неизменяемые характеристики типа здания
class BuildingTypeInfo {
private:
    BuildingType type;
    std::string name;
    char symbol;           // символ на карте для здания игрока
    int maxHealth;
    int armor;
    ResourceStock cost;
    int buildTurns;        // сколько ходов идёт строительство

public:
    BuildingTypeInfo(BuildingType buildingType, std::string typeName, char typeSymbol,
                     int hp, int armorValue, const ResourceStock& price, int turns);

    BuildingType getType() const { return type; }
    const std::string& getName() const { return name; }
    char getSymbol() const { return symbol; }
    int getMaxHealth() const { return maxHealth; }
    int getArmor() const { return armor; }
    const ResourceStock& getCost() const { return cost; }
    int getBuildTurns() const { return buildTurns; }

    static const BuildingTypeInfo& getInfo(BuildingType type);
};

std::optional<BuildingType> buildingTypeFromString(const std::string& name);
const std::string& buildingTypeToString(BuildingType type);
std::string buildingTypeList();
