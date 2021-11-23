#pragma once

#include <chrono>
#include <string>

#include "pawnshop/db.hpp"
#include "pawnshop/rails.hpp"
#include "pawnshop/scales.hpp"
#include "pawnshop/vec.hpp"
#include "pawnshop/mqtt_handler.hpp"

namespace pawnshop {

struct PositioningConfig {
    double safe_height;

    struct Dryer {
        vec::Vec3D coordinate;
        std::chrono::seconds duration;

        Dryer(const toml::table& table);
    };
    std::unique_ptr<Dryer> dryer;

    struct UltrasonicBath {
        vec::Vec3D coordinate;
        std::chrono::seconds duration;

        UltrasonicBath(const toml::table& table);
    };
    std::unique_ptr<UltrasonicBath> ultrasonic_bath;

    struct Scales {
        vec::Vec3D coordinate;
        struct Cup {
            vec::Vec3D coordinate;
            double desired_weight;

            Cup(const toml::table& table);
        };
        std::unique_ptr<Cup> cup;

        Scales(const toml::table& table);
    };
    std::unique_ptr<Scales> scales;

    PositioningConfig(const toml::table& table);
};

class Config {
    Config(const std::string& toml_path);

public:
    std::shared_ptr<DbConfig> db;
    std::shared_ptr<ScalesConfig> scales;
    std::shared_ptr<RailsConfig> rails;
    std::shared_ptr<PositioningConfig> positioning;
    std::shared_ptr<MqttConfig> mqtt;

    Config();
};

}  // namespace pawnshop
