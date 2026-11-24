
#include <algorithm>
#include <stdexcept>
#include "ResourceStock.h"
#include "ResourceSystem.h"

int ResourceStock::get(ResourceType type) const {
    switch (type) {
    case ResourceType::Wood:   return wood;
    case ResourceType::Metal:  return metal;
    case ResourceType::Energy: return energy;
    }
    throw std::invalid_argument("Неизвестный тип ресурса");
}

bool ResourceStock::covers(const ResourceStock& cost) const {
    return wood >= cost.wood && metal >= cost.metal && energy >= cost.energy;
}

bool ResourceStock::isEmpty() const {
    return wood == 0 && metal == 0 && energy == 0;
}

ResourceStock& ResourceStock::operator+=(const ResourceStock& other) {
    wood += other.wood;
    metal += other.metal;
    energy += other.energy;
    return *this;
}

// запас не должен уходить в минус, поэтому сначала проверяем, хватает ли ресурсов
ResourceStock& ResourceStock::operator-=(const ResourceStock& other) {
    if (!covers(other)) throw std::logic_error("Недостаточно ресурсов для списания");
    wood -= other.wood;
    metal -= other.metal;
    energy -= other.energy;
    return *this;
}

ResourceStock operator+(ResourceStock left, const ResourceStock& right) {
    left += right;
    return left;
}

ResourceStock operator-(ResourceStock left, const ResourceStock& right) {
    left -= right;
    return left;
}

ResourceStock makeStock(ResourceType type, int amount) {
    if (amount < 0) throw std::invalid_argument("Количество ресурса не может быть отрицательным");
    ResourceStock stock;
    switch (type) {
    case ResourceType::Wood:   stock.wood = amount; break;
    case ResourceType::Metal:  stock.metal = amount; break;
    case ResourceType::Energy: stock.energy = amount; break;
    }
    return stock;
}

std::string toString(const ResourceStock& stock) {
    return "дерево " + std::to_string(stock.wood) + ", металл " + std::to_string(stock.metal)
        + ", энергия " + std::to_string(stock.energy);
}

const char* resourceTypeToString(ResourceType type) {
    switch (type) {
    case ResourceType::Wood:   return "дерево";
    case ResourceType::Metal:  return "металл";
    case ResourceType::Energy: return "энергия";
    }
    throw std::invalid_argument("Неизвестный тип ресурса");
}

std::optional<ResourceType> resourceTypeFromString(const std::string& name) {
    if (name == "wood") return ResourceType::Wood;
    if (name == "metal") return ResourceType::Metal;
    if (name == "energy") return ResourceType::Energy;
    return std::nullopt;
}


ResourceSystem::ResourceSystem(const ResourceStock& start, int baseCapacity)
    : wood(start.wood), metal(start.metal), energy(start.energy), capacity(baseCapacity) {
    if (baseCapacity <= 0) throw std::invalid_argument("Вместимость склада должна быть больше нуля");
    if (start.wood < 0 || start.metal < 0 || start.energy < 0 ||
        start.wood > baseCapacity || start.metal > baseCapacity || start.energy > baseCapacity) {
        throw std::invalid_argument("Начальные запасы должны быть от нуля до вместимости склада");
    }
}

int& ResourceSystem::slot(ResourceType type) {
    switch (type) {
    case ResourceType::Wood:   return wood;
    case ResourceType::Metal:  return metal;
    case ResourceType::Energy: return energy;
    }
    throw std::invalid_argument("Неизвестный тип ресурса");
}

int ResourceSystem::addResources(ResourceType type, int amount) {
    if (amount < 0) throw std::invalid_argument("Количество ресурсов не может быть отрицательным");
    int& value = slot(type);
    const int stored = std::min(amount, capacity - value);
    value += stored;
    return stored;
}

ResourceStock ResourceSystem::addResources(const ResourceStock& amount) {
    ResourceStock stored;
    stored.wood = addResources(ResourceType::Wood, amount.wood);
    stored.metal = addResources(ResourceType::Metal, amount.metal);
    stored.energy = addResources(ResourceType::Energy, amount.energy);
    return stored;
}

bool ResourceSystem::canAfford(const ResourceStock& cost, int buildingId) const {
    return (getStock() + getAllocation(buildingId)).covers(cost);
}

bool ResourceSystem::spendResources(const ResourceStock& cost, int buildingId) {
    if (cost.wood < 0 || cost.metal < 0 || cost.energy < 0) {
        throw std::invalid_argument("Стоимость не может быть отрицательной");
    }
    if (!canAfford(cost, buildingId)) return false;

    // из выделенного зданию берём сколько есть, остальное из общего запаса
    ResourceStock fromAllocation;
    const auto allocation = allocations.find(buildingId);
    if (buildingId != 0 && allocation != allocations.end()) {
        fromAllocation.wood = std::min(cost.wood, allocation->second.wood);
        fromAllocation.metal = std::min(cost.metal, allocation->second.metal);
        fromAllocation.energy = std::min(cost.energy, allocation->second.energy);
        allocation->second -= fromAllocation;
    }
    const ResourceStock fromStock = cost - fromAllocation;
    wood -= fromStock.wood;
    metal -= fromStock.metal;
    energy -= fromStock.energy;
    return true;
}

bool ResourceSystem::distribute(ResourceType type, int buildingId, int amount) {
    if (amount <= 0) throw std::invalid_argument("Количество для распределения должно быть больше нуля");
    if (buildingId <= 0) throw std::invalid_argument("Неправильный идентификатор здания");
    int& value = slot(type);
    if (value < amount) return false;
    value -= amount;
    allocations[buildingId] += makeStock(type, amount);
    return true;
}

bool ResourceSystem::returnAllocation(ResourceType type, int buildingId, int amount) {
    if (amount <= 0) throw std::invalid_argument("Количество для возврата должно быть больше нуля");
    const auto allocation = allocations.find(buildingId);
    if (allocation == allocations.end() || allocation->second.get(type) < amount) return false;
    allocation->second -= makeStock(type, amount);
    addResources(type, amount);
    if (allocation->second.isEmpty()) allocations.erase(allocation);
    return true;
}

void ResourceSystem::releaseAllocation(int buildingId) {
    const auto allocation = allocations.find(buildingId);
    if (allocation == allocations.end()) return;
    addResources(allocation->second);
    allocations.erase(allocation);
}

void ResourceSystem::increaseCapacity(int amount) {
    if (amount < 0) throw std::invalid_argument("Прирост вместимости не может быть отрицательным");
    capacity += amount;
}

void ResourceSystem::addResourcePoint(const ResourcePoint& point) {
    if (findPoint(point.getId())) throw std::logic_error("Ресурсная точка с таким номером уже есть");
    points.push_back(point);
}

ResourcePoint* ResourceSystem::findPoint(int pointId) {
    for (ResourcePoint& point : points) {
        if (point.getId() == pointId) return &point;
    }
    return nullptr;
}

const ResourcePoint* ResourceSystem::findPoint(int pointId) const {
    for (const ResourcePoint& point : points) {
        if (point.getId() == pointId) return &point;
    }
    return nullptr;
}

const ResourcePoint* ResourceSystem::findPointAt(const Position& position) const {
    for (const ResourcePoint& point : points) {
        if (point.getPosition() == position) return &point;
    }
    return nullptr;
}

ResourceStock ResourceSystem::getAllocation(int buildingId) const {
    const auto allocation = allocations.find(buildingId);
    if (allocation == allocations.end()) return ResourceStock{};
    return allocation->second;
}
