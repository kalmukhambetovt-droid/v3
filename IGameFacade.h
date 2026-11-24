#pragma once
#include "CommandTypes.h"
#include "UnitTypeInfo.h"
#include "BuildingTypeInfo.h"
#include "ResourceStock.h"
#include "GameTypes.h"
#include <string>

class IGameFacade {
public:
    virtual ~IGameFacade() = default;

    // управление юнитами
    virtual CommandResult createUnit(UnitType type, const Position& position) = 0;
    virtual CommandResult moveUnit(int unitId, const Position& target) = 0;
    virtual CommandResult attack(int attackerId, int targetId) = 0;
    virtual CommandResult attackBuilding(int attackerId, int buildingId) = 0;
    virtual CommandResult collectResource(int unitId, int resourcePointId) = 0;
    virtual CommandResult heal(int unitId, int targetId) = 0;
    virtual CommandResult healAtHospital(int unitId, int hospitalId) = 0;

    // управление базой
    virtual CommandResult buildBuilding(BuildingType type, const Position& position) = 0;
    virtual CommandResult research(const std::string& technologyId) = 0;
    virtual CommandResult distributeResources(ResourceType from, const DistributionTarget& to, int amount) = 0;
    virtual CommandResult produceUnit(int buildingId, UnitType type) = 0;
    virtual std::string getStats(StatsScope scope, int unitId) = 0;

    virtual CommandResult endTurn(int turns) = 0;
    virtual CommandResult spawnEnemyUnit(UnitType type, const Position& position) = 0;
    virtual CommandResult spawnEnemyBuilding(BuildingType type, const Position& position) = 0;
    virtual CommandResult undo() = 0;
    virtual std::string getHistory() const = 0;
    virtual void printMessage(const std::string& message) = 0;
    virtual bool processInput(const std::string& input) = 0;
};
