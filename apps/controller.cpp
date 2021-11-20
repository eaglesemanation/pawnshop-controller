#include <mqtt/async_client.h>
#include <mqtt/callback.h>
#include <mqtt/iaction_listener.h>
#include <mqtt/message.h>
#include <readerwriterqueue/readerwriterqueue.h>
#include <signal.h>
#include <spdlog/spdlog.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <gpiod.hpp>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <pawnshop/measurement_db.hpp>
#include <pawnshop/mqtt_handler.hpp>
#include <pawnshop/rails.hpp>
#include <pawnshop/scales.hpp>
#include <pawnshop/util.hpp>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace std;
using system_clock = std::chrono::system_clock;
using namespace pawnshop;
using namespace pawnshop::vec;
using json = nlohmann::json;

// begin of laser532 code here
#define Ztop 150.0
#define Zbtm 50.0
#define Xcup 1.0
#define Ycup 350.0
#define Zcup 15
#define Xscl 60.0
#define Yscl 365.0
#define Zscl 2
#define Xus 320.0
#define Yus 50.0
#define Zus 70
#define Xdr 120.0
#define Ydr 35.0
#define Zdr 45
#define tdr 40s
#define tus 15s

// end code of laser532

// State machine with MQTT messages as events
class Controller {
    shared_ptr<mqtt::async_client> mqtt;
    shared_ptr<MqttHandler::MessageQueue> incoming_messages;

    shared_ptr<atomic<bool>> interrupted;

    unique_ptr<MeasurementDb> db;

    unique_ptr<Scales> scales;
    unique_ptr<Rails> rails;

    unique_ptr<thread> receiver;
    unique_ptr<thread> task;

    enum State { IDLE, MEASURING, MOVING };

    atomic<State> state = IDLE;
    shared_ptr<condition_variable> state_cv;

    void recieveMsg() {
        while (!interrupted->load()) {
            MqttMessage msg;
            if (!incoming_messages->wait_dequeue_timed(msg, 1s)) continue;
            if (state.load() == IDLE && task != nullptr) {
                task->join();
                task.reset();
            }
            try {
                if (msg.topic == "PawnShop/controller/measure") {
                    bool flag = msg.payload.get<bool>();
                    if (state.load() == IDLE && flag) {
                        // TODO: Add "product_id" to message
                        task =
                            make_unique<thread>(&Controller::measure, this, 0);
                        state = MEASURING;
                    } else if (state.load() == MEASURING && !flag) {
                        state = IDLE;
                        state_cv->notify_all();
                    }
                } else if (msg.topic == "PawnShop/controller/move") {
                    Vec3D pos = msg.payload.get<Vec3D>();
                    spdlog::debug("Moving to (x: {:.1f}, y: {:.1f}, z: {:.1f})",
                                  pos.at(0), pos.at(1), pos.at(2));
                    if (state.load() == IDLE) {
                        state = MOVING;
                        rails->move(pos);
                        state = IDLE;
                    }
                }
            } catch (json::exception& e) {
                spdlog::warn("Ill-formed message on topic \"{}\": {}",
                             msg.topic, e.what());
            }
        }
    }

    void measure(int64_t product_id) {
        Measurement m;
        m.product_id = product_id;

        rails->move({0.0, 0.0, Ztop});

        mqtt->publish("PawnShop/cmd", "FillUS");

        if (!scales->poweredOn()) {
            pressScalesButton();
        }
        std::optional<double> weight = scales->getWeight();

        // permanent weight control while filling
        double baseline_weight = weight.value();
        const double desired_weight = 40;
        if (baseline_weight < desired_weight) {
            mqtt->publish("PawnShop/cmd",
                          json{{"cmd", "FillCup"},
                               {"Value", desired_weight - baseline_weight}}
                              .dump());
        }

        double caret_weight = scaleWeighting() - baseline_weight;

        getCarret();
        shakeZ();

        m.dirty_weight = scaleWeighting() - caret_weight - baseline_weight;

        // washing
        rails->move({Xus, Yus, Ztop});
        rails->move({Xus, Yus, Zus});
        std::this_thread::sleep_for(1s);
        mqtt->publish("PawnShop/cmd", "US");
        std::this_thread::sleep_for(tus);
        mqtt->publish("PawnShop/cmd", "USoff");
        // mqtt.publish("PawnShop/cmd", "Empty");
        rails->move({Xus, Yus, Ztop});
        std::this_thread::sleep_for(1s);

        drying();

        m.clean_weight = scaleWeighting() - caret_weight - baseline_weight;
        m.submerged_weight = submergedWeighting() - baseline_weight - 0.81;
        m.density = m.clean_weight / m.submerged_weight;

        // id generated on insertion
        m.id = db->insert(m);

        json payload = m;
        mqtt->publish("PawnShop/report", payload.dump());

        drying();

        // go home
        rails->move({0.0, 0.0, 50.0});

        state = IDLE;
    }

    void getCarret() {
        rails->move({400.0, 500.0, 150.0});
        rails->move({400.0, 500.0, 150.0});
    }

    void pressScalesButton() {
        rails->move({400.0, 500.0, 150.0});
        rails->move({400.0, 515.0, 150.0});
        std::this_thread::sleep_for(1s);
        rails->move({400.0, 500.0, 150.0});
        std::this_thread::sleep_for(1s);
    }

    void shakeZ() {
        rails->move({400.0, 500.0, 0.0});
        rails->move({400.0, 500.0, 150.0});
        rails->move({400.0, 500.0, 0.0});
        rails->move({400.0, 500.0, 80.0});
    }

    void drying() {
        rails->move({Xdr, Ydr, Ztop});
        rails->move({Xdr, Ydr, Zdr});
        mqtt->publish("PawnShop/cmd", "Dry");
        std::this_thread::sleep_for(tdr);
        mqtt->publish("PawnShop/cmd", "SDry");
        std::this_thread::sleep_for(1s);
        rails->move({Xdr, Ydr, Ztop});
    }

    /**
     * Measure object weight directly on scales
     *
     * @returns measured weight or 0 in case of failure
     */
    double scaleWeighting() {
        rails->move({Xscl, Yscl, Ztop});
        rails->move({Xscl, Yscl, Zscl});
        auto weight = scales->getWeight();
        rails->move({Xscl, Yscl, Ztop});
        return weight.value_or(0);
    }

    /**
     * Measure object weight submerged in cup
     *
     * @returns measured weight or 0 in case of failure
     */
    double submergedWeighting() {
        rails->move({Xcup, Ycup, Ztop});
        rails->move({Xcup, Ycup, Zcup});
        auto weight = scales->getWeight();
        rails->move({Xcup, Ycup, Ztop});
        return weight.value_or(0);
    }

public:
    Controller(shared_ptr<mqtt::async_client> mqtt,
               shared_ptr<MqttHandler::MessageQueue> incoming_messages,
               shared_ptr<atomic<bool>> interrupted)
        : mqtt(mqtt),
          incoming_messages(incoming_messages),
          interrupted(interrupted) {
        gpiod::chip gpio_chip{"/dev/gpiochip0"};
        scales = make_unique<Scales>("/dev/ttyS0");
        // const std::array stepSizeLineOffsets { 22, 27, 17 };
        rails = make_unique<Rails>(array{
            //   len    steps   v_min v_max  accel
            Axis{435.0, 17250u, 30.0, 600.0, 100.0,
                 //              cl  dir inverted
                 Motor{gpio_chip, 06, 13, true}, LimitSwitch{gpio_chip, 25}},
            Axis{530.0, 26750u, 30.0, 600.0, 100.0, Motor{gpio_chip, 20, 16},
                 LimitSwitch{gpio_chip, 5}},
            Axis{150.0, 94000u, 30.0, 600.0, 100.0,
                 Motor{gpio_chip, 19, 26, true}, LimitSwitch{gpio_chip, 12}}});

        db = make_unique<MeasurementDb>("measurements.sqlite3");

        receiver = make_unique<thread>(&Controller::recieveMsg, this);
        state_cv = make_unique<condition_variable>();
    }

    ~Controller() {
        state = IDLE;
        state_cv->notify_all();

        if (receiver) receiver->join();
        if (task) task->join();

        if (mqtt->is_connected()) mqtt->disconnect()->wait();
    }
};

int main(int argc, char** argv) {
    // Masks SIGINT and SIGTERM for all forked threads
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

    auto shutdown_requested = make_shared<atomic<bool>>(false);
    auto shutdown_cv = make_shared<condition_variable>();

    // Show all types of messages including debug
    // If you want to hide debug messages - comment this line
    spdlog::set_level(spdlog::level::debug);

    auto mqtt = make_shared<mqtt::async_client>("tcp://lombardmat.local:1883",
                                                "controller");
    auto mqtt_options = mqtt::connect_options_builder()
                            .user_name("controller")
                            .password("controller")
                            .finalize();
    auto incoming_messages = make_shared<MqttHandler::MessageQueue>();
    MqttHandler mqtt_handler(mqtt, mqtt_options, shutdown_requested,
                             shutdown_cv, incoming_messages);
    mqtt->set_callback(mqtt_handler);
    mqtt->connect(mqtt_options, nullptr, mqtt_handler);

    Controller controller(mqtt, incoming_messages, shutdown_requested);

    int signum = 0;
    sigwait(&sigset, &signum);
    shutdown_requested->store(true);
    shutdown_cv->notify_all();
    spdlog::info("Recieved signal, terminating");
}
