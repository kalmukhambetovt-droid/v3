#pragma once
#include "BuildingTypeInfo.h"
#include "UnitTypeInfo.h"
#include "GameTypes.h"
#include "Health.h"
#include "Armor.h"
#include <string>
#include <vector>

// что здание сделало за один ход; менеджер зданий передаёт это посреднику
struct BuildingOutput {
    bool completedNow = false;
    ResourceStock produced;
    std::vector<UnitType> finishedUnits;
    std::string finishedResearch;
};

// абстрактное здание: тип, здоровье, позиция и прогресс строительства
class Building {
private:
    int id;
    Faction faction;

protected:
    const BuildingTypeInfo& typeInfo;
    Health health;
    Armor armor;
    Position position;
    int buildProgress;   // сколько ходов уже построено

    Building(int buildingId, Faction owner, const Position& place, const BuildingTypeInfo& info, bool completed);

public:
    virtual ~Building() = default;
    // здание уникально по id, копия дала бы два здания с одним номером в одной клетке
    Building(const Building&) = delete;
    Building& operator=(const Building&) = delete;

    BuildingOutput update();
    // работа готового здания за ход, по умолчанию здание ничего не производит
    virtual BuildingOutput produce();
    virtual std::string describeActivity() const;

    void takeDamage(int amount);

    int getId() const { return id; }
    Faction getFaction() const { return faction; }
    BuildingType getType() const { return typeInfo.getType(); }
    const std::string& getTypeName() const { return typeInfo.getName(); }
    int getHealth() const { return health.getValue(); }
    int getMaxHealth() const { return health.getMaxValue(); }
    int getArmorValue() const { return armor.getValue(); }
    const Position& getPosition() const { return position; }
    int getBuildProgress() const { return buildProgress; }
    int getBuildTurns() const { return typeInfo.getBuildTurns(); }
    bool isCompleted() const { return buildProgress >= typeInfo.getBuildTurns(); }
    bool isAlive() const { return !health.isZero(); }
    // здания игрока заглавными буквами, вражеские строчными
    char getSymbol() const;

    std::string describe() const;
};
