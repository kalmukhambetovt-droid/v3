#pragma once
#include "ResourceStock.h"
#include "UnitTypeInfo.h"
#include <memory>
#include <string>

class Unit;
class Building;

// итог одной атаки
struct CombatReport {
    int attackerId = 0;
    int targetId = 0;
    bool targetIsBuilding = false;
    int damage = 0;
    int targetHealthLeft = 0;
    bool targetDestroyed = false;
};

class IMediator {
public:
    virtual ~IMediator() = default;
    virtual void notifyUnitCreated(const std::shared_ptr<Unit>& unit) = 0;
    virtual void notifyResourceCollected(int unitId, ResourceType type, int amount) = 0;
    virtual void notifyCombatResolved(const CombatReport& report) = 0;
    virtual void notifyBuildingCompleted(const std::shared_ptr<Building>& building) = 0;
    virtual void notifyResearchCompleted(int laboratoryId, const std::string& technologyId) = 0;
    // false, если юнита некуда поставить и заказ надо отложить
    virtual bool notifyProductionFinished(int buildingId, UnitType type) = 0;
    virtual void notifyResourcesProduced(int buildingId, const ResourceStock& amount) = 0;
};
