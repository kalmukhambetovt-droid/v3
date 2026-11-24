#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Building.h"
#include "BuildingManager.h"
#include "Buildings.h"
#include "BuildingTypeInfo.h"
#include "TechnologyInfo.h"

Building::Building(int buildingId, Faction owner, const Position& place, const BuildingTypeInfo& info, bool completed)
    : id(buildingId), faction(owner), typeInfo(info),
      health(info.getMaxHealth(), info.getMaxHealth()),
      armor(info.getArmor()),
      position(place),
      buildProgress(completed ? info.getBuildTurns() : 0) {
    if (buildingId <= 0) throw std::invalid_argument("Идентификатор здания должен быть больше нуля");
}

BuildingOutput Building::update() {
    if (!isAlive()) return BuildingOutput{};
    if (!isCompleted()) {
        ++buildProgress;
        BuildingOutput output;
        output.completedNow = isCompleted();
        return output;
    }
    return produce();
}

BuildingOutput Building::produce() {
    return BuildingOutput{};
}

std::string Building::describeActivity() const {
    return isCompleted() ? "работает" : "строится";
}

void Building::takeDamage(int amount) {
    if (amount < 0) throw std::invalid_argument("Урон не может быть отрицательным");
    if (isAlive()) health.decrease(amount);
}

char Building::getSymbol() const {
    const char symbol = typeInfo.getSymbol();
    if (faction == Faction::Enemy) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
    }
    return symbol;
}

std::string Building::describe() const {
    std::ostringstream out;
    out << typeInfo.getName() << " #" << id << " [" << factionToString(faction) << "] " << toString(position)
        << " | HP " << health.getValue() << "/" << health.getMaxValue()
        << " | броня " << armor.getValue() << " | ";
    if (isCompleted()) {
        out << "построено";
    }
    else {
        out << "строится " << buildProgress << "/" << typeInfo.getBuildTurns()
            << " (" << buildProgress * 100 / typeInfo.getBuildTurns() << "%)";
    }
    out << " | " << describeActivity();
    return out.str();
}


ProductionBuilding::ProductionBuilding(int buildingId, Faction owner, const Position& place,
                                       BuildingType type, bool completed)
    : Building(buildingId, owner, place, BuildingTypeInfo::getInfo(type), completed), queue(), nextTicket(1) {
}

int ProductionBuilding::enqueue(UnitType type) {
    if (!isCompleted()) throw std::logic_error("Здание ещё строится");
    if (!canProduce(type)) throw std::invalid_argument("Здание не производит этот тип юнитов");
    if (queue.size() >= kMaxQueue) throw std::length_error("Очередь производства заполнена");
    const int ticket = nextTicket++;
    queue.push_back(ProductionOrder{ ticket, type, UnitTypeInfo::getInfo(type).getProductionTurns() });
    return ticket;
}

std::optional<UnitType> ProductionBuilding::cancel(int ticket) {
    for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (it->ticket == ticket) {
            const UnitType type = it->type;
            queue.erase(it);
            return type;
        }
    }
    return std::nullopt;
}

void ProductionBuilding::postpone(UnitType type) {
    queue.push_front(ProductionOrder{ nextTicket++, type, 1 });
}

// за ход продвигается только первый заказ, остальные ждут своей очереди
BuildingOutput ProductionBuilding::produce() {
    BuildingOutput output;
    if (queue.empty()) return output;
    ProductionOrder& current = queue.front();
    --current.turnsLeft;
    if (current.turnsLeft <= 0) {
        output.finishedUnits.push_back(current.type);
        queue.pop_front();
    }
    return output;
}

std::string ProductionBuilding::describeActivity() const {
    if (!isCompleted()) return "строится";
    if (queue.empty()) return "очередь пуста (производит: " + producibleList() + ")";
    std::ostringstream out;
    out << "очередь:";
    for (const ProductionOrder& order : queue) {
        out << " [#" << order.ticket << " " << unitTypeToString(order.type) << ", осталось " << order.turnsLeft << "]";
    }
    return out.str();
}

BuildingOutput PowerPlant::produce() {
    BuildingOutput output;
    output.produced.energy = kEnergyPerTurn;
    return output;
}

std::string PowerPlant::describeActivity() const {
    if (!isCompleted()) return "строится";
    return "даёт " + std::to_string(kEnergyPerTurn) + " энергии за ход";
}

void Laboratory::startResearch(const TechnologyInfo& technology) {
    if (!isCompleted()) throw std::logic_error("Лаборатория ещё строится");
    if (isBusy()) throw std::logic_error("Лаборатория уже занята исследованием");
    research = ActiveResearch{ technology.getId(), technology.getName(), technology.getResearchTurns() };
}

bool Laboratory::cancelResearch(const std::string& technologyId) {
    if (!research || research->technologyId != technologyId) return false;
    research.reset();
    return true;
}

BuildingOutput Laboratory::produce() {
    BuildingOutput output;
    if (!research) return output;
    --research->turnsLeft;
    if (research->turnsLeft <= 0) {
        output.finishedResearch = research->technologyId;
        research.reset();
    }
    return output;
}

std::string Laboratory::describeActivity() const {
    if (!isCompleted()) return "строится";
    if (!research) return "свободна";
    return "исследует " + research->technologyId + " (осталось ходов: " + std::to_string(research->turnsLeft) + ")";
}

std::string Warehouse::describeActivity() const {
    if (!isCompleted()) return "строится";
    return "вместимость склада +" + std::to_string(kCapacityBonus);
}

std::string Hospital::describeActivity() const {
    if (!isCompleted()) return "строится";
    return "лечение до полного HP за " + std::to_string(kTreatmentEnergyCost) + " энергии";
}

BuildingManager::BuildingManager() : buildings(), nextBuildingId(1) {
}

std::shared_ptr<Building> BuildingManager::makeBuilding(BuildingType type, int id, Faction faction,
                                                        const Position& position, bool completed) {
    switch (type) {
    case BuildingType::Barracks:   return std::make_shared<Barracks>(id, faction, position, completed);
    case BuildingType::Factory:    return std::make_shared<Factory>(id, faction, position, completed);
    case BuildingType::PowerPlant: return std::make_shared<PowerPlant>(id, faction, position, completed);
    case BuildingType::Laboratory: return std::make_shared<Laboratory>(id, faction, position, completed);
    case BuildingType::Warehouse:  return std::make_shared<Warehouse>(id, faction, position, completed);
    case BuildingType::Hospital:   return std::make_shared<Hospital>(id, faction, position, completed);
    }
    throw std::invalid_argument("Неизвестный тип здания");
}

std::shared_ptr<Building> BuildingManager::buildBuilding(BuildingType type, const Position& position,
                                                         Faction faction, bool instant) {
    if (getBuildingAt(position)) throw std::logic_error("В этой клетке уже есть здание");
    std::shared_ptr<Building> building = makeBuilding(type, nextBuildingId, faction, position, instant);
    ++nextBuildingId;
    buildings.push_back(building);
    return building;
}

void BuildingManager::startResearch(int laboratoryId, const TechnologyInfo& technology) {
    const auto laboratory = std::dynamic_pointer_cast<Laboratory>(getBuildingById(laboratoryId));
    if (!laboratory) throw std::invalid_argument("Здание не является лабораторией");
    laboratory->startResearch(technology);
}

int BuildingManager::produceUnit(int buildingId, UnitType type) {
    const auto producer = std::dynamic_pointer_cast<ProductionBuilding>(getBuildingById(buildingId));
    if (!producer) throw std::invalid_argument("Здание не производит юнитов");
    return producer->enqueue(type);
}

std::shared_ptr<Building> BuildingManager::getBuildingById(int buildingId) const {
    for (const auto& building : buildings) {
        if (building->getId() == buildingId) return building;
    }
    return nullptr;
}

std::shared_ptr<Building> BuildingManager::getBuildingAt(const Position& position) const {
    for (const auto& building : buildings) {
        if (building->isAlive() && building->getPosition() == position) return building;
    }
    return nullptr;
}

std::shared_ptr<Building> BuildingManager::findCompleted(BuildingType type, Faction faction) const {
    for (const auto& building : buildings) {
        if (building->getType() == type && building->getFaction() == faction &&
            building->isCompleted() && building->isAlive()) {
            return building;
        }
    }
    return nullptr;
}

std::shared_ptr<Laboratory> BuildingManager::findFreeLaboratory() const {
    for (const auto& building : buildings) {
        const auto laboratory = std::dynamic_pointer_cast<Laboratory>(building);
        if (laboratory && laboratory->getFaction() == Faction::Player && laboratory->isCompleted() &&
            laboratory->isAlive() && !laboratory->isBusy()) {
            return laboratory;
        }
    }
    return nullptr;
}

std::shared_ptr<Laboratory> BuildingManager::findResearching(const std::string& technologyId) const {
    for (const auto& building : buildings) {
        const auto laboratory = std::dynamic_pointer_cast<Laboratory>(building);
        if (laboratory && laboratory->getResearch() && laboratory->getResearch()->technologyId == technologyId) {
            return laboratory;
        }
    }
    return nullptr;
}

std::shared_ptr<Building> BuildingManager::removeBuilding(int buildingId) {
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        if ((*it)->getId() == buildingId) {
            std::shared_ptr<Building> removed = *it;
            buildings.erase(it);
            return removed;
        }
    }
    return nullptr;
}

std::optional<UnitType> BuildingManager::cancelProduction(int buildingId, int ticket) {
    const auto producer = std::dynamic_pointer_cast<ProductionBuilding>(getBuildingById(buildingId));
    if (!producer) return std::nullopt;
    return producer->cancel(ticket);
}

bool BuildingManager::cancelResearch(const std::string& technologyId) {
    const std::shared_ptr<Laboratory> laboratory = findResearching(technologyId);
    return laboratory && laboratory->cancelResearch(technologyId);
}

void BuildingManager::update() {
    const std::shared_ptr<IMediator> gameMediator = lockMediator();
    // идём по копии списка: посредник может удалить или добавить здание во время обработки
    const std::vector<std::shared_ptr<Building>> snapshot = buildings;
    for (const auto& building : snapshot) {
        // вражеские здания в сценариях только мишени и сами ничего не делают
        if (building->getFaction() != Faction::Player || !building->isAlive()) continue;

        const BuildingOutput output = building->update();
        if (output.completedNow) gameMediator->notifyBuildingCompleted(building);
        if (!output.produced.isEmpty()) gameMediator->notifyResourcesProduced(building->getId(), output.produced);
        for (UnitType type : output.finishedUnits) {
            if (!gameMediator->notifyProductionFinished(building->getId(), type)) {
                // вокруг здания нет места, юнит выйдет на следующем ходу
                const auto producer = std::dynamic_pointer_cast<ProductionBuilding>(building);
                if (producer) producer->postpone(type);
            }
        }
        if (!output.finishedResearch.empty()) {
            gameMediator->notifyResearchCompleted(building->getId(), output.finishedResearch);
        }
    }
}

int BuildingManager::count(Faction faction) const {
    return static_cast<int>(std::count_if(buildings.begin(), buildings.end(),
        [faction](const std::shared_ptr<Building>& building) {
            return building->isAlive() && building->getFaction() == faction;
        }));
}

BuildingTypeInfo::BuildingTypeInfo(BuildingType buildingType, std::string typeName, char typeSymbol,
                                   int hp, int armorValue, const ResourceStock& price, int turns)
    : type(buildingType), name(std::move(typeName)), symbol(typeSymbol), maxHealth(hp),
      armor(armorValue), cost(price), buildTurns(turns) {
    if (hp <= 0 || armorValue < 0 || turns <= 0) {
        throw std::invalid_argument("Неправильные характеристики типа здания");
    }
}

static const std::array<BuildingTypeInfo, kBuildingTypeCount>& buildingTable() {
    // порядок: тип, имя, символ, здоровье, броня, цена (дерево, металл, энергия), ходов на стройку
    static const std::array<BuildingTypeInfo, kBuildingTypeCount> table = { {
        BuildingTypeInfo(BuildingType::Barracks,   "barracks",   'K', 300, 3, ResourceStock{ 80, 20, 0 },  2),
        BuildingTypeInfo(BuildingType::Factory,    "factory",    'Z', 400, 4, ResourceStock{ 40, 80, 20 }, 3),
        BuildingTypeInfo(BuildingType::PowerPlant, "powerplant", 'P', 200, 2, ResourceStock{ 40, 40, 0 },  2),
        BuildingTypeInfo(BuildingType::Laboratory, "laboratory", 'L', 250, 2, ResourceStock{ 50, 50, 20 }, 3),
        BuildingTypeInfo(BuildingType::Warehouse,  "warehouse",  'D', 200, 1, ResourceStock{ 60, 0, 0 },   1),
        BuildingTypeInfo(BuildingType::Hospital,   "hospital",   'H', 250, 2, ResourceStock{ 40, 30, 10 }, 2)
    } };
    return table;
}

const BuildingTypeInfo& BuildingTypeInfo::getInfo(BuildingType type) {
    const size_t index = static_cast<size_t>(type);
    const auto& table = buildingTable();
    if (index >= table.size() || table[index].getType() != type) {
        throw std::invalid_argument("Неизвестный тип здания");
    }
    return table[index];
}

std::optional<BuildingType> buildingTypeFromString(const std::string& name) {
    // короткие варианты, которые удобно набирать руками
    if (name == "lab") return BuildingType::Laboratory;
    if (name == "power_plant") return BuildingType::PowerPlant;
    for (const BuildingTypeInfo& info : buildingTable()) {
        if (info.getName() == name) return info.getType();
    }
    return std::nullopt;
}

const std::string& buildingTypeToString(BuildingType type) {
    return BuildingTypeInfo::getInfo(type).getName();
}

std::string buildingTypeList() {
    std::string result;
    for (const BuildingTypeInfo& info : buildingTable()) {
        if (!result.empty()) result += ", ";
        result += info.getName();
    }
    return result;
}

TechnologyInfo::TechnologyInfo(std::string technologyId, std::string technologyName, const ResourceStock& price,
                               int turns, TechnologyEffect technologyEffect, int effectValue, UnitType unit)
    : id(std::move(technologyId)), name(std::move(technologyName)), cost(price), researchTurns(turns),
      effect(technologyEffect), value(effectValue), unlockedUnit(unit) {
    if (id.empty() || turns <= 0 || effectValue < 0) {
        throw std::invalid_argument("Неправильное описание технологии");
    }
}

const std::vector<TechnologyInfo>& TechnologyInfo::all() {
    // порядок: id, название, цена (дерево, металл, энергия), ходов, эффект, величина, открываемый юнит
    static const std::vector<TechnologyInfo> catalog = {
        TechnologyInfo("attack_upgrade", "Улучшение атаки", ResourceStock{ 0, 40, 30 }, 2,
                       TechnologyEffect::AttackBonus, 3, UnitType::Infantry),
        TechnologyInfo("armor_upgrade", "Улучшение брони", ResourceStock{ 20, 50, 30 }, 2,
                       TechnologyEffect::ArmorBonus, 1, UnitType::Infantry),
        TechnologyInfo("unlock_tank", "Танковые технологии", ResourceStock{ 0, 60, 40 }, 3,
                       TechnologyEffect::UnlockUnit, 0, UnitType::Tank)
    };
    return catalog;
}

const TechnologyInfo* TechnologyInfo::find(const std::string& technologyId) {
    for (const TechnologyInfo& technology : all()) {
        if (technology.getId() == technologyId) return &technology;
    }
    return nullptr;
}

std::string technologyList() {
    std::string result;
    for (const TechnologyInfo& technology : TechnologyInfo::all()) {
        if (!result.empty()) result += ", ";
        result += technology.getId();
    }
    return result;
}
