#pragma once
#include "Command.h"
#include "UnitTypeInfo.h"
#include "GameTypes.h"

class CreateUnitCommand : public Command {
private:
    UnitType type;
    int x;
    int y;
public:
    CreateUnitCommand(std::shared_ptr<GameMediator> mediator, UnitType unitType, int posX, int posY);
    void execute() override;
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "CreateUnitCommand"; }
    std::string describe() const override;
};

class MoveUnitCommand : public Command {
private:
    int unitId;
    int targetX;
    int targetY;
    Position from;   // откуда ушёл юнит, нужно для отмены
public:
    MoveUnitCommand(std::shared_ptr<GameMediator> mediator, int id, int x, int y);
    void execute() override;
    CommandResult undo() override;
    bool isUndoable() const override { return true; }
    std::string getName() const override { return "MoveUnitCommand"; }
    std::string describe() const override;
};

// атака необратима: урон и гибель юнита не откатываются
class AttackCommand : public Command {
private:
    int attackerId;
    int targetId;
    AttackTarget targetKind;
public:
    AttackCommand(std::shared_ptr<GameMediator> mediator, int attacker, int target,
                  AttackTarget kind = AttackTarget::Unit);
    void execute() override;
    std::string getName() const override { return "AttackCommand"; }
    std::string describe() const override;
};

// добыча необратима: ресурс уже извлечён из точки
class CollectResourceCommand : public Command {
private:
    int unitId;
    int resourcePointId;
public:
    CollectResourceCommand(std::shared_ptr<GameMediator> mediator, int id, int pointId);
    void execute() override;
    std::string getName() const override { return "CollectResourceCommand"; }
    std::string describe() const override;
};

class HealCommand : public Command {
private:
    int unitId;
    int targetId;    // id юнита-пациента для медика или id госпиталя
    HealMode mode;
public:
    HealCommand(std::shared_ptr<GameMediator> mediator, int id, int target, HealMode healMode = HealMode::Medic);
    void execute() override;
    std::string getName() const override { return "HealCommand"; }
    std::string describe() const override;
};
