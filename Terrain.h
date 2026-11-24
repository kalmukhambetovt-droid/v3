#pragma once
#include "UnitTypeInfo.h"
#include <memory>
#include <string>

class Terrain {
public:
    virtual ~Terrain() = default;
    virtual bool isPassable(UnitType unitType) const = 0;
    virtual double getSpeedModifier(UnitType unitType) const = 0;
    virtual int getDefenseBonus(UnitType unitType) const = 0;
    // сколько ходов юнит пропускает, если зашёл в клетку (застревание в болоте)
    virtual int getStuckTurns(UnitType unitType) const = 0;
    virtual bool isBuildable() const = 0;
    virtual char getSymbol() const = 0;
    virtual std::string getName() const = 0;
    virtual std::unique_ptr<Terrain> clone() const = 0;
};
