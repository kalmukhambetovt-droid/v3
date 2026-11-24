#pragma once
#include "Handler.h"

// create unit <type> at <x>,<y>
class CreateHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// move unit <id> to <x>,<y>
class MoveHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// attack unit <id> target <id> | attack unit <id> building <id>
class AttackHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// collect unit <id> from <resourceId>
class CollectHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// heal unit <id> target <id> | heal unit <id> at <hospitalId>
class HealHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// build <buildingType> at <x>,<y>
class BuildHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// research <technologyId>
class ResearchHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// distribute <resource> to <buildingType|buildingId> <amount>
class DistributeHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// produce <unitType> in <buildingId>
class ProduceHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// stats | stats units | stats buildings | stats resources | stats research | stats map | stats unit <id>
class StatsHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// next | next <n> | end turn
class TurnHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// spawn enemy <unitType|buildingType> at <x>,<y>
class SpawnHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// undo
class UndoHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// history
class HistoryHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};

// help
class HelpHandler : public Handler {
public:
    using Handler::Handler;
    bool handle(const std::string& cmd) override;
};
