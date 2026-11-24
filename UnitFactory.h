#pragma once
#include "Unit.h"
#include "MilitaryUnits.h"
#include "SupportUnits.h"
#include <array>
#include <memory>


class UnitFactory {
protected:
    virtual std::unique_ptr<Unit> createSupportedUnit(UnitType type, int id, Faction faction,
                                                      const Position& position) const = 0;
public:
    virtual ~UnitFactory() = default;
    virtual bool supports(UnitType type) const = 0;
    std::unique_ptr<Unit> createUnit(UnitType type, int id, Faction faction, const Position& position) const;
};

// боевые юниты: пехота, разведчик, танк
class MilitaryUnitFactory : public UnitFactory {
protected:
    std::unique_ptr<Unit> createSupportedUnit(UnitType type, int id, Faction faction,
                                              const Position& position) const override;
public:
    bool supports(UnitType type) const override;
};

// юниты поддержки: рабочий и медик
class SupportUnitFactory : public UnitFactory {
protected:
    std::unique_ptr<Unit> createSupportedUnit(UnitType type, int id, Faction faction,
                                              const Position& position) const override;
public:
    bool supports(UnitType type) const override;
};


class UnitFactoryRegistry {
private:
    MilitaryUnitFactory militaryFactory;
    SupportUnitFactory supportFactory;
    std::array<const UnitFactory*, kUnitTypeCount> factoryByType;

public:
    UnitFactoryRegistry();
    UnitFactoryRegistry(const UnitFactoryRegistry&) = delete;
    UnitFactoryRegistry& operator=(const UnitFactoryRegistry&) = delete;

    const UnitFactory& getFactory(UnitType type) const;
    std::unique_ptr<Unit> createUnit(UnitType type, int id, Faction faction, const Position& position) const;
};
