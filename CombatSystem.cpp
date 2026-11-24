#include "CombatSystem.h"
#include "Unit.h"
#include "Building.h"
#include <cmath>

double CombatSystem::typeModifier(UnitType attacker, UnitType target) {
    if (attacker == UnitType::Infantry && target == UnitType::Tank) return 0.5;
    if (attacker == UnitType::Tank && target != UnitType::Tank) return 1.25;
    return 1.0;
}

// урон = атака с поправкой за типы минус броня цели вместе с бонусом ландшафта
// броня может полностью поглотить слабый удар, как в ЛР2
int CombatSystem::calculateDamage(const Unit& attacker, const Unit& target) const {
    const double modified = attacker.getDamage() * typeModifier(attacker.getUnitType(), target.getUnitType());
    const Armor targetArmor(target.getArmorValue());
    return targetArmor.reduceDamage(static_cast<int>(std::lround(modified)));
}

int CombatSystem::calculateDamage(const Unit& attacker, const Building& target) const {
    const double modifier = attacker.getUnitType() == UnitType::Tank ? 1.5 : 1.0;
    const Armor targetArmor(target.getArmorValue());
    return targetArmor.reduceDamage(static_cast<int>(std::lround(attacker.getDamage() * modifier)));
}

CombatReport CombatSystem::resolve(Unit& attacker, Unit& target) {
    const int damage = calculateDamage(attacker, target);
    attacker.attack(target, damage);

    CombatReport report;
    report.attackerId = attacker.getId();
    report.targetId = target.getId();
    report.damage = damage;
    report.targetHealthLeft = target.getHealth();
    report.targetDestroyed = !target.isAlive();
    // что делать с погибшим юнитом, решает посредник, а не система боя
    lockMediator()->notifyCombatResolved(report);
    return report;
}

CombatReport CombatSystem::counterattack(Unit& defender, Unit& attacker) {
    const int damage = calculateDamage(defender, attacker);
    attacker.takeDamage(damage);

    CombatReport report;
    report.attackerId = defender.getId();
    report.targetId = attacker.getId();
    report.damage = damage;
    report.targetHealthLeft = attacker.getHealth();
    report.targetDestroyed = !attacker.isAlive();
    lockMediator()->notifyCombatResolved(report);
    return report;
}

CombatReport CombatSystem::resolve(Unit& attacker, Building& target) {
    const int damage = calculateDamage(attacker, target);
    attacker.attackBuilding(target, damage);

    CombatReport report;
    report.attackerId = attacker.getId();
    report.targetId = target.getId();
    report.targetIsBuilding = true;
    report.damage = damage;
    report.targetHealthLeft = target.getHealth();
    report.targetDestroyed = !target.isAlive();
    lockMediator()->notifyCombatResolved(report);
    return report;
}
