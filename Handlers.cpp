#include <cctype>
#include <charconv>
#include <sstream>

#include "Handlers.h"
#include "IGameFacade.h"
#include "InputParser.h"
#include "TechnologyInfo.h"

using InputParser::firstWord;
using InputParser::parseCoordinates;
using InputParser::parseId;
using InputParser::tokenize;

// полная разбивка на слова делается только в том обработчике, который узнал команду по первому слову

bool CreateHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "create") return passToNext(cmd);
    const std::string usage = "create unit <type> at <x>,<y>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() < 5 || tokens[1] != "unit" || tokens[3] != "at") {
        return reportUsage("неверный формат команды", usage);
    }
    const auto type = unitTypeFromString(tokens[2]);
    if (!type) return reportUsage("неизвестный тип юнита \"" + tokens[2] + "\" (есть: " + unitTypeList() + ")", usage);
    const auto position = parseCoordinates(tokens, 4);
    if (!position) return reportUsage("координаты должны быть целыми числами вида x,y", usage);
    facade_.createUnit(*type, *position);
    return true;
}

bool MoveHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "move") return passToNext(cmd);
    const std::string usage = "move unit <id> to <x>,<y>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() < 5 || tokens[1] != "unit" || tokens[3] != "to") {
        return reportUsage("неверный формат команды", usage);
    }
    const auto unitId = parseId(tokens[2]);
    if (!unitId) return reportUsage("id юнита должен быть целым числом больше нуля", usage);
    const auto target = parseCoordinates(tokens, 4);
    if (!target) return reportUsage("координаты должны быть целыми числами вида x,y", usage);
    facade_.moveUnit(*unitId, *target);
    return true;
}

bool AttackHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "attack") return passToNext(cmd);
    const std::string usage = "attack unit <id> target <id> или attack unit <id> building <id>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 5 || tokens[1] != "unit" || (tokens[3] != "target" && tokens[3] != "building")) {
        return reportUsage("неверный формат команды", usage);
    }
    const auto attackerId = parseId(tokens[2]);
    const auto targetId = parseId(tokens[4]);
    if (!attackerId || !targetId) return reportUsage("id должны быть целыми числами больше нуля", usage);
    if (tokens[3] == "building") facade_.attackBuilding(*attackerId, *targetId);
    else facade_.attack(*attackerId, *targetId);
    return true;
}

bool CollectHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "collect") return passToNext(cmd);
    const std::string usage = "collect unit <id> from <resourceId>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 5 || tokens[1] != "unit" || tokens[3] != "from") {
        return reportUsage("неверный формат команды", usage);
    }
    const auto unitId = parseId(tokens[2]);
    const auto pointId = parseId(tokens[4]);
    if (!unitId || !pointId) return reportUsage("id должны быть целыми числами больше нуля", usage);
    facade_.collectResource(*unitId, *pointId);
    return true;
}

bool HealHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "heal") return passToNext(cmd);
    const std::string usage = "heal unit <id медика> target <id> или heal unit <id> at <id госпиталя>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 5 || tokens[1] != "unit" || (tokens[3] != "target" && tokens[3] != "at")) {
        return reportUsage("неверный формат команды", usage);
    }
    const auto unitId = parseId(tokens[2]);
    const auto targetId = parseId(tokens[4]);
    if (!unitId || !targetId) return reportUsage("id должны быть целыми числами больше нуля", usage);
    if (tokens[3] == "at") facade_.healAtHospital(*unitId, *targetId);
    else facade_.heal(*unitId, *targetId);
    return true;
}

bool BuildHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "build") return passToNext(cmd);
    const std::string usage = "build <buildingType> at <x>,<y>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() < 4 || tokens[2] != "at") return reportUsage("неверный формат команды", usage);
    const auto type = buildingTypeFromString(tokens[1]);
    if (!type) {
        return reportUsage("неизвестный тип здания \"" + tokens[1] + "\" (есть: " + buildingTypeList() + ")", usage);
    }
    const auto position = parseCoordinates(tokens, 3);
    if (!position) return reportUsage("координаты должны быть целыми числами вида x,y", usage);
    facade_.buildBuilding(*type, *position);
    return true;
}

bool ResearchHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "research") return passToNext(cmd);
    const std::string usage = "research <technologyId> (есть: " + technologyList() + ")";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 2) return reportUsage("неверный формат команды", usage);
    // существование технологии проверяет посредник, чтобы список технологий был в одном месте
    facade_.research(tokens[1]);
    return true;
}

bool DistributeHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "distribute") return passToNext(cmd);
    const std::string usage = "distribute <wood|metal|energy> to <тип здания|id здания> <количество>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 5 || tokens[2] != "to") return reportUsage("неверный формат команды", usage);
    const auto resource = resourceTypeFromString(tokens[1]);
    if (!resource) return reportUsage("неизвестный ресурс \"" + tokens[1] + "\"", usage);

    DistributionTarget target;
    if (const auto buildingId = parseId(tokens[3])) {
        target.buildingId = *buildingId;
    }
    else if (const auto buildingType = buildingTypeFromString(tokens[3])) {
        target.buildingType = *buildingType;
    }
    else {
        return reportUsage("\"" + tokens[3] + "\" не является ни id, ни типом здания", usage);
    }
    const auto amount = parseId(tokens[4]);
    if (!amount) return reportUsage("количество должно быть целым числом больше нуля", usage);
    facade_.distributeResources(*resource, target, *amount);
    return true;
}

bool ProduceHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "produce") return passToNext(cmd);
    const std::string usage = "produce <unitType> in <buildingId>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() != 4 || tokens[2] != "in") return reportUsage("неверный формат команды", usage);
    const auto type = unitTypeFromString(tokens[1]);
    if (!type) return reportUsage("неизвестный тип юнита \"" + tokens[1] + "\" (есть: " + unitTypeList() + ")", usage);
    const auto buildingId = parseId(tokens[3]);
    if (!buildingId) return reportUsage("id здания должен быть целым числом больше нуля", usage);
    facade_.produceUnit(*buildingId, *type);
    return true;
}

bool StatsHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "stats") return passToNext(cmd);
    const std::string usage = "stats [units|buildings|resources|research|map] или stats unit <id>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() == 1) {
        facade_.getStats(StatsScope::Summary, 0);
        return true;
    }
    if (tokens.size() == 3 && tokens[1] == "unit") {
        const auto unitId = parseId(tokens[2]);
        if (!unitId) return reportUsage("id юнита должен быть целым числом больше нуля", usage);
        facade_.getStats(StatsScope::Unit, *unitId);
        return true;
    }
    if (tokens.size() != 2) return reportUsage("неверный формат команды", usage);

    const std::string& section = tokens[1];
    if (section == "units") facade_.getStats(StatsScope::Units, 0);
    else if (section == "buildings") facade_.getStats(StatsScope::Buildings, 0);
    else if (section == "resources") facade_.getStats(StatsScope::Resources, 0);
    else if (section == "research") facade_.getStats(StatsScope::Research, 0);
    else if (section == "map") facade_.getStats(StatsScope::Map, 0);
    else return reportUsage("неизвестный раздел \"" + section + "\"", usage);
    return true;
}

bool TurnHandler::handle(const std::string& cmd) {
    const std::string word = firstWord(cmd);
    if (word != "next" && word != "end") return passToNext(cmd);
    const std::string usage = "next, next <количество ходов> или end turn";
    const auto tokens = tokenize(cmd);
    if (word == "end") {
        if (tokens.size() != 2 || tokens[1] != "turn") return reportUsage("неверный формат команды", usage);
        facade_.endTurn(1);
        return true;
    }
    if (tokens.size() == 1) {
        facade_.endTurn(1);
        return true;
    }
    const auto turns = tokens.size() == 2 ? parseId(tokens[1]) : std::nullopt;
    if (!turns) return reportUsage("количество ходов должно быть целым числом больше нуля", usage);
    facade_.endTurn(*turns);
    return true;
}

bool SpawnHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "spawn") return passToNext(cmd);
    const std::string usage = "spawn enemy <unitType|buildingType> at <x>,<y>";
    const auto tokens = tokenize(cmd);
    if (tokens.size() < 5 || tokens[1] != "enemy" || tokens[3] != "at") {
        return reportUsage("неверный формат команды", usage);
    }
    const auto position = parseCoordinates(tokens, 4);
    if (!position) return reportUsage("координаты должны быть целыми числами вида x,y", usage);
    if (const auto unitType = unitTypeFromString(tokens[2])) {
        facade_.spawnEnemyUnit(*unitType, *position);
    }
    else if (const auto buildingType = buildingTypeFromString(tokens[2])) {
        facade_.spawnEnemyBuilding(*buildingType, *position);
    }
    else {
        return reportUsage("\"" + tokens[2] + "\" не является типом юнита или здания", usage);
    }
    return true;
}

bool UndoHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "undo") return passToNext(cmd);
    if (tokenize(cmd).size() != 1) return reportUsage("у команды нет аргументов", "undo");
    facade_.undo();
    return true;
}

bool HistoryHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "history") return passToNext(cmd);
    if (tokenize(cmd).size() != 1) return reportUsage("у команды нет аргументов", "history");
    facade_.printMessage(facade_.getHistory());
    return true;
}

bool HelpHandler::handle(const std::string& cmd) {
    if (firstWord(cmd) != "help") return passToNext(cmd);
    facade_.printMessage(
        "=== Команды ===\n"
        "Юниты:\n"
        "  create unit <type> at <x>,<y>          нанять юнита у базы (" + unitTypeList() + ")\n"
        "  move unit <id> to <x>,<y>              переместить юнита\n"
        "  attack unit <id> target <id>           атаковать вражеского юнита\n"
        "  attack unit <id> building <id>         атаковать вражеское здание\n"
        "  collect unit <id> from <resourceId>    добыть ресурс рабочим (точки 1, 2, 3)\n"
        "  heal unit <id> target <id>             медик лечит союзника\n"
        "  heal unit <id> at <hospitalId>         отправить юнита в госпиталь\n"
        "  stats unit <id>                        состояние юнита\n"
        "База:\n"
        "  build <type> at <x>,<y>                построить здание (" + buildingTypeList() + ")\n"
        "  research <technologyId>                исследование (" + technologyList() + ")\n"
        "  distribute <resource> to <здание> <n>  выделить зданию ресурс (wood, metal, energy)\n"
        "  produce <unitType> in <buildingId>     заказать юнита в казарме или на заводе\n"
        "Обзор:\n"
        "  stats | stats units | stats buildings | stats resources | stats research | stats map\n"
        "Прочее:\n"
        "  next | next <n> | end turn             следующий ход\n"
        "  spawn enemy <unit|building> at <x>,<y> поставить врага для проверки атаки\n"
        "  undo                                   отменить последнюю команду\n"
        "  history                                история команд\n"
        "  exit                                   выход");
    return true;
}


namespace InputParser {

std::string toLower(const std::string& text) {
    std::string result = text;
    for (char& symbol : result) {
        symbol = static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
    }
    return result;
}

std::string trim(const std::string& text) {
    const size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string firstWord(const std::string& text) {
    const std::string trimmed = trim(text);
    const size_t space = trimmed.find_first_of(" \t");
    return toLower(trimmed.substr(0, space));
}

std::vector<std::string> tokenize(const std::string& text) {
    std::istringstream stream(toLower(text));
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) tokens.push_back(token);
    return tokens;
}

std::optional<int> parseInt(const std::string& text) {
    if (text.empty()) return std::nullopt;
    int value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(begin, end, value);
    // строка должна быть числом целиком
    if (error != std::errc() || stop != end) return std::nullopt;
    return value;
}

std::optional<int> parseId(const std::string& text) {
    const std::optional<int> value = parseInt(text);
    if (!value || *value <= 0) return std::nullopt;
    return value;
}

std::optional<Position> parseCoordinates(const std::vector<std::string>& tokens, size_t from) {
    if (from >= tokens.size()) return std::nullopt;
    std::string joined;
    for (size_t i = from; i < tokens.size(); ++i) {
        if (!joined.empty()) joined += ' ';
        joined += tokens[i];
    }
    const size_t comma = joined.find(',');
    if (comma == std::string::npos || joined.find(',', comma + 1) != std::string::npos) return std::nullopt;
    const std::optional<int> x = parseInt(trim(joined.substr(0, comma)));
    const std::optional<int> y = parseInt(trim(joined.substr(comma + 1)));
    if (!x || !y) return std::nullopt;
    return Position{ *x, *y };
}

}
