#pragma once
#include <optional>
#include <string>

enum class ResourceType { Wood, Metal, Energy };

// набор из трёх ресурсов: им описываются и запасы, и стоимость юнитов, зданий, исследований
struct ResourceStock {
    int wood = 0;
    int metal = 0;
    int energy = 0;

    int get(ResourceType type) const;
    bool covers(const ResourceStock& cost) const;
    bool isEmpty() const;
    ResourceStock& operator+=(const ResourceStock& other);
    ResourceStock& operator-=(const ResourceStock& other);
};

ResourceStock operator+(ResourceStock left, const ResourceStock& right);
ResourceStock operator-(ResourceStock left, const ResourceStock& right);

// набор, где заполнен только один ресурс
ResourceStock makeStock(ResourceType type, int amount);

std::string toString(const ResourceStock& stock);
const char* resourceTypeToString(ResourceType type);
std::optional<ResourceType> resourceTypeFromString(const std::string& name);
