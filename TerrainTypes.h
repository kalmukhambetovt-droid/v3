#pragma once
#include "Terrain.h"

// равнина без ограничений, на ней можно строить
class Plains : public Terrain {
public:
    bool isPassable(UnitType) const override { return true; }
    double getSpeedModifier(UnitType) const override { return 1.0; }
    int getDefenseBonus(UnitType) const override { return 0; }
    int getStuckTurns(UnitType) const override { return 0; }
    bool isBuildable() const override { return true; }
    char getSymbol() const override { return '.'; }
    std::string getName() const override { return "равнина"; }
    std::unique_ptr<Terrain> clone() const override { return std::make_unique<Plains>(*this); }
};

// лес замедляет всех, кроме лёгких юнитов, не пускает тяжёлых и даёт укрытие
class Forest : public Terrain {
public:
    bool isPassable(UnitType unitType) const override {
        return !UnitTypeInfo::getInfo(unitType).isHeavy();
    }
    double getSpeedModifier(UnitType unitType) const override {
        if (UnitTypeInfo::getInfo(unitType).isLight()) return 1.0;
        return 0.5;
    }
    int getDefenseBonus(UnitType) const override { return 2; }
    int getStuckTurns(UnitType) const override { return 0; }
    bool isBuildable() const override { return false; }
    char getSymbol() const override { return 'f'; }
    std::string getName() const override { return "лес"; }
    std::unique_ptr<Terrain> clone() const override { return std::make_unique<Forest>(*this); }
};

// болото замедляет всех, тяжёлый юнит в нём застревает и пропускает ход
class Swamp : public Terrain {
public:
    bool isPassable(UnitType) const override { return true; }
    double getSpeedModifier(UnitType) const override { return 0.5; }
    int getDefenseBonus(UnitType) const override { return 0; }
    int getStuckTurns(UnitType unitType) const override {
        return UnitTypeInfo::getInfo(unitType).isHeavy() ? 1 : 0;
    }
    bool isBuildable() const override { return false; }
    char getSymbol() const override { return '~'; }
    std::string getName() const override { return "болото"; }
    std::unique_ptr<Terrain> clone() const override { return std::make_unique<Swamp>(*this); }
};

// горы проходят только лёгкие юниты (разведчик)
class Mountains : public Terrain {
public:
    bool isPassable(UnitType unitType) const override {
        return UnitTypeInfo::getInfo(unitType).isLight();
    }
    double getSpeedModifier(UnitType) const override { return 0.5; }
    int getDefenseBonus(UnitType) const override { return 3; }
    int getStuckTurns(UnitType) const override { return 0; }
    bool isBuildable() const override { return false; }
    char getSymbol() const override { return '^'; }
    std::string getName() const override { return "горы"; }
    std::unique_ptr<Terrain> clone() const override { return std::make_unique<Mountains>(*this); }
};
