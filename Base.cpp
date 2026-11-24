#include "Base.h"
#include "Unit.h"
#include "Building.h"
#include <algorithm>
#include <stdexcept>

Base::Base(const Position& place, int startHealth, int maxU)
    : health(startHealth), maxHealth(startHealth), defenseLevel(1), maxUnits(maxU), resources(0), position(place) {
    if (startHealth <= 0 || maxU <= 0) {
        throw std::invalid_argument("Здоровье базы и лимит юнитов должны быть больше нуля");
    }
}

void Base::registerUnit(const std::shared_ptr<Unit>& unit) {
    if (!unit) throw std::invalid_argument("Нельзя учитывать пустой указатель на юнита");
    if (!hasFreeSlot()) throw std::length_error("Достигнут максимальный лимит юнитов базы");
    if (std::find(units.begin(), units.end(), unit) != units.end()) {
        throw std::logic_error("Этот юнит уже учитывается базой");
    }
    units.push_back(unit);
}

void Base::registerUnitLost(int unitId) {
    for (auto it = units.begin(); it != units.end(); ++it) {
        if ((*it)->getId() == unitId) {
            // сначала запоминаем в истории, потом убираем из списка живых
            lostUnits.push_back(*it);
            units.erase(it);
            return;
        }
    }
}

void Base::forgetUnit(int unitId) {
    units.erase(std::remove_if(units.begin(), units.end(),
        [unitId](const std::shared_ptr<Unit>& unit) { return unit->getId() == unitId; }), units.end());
}

void Base::registerBuilding(const std::shared_ptr<Building>& building) {
    if (!building) throw std::invalid_argument("Нельзя учитывать пустой указатель на здание");
    if (std::find(buildings.begin(), buildings.end(), building) != buildings.end()) {
        throw std::logic_error("Это здание уже учитывается базой");
    }
    buildings.push_back(building);
}

void Base::registerBuildingLost(int buildingId) {
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        if ((*it)->getId() == buildingId) {
            lostBuildings.push_back(*it);
            buildings.erase(it);
            return;
        }
    }
}

void Base::forgetBuilding(int buildingId) {
    buildings.erase(std::remove_if(buildings.begin(), buildings.end(),
        [buildingId](const std::shared_ptr<Building>& building) { return building->getId() == buildingId; }),
        buildings.end());
}

void Base::registerDelivery(int amount) {
    if (amount < 0) throw std::invalid_argument("Количество ресурсов не может быть отрицательным");
    resources += amount;
}

void Base::registerTechnology(const std::string& technologyId) {
    if (!hasTechnology(technologyId)) technologies.push_back(technologyId);
}

bool Base::hasTechnology(const std::string& technologyId) const {
    return std::find(technologies.begin(), technologies.end(), technologyId) != technologies.end();
}

void Base::increaseDefense(int amount) {
    if (amount < 0) throw std::invalid_argument("Прирост защиты не может быть отрицательным");
    defenseLevel += amount;
}

void Base::takeDamage(int amount) {
    if (amount < 0) throw std::invalid_argument("Урон не может быть отрицательным");
    // защита базы может полностью поглотить слабый удар
    const int real = amount > defenseLevel ? amount - defenseLevel : 0;
    health = std::max(0, health - real);
}

bool Base::hasFreeSlot() const {
    return units.size() < static_cast<size_t>(maxUnits);
}
