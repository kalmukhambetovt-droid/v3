#include <ostream>
#include <stdexcept>
#include <sstream>

#include "BaseCommands.h"
#include "GameCommands.h"
#include "GameFacade.h"
#include "GameMediator.h"
#include "Handlers.h"
#include "InputParser.h"
#include "TechnologyInfo.h"
#include "TerrainTypes.h"
#include "UnitCommands.h"


GameFacade::GameFacade(std::ostream& output)
    : out_(output), mediator_(GameMediator::create(output)), history_(), chain_() {
    buildChain();
}

GameFacade::~GameFacade() = default;

void GameFacade::buildChain() {
    chain_ = std::make_shared<HelpHandler>(*this);
    chain_->setNext(std::make_shared<CreateHandler>(*this))
        ->setNext(std::make_shared<MoveHandler>(*this))
        ->setNext(std::make_shared<AttackHandler>(*this))
        ->setNext(std::make_shared<CollectHandler>(*this))
        ->setNext(std::make_shared<HealHandler>(*this))
        ->setNext(std::make_shared<BuildHandler>(*this))
        ->setNext(std::make_shared<ResearchHandler>(*this))
        ->setNext(std::make_shared<DistributeHandler>(*this))
        ->setNext(std::make_shared<ProduceHandler>(*this))
        ->setNext(std::make_shared<StatsHandler>(*this))
        ->setNext(std::make_shared<TurnHandler>(*this))
        ->setNext(std::make_shared<SpawnHandler>(*this))
        ->setNext(std::make_shared<UndoHandler>(*this))
        ->setNext(std::make_shared<HistoryHandler>(*this));
}

CommandResult GameFacade::executeCommand(std::shared_ptr<Command> cmd) {
    if (!cmd) throw std::invalid_argument("Фасад не может выполнить пустую команду");
    out_ << "[Команда] " << cmd->describe() << "\n";
    cmd->execute();

    const CommandResult& result = cmd->getResult();
    if (!cmd->storesInHistory() && result.success) {
        // ответ на запрос выводится как есть
        out_ << result.message << "\n";
    }
    else {
        out_ << "[Результат] " << (result.success ? "Успех: " : "Ошибка: ") << result.message << "\n";
    }
    if (result.success && cmd->storesInHistory()) history_.push(cmd);
    return result;
}

CommandResult GameFacade::createUnit(UnitType type, const Position& position) {
    return executeCommand(std::make_shared<CreateUnitCommand>(mediator_, type, position.x, position.y));
}

CommandResult GameFacade::moveUnit(int unitId, const Position& target) {
    return executeCommand(std::make_shared<MoveUnitCommand>(mediator_, unitId, target.x, target.y));
}

CommandResult GameFacade::attack(int attackerId, int targetId) {
    return executeCommand(std::make_shared<AttackCommand>(mediator_, attackerId, targetId, AttackTarget::Unit));
}

CommandResult GameFacade::attackBuilding(int attackerId, int buildingId) {
    return executeCommand(std::make_shared<AttackCommand>(mediator_, attackerId, buildingId, AttackTarget::Building));
}

CommandResult GameFacade::collectResource(int unitId, int resourcePointId) {
    return executeCommand(std::make_shared<CollectResourceCommand>(mediator_, unitId, resourcePointId));
}

CommandResult GameFacade::heal(int unitId, int targetId) {
    return executeCommand(std::make_shared<HealCommand>(mediator_, unitId, targetId, HealMode::Medic));
}

CommandResult GameFacade::healAtHospital(int unitId, int hospitalId) {
    return executeCommand(std::make_shared<HealCommand>(mediator_, unitId, hospitalId, HealMode::Hospital));
}

CommandResult GameFacade::buildBuilding(BuildingType type, const Position& position) {
    return executeCommand(std::make_shared<BuildBuildingCommand>(mediator_, type, position.x, position.y));
}

CommandResult GameFacade::research(const std::string& technologyId) {
    return executeCommand(std::make_shared<ResearchCommand>(mediator_, technologyId));
}

CommandResult GameFacade::distributeResources(ResourceType from, const DistributionTarget& to, int amount) {
    return executeCommand(std::make_shared<DistributeResourcesCommand>(mediator_, from, to, amount));
}

CommandResult GameFacade::produceUnit(int buildingId, UnitType type) {
    return executeCommand(std::make_shared<ProduceUnitCommand>(mediator_, buildingId, type));
}

std::string GameFacade::getStats(StatsScope scope, int unitId) {
    const auto command = std::make_shared<StatsCommand>(mediator_, scope, unitId);
    executeCommand(command);
    return command->getResult().message;
}

CommandResult GameFacade::endTurn(int turns) {
    return executeCommand(std::make_shared<EndTurnCommand>(mediator_, turns));
}

CommandResult GameFacade::spawnEnemyUnit(UnitType type, const Position& position) {
    return executeCommand(std::make_shared<SpawnEnemyUnitCommand>(mediator_, type, position.x, position.y));
}

CommandResult GameFacade::spawnEnemyBuilding(BuildingType type, const Position& position) {
    return executeCommand(std::make_shared<SpawnEnemyBuildingCommand>(mediator_, type, position.x, position.y));
}

CommandResult GameFacade::undo() {
    const std::shared_ptr<Command> last = history_.getLastCommand();
    out_ << "[Отмена] " << (last ? last->describe() : std::string("история пуста")) << "\n";
    const CommandResult result = history_.undo();
    out_ << "[Результат] " << (result.success ? "Успех: " : "Ошибка: ") << result.message << "\n";
    return result;
}

std::string GameFacade::getHistory() const {
    return history_.describe();
}

void GameFacade::printMessage(const std::string& message) {
    out_ << message << "\n";
}

bool GameFacade::processInput(const std::string& input) {
    const std::string line = InputParser::trim(input);
    if (line.empty()) return true;
    out_ << "[Ввод] " << line << "\n";
    if (chain_->handle(line)) return true;
    printMessage("[Ошибка ввода] Неизвестная команда \"" + line + "\". Список команд: help");
    return false;
}


GameMediator::GameMediator(PrivateTag, std::ostream& logStream)
    : log(logStream), turn(1), base(Position{ 2, 2 }), unitManager(), buildingManager(),
      resourceSystem(ResourceStock{ 300, 250, 150 }, 500), combatSystem(), map(kMapSize, kMapSize) {
}

std::shared_ptr<GameMediator> GameMediator::create(std::ostream& logStream) {
    std::shared_ptr<GameMediator> mediator = std::make_shared<GameMediator>(PrivateTag{}, logStream);
    // компоненты получают слабую ссылку: посредник ими владеет, а они его только знают
    const std::weak_ptr<IMediator> weakMediator = mediator;
    mediator->unitManager.setMediator(weakMediator);
    mediator->buildingManager.setMediator(weakMediator);
    mediator->combatSystem.setMediator(weakMediator);
    mediator->setupWorld();
    return mediator;
}

void GameMediator::setupWorld() {
    // карта 16x16: лес, болота и горы, база в клетке (2,2)
    static const char* const layout[kMapSize] = {
        "................",
        "................",
        "................",
        "................",
        "........fff.....",
        "........fff.....",
        "...~~...fff.....",
        "...~~...........",
        "............^^^.",
        "............^^^.",
        "................",
        "......~~........",
        "......~~........",
        ".ff.............",
        ".ff.............",
        "................"
    };
    for (int y = 0; y < kMapSize; ++y) {
        for (int x = 0; x < kMapSize; ++x) {
            switch (layout[y][x]) {
            case 'f': map.setTerrain(Position{ x, y }, std::make_unique<Forest>()); break;
            case '~': map.setTerrain(Position{ x, y }, std::make_unique<Swamp>()); break;
            case '^': map.setTerrain(Position{ x, y }, std::make_unique<Mountains>()); break;
            default: break;
            }
        }
    }
    resourceSystem.addResourcePoint(ResourcePoint(1, "Лесопилка", ResourceType::Wood, Position{ 9, 5 }, 300));
    resourceSystem.addResourcePoint(ResourcePoint(2, "Рудник", ResourceType::Metal, Position{ 13, 10 }, 300));
    resourceSystem.addResourcePoint(ResourcePoint(3, "Кристаллы", ResourceType::Energy, Position{ 5, 9 }, 200));
}

bool GameMediator::isCellFree(const Position& position, int ignoreUnitId) const {
    if (!map.inBounds(position) || position == base.getPosition()) return false;
    const std::shared_ptr<Unit> unit = unitManager.getUnitAt(position);
    if (unit && unit->getId() != ignoreUnitId) return false;
    if (buildingManager.getBuildingAt(position)) return false;
    return resourceSystem.findPointAt(position) == nullptr;
}

std::optional<Position> GameMediator::findFreeCellNear(const Position& center, UnitType type) const {
    for (int radius = 1; radius <= 2; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
                const Position cell{ center.x + dx, center.y + dy };
                if (isCellFree(cell) && map.getTerrain(cell).isPassable(type)) return cell;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<Position, int>> GameMediator::findApproach(const Unit& unit, const Position& target) const {
    if (distance(unit.getPosition(), target) <= 1) return std::make_pair(unit.getPosition(), 0);
    if (unit.isStuck()) return std::nullopt;

    std::optional<std::pair<Position, int>> best;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const Position cell{ target.x + dx, target.y + dy };
            if (!isCellFree(cell, unit.getId())) continue;
            const int cost = map.moveCost(unit.getUnitType(), unit.getPosition(), cell);
            if (cost < 0 || cost > unit.getMovePoints()) continue;
            if (!best || cost < best->second) best = std::make_pair(cell, cost);
        }
    }
    return best;
}

std::shared_ptr<Unit> GameMediator::findPlayerUnit(int unitId, CommandResult& error) const {
    const std::shared_ptr<Unit> unit = unitManager.getUnitById(unitId);
    if (!unit || !unit->isAlive()) {
        error = CommandResult::fail("Юнит #" + std::to_string(unitId) + " не найден");
        return nullptr;
    }
    if (unit->getFaction() != Faction::Player) {
        error = CommandResult::fail("Юнит #" + std::to_string(unitId) + " вражеский, им нельзя управлять");
        return nullptr;
    }
    return unit;
}

std::shared_ptr<Building> GameMediator::resolveDistributionTarget(const DistributionTarget& target) const {
    if (target.buildingType) return buildingManager.findCompleted(*target.buildingType, Faction::Player);
    return buildingManager.getBuildingById(target.buildingId);
}

void GameMediator::walkTo(Unit& unit, const Position& target, int cost) {
    const TerrainProxy& terrain = map.getTerrain(target);
    const UnitType type = unit.getUnitType();
    unitManager.moveUnit(unit.getId(), target, cost, terrain.getDefenseBonus(type), terrain.getStuckTurns(type));
}

static std::string unitName(const Unit& unit) {
    return unit.getTypeName() + " #" + std::to_string(unit.getId());
}

static std::string buildingName(const Building& building) {
    return building.getTypeName() + " #" + std::to_string(building.getId());
}

// операции команд

CommandResult GameMediator::createUnit(UnitType type, const Position& position) {
    const UnitTypeInfo& info = UnitTypeInfo::getInfo(type);
    if (!base.isAlive()) return CommandResult::fail("База уничтожена и не может нанимать юнитов");
    if (!base.hasFreeSlot()) {
        return CommandResult::fail("Достигнут лимит юнитов базы (" + std::to_string(base.getMaxUnits()) + ")");
    }
    const std::string& technology = info.getRequiredTechnology();
    if (!technology.empty() && !base.hasTechnology(technology)) {
        return CommandResult::fail("Для юнита " + info.getName() + " нужно исследование " + technology);
    }
    if (!map.inBounds(position)) {
        return CommandResult::fail("Клетка " + toString(position) + " за пределами карты 16x16");
    }
    if (distance(position, base.getPosition()) > kSpawnRadius) {
        return CommandResult::fail("Юнит появляется у базы: клетка должна быть не дальше " +
            std::to_string(kSpawnRadius) + " клеток от базы " + toString(base.getPosition()));
    }
    if (!isCellFree(position)) return CommandResult::fail("Клетка " + toString(position) + " занята");
    const TerrainProxy& terrain = map.getTerrain(position);
    if (!terrain.isPassable(type)) {
        return CommandResult::fail("Клетка " + toString(position) + " (" + terrain.getName() +
            ") непроходима для " + info.getName());
    }
    if (!resourceSystem.spendResources(info.getCost())) {
        return CommandResult::fail("Недостаточно ресурсов: нужно " + toString(info.getCost()) +
            ", на складе " + toString(resourceSystem.getStock()));
    }
    const std::shared_ptr<Unit> unit = unitManager.createUnit(type, position, Faction::Player,
        terrain.getDefenseBonus(type));
    return CommandResult::ok(unitName(*unit) + " создан в " + toString(position) +
        " (потрачено: " + toString(info.getCost()) + ")", unit->getId());
}

CommandResult GameMediator::spawnEnemyUnit(UnitType type, const Position& position) {
    if (!isCellFree(position)) return CommandResult::fail("Клетка " + toString(position) + " занята или вне карты");
    const TerrainProxy& terrain = map.getTerrain(position);
    if (!terrain.isPassable(type)) {
        return CommandResult::fail("Клетка " + toString(position) + " непроходима для " + unitTypeToString(type));
    }
    const std::shared_ptr<Unit> unit = unitManager.createUnit(type, position, Faction::Enemy,
        terrain.getDefenseBonus(type));
    return CommandResult::ok("Вражеский " + unitName(*unit) + " появился в " + toString(position), unit->getId());
}

CommandResult GameMediator::spawnEnemyBuilding(BuildingType type, const Position& position) {
    if (!isCellFree(position)) return CommandResult::fail("Клетка " + toString(position) + " занята или вне карты");
    if (!map.getTerrain(position).isBuildable()) {
        return CommandResult::fail("На клетке " + toString(position) + " нельзя строить");
    }
    const std::shared_ptr<Building> building = buildingManager.buildBuilding(type, position, Faction::Enemy, true);
    return CommandResult::ok("Вражеское здание " + buildingName(*building) + " появилось в " + toString(position),
        building->getId());
}

CommandResult GameMediator::moveUnit(int unitId, const Position& target) {
    CommandResult error;
    const std::shared_ptr<Unit> unit = findPlayerUnit(unitId, error);
    if (!unit) return error;
    if (!map.inBounds(target)) return CommandResult::fail("Клетка " + toString(target) + " за пределами карты 16x16");
    if (unit->getPosition() == target) {
        return CommandResult::fail(unitName(*unit) + " уже стоит в " + toString(target));
    }
    if (!isCellFree(target, unitId)) return CommandResult::fail("Клетка " + toString(target) + " занята");

    // прокси одной проверкой учитывает и тип юнита, и его состояние
    const TerrainProxy& terrain = map.getTerrain(target);
    if (!terrain.canEnter(*unit)) {
        if (unit->isStuck()) {
            return CommandResult::fail(unitName(*unit) + " застрял в болоте и пропускает ход");
        }
        return CommandResult::fail("Клетка " + toString(target) + " (" + terrain.getName() +
            ") непроходима для " + unit->getTypeName());
    }
    const int cost = map.moveCost(unit->getUnitType(), unit->getPosition(), target);
    if (cost > unit->getMovePoints()) {
        return CommandResult::fail("Не хватает запаса хода: нужно " + std::to_string(cost) +
            ", осталось " + std::to_string(unit->getMovePoints()));
    }

    const Position from = unit->getPosition();
    walkTo(*unit, target, cost);
    std::string message = unitName(*unit) + ": " + toString(from) + " -> " + toString(target) +
        " (" + terrain.getName() + "), потрачено хода " + std::to_string(cost) +
        ", осталось " + std::to_string(unit->getMovePoints());
    if (unit->isStuck()) message += "; юнит увяз в болоте и пропустит следующий ход";
    return CommandResult::ok(message, unitId, cost);
}

CommandResult GameMediator::resolveCombat(int attackerId, int targetId) {
    CommandResult error;
    const std::shared_ptr<Unit> attacker = findPlayerUnit(attackerId, error);
    if (!attacker) return error;
    const std::shared_ptr<Unit> target = unitManager.getUnitById(targetId);
    if (!target || !target->isAlive()) return CommandResult::fail("Цель #" + std::to_string(targetId) + " не найдена");
    if (targetId == attackerId) return CommandResult::fail("Юнит не может атаковать сам себя");
    if (target->getFaction() == attacker->getFaction()) return CommandResult::fail("Нельзя атаковать союзного юнита");
    if (attacker->hasActed()) {
        return CommandResult::fail(unitName(*attacker) + " уже действовал в этом ходу (next - следующий ход)");
    }
    const int range = distance(attacker->getPosition(), target->getPosition());
    if (range > attacker->getAttackRange()) {
        return CommandResult::fail("Цель вне дальности: расстояние " + std::to_string(range) +
            ", дальность атаки " + std::to_string(attacker->getAttackRange()));
    }

    // имена запоминаем заранее: погибшего юнита посредник уберёт из реестра во время боя
    const std::string attackerName = unitName(*attacker);
    const std::string targetName = unitName(*target);
    const CombatReport report = combatSystem.resolve(*attacker, *target);

    std::string message = attackerName + " атаковал " + targetName + ": урон " + std::to_string(report.damage);
    if (report.damage == 0) message += " (броня полностью поглотила удар)";
    if (report.targetDestroyed) message += ", цель уничтожена";
    else message += ", у цели осталось " + std::to_string(report.targetHealthLeft) + " HP";

    // выжившая цель отвечает, если атакующий в пределах её дальности
    if (!report.targetDestroyed && distance(attacker->getPosition(), target->getPosition()) <= target->getAttackRange()) {
        const CombatReport counter = combatSystem.counterattack(*target, *attacker);
        message += "; ответный удар: урон " + std::to_string(counter.damage);
        if (counter.targetDestroyed) message += ", " + attackerName + " погиб";
        else message += ", у " + attackerName + " осталось " + std::to_string(counter.targetHealthLeft) + " HP";
    }
    return CommandResult::ok(message, targetId, report.damage);
}

CommandResult GameMediator::attackBuilding(int attackerId, int buildingId) {
    CommandResult error;
    const std::shared_ptr<Unit> attacker = findPlayerUnit(attackerId, error);
    if (!attacker) return error;
    const std::shared_ptr<Building> target = buildingManager.getBuildingById(buildingId);
    if (!target || !target->isAlive()) {
        return CommandResult::fail("Здание #" + std::to_string(buildingId) + " не найдено");
    }
    if (target->getFaction() == attacker->getFaction()) return CommandResult::fail("Нельзя атаковать своё здание");
    if (attacker->hasActed()) {
        return CommandResult::fail(unitName(*attacker) + " уже действовал в этом ходу (next - следующий ход)");
    }
    const int range = distance(attacker->getPosition(), target->getPosition());
    if (range > attacker->getAttackRange()) {
        return CommandResult::fail("Здание вне дальности: расстояние " + std::to_string(range) +
            ", дальность атаки " + std::to_string(attacker->getAttackRange()));
    }

    const std::string attackerName = unitName(*attacker);
    const std::string targetName = buildingName(*target);
    const CombatReport report = combatSystem.resolve(*attacker, *target);

    std::string message = attackerName + " атаковал здание " + targetName + ": урон " + std::to_string(report.damage);
    if (report.targetDestroyed) message += ", здание разрушено";
    else message += ", у здания осталось " + std::to_string(report.targetHealthLeft) + " HP";
    return CommandResult::ok(message, buildingId, report.damage);
}

CommandResult GameMediator::collectResource(int unitId, int resourcePointId) {
    CommandResult error;
    const std::shared_ptr<Unit> unit = findPlayerUnit(unitId, error);
    if (!unit) return error;
    if (unit->getGatherAmount() == 0) {
        return CommandResult::fail(unitName(*unit) + " не умеет добывать ресурсы, нужен рабочий (worker)");
    }
    if (unit->hasActed()) {
        return CommandResult::fail(unitName(*unit) + " уже действовал в этом ходу (next - следующий ход)");
    }
    ResourcePoint* point = resourceSystem.findPoint(resourcePointId);
    if (!point) return CommandResult::fail("Ресурсная точка #" + std::to_string(resourcePointId) + " не найдена");
    if (point->isDepleted()) return CommandResult::fail("Ресурсная точка " + point->getName() + " истощена");

    const auto approach = findApproach(*unit, point->getPosition());
    if (!approach) {
        return CommandResult::fail(unitName(*unit) + " не может дойти до точки " + point->getName() + " " +
            toString(point->getPosition()) + " в этом ходу (запас хода " + std::to_string(unit->getMovePoints()) + ")");
    }
    std::string route;
    if (approach->first != unit->getPosition()) {
        const Position from = unit->getPosition();
        walkTo(*unit, approach->first, approach->second);
        route = "подошёл " + toString(from) + " -> " + toString(approach->first) + " и ";
    }

    const ResourceType type = point->getType();
    const int extracted = point->extract(unit->getGatherAmount());
    const int before = resourceSystem.getStock().get(type);
    // рабочий докладывает через менеджер юнитов, а склад и база узнают о добыче от посредника
    unitManager.reportCollected(unitId, type, extracted);
    const int stored = resourceSystem.getStock().get(type) - before;

    std::string message = unitName(*unit) + " " + route + "добыл " + std::to_string(extracted) + " (" +
        resourceTypeToString(type) + ") на точке " + point->getName() +
        ", осталось на точке " + std::to_string(point->getRemaining());
    if (stored < extracted) message += "; на склад поместилось только " + std::to_string(stored);
    return CommandResult::ok(message, resourcePointId, stored);
}

CommandResult GameMediator::healUnit(int healerId, int targetId) {
    CommandResult error;
    const std::shared_ptr<Unit> healer = findPlayerUnit(healerId, error);
    if (!healer) return error;
    if (healer->getHealPower() == 0) {
        return CommandResult::fail(unitName(*healer) + " не медик. Медик лечит так: heal unit <id медика> target <id>, "
            "в госпиталь отправляют так: heal unit <id> at <id госпиталя>");
    }
    if (healer->hasActed()) {
        return CommandResult::fail(unitName(*healer) + " уже действовал в этом ходу (next - следующий ход)");
    }
    const std::shared_ptr<Unit> target = findPlayerUnit(targetId, error);
    if (!target) return error;
    if (targetId == healerId) return CommandResult::fail("Медик не может лечить сам себя");
    if (target->getHealth() == target->getMaxHealth()) {
        return CommandResult::fail(unitName(*target) + " полностью здоров");
    }
    const int range = distance(healer->getPosition(), target->getPosition());
    if (range > healer->getHealRange()) {
        return CommandResult::fail("Цель далеко: расстояние " + std::to_string(range) +
            ", радиус лечения " + std::to_string(healer->getHealRange()));
    }

    const int before = target->getHealth();
    healer->healUnit(*target);
    const int healed = target->getHealth() - before;
    return CommandResult::ok(unitName(*healer) + " вылечил " + unitName(*target) + " на " + std::to_string(healed) +
        " HP (" + std::to_string(target->getHealth()) + "/" + std::to_string(target->getMaxHealth()) + ")",
        targetId, healed);
}

CommandResult GameMediator::healAtHospital(int unitId, int hospitalId) {
    CommandResult error;
    const std::shared_ptr<Unit> unit = findPlayerUnit(unitId, error);
    if (!unit) return error;
    const auto hospital = std::dynamic_pointer_cast<Hospital>(buildingManager.getBuildingById(hospitalId));
    if (!hospital || hospital->getFaction() != Faction::Player || !hospital->isAlive()) {
        return CommandResult::fail("Здание #" + std::to_string(hospitalId) + " не является вашим госпиталем");
    }
    if (!hospital->isCompleted()) return CommandResult::fail("Госпиталь ещё строится");
    if (unit->getHealth() == unit->getMaxHealth()) return CommandResult::fail(unitName(*unit) + " полностью здоров");
    if (unit->hasActed()) {
        return CommandResult::fail(unitName(*unit) + " уже действовал в этом ходу (next - следующий ход)");
    }
    const ResourceStock treatmentCost = makeStock(ResourceType::Energy, Hospital::kTreatmentEnergyCost);
    if (!resourceSystem.canAfford(treatmentCost, hospitalId)) {
        return CommandResult::fail("Для лечения нужно " + std::to_string(Hospital::kTreatmentEnergyCost) + " энергии");
    }
    const auto approach = findApproach(*unit, hospital->getPosition());
    if (!approach) {
        return CommandResult::fail(unitName(*unit) + " не может дойти до госпиталя в этом ходу");
    }

    std::string route;
    if (approach->first != unit->getPosition()) {
        walkTo(*unit, approach->first, approach->second);
        route = " (подошёл к госпиталю в " + toString(approach->first) + ")";
    }
    resourceSystem.spendResources(treatmentCost, hospitalId);
    unit->receiveTreatment();
    return CommandResult::ok(unitName(*unit) + " вылечен в госпитале #" + std::to_string(hospitalId) + route +
        ": HP " + std::to_string(unit->getHealth()) + "/" + std::to_string(unit->getMaxHealth()), unitId);
}

CommandResult GameMediator::buildBuilding(BuildingType type, const Position& position) {
    const BuildingTypeInfo& info = BuildingTypeInfo::getInfo(type);
    if (!map.inBounds(position)) {
        return CommandResult::fail("Клетка " + toString(position) + " за пределами карты 16x16");
    }
    if (distance(position, base.getPosition()) > kBuildRadius) {
        return CommandResult::fail("Строить можно не дальше " + std::to_string(kBuildRadius) +
            " клеток от базы " + toString(base.getPosition()));
    }
    if (!isCellFree(position)) return CommandResult::fail("Клетка " + toString(position) + " занята");
    const TerrainProxy& terrain = map.getTerrain(position);
    if (!terrain.isBuildable()) {
        return CommandResult::fail("На клетке " + toString(position) + " (" + terrain.getName() +
            ") строить нельзя, нужна равнина");
    }
    if (!resourceSystem.spendResources(info.getCost())) {
        return CommandResult::fail("Недостаточно ресурсов: нужно " + toString(info.getCost()) +
            ", на складе " + toString(resourceSystem.getStock()));
    }
    const std::shared_ptr<Building> building = buildingManager.buildBuilding(type, position, Faction::Player);
    base.registerBuilding(building);
    return CommandResult::ok(buildingName(*building) + " заложено в " + toString(position) +
        ", стройка займёт ходов: " + std::to_string(info.getBuildTurns()) +
        " (потрачено: " + toString(info.getCost()) + ")", building->getId());
}

CommandResult GameMediator::startResearch(const std::string& technologyId) {
    const TechnologyInfo* technology = TechnologyInfo::find(technologyId);
    if (!technology) {
        return CommandResult::fail("Неизвестная технология \"" + technologyId + "\". Доступны: " + technologyList());
    }
    if (base.hasTechnology(technologyId)) return CommandResult::fail("Технология " + technologyId + " уже изучена");
    if (buildingManager.findResearching(technologyId)) {
        return CommandResult::fail("Технология " + technologyId + " уже исследуется");
    }

    const std::shared_ptr<Laboratory> laboratory = buildingManager.findFreeLaboratory();
    if (!laboratory) {
        // почему нет свободной лаборатории
        bool hasLaboratory = false;
        bool underConstruction = false;
        for (const auto& building : buildingManager.getBuildings()) {
            if (building->getType() == BuildingType::Laboratory && building->getFaction() == Faction::Player) {
                hasLaboratory = true;
                if (!building->isCompleted()) underConstruction = true;
            }
        }
        if (!hasLaboratory) {
            return CommandResult::fail("Для исследования нужна лаборатория: build laboratory at <x>,<y>");
        }
        if (underConstruction) {
            return CommandResult::fail("Лаборатория ещё строится, исследование можно начать после постройки (next)");
        }
        return CommandResult::fail("Все лаборатории заняты исследованиями");
    }

    if (!resourceSystem.spendResources(technology->getCost(), laboratory->getId())) {
        return CommandResult::fail("Недостаточно ресурсов: нужно " + toString(technology->getCost()) +
            ", на складе " + toString(resourceSystem.getStock()));
    }
    buildingManager.startResearch(laboratory->getId(), *technology);
    return CommandResult::ok("Лаборатория #" + std::to_string(laboratory->getId()) + " начала исследование " +
        technologyId + " (" + technology->getName() + "), осталось ходов: " +
        std::to_string(technology->getResearchTurns()), laboratory->getId());
}

CommandResult GameMediator::distributeResources(ResourceType type, const DistributionTarget& target, int amount) {
    if (amount <= 0) return CommandResult::fail("Количество ресурсов должно быть больше нуля");
    const std::shared_ptr<Building> building = resolveDistributionTarget(target);
    if (!building || building->getFaction() != Faction::Player || !building->isAlive()) {
        return CommandResult::fail("Не найдено готовое здание: " + target.describe());
    }
    if (!building->isCompleted()) return CommandResult::fail(buildingName(*building) + " ещё строится");
    if (!resourceSystem.distribute(type, building->getId(), amount)) {
        return CommandResult::fail(std::string("На складе только ") +
            std::to_string(resourceSystem.getStock().get(type)) + " (" + resourceTypeToString(type) +
            "), нужно " + std::to_string(amount));
    }
    return CommandResult::ok("Зданию " + buildingName(*building) + " выделено " + std::to_string(amount) + " (" +
        resourceTypeToString(type) + "); у здания теперь: " + toString(resourceSystem.getAllocation(building->getId())) +
        "; на складе: " + toString(resourceSystem.getStock()), building->getId(), amount);
}

CommandResult GameMediator::produceUnit(int buildingId, UnitType type) {
    const std::shared_ptr<Building> building = buildingManager.getBuildingById(buildingId);
    if (!building || building->getFaction() != Faction::Player || !building->isAlive()) {
        return CommandResult::fail("Здание #" + std::to_string(buildingId) + " не найдено");
    }
    const auto producer = std::dynamic_pointer_cast<ProductionBuilding>(building);
    if (!producer) {
        return CommandResult::fail(buildingName(*building) + " не производит юнитов (юнитов выпускают barracks и factory)");
    }
    if (!producer->isCompleted()) return CommandResult::fail(buildingName(*building) + " ещё строится");
    const UnitTypeInfo& info = UnitTypeInfo::getInfo(type);
    if (!producer->canProduce(type)) {
        return CommandResult::fail(buildingName(*building) + " производит только: " + producer->producibleList());
    }
    const std::string& technology = info.getRequiredTechnology();
    if (!technology.empty() && !base.hasTechnology(technology)) {
        return CommandResult::fail("Для юнита " + info.getName() + " нужно исследование " + technology);
    }
    if (producer->getQueue().size() >= ProductionBuilding::kMaxQueue) {
        return CommandResult::fail("Очередь производства " + buildingName(*building) + " заполнена");
    }
    if (!base.hasFreeSlot()) {
        return CommandResult::fail("Достигнут лимит юнитов базы (" + std::to_string(base.getMaxUnits()) + ")");
    }
    if (!resourceSystem.spendResources(info.getCost(), buildingId)) {
        return CommandResult::fail("Недостаточно ресурсов: нужно " + toString(info.getCost()) +
            ", у здания и на складе вместе " + toString(resourceSystem.getStock() + resourceSystem.getAllocation(buildingId)));
    }
    const int ticket = buildingManager.produceUnit(buildingId, type);
    return CommandResult::ok(buildingName(*building) + ": заказ #" + std::to_string(ticket) + " на " + info.getName() +
        ", будет готов через ходов: " + std::to_string(info.getProductionTurns()), buildingId, ticket);
}

CommandResult GameMediator::advanceTurn(int turns) {
    if (turns <= 0 || turns > kMaxTurnsAtOnce) {
        return CommandResult::fail("Число ходов должно быть от 1 до " + std::to_string(kMaxTurnsAtOnce));
    }
    for (int i = 0; i < turns; ++i) {
        ++turn;
        log << " Ход " << turn << " \n";
        unitManager.startTurn();
        buildingManager.update();
    }
    return CommandResult::ok("Наступил ход " + std::to_string(turn), turn);
}

CommandResult GameMediator::getStats(StatsScope scope, int unitId) const {
    switch (scope) {
    case StatsScope::Summary:   return CommandResult::ok(summaryStats());
    case StatsScope::Units:     return CommandResult::ok(unitStats());
    case StatsScope::Buildings: return CommandResult::ok(buildingStats());
    case StatsScope::Resources: return CommandResult::ok(resourceStats());
    case StatsScope::Research:  return CommandResult::ok(researchStats());
    case StatsScope::Map:       return CommandResult::ok(mapView());
    case StatsScope::Unit:
        if (!unitManager.getUnitById(unitId)) {
            return CommandResult::fail("Юнит #" + std::to_string(unitId) + " не найден");
        }
        return CommandResult::ok(unitStatus(unitId), unitId);
    }
    throw std::invalid_argument("Неизвестный раздел статистики");
}

// отмена

CommandResult GameMediator::undoCreateUnit(int unitId) {
    const std::shared_ptr<Unit> unit = unitManager.getUnitById(unitId);
    if (!unit || !unit->isAlive()) {
        return CommandResult::fail("Юнит #" + std::to_string(unitId) + " уже не существует, отмена невозможна");
    }
    const ResourceStock cost = unit->getTypeInfo().getCost();
    const std::string name = unitName(*unit);
    unitManager.removeUnit(unitId);
    base.forgetUnit(unitId);
    resourceSystem.addResources(cost);
    return CommandResult::ok("Создание " + name + " отменено, возвращено: " + toString(cost), unitId);
}

CommandResult GameMediator::undoMove(int unitId, const Position& from, int spentPoints) {
    const std::shared_ptr<Unit> unit = unitManager.getUnitById(unitId);
    if (!unit || !unit->isAlive()) return CommandResult::fail("Юнит #" + std::to_string(unitId) + " уже не существует");
    if (!isCellFree(from, unitId)) {
        return CommandResult::fail("Клетка " + toString(from) + " уже занята, вернуть юнита нельзя");
    }
    unit->returnTo(from, spentPoints, map.getTerrain(from).getDefenseBonus(unit->getUnitType()));
    return CommandResult::ok(unitName(*unit) + " возвращён в " + toString(from) +
        ", запас хода: " + std::to_string(unit->getMovePoints()), unitId);
}

CommandResult GameMediator::cancelBuilding(int buildingId) {
    const std::shared_ptr<Building> building = buildingManager.getBuildingById(buildingId);
    if (!building) return CommandResult::fail("Здание #" + std::to_string(buildingId) + " уже не существует");
    if (building->isCompleted()) {
        return CommandResult::fail(buildingName(*building) + " уже построено, отменить строительство нельзя");
    }
    const ResourceStock cost = BuildingTypeInfo::getInfo(building->getType()).getCost();
    const std::string name = buildingName(*building);
    buildingManager.removeBuilding(buildingId);
    base.forgetBuilding(buildingId);
    resourceSystem.releaseAllocation(buildingId);
    resourceSystem.addResources(cost);
    return CommandResult::ok("Строительство " + name + " отменено, возвращено: " + toString(cost), buildingId);
}

CommandResult GameMediator::cancelResearch(const std::string& technologyId) {
    const TechnologyInfo* technology = TechnologyInfo::find(technologyId);
    if (!technology || !buildingManager.cancelResearch(technologyId)) {
        return CommandResult::fail("Исследование " + technologyId + " уже завершено, отменить нельзя");
    }
    resourceSystem.addResources(technology->getCost());
    return CommandResult::ok("Исследование " + technologyId + " отменено, возвращено: " +
        toString(technology->getCost()));
}

CommandResult GameMediator::undoDistribution(ResourceType type, int buildingId, int amount) {
    if (!resourceSystem.returnAllocation(type, buildingId, amount)) {
        return CommandResult::fail("Выделенные зданию ресурсы уже потрачены, вернуть их нельзя");
    }
    return CommandResult::ok("Возвращено на склад " + std::to_string(amount) + " (" + resourceTypeToString(type) +
        ") от здания #" + std::to_string(buildingId), buildingId);
}

CommandResult GameMediator::cancelProduction(int buildingId, int ticket) {
    const std::optional<UnitType> cancelled = buildingManager.cancelProduction(buildingId, ticket);
    if (!cancelled) return CommandResult::fail("Заказ #" + std::to_string(ticket) + " уже выполнен, отменить нельзя");
    const ResourceStock cost = UnitTypeInfo::getInfo(*cancelled).getCost();
    resourceSystem.addResources(cost);
    return CommandResult::ok("Заказ #" + std::to_string(ticket) + " на " + unitTypeToString(*cancelled) +
        " отменён, возвращено: " + toString(cost), buildingId);
}

std::optional<Position> GameMediator::getUnitPosition(int unitId) const {
    const std::shared_ptr<Unit> unit = unitManager.getUnitById(unitId);
    if (!unit) return std::nullopt;
    return unit->getPosition();
}

// реакция на события компонентов

void GameMediator::notifyUnitCreated(const std::shared_ptr<Unit>& unit) {
    if (unit->getFaction() == Faction::Player) {
        base.registerUnit(unit);
        log << "[Mediator] база учла " << unitName(*unit) << " (юнитов " << base.getUnitCount()
            << "/" << base.getMaxUnits() << ")\n";
    }
    else {
        log << "[Mediator] на карте появился вражеский " << unitName(*unit) << "\n";
    }
}

void GameMediator::notifyResourceCollected(int unitId, ResourceType type, int amount) {
    const int stored = resourceSystem.addResources(type, amount);
    base.registerDelivery(stored);
    log << "[Mediator] юнит #" << unitId << " доложил о добыче " << amount << " (" << resourceTypeToString(type)
        << "); на склад принято " << stored << ", всего доставлено на базу " << base.getResources() << "\n";
}

void GameMediator::notifyCombatResolved(const CombatReport& report) {
    log << "[Mediator] бой: юнит #" << report.attackerId << " -> " << (report.targetIsBuilding ? "здание #" : "юнит #")
        << report.targetId << ", урон " << report.damage << "\n";
    if (!report.targetDestroyed) return;

    if (report.targetIsBuilding) {
        const std::shared_ptr<Building> removed = buildingManager.removeBuilding(report.targetId);
        if (removed && removed->getFaction() == Faction::Player) base.registerBuildingLost(report.targetId);
        resourceSystem.releaseAllocation(report.targetId);
        log << "[Mediator] здание #" << report.targetId << " разрушено и снято с учёта\n";
    }
    else {
        const std::shared_ptr<Unit> removed = unitManager.removeUnit(report.targetId);
        if (removed && removed->getFaction() == Faction::Player) base.registerUnitLost(report.targetId);
        log << "[Mediator] юнит #" << report.targetId << " уничтожен и снят с учёта\n";
    }
}

void GameMediator::notifyBuildingCompleted(const std::shared_ptr<Building>& building) {
    log << "[Mediator] строительство завершено: " << buildingName(*building) << "\n";
    if (building->getType() == BuildingType::Warehouse) {
        resourceSystem.increaseCapacity(Warehouse::kCapacityBonus);
        log << "[Mediator] вместимость склада выросла до " << resourceSystem.getCapacity() << "\n";
    }
}

void GameMediator::notifyResearchCompleted(int laboratoryId, const std::string& technologyId) {
    const TechnologyInfo* technology = TechnologyInfo::find(technologyId);
    if (!technology) throw std::logic_error("Лаборатория завершила неизвестную технологию");
    base.registerTechnology(technologyId);
    log << "[Mediator] лаборатория #" << laboratoryId << " завершила исследование " << technologyId << "\n";

    switch (technology->getEffect()) {
    case TechnologyEffect::AttackBonus:
        unitManager.applyAttackUpgrade(technology->getValue());
        log << "[Mediator] атака всех юнитов игрока +" << technology->getValue() << "\n";
        break;
    case TechnologyEffect::ArmorBonus:
        unitManager.applyArmorUpgrade(technology->getValue());
        base.increaseDefense(technology->getValue());
        log << "[Mediator] броня юнитов и защита базы +" << technology->getValue() << "\n";
        break;
    case TechnologyEffect::UnlockUnit:
        log << "[Mediator] открыт новый юнит: " << unitTypeToString(technology->getUnlockedUnit()) << "\n";
        break;
    }
}

bool GameMediator::notifyProductionFinished(int buildingId, UnitType type) {
    const std::shared_ptr<Building> building = buildingManager.getBuildingById(buildingId);
    if (!building) return false;
    if (!base.hasFreeSlot()) {
        log << "[Mediator] " << buildingName(*building) << ": лимит юнитов, выпуск отложен\n";
        return false;
    }
    const std::optional<Position> cell = findFreeCellNear(building->getPosition(), type);
    if (!cell) {
        log << "[Mediator] " << buildingName(*building) << ": вокруг нет свободной клетки, выпуск отложен\n";
        return false;
    }
    const std::shared_ptr<Unit> unit = unitManager.createUnit(type, *cell, Faction::Player,
        map.getTerrain(*cell).getDefenseBonus(type));
    log << "[Mediator] " << buildingName(*building) << " выпустил " << unitName(*unit)
        << " в клетку " << toString(*cell) << "\n";
    return true;
}

void GameMediator::notifyResourcesProduced(int buildingId, const ResourceStock& amount) {
    const ResourceStock stored = resourceSystem.addResources(amount);
    log << "[Mediator] здание #" << buildingId << " произвело ресурсы, на склад принято: " << toString(stored) << "\n";
}


// вывод статистики и логика посредника находятся в GameCore.cpp

std::string GameMediator::summaryStats() const {
    std::ostringstream out;
    out << " Статистика базы (ход " << turn << ") \n"
        << "База " << toString(base.getPosition()) << ": HP " << base.getHealth() << "/" << base.getMaxHealth()
        << ", защита " << base.getDefenseLevel()
        << ", юнитов " << base.getUnitCount() << "/" << base.getMaxUnits()
        << ", потеряно юнитов " << base.getLostUnitCount()
        << ", зданий " << base.getBuildingCount()
        << ", потеряно зданий " << base.getLostBuildingCount() << "\n"
        << "Ресурсы: " << toString(resourceSystem.getStock())
        << " (вместимость " << resourceSystem.getCapacity() << ")\n"
        << "Доставлено рабочими за игру: " << base.getResources() << "\n"
        << "Активные исследования: ";
    bool anyResearch = false;
    for (const auto& building : buildingManager.getBuildings()) {
        const auto laboratory = std::dynamic_pointer_cast<Laboratory>(building);
        if (laboratory && laboratory->getResearch()) {
            out << (anyResearch ? ", " : "") << laboratory->getResearch()->technologyId
                << " (лаборатория #" << laboratory->getId() << ", осталось ходов "
                << laboratory->getResearch()->turnsLeft << ")";
            anyResearch = true;
        }
    }
    if (!anyResearch) out << "нет";
    out << "\nИзученные технологии: ";
    if (base.getTechnologies().empty()) out << "нет";
    for (size_t i = 0; i < base.getTechnologies().size(); ++i) {
        out << (i > 0 ? ", " : "") << base.getTechnologies()[i];
    }
    out << "\nПротивник: юнитов " << unitManager.count(Faction::Enemy)
        << ", зданий " << buildingManager.count(Faction::Enemy);
    return out.str();
}

std::string GameMediator::unitStats() const {
    std::ostringstream out;
    out << " Юниты (" << unitManager.getUnits().size() << ") ";
    if (unitManager.getUnits().empty()) out << "\nюнитов нет";
    for (const auto& unit : unitManager.getUnits()) {
        out << "\n  " << unit->describe();
    }
    return out.str();
}

std::string GameMediator::unitStatus(int unitId) const {
    const std::shared_ptr<Unit> unit = unitManager.getUnitById(unitId);
    std::ostringstream out;
    out << " Состояние юнита #" << unitId << " \n"
        << unit->describe() << "\n"
        << "Способность: " << unit->describeAbility() << "\n"
        << "Дальность атаки: " << unit->getAttackRange()
        << ", действие в этом ходу: " << (unit->hasActed() ? "использовано" : "доступно") << "\n"
        << "Местность: " << map.getTerrain(unit->getPosition()).getName();
    return out.str();
}

std::string GameMediator::buildingStats() const {
    std::ostringstream out;
    out << " Здания (" << buildingManager.getBuildings().size() << ") ";
    if (buildingManager.getBuildings().empty()) out << "\nзданий нет";
    for (const auto& building : buildingManager.getBuildings()) {
        out << "\n  " << building->describe();
    }
    return out.str();
}

std::string GameMediator::resourceStats() const {
    std::ostringstream out;
    out << " Ресурсы \n"
        << "Склад: " << toString(resourceSystem.getStock()) << " (вместимость " << resourceSystem.getCapacity() << ")\n"
        << "Выделено зданиям:";
    if (resourceSystem.getAllocations().empty()) out << " нет";
    for (const auto& allocation : resourceSystem.getAllocations()) {
        const std::shared_ptr<Building> building = buildingManager.getBuildingById(allocation.first);
        if (!building) throw std::logic_error("Ресурсы выделены несуществующему зданию");
        out << "\n  " << building->getTypeName() << " #" << allocation.first
            << ": " << toString(allocation.second);
    }
    out << "\nРесурсные точки:";
    for (const ResourcePoint& point : resourceSystem.getPoints()) {
        out << "\n  #" << point.getId() << " " << point.getName() << " " << toString(point.getPosition())
            << ": " << resourceTypeToString(point.getType()) << ", осталось " << point.getRemaining();
    }
    return out.str();
}

std::string GameMediator::researchStats() const {
    std::ostringstream out;
    out << " Исследования ";
    for (const TechnologyInfo& technology : TechnologyInfo::all()) {
        out << "\n  " << technology.getId() << " (" << technology.getName() << "): цена "
            << toString(technology.getCost()) << ", ходов " << technology.getResearchTurns() << " - ";
        const std::shared_ptr<Laboratory> laboratory = buildingManager.findResearching(technology.getId());
        if (base.hasTechnology(technology.getId())) {
            out << "изучено";
        }
        else if (laboratory) {
            out << "исследуется в лаборатории #" << laboratory->getId() << ", осталось ходов "
                << laboratory->getResearch()->turnsLeft;
        }
        else {
            out << "доступно";
        }
    }
    return out.str();
}

std::string GameMediator::mapView() const {
    std::ostringstream out;
    out << " Карта (ход " << turn << ") \n   ";
    for (int x = 0; x < map.getWidth(); ++x) out << x % 10 << " ";
    for (int y = 0; y < map.getHeight(); ++y) {
        out << "\n" << (y < 10 ? " " : "") << y << " ";
        for (int x = 0; x < map.getWidth(); ++x) {
            const Position cell{ x, y };
            char symbol = map.getTerrain(cell).getSymbol();
            if (resourceSystem.findPointAt(cell)) symbol = '*';
            if (const auto building = buildingManager.getBuildingAt(cell)) symbol = building->getSymbol();
            if (const auto unit = unitManager.getUnitAt(cell)) symbol = unit->getSymbol();
            if (cell == base.getPosition()) symbol = 'B';
            out << symbol << " ";
        }
    }
    out << "\nЛегенда: . равнина, f лес, ~ болото, ^ горы, * ресурсная точка, B база\n"
        << "         здания игрока K казарма, Z завод, P электростанция, L лаборатория, D склад, H госпиталь"
        << " (вражеские строчными)\n"
        << "         юниты игрока i пехота, s разведчик, t танк, w рабочий, m медик (вражеские заглавными)";
    return out.str();
}
