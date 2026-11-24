# ЛР3. Логическое разделение классов

В проекте применены паттерны **Фасад** (`GameFacade`), **Посредник** (`GameMediator`), **Команда** (`Command` + `CommandHistory`) и **Цепочка обязанностей** (`Handler` + обработчики ввода).

Без аргументов программа запускает проверки и демонстрационные сценарии, а затем переходит в интерактивный режим. `--demo` запускает только проверки и сценарии, а `--play` сразу открывает интерактивный режим,  `--demo` возвращает 0, если проверки пройдены.

create unit <infantry|scout|tank|worker|medic> at <x>,<y>
move unit <id> to <x>,<y>
attack unit <id> target <id>        | attack unit <id> building <id>
collect unit <id> from <resourceId>
heal unit <id> target <id>          | heal unit <id> at <hospitalId>
build <barracks|factory|powerplant|laboratory|warehouse|hospital> at <x>,<y>
research <attack_upgrade|armor_upgrade|unlock_tank>
distribute <wood|metal|energy> to <тип здания|id здания> <количество>
produce <unitType> in <buildingId>
stats | stats units | stats buildings | stats resources | stats research | stats map | stats unit <id>
next | next <n> | end turn
spawn enemy <unitType|buildingType> at <x>,<y>
undo | history | help | exit

## Структура файлов

| Фасад и посредник | `IGameFacade.h`, `IMediator.h`, `MediatorColleague.h`, `GameFacade.h`, `GameMediator.h`, `GameTypes.h`, `GameCore.cpp` |
| Команды | `Command.h`, `CommandHistory.h`, `CommandTypes.h`, `UnitCommands.h`, `BaseCommands.h`, `GameCommands.h`, `Commands.cpp` |
| Цепочка обязанностей и разбор ввода | `Handler.h`, `Handlers.h`, `InputParser.h`, `Handlers.cpp` |
| База | `Base.h`, `Base.cpp` |
| Юниты и фабрики | `Unit.h`, `MilitaryUnits.h`, `SupportUnits.h`, `UnitFactory.h`, `UnitManager.h`, `UnitTypeInfo.h`, `Health.h`, `Armor.h`, `Attack.h`, `UnitSystem.cpp` |
| Здания и технологии | `Building.h`, `Buildings.h`, `BuildingManager.h`, `BuildingTypeInfo.h`, `TechnologyInfo.h`, `BuildingSystem.cpp` |
| Ресурсы и бой | `ResourceStock.h`, `ResourcePoint.h`, `ResourceSystem.h`, `CombatSystem.h`, `Resources.cpp`, `CombatSystem.cpp` |
| Карта и ландшафт | `Terrain.h`, `TerrainTypes.h`, `TerrainProxy.h`, `GameMap.h`, `GameWorld.cpp` |
| Запуск и сборка | `main.cpp`|

## Владение объектами

- `GameFacade` создаётся на стеке в `main`; копирование и перемещение запрещены, так как обработчики хранят ссылку на фасад.
- `GameMediator` создаётся через `GameMediator::create()` и хранится в `shared_ptr`: им совместно владеют фасад и команды в истории. Копирование `shared_ptr` создаёт ещё одну ссылку на тот же объект, а не копию объекта.
- Посредник владеет `Base`, `UnitManager`, `BuildingManager`, `ResourceSystem`, `CombatSystem` и `GameMap` как полями. Компоненты, которые сообщают о событиях, держат `weak_ptr<IMediator>`, поэтому цикла сильных ссылок нет.
- Юниты и здания хранятся в `shared_ptr`, поскольку ими совместно владеют менеджеры и `Base` (учёт и история потерь).
- `GameMap` владеет клетками через `unique_ptr` и поддерживает глубокое копирование через `clone()`, копирующее присваивание и перемещение.
- Фабрика создаёт `unique_ptr<Unit>` и явно передаёт владение в `UnitManager`.

## Проверки и демонстрация

В `main.cpp` находятся автоматические проверки разбора ввода, истории команд, работы посредника, фасада, владения объектами и копирования, перемещения карты, а также шесть демонстрационных сценариев. Проверка выполняется командой `--demo`.

## Обработка ошибок

Ошибочные пользовательские команды и нарушения игровых правил возвращаются как `CommandResult::fail()` с понятным сообщением. Неожиданные исключения не перехватываются общим обработчиком внутри фасада: они доходят до верхнего уровня `main()`, где выводится `[FATAL]`, а программа завершается с ненулевым кодом. Это не позволяет скрывать ошибки реализации под обычным отказом команды.
