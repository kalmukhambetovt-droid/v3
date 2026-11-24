#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "GameFacade.h"
#include "GameMediator.h"
#include "UnitCommands.h"
#include "InputParser.h"
#include "TerrainTypes.h"

static int failedChecks = 0;

static void check(bool condition, const char* name) {
    if (condition) std::cout << "[OK]   " << name << "\n";
    else {
        std::cout << "[FAIL] " << name << "\n";
        ++failedChecks;
    }
}

// выполняет строки так, как будто их ввёл пользователь
static void runScript(GameFacade& game, const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cout << "\n";
        game.processInput(line);
    }
}

static std::shared_ptr<Unit> findUnit(const GameFacade& game, int unitId) {
    return game.getMediator()->getUnits().getUnitById(unitId);
}

static bool sameStock(const ResourceStock& left, const ResourceStock& right) {
    return left.wood == right.wood && left.metal == right.metal && left.energy == right.energy;
}

// тестовая команда, считает вызовы, чтобы проверить фасад отдельно от игровой логики
class CountingCommand : public Command {
private:
    int& counter;
public:
    CountingCommand(std::shared_ptr<GameMediator> mediator, int& executions)
        : Command(std::move(mediator)), counter(executions) {
    }
    void execute() override {
        if (!beginExecution()) return;
        ++counter;
        result_ = CommandResult::ok("тестовая команда выполнена");
    }
    std::string getName() const override { return "CountingCommand"; }
    std::string describe() const override { return "CountingCommand: тест"; }
};

// автоматические проверки

static void runParserTests() {
    std::cout << "\nТесты 1. Разбор команд и цепочка обработчиков\n";
    const std::vector<std::string> tokens = InputParser::tokenize("  MOVE  Unit 3 TO 5,7 ");
    check(tokens == std::vector<std::string>{ "move", "unit", "3", "to", "5,7" },
        "tokenize переводит в нижний регистр и убирает лишние пробелы");
    const auto spaced = InputParser::parseCoordinates({ "5", ",", "7" }, 0);
    check(spaced && spaced->x == 5 && spaced->y == 7, "координаты \"5 , 7\" разбираются как 5,7");
    check(!InputParser::parseCoordinates({ "5", "7" }, 0), "координаты без запятой отклоняются");
    check(!InputParser::parseCoordinates({ "a,7" }, 0), "нечисловые координаты отклоняются");
    check(!InputParser::parseInt("12abc") && !InputParser::parseInt("99999999999"),
        "parseInt не принимает мусор и переполнение");
    check(!InputParser::parseId("0") && !InputParser::parseId("-3") && InputParser::parseId("42") == 42,
        "id должен быть положительным числом");

    std::ostringstream sink;
    GameFacade game(sink);
    check(!game.processInput("fly to the moon"), "неизвестную команду цепочка не обрабатывает");
    check(game.processInput("move unit abc to 1,1") && game.getHistorySize() == 0 &&
        sink.str().find("[Ошибка ввода]") != std::string::npos,
        "MoveHandler узнаёт команду и сообщает о неверном id");
    check(game.processInput("CREATE UNIT INFANTRY AT 5,5") && game.getHistorySize() == 1,
        "CreateHandler понимает команду в верхнем регистре");
    check(game.processInput("create unit worker at 5 , 6") && game.getHistorySize() == 2,
        "пробелы вокруг запятой в координатах допустимы");
    check(game.processInput("stats units") && game.getHistorySize() == 2, "StatsHandler не пишет запрос в историю");
    check(game.processInput("build barracks at 3,3") && game.processInput("distribute wood to 1 30") &&
        game.getHistorySize() == 3, "команда отклонена посредником (казарма строится) и не попала в историю");
    check(game.processInput("research") && sink.str().find("Формат: research") != std::string::npos,
        "ResearchHandler подсказывает формат");
}

static void runHistoryTests() {
    std::cout << "\nТесты 2. История команд и отмена\n";
    std::ostringstream sink;
    GameFacade game(sink);
    const ResourceStock start = game.getMediator()->getResources().getStock();

    check(game.createUnit(UnitType::Infantry, Position{ 5, 5 }).success, "юнит создан через фасад");
    check(game.moveUnit(1, Position{ 7, 7 }).success && findUnit(game, 1)->getPosition() == Position{ 7, 7 },
        "юнит перемещён");
    check(game.undo().success && findUnit(game, 1)->getPosition() == Position{ 5, 5 } &&
        findUnit(game, 1)->getMovePoints() == 5, "undo возвращает юнита и запас хода");
    check(game.undo().success && !findUnit(game, 1) && sameStock(game.getMediator()->getResources().getStock(), start),
        "undo создания убирает юнита и возвращает ресурсы");
    check(!game.undo().success && !game.getLastCommand(), "пустую историю отменить нельзя");

    game.createUnit(UnitType::Infantry, Position{ 4, 4 });
    game.spawnEnemyUnit(UnitType::Infantry, Position{ 5, 4 });
    game.attack(2, 3);
    const size_t sizeBeforeUndo = game.getHistorySize();
    check(!game.undo().success && game.getHistorySize() == sizeBeforeUndo,
        "атака необратима и не даёт отменить команды до неё");

    std::ostringstream otherSink;
    const std::shared_ptr<GameMediator> mediator = GameMediator::create(otherSink);
    const auto command = std::make_shared<CreateUnitCommand>(mediator, UnitType::Worker, 4, 4);
    command->execute();
    command->execute();
    check(!command->getResult().success && mediator->getUnits().getUnits().size() == 1,
        "один объект команды нельзя выполнить дважды");
}

static void runMediatorTests() {
    std::cout << "\nТесты 3. Взаимодействие через посредника\n";
    std::ostringstream sink;
    GameFacade game(sink);
    const GameMediator& mediator = *game.getMediator();

    game.createUnit(UnitType::Worker, Position{ 6, 4 });
    check(game.collectResource(1, 1).success, "рабочий дошёл до лесопилки и добыл дерево");
    check(mediator.getBase().getResources() == 25 && mediator.getResources().getWood() == 300,
        "посредник передал добычу складу и базе");
    check(mediator.getResources().findPoint(1)->getRemaining() == 275, "запас ресурсной точки уменьшился");
    check(!game.collectResource(1, 1).success, "второй раз за ход рабочий не добывает");
    game.createUnit(UnitType::Infantry, Position{ 5, 5 });
    check(!game.collectResource(2, 1).success, "пехотинец не умеет добывать ресурсы");

    game.buildBuilding(BuildingType::Warehouse, Position{ 4, 2 });
    game.buildBuilding(BuildingType::Laboratory, Position{ 5, 2 });
    check(!game.research("attack_upgrade").success, "исследование не стартует, пока лаборатория строится");
    game.endTurn(1);
    check(mediator.getResources().getCapacity() == 750, "готовый склад через посредника увеличил вместимость");
    game.endTurn(2);
    check(game.research("attack_upgrade").success, "исследование запущено в готовой лаборатории");
    game.endTurn(2);
    check(mediator.getBase().hasTechnology("attack_upgrade") && findUnit(game, 2)->getDamage() == 15,
        "завершённое исследование усилило уже существующего юнита");

    game.spawnEnemyUnit(UnitType::Tank, Position{ 5, 6 });
    for (int i = 0; i < 3; ++i) {
        game.attack(2, 3);
        game.endTurn(1);
    }
    check(!findUnit(game, 2) && mediator.getBase().getLostUnitCount() == 1,
        "погибший от ответного удара юнит снят с учёта и попал в историю потерь базы");

    check(!game.createUnit(UnitType::Tank, Position{ 3, 5 }).success, "танк недоступен без исследования");
    game.research("unlock_tank");
    game.endTurn(3);
    check(game.createUnit(UnitType::Tank, Position{ 3, 5 }).success, "unlock_tank открыл танк");
    const int tankId = mediator.getUnits().getUnitAt(Position{ 3, 5 })->getId();
    check(game.moveUnit(tankId, Position{ 3, 6 }).success && findUnit(game, tankId)->isStuck(),
        "тяжёлый танк увяз в болоте");
    check(!game.moveUnit(tankId, Position{ 3, 7 }).success, "застрявший танк не может двигаться");
    game.endTurn(1);
    check(!game.moveUnit(tankId, Position{ 3, 5 }).success, "в следующем ходу танк пропускает ход");
    game.endTurn(1);
    check(game.moveUnit(tankId, Position{ 3, 5 }).success, "через ход танк снова может ехать");

    const TerrainProxy& forest = mediator.getMap().getTerrain(Position{ 8, 4 });
    const int requestsBefore = forest.getRealRequestCount();
    forest.isPassable(UnitType::Medic);
    forest.getSpeedModifier(UnitType::Medic);
    forest.getDefenseBonus(UnitType::Medic);
    forest.getStuckTurns(UnitType::Medic);
    check(forest.getRealRequestCount() - requestsBefore == 1, "прокси обращается к ландшафту один раз и кеширует ответ");
}

static void runFacadeTests() {
    std::cout << "\nТесты 4. Фасад и владение объектами\n";
    std::ostringstream sink;
    GameFacade game(sink);

    int executions = 0;
    std::ostringstream otherSink;
    const auto command = std::make_shared<CountingCommand>(GameMediator::create(otherSink), executions);
    check(game.executeCommand(command).success && executions == 1, "executeCommand вызывает execute() команды");
    check(game.getLastCommand() == command && game.getHistorySize() == 1, "выполненная команда сохранена в истории");
    bool nullRejected = false;
    try { game.executeCommand(nullptr); }
    catch (const std::invalid_argument&) { nullRejected = true; }
    check(nullRejected, "фасад отклоняет пустую команду");

    check(game.createUnit(UnitType::Scout, Position{ 3, 3 }).success &&
        game.getMediator()->getUnits().getUnits().size() == 1, "метод фасада делегирует работу посреднику");
    check(sink.str().find("[Команда] CreateUnitCommand") != std::string::npos &&
        sink.str().find("[Mediator] база учла") != std::string::npos, "действие и реакция посредника попали в лог");

    const std::shared_ptr<const GameMediator> first = game.getMediator();
    const std::shared_ptr<const GameMediator> second = first;
    check(first.get() == second.get() && second.use_count() >= 3,
        "вторая переменная shared_ptr ссылается на того же посредника");

    GameMap original(4, 4);
    original.setTerrain(Position{ 1, 1 }, std::make_unique<Forest>());
    GameMap copy = original;
    copy.setTerrain(Position{ 1, 1 }, std::make_unique<Mountains>());
    check(original.getTerrain(Position{ 1, 1 }).getSymbol() == 'f' && copy.getTerrain(Position{ 1, 1 }).getSymbol() == '^',
        "копия карты независима от оригинала");
    GameMap assigned(2, 2);
    assigned = original;
    check(assigned.getWidth() == 4 && assigned.getTerrain(Position{ 1, 1 }).getSymbol() == 'f',
        "копирующее присваивание карты");
    GameMap moved = std::move(copy);
    check(moved.getWidth() == 4 && moved.getTerrain(Position{ 1, 1 }).getSymbol() == '^' && copy.getWidth() == 0,
        "перемещение забирает клетки, исходная карта остаётся пустой");
}

// демонстрационные сценарии

static void runScenarios() {
    std::cout << "\nСценарий 1. База, юниты и статистика\n";
    {
        GameFacade game(std::cout);
        runScript(game, {
            "create unit infantry at 5,5",
            "create unit worker at 4,3",
            "create unit medic at 6,2",
            "create unit dragon at 1,1",
            "create unit infantry at 15,15",
            "stats",
            "stats map"
            });
        check(game.getMediator()->getBase().getUnitCount() == 3, "сценарий 1: база учитывает трёх юнитов");
    }

    std::cout << "\nСценарий 2. Перемещение и атака\n";
    {
        GameFacade game(std::cout);
        runScript(game, {
            "create unit infantry at 5,5",
            "spawn enemy infantry at 11,10",
            "move unit 1 to 10,10",
            "attack unit 1 target 2",
            "stats units",
            "attack unit 1 target 2",
            "next",
            "attack unit 1 target 2",
            "stats unit 2"
            });
        check(findUnit(game, 1)->getPosition() == Position{ 10, 10 }, "сценарий 2: юнит дошёл до (10,10)");
        check(findUnit(game, 2)->getHealth() == 80 && findUnit(game, 1)->getHealth() == 80,
            "сценарий 2: урон с учётом брони и ответный удар");
    }

    std::cout << "\n Сценарий 3. Строительство, исследование, производство \n";
    {
        GameFacade game(std::cout);
        runScript(game, {
            "build barracks at 3,3",
            "build laboratory at 5,3",
            "research attack_upgrade",
            "next 3",
            "research attack_upgrade",
            "produce infantry in 1",
            "stats buildings",
            "next",
            "stats research",
            "next",
            "stats units",
            "stats"
            });
        const auto produced = findUnit(game, 1);
        check(produced && produced->getDamage() == 15, "сценарий 3: казарма выпустила пехотинца, исследование усилило его");
    }

    std::cout << "\n Сценарий 4. Сбор и распределение ресурсов \n";
    {
        GameFacade game(std::cout);
        runScript(game, {
            "create unit worker at 6,4",
            "collect unit 1 from 1",
            "stats resources",
            "build barracks at 3,3",
            "next 2",
            "distribute wood to barracks 50",
            "produce infantry in 1",
            "stats resources"
            });
        const GameMediator& mediator = *game.getMediator();
        check(mediator.getBase().getResources() == 25, "сценарий 4: база получила 25 дерева от рабочего");
        check(mediator.getResources().getAllocation(1).wood == 40,
            "сценарий 4: производство взяло дерево из выделенных казарме ресурсов");
    }

    std::cout << "\n Сценарий 5. Отмена команд \n";
    {
        GameFacade game(std::cout);
        const ResourceStock start = game.getMediator()->getResources().getStock();
        runScript(game, {
            "create unit scout at 4,4",
            "move unit 1 to 7,7",
            "build warehouse at 4,2",
            "history",
            "undo",
            "undo",
            "undo",
            "undo",
            "stats"
            });
        check(sameStock(game.getMediator()->getResources().getStock(), start) &&
            game.getMediator()->getUnits().getUnits().empty(), "сценарий 5: после отмены всё вернулось к началу");
    }

    std::cout << "\n Сценарий 6. Лечение и госпиталь \n";
    {
        GameFacade game(std::cout);
        runScript(game, {
            "create unit infantry at 4,4",
            "create unit medic at 3,4",
            "spawn enemy infantry at 5,4",
            "attack unit 1 target 3",
            "heal unit 2 target 1",
            "undo",
            "build hospital at 2,5",
            "next 2",
            "attack unit 1 target 3",
            "heal unit 1 at 1",
            "next",
            "heal unit 1 at 1",
            "stats unit 1"
            });
        // энергия: 150 - медик 10 - госпиталь 10 - лечение 10
        check(findUnit(game, 1)->getHealth() == 100 && game.getMediator()->getResources().getEnergy() == 120,
            "сценарий 6: медик и госпиталь вылечили юнита, лечение стоило энергии");
    }
}

static void runInteractive() {
    GameFacade game(std::cout);
    std::cout << "\n Интерактивный режим \n"
        << "help - список команд, exit - выход\n";
    game.processInput("stats map");
    std::string line;
    while (true) {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, line)) break;
        const std::string word = InputParser::firstWord(line);
        if (word == "exit" || word == "quit") break;
        game.processInput(line);
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // --demo: только проверки и сценарии, --play: сразу интерактивный режим
    const std::string mode = argc > 1 ? argv[1] : "";
    try {
        if (mode != "--play") {
            runParserTests();
            runHistoryTests();
            runMediatorTests();
            runFacadeTests();
            runScenarios();
            std::cout << "\nПроверок не пройдено: " << failedChecks << "\n";
            if (failedChecks == 0) std::cout << "Все проверки пройдены.\n";
        }
        if (mode != "--demo") runInteractive();
    }
    catch (const std::exception& ex) {
        std::cerr << "[FATAL] " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
    return failedChecks == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
