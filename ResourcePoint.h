#pragma once
#include "ResourceStock.h"
#include "GameTypes.h"
#include <stdexcept>
#include <string>
#include <utility>

// ресурсная точка на карте (лесопилка, рудник, кристаллы), запас со временем кончается
class ResourcePoint {
private:
    int id;
    std::string name;
    ResourceType type;
    Position position;
    int remaining;

public:
    ResourcePoint(int pointId, std::string pointName, ResourceType resourceType, const Position& place, int amount)
        : id(pointId), name(std::move(pointName)), type(resourceType), position(place), remaining(amount) {
        if (pointId <= 0 || amount < 0) throw std::invalid_argument("Неправильные параметры ресурсной точки");
    }

    // возвращает, сколько реально удалось добыть
    int extract(int amount) {
        if (amount < 0) throw std::invalid_argument("Нельзя добыть отрицательное количество");
        const int taken = amount < remaining ? amount : remaining;
        remaining -= taken;
        return taken;
    }

    int getId() const { return id; }
    const std::string& getName() const { return name; }
    ResourceType getType() const { return type; }
    const Position& getPosition() const { return position; }
    int getRemaining() const { return remaining; }
    bool isDepleted() const { return remaining == 0; }
};
