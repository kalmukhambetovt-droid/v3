#pragma once
#include "Building.h"
#include "TechnologyInfo.h"
#include <deque>
#include <optional>
#include <string>

// заказ в очереди производства
struct ProductionOrder {
    int ticket;      // номер заказа, по нему заказ можно отменить
    UnitType type;
    int turnsLeft;
};

// общий предок зданий с очередью производства юнитов
class ProductionBuilding : public Building {
private:
    std::deque<ProductionOrder> queue;
    int nextTicket;

protected:
    ProductionBuilding(int buildingId, Faction owner, const Position& place, BuildingType type, bool completed);

public:
    static constexpr size_t kMaxQueue = 5;

    virtual bool canProduce(UnitType type) const = 0;
    virtual std::string producibleList() const = 0;

    int enqueue(UnitType type);
    // тип отменённого заказа, если заказ ещё в очереди
    std::optional<UnitType> cancel(int ticket);
    // юнита некуда поставить: заказ возвращается в начало очереди на следующий ход
    void postpone(UnitType type);

    BuildingOutput produce() override;
    std::string describeActivity() const override;
    const std::deque<ProductionOrder>& getQueue() const { return queue; }
};

// казарма: пехота, разведчики, медики
class Barracks : public ProductionBuilding {
public:
    Barracks(int buildingId, Faction owner, const Position& place, bool completed)
        : ProductionBuilding(buildingId, owner, place, BuildingType::Barracks, completed) {
    }
    bool canProduce(UnitType type) const override {
        return type == UnitType::Infantry || type == UnitType::Scout || type == UnitType::Medic;
    }
    std::string producibleList() const override { return "infantry, scout, medic"; }
};

// завод: танки и рабочие
class Factory : public ProductionBuilding {
public:
    Factory(int buildingId, Faction owner, const Position& place, bool completed)
        : ProductionBuilding(buildingId, owner, place, BuildingType::Factory, completed) {
    }
    bool canProduce(UnitType type) const override {
        return type == UnitType::Tank || type == UnitType::Worker;
    }
    std::string producibleList() const override { return "tank, worker"; }
};

// электростанция: каждый ход даёт энергию
class PowerPlant : public Building {
public:
    static constexpr int kEnergyPerTurn = 15;

    PowerPlant(int buildingId, Faction owner, const Position& place, bool completed)
        : Building(buildingId, owner, place, BuildingTypeInfo::getInfo(BuildingType::PowerPlant), completed) {
    }
    BuildingOutput produce() override;
    std::string describeActivity() const override;
};

// активное исследование лаборатории
struct ActiveResearch {
    std::string technologyId;
    std::string name;
    int turnsLeft;
};

// лаборатория: одно исследование за раз
class Laboratory : public Building {
private:
    std::optional<ActiveResearch> research;

public:
    Laboratory(int buildingId, Faction owner, const Position& place, bool completed)
        : Building(buildingId, owner, place, BuildingTypeInfo::getInfo(BuildingType::Laboratory), completed) {
    }
    bool isBusy() const { return research.has_value(); }
    void startResearch(const TechnologyInfo& technology);
    bool cancelResearch(const std::string& technologyId);
    const std::optional<ActiveResearch>& getResearch() const { return research; }

    BuildingOutput produce() override;
    std::string describeActivity() const override;
};

// склад: после постройки увеличивает вместимость запасов базы
class Warehouse : public Building {
public:
    static constexpr int kCapacityBonus = 250;

    Warehouse(int buildingId, Faction owner, const Position& place, bool completed)
        : Building(buildingId, owner, place, BuildingTypeInfo::getInfo(BuildingType::Warehouse), completed) {
    }
    std::string describeActivity() const override;
};

// госпиталь: полностью лечит пришедшего юнита за энергию
class Hospital : public Building {
public:
    static constexpr int kTreatmentEnergyCost = 10;

    Hospital(int buildingId, Faction owner, const Position& place, bool completed)
        : Building(buildingId, owner, place, BuildingTypeInfo::getInfo(BuildingType::Hospital), completed) {
    }
    std::string describeActivity() const override;
};
