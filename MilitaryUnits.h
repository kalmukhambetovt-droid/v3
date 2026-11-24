#pragma once
#include "Unit.h"

// пехотинец: основной юнит ближнего боя
class Infantry : public Unit {
public:
    Infantry(int unitId, Faction owner, const Position& start)
        : Unit(unitId, owner, start, UnitTypeInfo::getInfo(UnitType::Infantry)) {
    }
    std::string getTypeName() const override { return "Infantry"; }
    std::string describeAbility() const override { return "ближний бой"; }
};

// разведчик: быстрый и лёгкий, проходит горы и не замедляется в лесу
class Scout : public Unit {
public:
    Scout(int unitId, Faction owner, const Position& start)
        : Unit(unitId, owner, start, UnitTypeInfo::getInfo(UnitType::Scout)) {
    }
    std::string getTypeName() const override { return "Scout"; }
    std::string describeAbility() const override { return "разведка: проходит горы, лес не замедляет"; }
};

// танк: сильная броня и дальний выстрел, но не проходит лес и вязнет в болоте
class Tank : public Unit {
public:
    Tank(int unitId, Faction owner, const Position& start)
        : Unit(unitId, owner, start, UnitTypeInfo::getInfo(UnitType::Tank)) {
    }
    std::string getTypeName() const override { return "Tank"; }
    std::string describeAbility() const override { return "тяжёлая броня, выстрел на 3 клетки"; }
};
