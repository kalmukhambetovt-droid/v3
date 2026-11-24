#pragma once
#include "IMediator.h"
#include "CommandTypes.h"
#include "Base.h"
#include "UnitManager.h"
#include "BuildingManager.h"
#include "ResourceSystem.h"
#include "CombatSystem.h"
#include "GameMap.h"
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <utility>


class GameMediator : public IMediator {
private:

    struct PrivateTag {
        explicit PrivateTag() = default;
    };

    std::ostream& log;
    int turn;
    Base base;
    UnitManager unitManager;
    BuildingManager buildingManager;
    ResourceSystem resourceSystem;
    CombatSystem combatSystem;
    GameMap map;

    static constexpr int kMapSize = 16;
    static constexpr int kSpawnRadius = 5;   // юнит появляется у базы
    static constexpr int kBuildRadius = 8;   // строить можно недалеко от базы
    static constexpr int kMaxTurnsAtOnce = 20;

    void setupWorld();
    bool isCellFree(const Position& position, int ignoreUnitId = 0) const;
    std::optional<Position> findFreeCellNear(const Position& center, UnitType type) const;
    // клетка рядом с целью, до которой юнит дойдёт в этом ходу, и цена пути
    std::optional<std::pair<Position, int>> findApproach(const Unit& unit, const Position& target) const;
    std::shared_ptr<Unit> findPlayerUnit(int unitId, CommandResult& error) const;
    std::shared_ptr<Building> resolveDistributionTarget(const DistributionTarget& target) const;
    void walkTo(Unit& unit, const Position& target, int cost);

    void notifyUnitCreated(const std::shared_ptr<Unit>& unit) override;
    void notifyResourceCollected(int unitId, ResourceType type, int amount) override;
    void notifyCombatResolved(const CombatReport& report) override;
    void notifyBuildingCompleted(const std::shared_ptr<Building>& building) override;
    void notifyResearchCompleted(int laboratoryId, const std::string& technologyId) override;
    bool notifyProductionFinished(int buildingId, UnitType type) override;
    void notifyResourcesProduced(int buildingId, const ResourceStock& amount) override;

    // части статистики
    std::string summaryStats() const;
    std::string unitStats() const;
    std::string unitStatus(int unitId) const;
    std::string buildingStats() const;
    std::string resourceStats() const;
    std::string researchStats() const;
    std::string mapView() const;

public:
    GameMediator(PrivateTag, std::ostream& logStream);
    static std::shared_ptr<GameMediator> create(std::ostream& logStream);

    GameMediator(const GameMediator&) = delete;
    GameMediator& operator=(const GameMediator&) = delete;
    GameMediator(GameMediator&&) = delete;
    GameMediator& operator=(GameMediator&&) = delete;

    CommandResult createUnit(UnitType type, const Position& position);
    CommandResult spawnEnemyUnit(UnitType type, const Position& position);
    CommandResult spawnEnemyBuilding(BuildingType type, const Position& position);
    CommandResult moveUnit(int unitId, const Position& target);
    CommandResult resolveCombat(int attackerId, int targetId);
    CommandResult attackBuilding(int attackerId, int buildingId);
    CommandResult collectResource(int unitId, int resourcePointId);
    CommandResult healUnit(int healerId, int targetId);
    CommandResult healAtHospital(int unitId, int hospitalId);
    CommandResult buildBuilding(BuildingType type, const Position& position);
    CommandResult startResearch(const std::string& technologyId);
    CommandResult distributeResources(ResourceType type, const DistributionTarget& target, int amount);
    CommandResult produceUnit(int buildingId, UnitType type);
    CommandResult advanceTurn(int turns);
    CommandResult getStats(StatsScope scope, int unitId = 0) const;

    // операции отмены 
    CommandResult undoCreateUnit(int unitId);
    CommandResult undoMove(int unitId, const Position& from, int spentPoints);
    CommandResult cancelBuilding(int buildingId);
    CommandResult cancelResearch(const std::string& technologyId);
    CommandResult undoDistribution(ResourceType type, int buildingId, int amount);
    CommandResult cancelProduction(int buildingId, int ticket);

    std::optional<Position> getUnitPosition(int unitId) const;
    int getTurn() const { return turn; }
    const Base& getBase() const { return base; }
    const UnitManager& getUnits() const { return unitManager; }
    const BuildingManager& getBuildings() const { return buildingManager; }
    const ResourceSystem& getResources() const { return resourceSystem; }
    const GameMap& getMap() const { return map; }
};
