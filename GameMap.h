#pragma once
#include "TerrainProxy.h"
#include "GameTypes.h"
#include <memory>
#include <vector>

class GameMap {
private:
    int width;
    int height;
    // плоский массив клеток: индекс y * width + x
    std::vector<std::unique_ptr<TerrainProxy>> cells;

    size_t indexOf(const Position& position) const;

public:
    GameMap(int w, int h);
    ~GameMap() = default;
    GameMap(const GameMap& other);
    GameMap& operator=(const GameMap& other);
    GameMap(GameMap&& other) noexcept;
    GameMap& operator=(GameMap&& other) noexcept;

    bool inBounds(const Position& position) const;
    void setTerrain(const Position& position, std::unique_ptr<Terrain> terrain);
    const TerrainProxy& getTerrain(const Position& position) const;

    // стоимость хода по клетке назначения, -1 если клетка непроходима
    // промежуточные клетки пути не учитываются
    int moveCost(UnitType unitType, const Position& from, const Position& to) const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};
