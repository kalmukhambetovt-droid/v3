#pragma once
#include <stdexcept>
#include "Armor.h"

// атака юнита
// базовое значение берётся из UnitTypeInfo, постоянный бонус даёт исследование attack_upgrade
class Attack {
private:
    int baseValue;
    int upgradeBonus;

public:
    explicit Attack(int startValue) : baseValue(startValue), upgradeBonus(0) {
        if (startValue < 0) throw std::invalid_argument("Атака не может быть отрицательной");
    }

    void upgrade(int amount) {
        if (amount < 0) throw std::invalid_argument("Улучшение атаки не может быть отрицательным");
        upgradeBonus += amount;
    }

    // учитывает броню цели и возвращает реальный урон
    int calculateDamage(const Armor& targetArmor) const {
        return targetArmor.reduceDamage(getValue());
    }

    int getValue() const { return baseValue + upgradeBonus; }
    int getBaseValue() const { return baseValue; }
    int getBonus() const { return upgradeBonus; }
};
