#pragma once
#include "CommandTypes.h"
#include <memory>
#include <string>

class GameMediator;

// Команда: каждое действие пользователя - отдельный объект с методом execute()
// команда ничего не решает сама, она только вызывает нужный метод посредника и хранит результат
class Command {
protected:
    // посредником совместно владеют фасад и команды в истории: команда должна суметь выполнить undo()
    std::shared_ptr<GameMediator> mediator_;
    CommandResult result_;
    bool executed_;
    bool undone_;

    // защита от повторного выполнения одного и того же объекта команды
    bool beginExecution();
    // отменить можно только успешно выполненную и ещё не отменённую команду
    bool canUndo() const { return executed_ && result_.success && !undone_; }

public:
    explicit Command(std::shared_ptr<GameMediator> mediator);
    virtual ~Command() = default;
    // команда хранит результат и данные для отмены, её копия могла бы отменить действие дважды
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;

    virtual void execute() = 0;
    // по умолчанию команда необратима (атака, добыча, новый ход)
    virtual CommandResult undo();
    virtual bool isUndoable() const { return false; }
    // запросы не меняют игру и в историю не попадают
    virtual bool storesInHistory() const { return true; }
    virtual std::string getName() const = 0;
    virtual std::string describe() const = 0;

    const CommandResult& getResult() const { return result_; }
    bool wasExecuted() const { return executed_; }
    bool wasUndone() const { return undone_; }
};
