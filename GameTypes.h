#pragma once
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>

struct Position {
    int x = 0;
    int y = 0;
};

inline bool operator==(const Position& left, const Position& right) {
    return left.x == right.x && left.y == right.y;
}

inline bool operator!=(const Position& left, const Position& right) {
    return !(left == right);
}

// шаг по диагонали стоит столько же, сколько шаг по прямой
inline int distance(const Position& from, const Position& to) {
    return std::max(std::abs(from.x - to.x), std::abs(from.y - to.y));
}

inline std::string toString(const Position& position) {
    return "(" + std::to_string(position.x) + "," + std::to_string(position.y) + ")";
}

// игрок управляет своими объектами, враги нужны для сценариев атаки
enum class Faction { Player, Enemy };

inline const char* factionToString(Faction faction) {
    switch (faction) {
    case Faction::Player: return "игрок";
    case Faction::Enemy:  return "враг";
    }
    throw std::invalid_argument("Неизвестная сторона конфликта");
}

// текущий приказ юнита, показывается в состоянии юнита
enum class UnitOrder { Idle, Move, Attack, Collect, Heal, Stuck };

inline const char* orderToString(UnitOrder order) {
    switch (order) {
    case UnitOrder::Idle:    return "ожидание";
    case UnitOrder::Move:    return "перемещение";
    case UnitOrder::Attack:  return "атака";
    case UnitOrder::Collect: return "сбор ресурсов";
    case UnitOrder::Heal:    return "лечение";
    case UnitOrder::Stuck:   return "застрял в болоте";
    }
    throw std::invalid_argument("Неизвестный приказ юнита");
}
