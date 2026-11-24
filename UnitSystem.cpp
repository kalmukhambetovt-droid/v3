#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Building.h"
#include "Unit.h"
#include "UnitFactory.h"
#include "UnitManager.h"
#include "UnitTypeInfo.h"

Unit::Unit(int unitId, Faction owner, const Position& start, const UnitTypeInfo& info)
    : id(unitId), faction(owner), typeInfo(info),
      health(info.getMaxHealth(), info.getMaxHealth()),
      armor(info.getArmor()),
      damage(info.getDamage()),
      position(start),
      currentOrder(UnitOrder::Idle),
      movePoints(info.getMoveRange()),
      stuckTurns(0),
      terrainDefense(0),
      actedThisTurn(false) {
    if (unitId <= 0) throw std::invalid_argument("Идентификатор юнита должен быть больше нуля");
}

void Unit::move(const Position& target, int cost, int newTerrainDefense, int stuckFor) {
    // посредник проверяет всё заранее, здесь защищаем инварианты самого юнита
    if (!isAlive()) throw std::logic_error("Уничтоженный юнит не может двигаться");
    if (isStuck()) throw std::logic_error("Юнит застрял и пропускает ход");
    if (cost < 0 || cost > movePoints) throw std::logic_error("Не хватает запаса хода");

    position = target;
    movePoints -= cost;
    terrainDefense = newTerrainDefense;
    currentOrder = UnitOrder::Move;
    if (stuckFor > 0) {
        stuckTurns = stuckFor;
        movePoints = 0;
        currentOrder = UnitOrder::Stuck;
    }
}

void Unit::returnTo(const Position& previous, int restoredPoints, int previousTerrainDefense) {
    if (restoredPoints < 0) throw std::invalid_argument("Возвращаемый запас хода не может быть отрицательным");
    position = previous;
    terrainDefense = previousTerrainDefense;
    stuckTurns = 0;
    movePoints += restoredPoints;
    if (movePoints > typeInfo.getMoveRange()) movePoints = typeInfo.getMoveRange();
    currentOrder = UnitOrder::Idle;
}

void Unit::attack(Unit& target, int amount) {
    if (!isAlive()) throw std::logic_error("Уничтоженный юнит не может атаковать");
    if (actedThisTurn) throw std::logic_error("Юнит уже действовал в этом ходу");
    currentOrder = UnitOrder::Attack;
    actedThisTurn = true;
    target.takeDamage(amount);
}

void Unit::attackBuilding(Building& target, int amount) {
    if (!isAlive()) throw std::logic_error("Уничтоженный юнит не может атаковать");
    if (actedThisTurn) throw std::logic_error("Юнит уже действовал в этом ходу");
    currentOrder = UnitOrder::Attack;
    actedThisTurn = true;
    target.takeDamage(amount);
}

void Unit::takeDamage(int amount) {
    if (amount < 0) throw std::invalid_argument("Урон не может быть отрицательным");
    if (isAlive()) health.decrease(amount);
}

void Unit::heal(int amount) {
    if (amount < 0) throw std::invalid_argument("Лечение не может быть отрицательным");
    if (isAlive()) health.increase(amount);
}

void Unit::healUnit(Unit& target) {
    if (getHealPower() == 0) throw std::logic_error("Этот юнит не умеет лечить");
    if (actedThisTurn) throw std::logic_error("Юнит уже действовал в этом ходу");
    currentOrder = UnitOrder::Heal;
    actedThisTurn = true;
    target.heal(getHealPower());
}

// лечение в госпитале восстанавливает здоровье полностью и занимает действие юнита
void Unit::receiveTreatment() {
    if (!isAlive()) return;
    health.increase(health.getMaxValue() - health.getValue());
    currentOrder = UnitOrder::Heal;
    actedThisTurn = true;
}

void Unit::markCollecting() {
    if (getGatherAmount() == 0) throw std::logic_error("Этот юнит не умеет добывать ресурсы");
    currentOrder = UnitOrder::Collect;
    actedThisTurn = true;
}

// начало нового хода: восстанавливаем запас хода, застрявший юнит пропускает ход
void Unit::startTurn() {
    actedThisTurn = false;
    if (stuckTurns > 0) {
        --stuckTurns;
        movePoints = 0;
        currentOrder = UnitOrder::Stuck;
        return;
    }
    movePoints = typeInfo.getMoveRange();
    currentOrder = UnitOrder::Idle;
}

void Unit::upgradeAttack(int amount) {
    damage.upgrade(amount);
}

void Unit::upgradeArmor(int amount) {
    if (amount < 0) throw std::invalid_argument("Улучшение брони не может быть отрицательным");
    armor = Armor(armor.getValue() + amount);
}

void Unit::setTerrainDefense(int defense) {
    if (defense < 0) throw std::invalid_argument("Бонус защиты не может быть отрицательным");
    terrainDefense = defense;
}

char Unit::getSymbol() const {
    const char symbol = typeInfo.getSymbol();
    if (faction == Faction::Enemy) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(symbol)));
    }
    return symbol;
}

std::string Unit::describe() const {
    std::ostringstream out;
    out << getTypeName() << " #" << id << " [" << factionToString(faction) << "] " << toString(position)
        << " | HP " << health.getValue() << "/" << health.getMaxValue()
        << " | броня " << getArmorValue();
    if (terrainDefense > 0) out << " (в т.ч. ландшафт +" << terrainDefense << ")";
    out << " | урон " << damage.getValue();
    if (damage.getBonus() > 0) out << " (в т.ч. исследования +" << damage.getBonus() << ")";
    out << " | ход " << movePoints << "/" << typeInfo.getMoveRange()
        << " | приказ: " << orderToString(currentOrder);
    if (stuckTurns > 0) out << " (пропуск ходов: " << stuckTurns << ")";
    return out.str();
}


std::unique_ptr<Unit> UnitFactory::createUnit(UnitType type, int id, Faction faction, const Position& position) const {
    if (position.x < 0 || position.y < 0) throw std::invalid_argument("Координаты юнита не могут быть отрицательными");
    if (!supports(type)) throw std::invalid_argument("Эта фабрика не поддерживает запрошенный тип юнита");
    return createSupportedUnit(type, id, faction, position);
}

bool MilitaryUnitFactory::supports(UnitType type) const {
    return type == UnitType::Infantry || type == UnitType::Scout || type == UnitType::Tank;
}

std::unique_ptr<Unit> MilitaryUnitFactory::createSupportedUnit(UnitType type, int id, Faction faction,
                                                               const Position& position) const {
    switch (type) {
    case UnitType::Infantry: return std::make_unique<Infantry>(id, faction, position);
    case UnitType::Scout:    return std::make_unique<Scout>(id, faction, position);
    case UnitType::Tank:     return std::make_unique<Tank>(id, faction, position);
    default: throw std::logic_error("Ошибка маршрутизации MilitaryUnitFactory");
    }
}

bool SupportUnitFactory::supports(UnitType type) const {
    return type == UnitType::Worker || type == UnitType::Medic;
}

std::unique_ptr<Unit> SupportUnitFactory::createSupportedUnit(UnitType type, int id, Faction faction,
                                                              const Position& position) const {
    switch (type) {
    case UnitType::Worker: return std::make_unique<Worker>(id, faction, position);
    case UnitType::Medic:  return std::make_unique<Medic>(id, faction, position);
    default: throw std::logic_error("Ошибка маршрутизации SupportUnitFactory");
    }
}

UnitFactoryRegistry::UnitFactoryRegistry() : factoryByType() {
    const UnitFactory* factories[] = { &militaryFactory, &supportFactory };
    for (int i = 0; i < kUnitTypeCount; ++i) {
        const UnitType type = static_cast<UnitType>(i);
        for (const UnitFactory* factory : factories) {
            if (factory->supports(type)) {
                factoryByType[static_cast<size_t>(i)] = factory;
                break;
            }
        }
        // если для типа нет фабрики, лучше узнать об этом сразу при старте
        if (!factoryByType[static_cast<size_t>(i)]) {
            throw std::logic_error("Для одного из типов юнитов не зарегистрирована фабрика");
        }
    }
}

const UnitFactory& UnitFactoryRegistry::getFactory(UnitType type) const {
    const size_t index = static_cast<size_t>(type);
    if (index >= factoryByType.size()) throw std::invalid_argument("Неизвестный тип юнита");
    return *factoryByType[index];
}

std::unique_ptr<Unit> UnitFactoryRegistry::createUnit(UnitType type, int id, Faction faction,
                                                      const Position& position) const {
    return getFactory(type).createUnit(type, id, faction, position);
}


UnitManager::UnitManager() : factories(), units(), nextUnitId(1), attackUpgrade(0), armorUpgrade(0) {
}

std::shared_ptr<Unit> UnitManager::createUnit(UnitType type, const Position& position, Faction faction,
                                              int terrainDefense) {
    // фабрика отдаёт unique_ptr, а дальше юнитом совместно владеют менеджер и учёт базы
    std::shared_ptr<Unit> unit(factories.createUnit(type, nextUnitId, faction, position));
    ++nextUnitId;
    unit->setTerrainDefense(terrainDefense);
    if (faction == Faction::Player) {
        if (attackUpgrade > 0) unit->upgradeAttack(attackUpgrade);
        if (armorUpgrade > 0) unit->upgradeArmor(armorUpgrade);
    }
    // сначала юнита учитывает база, и только потом он попадает в реестр:
    // если учёт не удался, исключение летит дальше, а реестр остаётся прежним
    lockMediator()->notifyUnitCreated(unit);
    units.push_back(unit);
    return unit;
}

bool UnitManager::moveUnit(int unitId, const Position& target, int cost, int terrainDefense, int stuckTurns) {
    const std::shared_ptr<Unit> unit = getUnitById(unitId);
    if (!unit || !unit->isAlive()) return false;
    unit->move(target, cost, terrainDefense, stuckTurns);
    return true;
}

std::shared_ptr<Unit> UnitManager::removeUnit(int unitId) {
    for (auto it = units.begin(); it != units.end(); ++it) {
        if ((*it)->getId() == unitId) {
            std::shared_ptr<Unit> removed = *it;
            units.erase(it);
            return removed;
        }
    }
    return nullptr;
}

std::shared_ptr<Unit> UnitManager::getUnitById(int unitId) const {
    for (const auto& unit : units) {
        if (unit->getId() == unitId) return unit;
    }
    return nullptr;
}

std::shared_ptr<Unit> UnitManager::getUnitAt(const Position& position) const {
    for (const auto& unit : units) {
        if (unit->isAlive() && unit->getPosition() == position) return unit;
    }
    return nullptr;
}

void UnitManager::reportCollected(int unitId, ResourceType type, int amount) {
    const std::shared_ptr<Unit> unit = getUnitById(unitId);
    if (!unit || !unit->isAlive()) throw std::logic_error("Докладывать о добыче может только живой юнит");
    unit->markCollecting();
    lockMediator()->notifyResourceCollected(unitId, type, amount);
}

void UnitManager::applyAttackUpgrade(int amount) {
    if (amount < 0) throw std::invalid_argument("Улучшение не может быть отрицательным");
    attackUpgrade += amount;
    for (const auto& unit : units) {
        if (unit->getFaction() == Faction::Player && unit->isAlive()) unit->upgradeAttack(amount);
    }
}

void UnitManager::applyArmorUpgrade(int amount) {
    if (amount < 0) throw std::invalid_argument("Улучшение не может быть отрицательным");
    armorUpgrade += amount;
    for (const auto& unit : units) {
        if (unit->getFaction() == Faction::Player && unit->isAlive()) unit->upgradeArmor(amount);
    }
}

void UnitManager::startTurn() {
    for (const auto& unit : units) {
        if (unit->isAlive()) unit->startTurn();
    }
}

int UnitManager::count(Faction faction) const {
    return static_cast<int>(std::count_if(units.begin(), units.end(), [faction](const std::shared_ptr<Unit>& unit) {
        return unit->isAlive() && unit->getFaction() == faction;
    }));
}

UnitTypeInfo::UnitTypeInfo(UnitType unitType, std::string typeName, char typeSymbol, int hp, int damageValue,
                           int armorValue, int move, int range, const ResourceStock& price, int turns,
                           bool isHeavy, bool isLight, std::string technology)
    : type(unitType), name(std::move(typeName)), symbol(typeSymbol), maxHealth(hp), damage(damageValue),
      armor(armorValue), moveRange(move), attackRange(range), cost(price), productionTurns(turns),
      heavy(isHeavy), light(isLight), requiredTechnology(std::move(technology)) {
    if (hp <= 0 || damageValue < 0 || armorValue < 0 || move <= 0 || range <= 0 || turns <= 0) {
        throw std::invalid_argument("Неправильные характеристики типа юнита");
    }
}

// таблица создаётся один раз при первом обращении
static const std::array<UnitTypeInfo, kUnitTypeCount>& unitTable() {
    // порядок: тип, имя, символ, здоровье, урон, броня, ход, дальность, цена (дерево, металл, энергия),
    // ходов на производство, тяжёлый, лёгкий, нужное исследование
    static const std::array<UnitTypeInfo, kUnitTypeCount> table = { {
        UnitTypeInfo(UnitType::Infantry, "infantry", 'i', 100, 12, 2, 5, 1, ResourceStock{ 10, 20, 0 }, 1, false, false, ""),
        UnitTypeInfo(UnitType::Scout,    "scout",    's',  60,  8, 1, 8, 2, ResourceStock{ 15, 10, 0 }, 1, false, true,  ""),
        UnitTypeInfo(UnitType::Tank,     "tank",     't', 200, 30, 6, 4, 3, ResourceStock{ 0, 60, 20 }, 2, true,  false, "unlock_tank"),
        UnitTypeInfo(UnitType::Worker,   "worker",   'w',  50,  4, 0, 5, 1, ResourceStock{ 25, 0, 0 },  1, false, false, ""),
        UnitTypeInfo(UnitType::Medic,    "medic",    'm',  70,  5, 1, 5, 1, ResourceStock{ 10, 10, 10 }, 1, false, false, "")
    } };
    return table;
}

const UnitTypeInfo& UnitTypeInfo::getInfo(UnitType type) {
    const size_t index = static_cast<size_t>(type);
    const auto& table = unitTable();
    if (index >= table.size() || table[index].getType() != type) {
        throw std::invalid_argument("Неизвестный тип юнита");
    }
    return table[index];
}

std::optional<UnitType> unitTypeFromString(const std::string& name) {
    for (const UnitTypeInfo& info : unitTable()) {
        if (info.getName() == name) return info.getType();
    }
    return std::nullopt;
}

const std::string& unitTypeToString(UnitType type) {
    return UnitTypeInfo::getInfo(type).getName();
}

std::string unitTypeList() {
    std::string result;
    for (const UnitTypeInfo& info : unitTable()) {
        if (!result.empty()) result += ", ";
        result += info.getName();
    }
    return result;
}
