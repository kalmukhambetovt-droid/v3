#pragma once
#include "MediatorColleague.h"

class Unit;
class Building;

// расчёт и применение урона, результат боя уходит посреднику
class CombatSystem : public MediatorColleague {
private:
    // поправка за сочетание типов: танк хорошо ломает здания, пехоте танк пробить трудно
    static double typeModifier(UnitType attacker, UnitType target);

public:
    int calculateDamage(const Unit& attacker, const Unit& target) const;
    int calculateDamage(const Unit& attacker, const Building& target) const;

    CombatReport resolve(Unit& attacker, Unit& target);
    CombatReport resolve(Unit& attacker, Building& target);
    // ответный удар выжившей цели, не тратит её действие в ходу
    CombatReport counterattack(Unit& defender, Unit& attacker);
};
