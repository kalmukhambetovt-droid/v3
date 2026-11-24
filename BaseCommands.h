#pragma once
#include "Command.h"
#include "BuildingTypeInfo.h"
#include "UnitTypeInfo.h"
#include "ResourceStock.h"
#include <string>

// команды управления базой

class BuildBuildingCommand : public Command {
private:
    BuildingType buildingType;
    int x;
    int y;
public:
    BuildBuildingCommand(std::shared_ptr<GameMediator> mediator, BuildingType type, int posX, int posY);
    void execute() override;
    // отменить можно только недостроенное здание
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "BuildBuildingCommand"; }
    std::string describe() const override;
};

class ResearchCommand : public Command {
private:
    std::string technologyId;
public:
    ResearchCommand(std::shared_ptr<GameMediator> mediator, std::string technology);
    void execute() override;
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "ResearchCommand"; }
    std::string describe() const override;
};

class DistributeResourcesCommand : public Command {
private:
    ResourceType from;       // какой ресурс берём со склада
    DistributionTarget to;   // какому зданию отдаём
    int amount;
public:
    DistributeResourcesCommand(std::shared_ptr<GameMediator> mediator, ResourceType resource,
                               const DistributionTarget& target, int count);
    void execute() override;
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "DistributeResourcesCommand"; }
    std::string describe() const override;
};

class ProduceUnitCommand : public Command {
private:
    int buildingId;
    UnitType unitType;
public:
    ProduceUnitCommand(std::shared_ptr<GameMediator> mediator, int building, UnitType type);
    void execute() override;
    // отменить можно заказ, который ещё в очереди
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "ProduceUnitCommand"; }
    std::string describe() const override;
};
