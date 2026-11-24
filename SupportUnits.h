#pragma once
#include "Unit.h"

// рабочий: единственный, кто добывает ресурсы на ресурсных точках
class Worker : public Unit {
public:
    static constexpr int kGatherAmount = 25;

    Worker(int unitId, Faction owner, const Position& start)
        : Unit(unitId, owner, start, UnitTypeInfo::getInfo(UnitType::Worker)) {
    }
    std::string getTypeName() const override { return "Worker"; }
    std::string describeAbility() const override { return "добыча: 25 единиц ресурса за ход"; }
    int getGatherAmount() const override { return kGatherAmount; }
};

// медик: лечит союзника рядом с собой
class Medic : public Unit {
public:
    static constexpr int kHealPower = 25;
    static constexpr int kHealRange = 2;

    Medic(int unitId, Faction owner, const Position& start)
        : Unit(unitId, owner, start, UnitTypeInfo::getInfo(UnitType::Medic)) {
    }
    std::string getTypeName() const override { return "Medic"; }
    std::string describeAbility() const override { return "лечение союзника на 25 HP в радиусе 2"; }
    int getHealPower() const override { return kHealPower; }
    int getHealRange() const override { return kHealRange; }
};
