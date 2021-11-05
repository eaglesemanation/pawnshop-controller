#pragma once
#include <optional>
#include <istream>
#include <chrono>

namespace pawnshop {

    std::optional<std::string> getline_timeout(std::istream &is, std::chrono::duration<int> timeout);

}
