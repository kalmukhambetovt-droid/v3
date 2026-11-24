#pragma once
#include "GameTypes.h"
#include <optional>
#include <string>
#include <vector>

namespace InputParser {
    std::string toLower(const std::string& text);
    std::string trim(const std::string& text);
    std::string firstWord(const std::string& text);
    std::vector<std::string> tokenize(const std::string& text);
    std::optional<int> parseInt(const std::string& text);
    std::optional<int> parseId(const std::string& text);
    std::optional<Position> parseCoordinates(const std::vector<std::string>& tokens, size_t from);
}
