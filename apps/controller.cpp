#include <gpiod.hpp>
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <vector>
#include <pawnshop/rails.hpp>
#include <pawnshop/scales.hpp>

using namespace std::chrono_literals;
using namespace pawnshop;
using namespace pawnshop::vec;
using json = nlohmann::json;

int main(int argc, char** argv) {

    mqtt::async_client mqtt("tcp://lombardmat.local:1883", "controller");
    auto mqttOptions = mqtt::connect_options_builder()
        .user_name("controller")
        .password("controller")
        .finalize();
    mqtt.start_consuming();
    auto mqttToken = mqtt.connect(mqttOptions);

    Scales scales("/dev/ttyS0");

    gpiod::chip gpioChip("/dev/gpiochip0");
    // const std::array stepSizeLineOffsets { 22, 27, 17 };
    Rails rails({
        //    len    steps   v_min v_max  accel                  cl  dir inverted 
        Axis{ 435.0, 17250u, 30.0, 600.0, 100.0, Motor{gpioChip, 06, 13, true}, LimitSwitch{gpioChip, 25} },
        Axis{ 530.0, 26750u, 30.0, 600.0, 100.0, Motor{gpioChip, 20, 16}, LimitSwitch{gpioChip, 5} },
        Axis{ 150.0, 47000u, 30.0, 600.0, 100.0, Motor{gpioChip, 19, 26, true}, LimitSwitch{gpioChip, 12} }
    });

    // rails.move({435.0, 530.0, 150.0});
    rails.move({400.0, 500.0, 150.0});
    rails.move({0.0, 0.0, 0.0});
    rails.move({5.0, 5.0, 5.0});

    // double netVolume, netWeight;
    // {
    //     rails.move({15900, 25000, 5000});
    //     rails.move({15900, 25000, 1900});
    //     rails.move({15900, 23000, 1900});
    //     rails.move({15900, 23000, 20000});

    //     rails.move({0, 21000, 47000});
    //     rails.move({0, 21000, 1000});
    //     rails.move({0, 21000, 25000});
    //     double before = scales.getWeight();
    //     rails.move({0, 21000, 1000});
    //     std::this_thread::sleep_for(5s);
    //     double laying = scales.getWeight();
    //     rails.move({0, 21000, 15000});
    //     std::this_thread::sleep_for(5s);
    //     double floating = scales.getWeight();
    //     netVolume = floating - before;
    //     netWeight = laying - before;
    //     rails.move({0, 21000, 47000});

    //     rails.move({15900, 23000, 1900});
    //     rails.move({15900, 25000, 1900});
    //     rails.move({15900, 25000, 5000});
    //     rails.move({0, 0, 0});
    // }

    auto mqttResp = mqttToken->get_connect_response();
    if(!mqttResp.is_session_present()) {
        mqtt.subscribe("display.measurement", 1)->wait();
    }

    // while (true) {
    //     auto msg = mqtt.consume_message();
    //     if (!msg) break;
    //     if(msg->get_topic() == "display.measurement") {
    //         rails.move({15900, 25000, 5000});
    //         rails.move({15900, 25000, 1900});
    //         rails.move({15900, 23000, 1900});
    //         rails.move({15900, 23000, 20000});

    //         rails.move({0, 21000, 47000});
    //         double before = scales.getWeight();
    //         rails.move({0, 21000, 1000});
    //         std::this_thread::sleep_for(5s);
    //         double laying = scales.getWeight();
    //         rails.move({0, 21000, 15000});
    //         std::this_thread::sleep_for(5s);
    //         double floating = scales.getWeight();
    //         double objVolume = floating - before - netVolume;
    //         double objWeight = laying - before - netWeight;
    //         double objDensity = objWeight / objVolume;
    //         json resp {
    //             {"volume", objVolume},
    //             {"weight", objWeight},
    //             {"density", objDensity}
    //         };
    //         mqtt.publish("controller.density", resp.dump());
    //         rails.move({0, 21000, 47000});

    //         rails.move({15900, 23000, 1900});
    //         rails.move({15900, 25000, 1900});
    //         rails.move({15900, 25000, 5000});
    //         rails.move({0, 0, 0});
    //     }
    // }
}