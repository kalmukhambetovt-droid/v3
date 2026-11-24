#pragma once
#include "IGameFacade.h"
#include "CommandHistory.h"
#include <iosfwd>
#include <memory>
#include <string>

class GameMediator;
class Handler;

class GameFacade : public IGameFacade {
private:
    std::ostream& out_;
    std::shared_ptr<GameMediator> mediator_;
    CommandHistory history_;
    std::shared_ptr<Handler> chain_;   // первый обработчик цепочки ввода

    void buildChain();

public:
    explicit GameFacade(std::ostream& output);
    // обработчики цепочки хранят ссылку на этот фасад, поэтому копирование и перемещение запрещены
    GameFacade(const GameFacade&) = delete;
    GameFacade& operator=(const GameFacade&) = delete;
    GameFacade(GameFacade&&) = delete;
    GameFacade& operator=(GameFacade&&) = delete;
    ~GameFacade() override;

    CommandResult executeCommand(std::shared_ptr<Command> cmd);

    CommandResult createUnit(UnitType type, const Position& position) override;
    CommandResult moveUnit(int unitId, const Position& target) override;
    CommandResult attack(int attackerId, int targetId) override;
    CommandResult attackBuilding(int attackerId, int buildingId) override;
    CommandResult collectResource(int unitId, int resourcePointId) override;
    CommandResult heal(int unitId, int targetId) override;
    CommandResult healAtHospital(int unitId, int hospitalId) override;

    CommandResult buildBuilding(BuildingType type, const Position& position) override;
    CommandResult research(const std::string& technologyId) override;
    CommandResult distributeResources(ResourceType from, const DistributionTarget& to, int amount) override;
    CommandResult produceUnit(int buildingId, UnitType type) override;
    std::string getStats(StatsScope scope, int unitId) override;

    CommandResult endTurn(int turns) override;
    CommandResult spawnEnemyUnit(UnitType type, const Position& position) override;
    CommandResult spawnEnemyBuilding(BuildingType type, const Position& position) override;
    CommandResult undo() override;
    std::string getHistory() const override;
    void printMessage(const std::string& message) override;
    bool processInput(const std::string& input) override;

    size_t getHistorySize() const { return history_.size(); }
    std::shared_ptr<const Command> getLastCommand() const { return history_.getLastCommand(); }
    std::shared_ptr<const GameMediator> getMediator() const { return mediator_; }
};
