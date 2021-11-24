#pragma once

#include <chrono>
#include <string>

#include "pawnshop/db.hpp"
#include "pawnshop/mqtt_handler.hpp"
#include "pawnshop/rails.hpp"
#include "pawnshop/scales.hpp"
#include "pawnshop/vec.hpp"

namespace pawnshop {

struct DevicesConfig {
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

        struct PowerButton {
            vec::Vec3D coordinate;

            PowerButton(const toml::table& table);
        };
        std::unique_ptr<PowerButton> power_button;

        Scales(const toml::table& table);
    };
    std::unique_ptr<Scales> scales;

    struct GoldReciever {
        vec::Vec3D coordinate;

        GoldReciever(const toml::table& table);
    };
    std::unique_ptr<GoldReciever> gold_reciever;

    DevicesConfig(const toml::table& table);
};

class Config {
    Config(const std::string& toml_path);

public:
    std::unique_ptr<DbConfig> db;
    std::unique_ptr<ScalesConfig> scales;
    std::unique_ptr<RailsConfig> rails;
    std::unique_ptr<DevicesConfig> devices;
    std::unique_ptr<MqttConfig> mqtt;

    Config();
};

}  // namespace pawnshop
