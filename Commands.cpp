#include <sstream>
#include <stdexcept>
#include <utility>

#include "BaseCommands.h"
#include "Command.h"
#include "CommandHistory.h"
#include "GameCommands.h"
#include "GameMediator.h"
#include "UnitCommands.h"

Command::Command(std::shared_ptr<GameMediator> mediator)
    : mediator_(std::move(mediator)), result_(), executed_(false), undone_(false) {
    if (!mediator_) throw std::invalid_argument("Команде нужен посредник");
}

bool Command::beginExecution() {
    if (executed_) {
        result_ = CommandResult::fail("Команда " + getName() + " уже выполнена");
        return false;
    }
    executed_ = true;
    return true;
}

CommandResult Command::undo() {
    return CommandResult::fail("Команду " + getName() + " отменить нельзя");
}


CommandHistory::CommandHistory(size_t maxSize) : commands(), limit(maxSize) {
    if (maxSize == 0) throw std::invalid_argument("Размер истории должен быть больше нуля");
}

void CommandHistory::push(std::shared_ptr<Command> command) {
    if (!command) throw std::invalid_argument("Нельзя сохранить пустую команду");
    // самая старая команда вытесняется, чтобы история не росла бесконечно
    if (commands.size() >= limit) commands.erase(commands.begin());
    commands.push_back(std::move(command));
}

CommandResult CommandHistory::undo() {
    if (commands.empty()) return CommandResult::fail("История команд пуста, отменять нечего");
    const std::shared_ptr<Command>& last = commands.back();
    if (!last->isUndoable()) {
        return CommandResult::fail("Последнее действие (" + last->describe() + ") необратимо, отмена невозможна");
    }
    CommandResult result = last->undo();
    // если отмена не удалась, команда остаётся в истории
    if (result.success) commands.pop_back();
    return result;
}

std::shared_ptr<Command> CommandHistory::getLastCommand() const {
    if (commands.empty()) return nullptr;
    return commands.back();
}

std::string CommandHistory::describe() const {
    std::ostringstream out;
    out << " История команд (" << commands.size() << ") ";
    if (commands.empty()) out << "\nпусто";
    for (size_t i = 0; i < commands.size(); ++i) {
        out << "\n  " << i + 1 << ". " << commands[i]->describe()
            << (commands[i]->isUndoable() ? "" : " [необратима]");
    }
    return out.str();
}

CreateUnitCommand::CreateUnitCommand(std::shared_ptr<GameMediator> mediator, UnitType unitType, int posX, int posY)
    : Command(std::move(mediator)), type(unitType), x(posX), y(posY) {
}

void CreateUnitCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->createUnit(type, Position{ x, y });
}

CommandResult CreateUnitCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Создание юнита нечего отменять");
    CommandResult undoResult = mediator_->undoCreateUnit(result_.objectId);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string CreateUnitCommand::describe() const {
    return "CreateUnitCommand: " + unitTypeToString(type) + " в (" + std::to_string(x) + "," + std::to_string(y) + ")";
}

MoveUnitCommand::MoveUnitCommand(std::shared_ptr<GameMediator> mediator, int id, int x, int y)
    : Command(std::move(mediator)), unitId(id), targetX(x), targetY(y), from() {
}

void MoveUnitCommand::execute() {
    if (!beginExecution()) return;
    // запоминаем исходную клетку до перемещения, иначе отменять будет некуда
    const std::optional<Position> start = mediator_->getUnitPosition(unitId);
    if (start) from = *start;
    result_ = mediator_->moveUnit(unitId, Position{ targetX, targetY });
}

CommandResult MoveUnitCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Перемещение нечего отменять");
    // в value посредник вернул, сколько запаса хода потрачено
    CommandResult undoResult = mediator_->undoMove(unitId, from, result_.value);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string MoveUnitCommand::describe() const {
    return "MoveUnitCommand: юнит #" + std::to_string(unitId) + " -> (" + std::to_string(targetX) + "," +
        std::to_string(targetY) + ")";
}

AttackCommand::AttackCommand(std::shared_ptr<GameMediator> mediator, int attacker, int target, AttackTarget kind)
    : Command(std::move(mediator)), attackerId(attacker), targetId(target), targetKind(kind) {
}

void AttackCommand::execute() {
    if (!beginExecution()) return;
    if (targetKind == AttackTarget::Building) {
        result_ = mediator_->attackBuilding(attackerId, targetId);
    }
    else {
        result_ = mediator_->resolveCombat(attackerId, targetId);
    }
}

std::string AttackCommand::describe() const {
    return "AttackCommand: юнит #" + std::to_string(attackerId) + " атакует " +
        (targetKind == AttackTarget::Building ? "здание #" : "юнита #") + std::to_string(targetId);
}

CollectResourceCommand::CollectResourceCommand(std::shared_ptr<GameMediator> mediator, int id, int pointId)
    : Command(std::move(mediator)), unitId(id), resourcePointId(pointId) {
}

void CollectResourceCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->collectResource(unitId, resourcePointId);
}

std::string CollectResourceCommand::describe() const {
    return "CollectResourceCommand: юнит #" + std::to_string(unitId) + " добывает на точке #" +
        std::to_string(resourcePointId);
}

HealCommand::HealCommand(std::shared_ptr<GameMediator> mediator, int id, int target, HealMode healMode)
    : Command(std::move(mediator)), unitId(id), targetId(target), mode(healMode) {
}

void HealCommand::execute() {
    if (!beginExecution()) return;
    if (mode == HealMode::Hospital) {
        result_ = mediator_->healAtHospital(unitId, targetId);
    }
    else {
        result_ = mediator_->healUnit(unitId, targetId);
    }
}

std::string HealCommand::describe() const {
    if (mode == HealMode::Hospital) {
        return "HealCommand: юнит #" + std::to_string(unitId) + " идёт в госпиталь #" + std::to_string(targetId);
    }
    return "HealCommand: медик #" + std::to_string(unitId) + " лечит юнита #" + std::to_string(targetId);
}

BuildBuildingCommand::BuildBuildingCommand(std::shared_ptr<GameMediator> mediator, BuildingType type, int posX, int posY)
    : Command(std::move(mediator)), buildingType(type), x(posX), y(posY) {
}

void BuildBuildingCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->buildBuilding(buildingType, Position{ x, y });
}

CommandResult BuildBuildingCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Строительство нечего отменять");
    CommandResult undoResult = mediator_->cancelBuilding(result_.objectId);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string BuildBuildingCommand::describe() const {
    return "BuildBuildingCommand: " + buildingTypeToString(buildingType) + " в (" + std::to_string(x) + "," +
        std::to_string(y) + ")";
}

ResearchCommand::ResearchCommand(std::shared_ptr<GameMediator> mediator, std::string technology)
    : Command(std::move(mediator)), technologyId(std::move(technology)) {
}

void ResearchCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->startResearch(technologyId);
}

CommandResult ResearchCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Исследование нечего отменять");
    CommandResult undoResult = mediator_->cancelResearch(technologyId);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string ResearchCommand::describe() const {
    return "ResearchCommand: " + technologyId;
}

DistributeResourcesCommand::DistributeResourcesCommand(std::shared_ptr<GameMediator> mediator, ResourceType resource,
                                                       const DistributionTarget& target, int count)
    : Command(std::move(mediator)), from(resource), to(target), amount(count) {
}

void DistributeResourcesCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->distributeResources(from, to, amount);
}

CommandResult DistributeResourcesCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Распределение нечего отменять");
    // посредник вернул в objectId здание, которому реально достались ресурсы
    CommandResult undoResult = mediator_->undoDistribution(from, result_.objectId, amount);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string DistributeResourcesCommand::describe() const {
    return "DistributeResourcesCommand: " + std::to_string(amount) + " (" + resourceTypeToString(from) + ") -> " +
        to.describe();
}

ProduceUnitCommand::ProduceUnitCommand(std::shared_ptr<GameMediator> mediator, int building, UnitType type)
    : Command(std::move(mediator)), buildingId(building), unitType(type) {
}

void ProduceUnitCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->produceUnit(buildingId, unitType);
}

CommandResult ProduceUnitCommand::undo() {
    if (!canUndo()) return CommandResult::fail("Производство нечего отменять");
    // в value посредник вернул номер заказа в очереди здания
    CommandResult undoResult = mediator_->cancelProduction(buildingId, result_.value);
    if (undoResult.success) undone_ = true;
    return undoResult;
}

std::string ProduceUnitCommand::describe() const {
    return "ProduceUnitCommand: " + unitTypeToString(unitType) + " в здании #" + std::to_string(buildingId);
}

StatsCommand::StatsCommand(std::shared_ptr<GameMediator> mediator, StatsScope statsScope, int id)
    : Command(std::move(mediator)), scope(statsScope), unitId(id) {
}

void StatsCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->getStats(scope, unitId);
}

std::string StatsCommand::describe() const {
    switch (scope) {
    case StatsScope::Summary:   return "StatsCommand: общая статистика";
    case StatsScope::Units:     return "StatsCommand: юниты";
    case StatsScope::Buildings: return "StatsCommand: здания";
    case StatsScope::Resources: return "StatsCommand: ресурсы";
    case StatsScope::Research:  return "StatsCommand: исследования";
    case StatsScope::Map:       return "StatsCommand: карта";
    case StatsScope::Unit:      return "StatsCommand: юнит #" + std::to_string(unitId);
    }
    throw std::invalid_argument("Неизвестный раздел статистики");
}

EndTurnCommand::EndTurnCommand(std::shared_ptr<GameMediator> mediator, int count)
    : Command(std::move(mediator)), turns(count) {
}

void EndTurnCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->advanceTurn(turns);
}

std::string EndTurnCommand::describe() const {
    return "EndTurnCommand: ходов " + std::to_string(turns);
}

SpawnEnemyUnitCommand::SpawnEnemyUnitCommand(std::shared_ptr<GameMediator> mediator, UnitType unitType, int posX, int posY)
    : Command(std::move(mediator)), type(unitType), x(posX), y(posY) {
}

void SpawnEnemyUnitCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->spawnEnemyUnit(type, Position{ x, y });
}

std::string SpawnEnemyUnitCommand::describe() const {
    return "SpawnEnemyUnitCommand: враг " + unitTypeToString(type) + " в (" + std::to_string(x) + "," +
        std::to_string(y) + ")";
}

SpawnEnemyBuildingCommand::SpawnEnemyBuildingCommand(std::shared_ptr<GameMediator> mediator, BuildingType buildingType,
                                                     int posX, int posY)
    : Command(std::move(mediator)), type(buildingType), x(posX), y(posY) {
}

void SpawnEnemyBuildingCommand::execute() {
    if (!beginExecution()) return;
    result_ = mediator_->spawnEnemyBuilding(type, Position{ x, y });
}

std::string SpawnEnemyBuildingCommand::describe() const {
    return "SpawnEnemyBuildingCommand: вражеское здание " + buildingTypeToString(type) + " в (" +
        std::to_string(x) + "," + std::to_string(y) + ")";
}
