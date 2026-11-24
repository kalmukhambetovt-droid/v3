#pragma once
#include "Terrain.h"
#include <array>
#include <memory>
#include <optional>

class Unit;

class TerrainProxy : public Terrain {
private:
    struct TerrainEffect {
        bool passable;
        double speed;
        int defense;
        int stuckTurns;
    };

    std::unique_ptr<Terrain> realTerrain;
    mutable std::array<std::optional<TerrainEffect>, kUnitTypeCount> cache;
    mutable int realRequests;

    const TerrainEffect& effectFor(UnitType unitType) const;

public:
    explicit TerrainProxy(std::unique_ptr<Terrain> terrain);
    TerrainProxy(const TerrainProxy& other);
    TerrainProxy& operator=(const TerrainProxy&) = delete;

    bool isPassable(UnitType unitType) const override;
    double getSpeedModifier(UnitType unitType) const override;
    int getDefenseBonus(UnitType unitType) const override;
    int getStuckTurns(UnitType unitType) const override;
    bool isBuildable() const override;
    char getSymbol() const override;
    std::string getName() const override;
    std::unique_ptr<Terrain> clone() const override;

    bool canEnter(const Unit& unit) const;
    int getRealRequestCount() const { return realRequests; }
};
