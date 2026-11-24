#pragma once
#include "Command.h"
#include <memory>
#include <string>
#include <vector>

class CommandHistory {
private:
    std::vector<std::shared_ptr<Command>> commands;
    size_t limit;

public:
    explicit CommandHistory(size_t maxSize = 100);
    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;
    CommandHistory(CommandHistory&&) noexcept = default;
    CommandHistory& operator=(CommandHistory&&) noexcept = default;

    void push(std::shared_ptr<Command> command);
    CommandResult undo();
    std::shared_ptr<Command> getLastCommand() const;
    size_t size() const { return commands.size(); }
    bool empty() const { return commands.empty(); }
    std::string describe() const;
};
