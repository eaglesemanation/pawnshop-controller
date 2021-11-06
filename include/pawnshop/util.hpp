#pragma once
#include <chrono>
#include <istream>
#include <optional>

namespace pawnshop {

std::optional<std::string> getline_timeout(std::istream &is,
                                           std::chrono::duration<int> timeout);

}
