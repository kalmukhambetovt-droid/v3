#include <cmath>
#include <stdexcept>
#include <utility>

#include "GameMap.h"
#include "TerrainProxy.h"
#include "TerrainTypes.h"
#include "Unit.h"

GameMap::GameMap(int w, int h) : width(w), height(h) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("Размеры карты должны быть больше нуля");
    cells.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));
    for (int i = 0; i < w * h; ++i) {
        cells.push_back(std::make_unique<TerrainProxy>(std::make_unique<Plains>()));
    }
}

GameMap::GameMap(const GameMap& other) : width(other.width), height(other.height) {
    cells.reserve(other.cells.size());
    for (const auto& cell : other.cells) {
        cells.push_back(std::make_unique<TerrainProxy>(*cell));
    }
}

// если копия бросит исключение, текущая карта останется прежней
GameMap& GameMap::operator=(const GameMap& other) {
    if (this != &other) {
        GameMap copy(other);
        std::swap(width, copy.width);
        std::swap(height, copy.height);
        cells.swap(copy.cells);
    }
    return *this;
}

GameMap::GameMap(GameMap&& other) noexcept
    : width(other.width), height(other.height), cells(std::move(other.cells)) {
    other.width = 0;
    other.height = 0;
}

GameMap& GameMap::operator=(GameMap&& other) noexcept {
    if (this != &other) {
        width = other.width;
        height = other.height;
        cells = std::move(other.cells);
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

size_t GameMap::indexOf(const Position& position) const {
    if (!inBounds(position)) throw std::out_of_range("Координаты клетки выходят за границы карты");
    return static_cast<size_t>(position.y) * static_cast<size_t>(width) + static_cast<size_t>(position.x);
}

bool GameMap::inBounds(const Position& position) const {
    return position.x >= 0 && position.x < width && position.y >= 0 && position.y < height;
}

void GameMap::setTerrain(const Position& position, std::unique_ptr<Terrain> terrain) {
    if (!terrain) throw std::invalid_argument("Нельзя установить пустой ландшафт");
    cells[indexOf(position)] = std::make_unique<TerrainProxy>(std::move(terrain));
}

const TerrainProxy& GameMap::getTerrain(const Position& position) const {
    return *cells[indexOf(position)];
}

int GameMap::moveCost(UnitType unitType, const Position& from, const Position& to) const {
    const TerrainProxy& destination = getTerrain(to);
    if (!destination.isPassable(unitType)) return -1;
    const int steps = distance(from, to);
    const double speed = destination.getSpeedModifier(unitType);
    // медленная клетка делает каждый шаг дороже: при скорости 0.5 шаг стоит 2 очка хода
    return static_cast<int>(std::ceil(steps / speed - 1e-9));
}


TerrainProxy::TerrainProxy(std::unique_ptr<Terrain> terrain)
    : realTerrain(std::move(terrain)), cache(), realRequests(0) {
    if (!realTerrain) throw std::invalid_argument("Прокси не может работать без реального ландшафта");
}

TerrainProxy::TerrainProxy(const TerrainProxy& other)
    : realTerrain(other.realTerrain->clone()), cache(other.cache), realRequests(0) {
}

// единственная точка, где прокси обращается к реальному ландшафту
const TerrainProxy::TerrainEffect& TerrainProxy::effectFor(UnitType unitType) const {
    const size_t index = static_cast<size_t>(unitType);
    if (index >= cache.size()) throw std::invalid_argument("Неизвестный тип юнита");
    if (!cache[index]) {
        ++realRequests;
        cache[index] = TerrainEffect{
            realTerrain->isPassable(unitType),
            realTerrain->getSpeedModifier(unitType),
            realTerrain->getDefenseBonus(unitType),
            realTerrain->getStuckTurns(unitType)
        };
    }
    return *cache[index];
}

bool TerrainProxy::isPassable(UnitType unitType) const { return effectFor(unitType).passable; }
double TerrainProxy::getSpeedModifier(UnitType unitType) const { return effectFor(unitType).speed; }
int TerrainProxy::getDefenseBonus(UnitType unitType) const { return effectFor(unitType).defense; }
int TerrainProxy::getStuckTurns(UnitType unitType) const { return effectFor(unitType).stuckTurns; }
bool TerrainProxy::isBuildable() const { return realTerrain->isBuildable(); }
char TerrainProxy::getSymbol() const { return realTerrain->getSymbol(); }
std::string TerrainProxy::getName() const { return realTerrain->getName(); }

std::unique_ptr<Terrain> TerrainProxy::clone() const {
    return std::make_unique<TerrainProxy>(*this);
}

// кроме типа учитываем состояние конкретного юнита
bool TerrainProxy::canEnter(const Unit& unit) const {
    if (!unit.isAlive() || unit.isStuck()) return false;
    return effectFor(unit.getUnitType()).passable;
}
