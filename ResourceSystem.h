#pragma once
#include "ResourceStock.h"
#include "ResourcePoint.h"
#include <map>
#include <vector>

// склад базы: общий запас, выделенные зданиям ресурсы и ресурсные точки карты
class ResourceSystem {
private:
    int wood;
    int metal;
    int energy;
    int capacity;                          // предел по каждому ресурсу на общем складе
    std::map<int, ResourceStock> allocations; // id здания = выделенные ему ресурсы
    std::vector<ResourcePoint> points;

    int& slot(ResourceType type);

public:
    ResourceSystem(const ResourceStock& start, int baseCapacity);

    int addResources(ResourceType type, int amount);
    ResourceStock addResources(const ResourceStock& amount);
    bool canAfford(const ResourceStock& cost, int buildingId = 0) const;
    bool spendResources(const ResourceStock& cost, int buildingId = 0);
    bool distribute(ResourceType type, int buildingId, int amount);
    bool returnAllocation(ResourceType type, int buildingId, int amount);
    void releaseAllocation(int buildingId);
    void increaseCapacity(int amount);

    void addResourcePoint(const ResourcePoint& point);
    ResourcePoint* findPoint(int pointId);
    const ResourcePoint* findPoint(int pointId) const;
    const ResourcePoint* findPointAt(const Position& position) const;

    int getWood() const { return wood; }
    int getMetal() const { return metal; }
    int getEnergy() const { return energy; }
    ResourceStock getStock() const { return ResourceStock{ wood, metal, energy }; }
    int getCapacity() const { return capacity; }
    ResourceStock getAllocation(int buildingId) const;
    const std::map<int, ResourceStock>& getAllocations() const { return allocations; }
    const std::vector<ResourcePoint>& getPoints() const { return points; }
};
