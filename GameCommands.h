#pragma once
#include "Command.h"
#include "UnitTypeInfo.h"
#include "BuildingTypeInfo.h"


class StatsCommand : public Command {
private:
    StatsScope scope;
    int unitId;
public:
    StatsCommand(std::shared_ptr<GameMediator> mediator, StatsScope statsScope, int id = 0);
    void execute() override;
    bool storesInHistory() const override { return false; }
    std::string getName() const override { return "StatsCommand"; }
    std::string describe() const override;
};

// новый ход необратим и служит границей для отмены
class EndTurnCommand : public Command {
private:
    int turns;
public:
    EndTurnCommand(std::shared_ptr<GameMediator> mediator, int count = 1);
    void execute() override;
    std::string getName() const override { return "EndTurnCommand"; }
    std::string describe() const override;
};

class SpawnEnemyUnitCommand : public Command {
private:
    UnitType type;
    int x;
    int y;
public:
    SpawnEnemyUnitCommand(std::shared_ptr<GameMediator> mediator, UnitType unitType, int posX, int posY);
    void execute() override;
    std::string getName() const override { return "SpawnEnemyUnitCommand"; }
    std::string describe() const override;
};

class SpawnEnemyBuildingCommand : public Command {
private:
    BuildingType type;
    int x;
    int y;
public:
    SpawnEnemyBuildingCommand(std::shared_ptr<GameMediator> mediator, BuildingType buildingType, int posX, int posY);
    void execute() override;
    std::string getName() const override { return "SpawnEnemyBuildingCommand"; }
    std::string describe() const override;
};
