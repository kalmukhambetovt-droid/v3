#pragma once
#include "GameTypes.h"
#include <memory>
#include <string>
#include <vector>

class Unit;
class Building;

// база игрока: характеристики и учёт своих юнитов, зданий и изученных технологий
class Base {
private:
    int health;
    int maxHealth;
    int defenseLevel;
    int maxUnits;
    int resources;      // сколько ресурсов рабочие доставили на базу за игру
    Position position;

    // юнитами и зданиями база владеет совместно с менеджерами
    // менеджер убирает погибший объект, а база оставляет его в истории потерь
    std::vector<std::shared_ptr<Unit>> units;
    std::vector<std::shared_ptr<Building>> buildings;
    std::vector<std::shared_ptr<Unit>> lostUnits;
    std::vector<std::shared_ptr<Building>> lostBuildings;
    std::vector<std::string> technologies;

public:
    Base(const Position& place, int startHealth = 500, int maxU = 20);
    // база учитывает объекты одной конкретной игры, копирование не имеет смысла
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;

    void registerUnit(const std::shared_ptr<Unit>& unit);
    void registerUnitLost(int unitId);
    // отмена создания: юнит не погиб, а просто исчез из учёта
    void forgetUnit(int unitId);
    void registerBuilding(const std::shared_ptr<Building>& building);
    void registerBuildingLost(int buildingId);
    void forgetBuilding(int buildingId);
    void registerDelivery(int amount);
    void registerTechnology(const std::string& technologyId);
    bool hasTechnology(const std::string& technologyId) const;
    void increaseDefense(int amount);
    void takeDamage(int amount);

    bool hasFreeSlot() const;
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getDefenseLevel() const { return defenseLevel; }
    int getMaxUnits() const { return maxUnits; }
    int getResources() const { return resources; }
    const Position& getPosition() const { return position; }
    int getUnitCount() const { return static_cast<int>(units.size()); }
    int getBuildingCount() const { return static_cast<int>(buildings.size()); }
    int getLostUnitCount() const { return static_cast<int>(lostUnits.size()); }
    int getLostBuildingCount() const { return static_cast<int>(lostBuildings.size()); }
    const std::vector<std::shared_ptr<Unit>>& getUnits() const { return units; }
    const std::vector<std::shared_ptr<Building>>& getBuildings() const { return buildings; }
    const std::vector<std::string>& getTechnologies() const { return technologies; }
    bool isAlive() const { return health > 0; }
};
