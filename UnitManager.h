#pragma once
#include "MediatorColleague.h"
#include "UnitFactory.h"
#include <memory>
#include <vector>

// реестр живых юнитов игры: создание через фабрики, перемещение, удаление, поиск
class UnitManager : public MediatorColleague {
private:
    UnitFactoryRegistry factories;
    std::vector<std::shared_ptr<Unit>> units;
    int nextUnitId;
    int attackUpgrade;
    int armorUpgrade;

public:
    UnitManager();
    UnitManager(const UnitManager&) = delete;
    UnitManager& operator=(const UnitManager&) = delete;

    std::shared_ptr<Unit> createUnit(UnitType type, const Position& position, Faction faction, int terrainDefense);
    bool moveUnit(int unitId, const Position& target, int cost, int terrainDefense, int stuckTurns);
    std::shared_ptr<Unit> removeUnit(int unitId);
    std::shared_ptr<Unit> getUnitById(int unitId) const;
    std::shared_ptr<Unit> getUnitAt(const Position& position) const;

    void reportCollected(int unitId, ResourceType type, int amount);
    void applyAttackUpgrade(int amount);
    void applyArmorUpgrade(int amount);
    void startTurn();

    int count(Faction faction) const;
    const std::vector<std::shared_ptr<Unit>>& getUnits() const { return units; }
};
