#pragma once
#include "MediatorColleague.h"
#include "Buildings.h"
#include <memory>
#include <vector>

// реестр зданий: строительство, исследования, производство, ход зданий
class BuildingManager : public MediatorColleague {
private:
    std::vector<std::shared_ptr<Building>> buildings;
    int nextBuildingId;

    static std::shared_ptr<Building> makeBuilding(BuildingType type, int id, Faction faction,
                                                  const Position& position, bool completed);

public:
    BuildingManager();
    BuildingManager(const BuildingManager&) = delete;
    BuildingManager& operator=(const BuildingManager&) = delete;

    // instant = true для вражеских зданий сценария, они появляются уже построенными
    std::shared_ptr<Building> buildBuilding(BuildingType type, const Position& position, Faction faction,
                                            bool instant = false);
    void startResearch(int laboratoryId, const TechnologyInfo& technology);
    // номер заказа в очереди здания
    int produceUnit(int buildingId, UnitType type);
    std::shared_ptr<Building> getBuildingById(int buildingId) const;
    std::shared_ptr<Building> getBuildingAt(const Position& position) const;
    std::shared_ptr<Building> findCompleted(BuildingType type, Faction faction) const;
    std::shared_ptr<Laboratory> findFreeLaboratory() const;
    std::shared_ptr<Laboratory> findResearching(const std::string& technologyId) const;
    std::shared_ptr<Building> removeBuilding(int buildingId);

    std::optional<UnitType> cancelProduction(int buildingId, int ticket);
    bool cancelResearch(const std::string& technologyId);

    // один ход для зданий игрока: о результатах узнаёт посредник
    void update();

    int count(Faction faction) const;
    const std::vector<std::shared_ptr<Building>>& getBuildings() const { return buildings; }
};
