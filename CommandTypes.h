#pragma once
#include "BuildingTypeInfo.h"
#include <optional>
#include <string>

// результат операции посредника, его хранит команда и выводит фасад
struct CommandResult {
    bool success = false;
    std::string message;
    int objectId = 0;   // id созданного или затронутого объекта, нужен для отмены
    int value = 0;      

    static CommandResult ok(const std::string& text, int id = 0, int extra = 0) {
        return CommandResult{ true, text, id, extra };
    }
    static CommandResult fail(const std::string& text) {
        return CommandResult{ false, text, 0, 0 };
    }
};

enum class StatsScope { Summary, Units, Buildings, Resources, Research, Unit, Map };

enum class AttackTarget { Unit, Building };

enum class HealMode { Medic, Hospital };

// куда направить ресурсы: конкретное здание по id или первое готовое здание нужного типа
struct DistributionTarget {
    int buildingId = 0;
    std::optional<BuildingType> buildingType;

    std::string describe() const {
        if (buildingType) return buildingTypeToString(*buildingType);
        return "здание #" + std::to_string(buildingId);
    }
};
