#include "pawnshop/config.hpp"

#include <toml++/toml.h>

#include <exception>

using namespace std;
using namespace pawnshop::vec;

namespace pawnshop {

inline chrono::seconds parseDuration(const toml::table& table) {
    uint32_t value = table["value"].value_or(0);
    string unit = table["unit"].value_or("s");
    if (unit == "s") {
        return chrono::seconds(value);
    } else {
        throw bad_optional_access{};
    }
}

inline Vec3D parseCoord(const toml::array& array) {
    Vec3D coord;
    for (size_t i = 0; i < coord.size(); i++) {
        coord[i] = array[i].value<double>().value();
    }
    return coord;
}

PositioningConfig::Dryer::Dryer(const toml::table& table) {
    coordinate = parseCoord(*table["coordinate"].as_array());
    duration = parseDuration(*table["duration"].as_table());
}

PositioningConfig::UltrasonicBath::UltrasonicBath(const toml::table& table) {
    coordinate = parseCoord(*table["coordinate"].as_array());
    duration = parseDuration(*table["duration"].as_table());
}

PositioningConfig::Scales::Scales(const toml::table& table) {
    coordinate = parseCoord(*table["coordinate"].as_array());

    cup = make_unique<Cup>(*table["cup"].as_table());
}

PositioningConfig::Scales::Cup::Cup(const toml::table& table) {
    coordinate = parseCoord(*table["coordinate"].as_array());
    desired_weight = table["desired_weight"].value<double>().value();
}

PositioningConfig::PositioningConfig(const toml::table& table) {
    safe_height = table["safe_height"].value<double>().value();

    dryer = make_unique<Dryer>(*table["dryer"].as_table());
    ultrasonic_bath =
        make_unique<UltrasonicBath>(*table["ultrasonic_bath"].as_table());
    scales = make_unique<Scales>(*table["scales"].as_table());
}

Config::Config(const string& toml_path) {
    auto table = toml::parse_file(toml_path);

    scales = make_shared<ScalesConfig>(*table["scales"].as_table());
    rails = make_shared<RailsConfig>(*table["rails"].as_table());
    db = make_shared<DbConfig>(*table["db"].as_table());
    positioning =
        make_shared<PositioningConfig>(*table["positioning"].as_table());
}

Config::Config() : Config("./dist/config.toml") {}

}  // namespace pawnshop
