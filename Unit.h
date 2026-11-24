#pragma once
#include "UnitTypeInfo.h"
#include "GameTypes.h"
#include "Health.h"
#include "Armor.h"
#include "Attack.h"
#include <string>

class Building;

class Unit {
private:
    int id;
    Faction faction;

protected:
    const UnitTypeInfo& typeInfo;
    Health health;
    Armor armor;
    Attack damage;
    Position position;
    UnitOrder currentOrder;
    int movePoints;      // оставшийся запас хода в текущем ходу
    int stuckTurns;      // сколько ходов юнит ещё пропускает
    int terrainDefense;  // бонус защиты клетки, на которой стоит юнит
    bool actedThisTurn;  // атака, лечение или добыча доступны один раз за ход

    Unit(int unitId, Faction owner, const Position& start, const UnitTypeInfo& info);

public:
    virtual ~Unit() = default;
    Unit(const Unit&) = delete;
    Unit& operator=(const Unit&) = delete;

    virtual std::string getTypeName() const = 0;
    virtual std::string describeAbility() const = 0;
    virtual int getGatherAmount() const { return 0; }
    virtual int getHealPower() const { return 0; }
    virtual int getHealRange() const { return 0; }

    void move(const Position& target, int cost, int newTerrainDefense, int stuckFor);
    void returnTo(const Position& previous, int restoredPoints, int previousTerrainDefense);
    void attack(Unit& target, int amount);
    void attackBuilding(Building& target, int amount);
    void takeDamage(int amount);
    void heal(int amount);
    void healUnit(Unit& target);
    void receiveTreatment();
    void markCollecting();
    void startTurn();
    void upgradeAttack(int amount);
    void upgradeArmor(int amount);
    void setTerrainDefense(int defense);

    int getId() const { return id; }
    Faction getFaction() const { return faction; }
    UnitType getUnitType() const { return typeInfo.getType(); }
    const UnitTypeInfo& getTypeInfo() const { return typeInfo; }
    int getHealth() const { return health.getValue(); }
    int getMaxHealth() const { return health.getMaxValue(); }
    int getBaseArmor() const { return armor.getValue(); }
    int getArmorValue() const { return armor.getValue() + terrainDefense; }
    int getTerrainDefense() const { return terrainDefense; }
    int getDamage() const { return damage.getValue(); }
    const Position& getPosition() const { return position; }
    UnitOrder getOrder() const { return currentOrder; }
    int getMovePoints() const { return movePoints; }
    int getMoveRange() const { return typeInfo.getMoveRange(); }
    int getAttackRange() const { return typeInfo.getAttackRange(); }
    int getStuckTurns() const { return stuckTurns; }
    bool hasActed() const { return actedThisTurn; }
    bool isAlive() const { return !health.isZero(); }
    bool isStuck() const { return stuckTurns > 0 || currentOrder == UnitOrder::Stuck; }
    // юниты игрока строчными буквами, вражеские заглавными
    char getSymbol() const;

    std::string describe() const;
};
