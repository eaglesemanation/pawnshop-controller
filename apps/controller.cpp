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
#include <cmath>
#include <condition_variable>
#include <future>
#include <gpiod.hpp>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <pawnshop/config.hpp>
#include <pawnshop/db.hpp>
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

// State machine with MQTT messages as events
class Controller {
    shared_ptr<mqtt::async_client> mqtt;
    shared_ptr<MqttHandler::MessageQueue> incoming_messages;

    shared_ptr<atomic<bool>> interrupted;

    unique_ptr<Db> db;
    CalibrationInfo calibration_info;

    unique_ptr<Scales> scales;
    unique_ptr<Rails> rails;
    unique_ptr<DevicesConfig> dev;

    unique_ptr<thread> receiver;
    unique_ptr<thread> task;

    enum State { IDLE, MEASURING, MOVING, CALIBRATING };

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
                spdlog::debug("Recieved message, topic: {}, current state: {}",
                              msg.topic, state.load());
                if (msg.topic == "PawnShop/controller/measure") {
                    bool flag = msg.payload.get<bool>();
                    if (state.load() == IDLE && flag) {
                        // TODO: Add "product_id" to message
                        task =
                            make_unique<thread>(&Controller::measure, this, 0);
                    } else if (state.load() == MEASURING && !flag) {
                        state.store(IDLE);
                        state_cv->notify_all();
                    }
                } else if (msg.topic == "PawnShop/controller/move") {
                    Vec3D pos = msg.payload.get<Vec3D>();
                    spdlog::debug("Moving to (x: {:.1f}, y: {:.1f}, z: {:.1f})",
                                  pos.at(0), pos.at(1), pos.at(2));
                    if (state.load() == IDLE) {
                        state.store(MOVING);
                        rails->move(pos);
                        state.store(IDLE);
                    }
                } else if (msg.topic == "PawnShop/controller/calibrate") {
                    bool flag = msg.payload.get<bool>();
                    if (state.load() == IDLE && flag) {
                        calibrate();
                    }
                }
            } catch (json::exception& e) {
                spdlog::warn("Ill-formed message on topic \"{}\": {}",
                             msg.topic, e.what());
            }
        }
    }

    void calibrate() {
        state.store(CALIBRATING);

        auto prev_info = db->getCalibrationInfo();

        rails->calibrate();
        // Move to the safe height to avoid collisions
        rails->move({0.0, 0.0, dev->safe_height});

        if (!scales->poweredOn()) {
            pressScalesButton();
        }

        double baseline_weight = scales->getWeight().value_or(0);

        calibration_info.caret_weight = scaleWeighting(baseline_weight);
        if (prev_info) {
            if (abs(prev_info->caret_weight - calibration_info.caret_weight) /
                    prev_info->caret_weight >
                0.10) {
                spdlog::warn(
                    "Caret weight differs from last calibration by more than "
                    "10%");
            }
        }

        calibration_info.caret_submerged_weight =
            submergedWeighting(baseline_weight);
        if (prev_info) {
            if (abs(prev_info->caret_submerged_weight -
                    calibration_info.caret_submerged_weight) /
                    prev_info->caret_submerged_weight >
                0.10) {
                spdlog::warn(
                    "Caret submerged weight differs from last calibration by "
                    "more that 10%");
            }
        }

        drying();

        const auto& reciever_coord = dev->gold_reciever->coordinate;
        rails->move({reciever_coord[0], reciever_coord[1], dev->safe_height});

        db->updateCalibrationInfo(calibration_info);

        state.store(IDLE);
    }

    void measure(int64_t product_id) {
        state.store(MEASURING);

        Measurement m;
        m.product_id = product_id;

        mqtt->publish("PawnShop/cmd", "FillUS");

        if (!scales->poweredOn()) {
            pressScalesButton();
        }

        // permanent weight control while filling
        double baseline_weight = scales->getWeight().value_or(0);
        getGold();

        m.dirty_weight =
            scaleWeighting(baseline_weight) - calibration_info.caret_weight;

        washing();

        drying();

        m.clean_weight =
            scaleWeighting(baseline_weight) - calibration_info.caret_weight;
        m.submerged_weight = submergedWeighting(baseline_weight) -
                             calibration_info.caret_submerged_weight;
        m.density = m.clean_weight / m.submerged_weight;

        // id generated on insertion
        m.id = db->insertMeasurement(m);

        json payload = m;
        // FIXME: Include calibration info for debugging purpuses, should be
        // removed
        payload.update(calibration_info);
        mqtt->publish("PawnShop/report", payload.dump());

        drying();

        const auto& reciever_coord = dev->gold_reciever->coordinate;
        rails->move({reciever_coord[0], reciever_coord[1], dev->safe_height});

        state.store(IDLE);
    }

    void getGold() {
        const auto& reciever_coord = dev->gold_reciever->coordinate;
        const Vec3D reciever_top_coord = {reciever_coord[0], reciever_coord[1],
                                          dev->safe_height};

        rails->move(reciever_top_coord);
        rails->move(reciever_coord);
        rails->move(reciever_top_coord);
    }

    void pressScalesButton() {
        const auto& btn_coord = dev->scales->power_button->coordinate;
        const Vec3D btn_offset_coord = {
            btn_coord[0], dev->gold_reciever->coordinate[1], btn_coord[2]};

        rails->move(btn_offset_coord);
        rails->move(btn_coord);
        std::this_thread::sleep_for(1s);
        rails->move(btn_offset_coord);
        std::this_thread::sleep_for(1s);
    }

    void washing() {
        const auto& usbath_coord = dev->ultrasonic_bath->coordinate;
        const Vec3D usbath_top_coord = {usbath_coord[0], usbath_coord[1],
                                        dev->safe_height};

        rails->move(usbath_top_coord);
        rails->move(usbath_coord);
        std::this_thread::sleep_for(1s);
        mqtt->publish("PawnShop/cmd", "US");
        std::this_thread::sleep_for(dev->ultrasonic_bath->duration);
        mqtt->publish("PawnShop/cmd", "USoff");
        // mqtt.publish("PawnShop/cmd", "Empty");
        rails->move(usbath_top_coord);
        std::this_thread::sleep_for(1s);
    }

    void drying() {
        const auto& dryer_coord = dev->dryer->coordinate;
        const Vec3D dryer_top_coord = {dryer_coord[0], dryer_coord[1],
                                       dev->safe_height};

        rails->move(dryer_top_coord);
        rails->move(dryer_coord);
        mqtt->publish("PawnShop/cmd", "Dry");
        std::this_thread::sleep_for(dev->dryer->duration);
        mqtt->publish("PawnShop/cmd", "SDry");
        std::this_thread::sleep_for(1s);
        rails->move(dryer_top_coord);
    }

    /**
     * Measure object weight directly on scales
     *
     * @returns measured weight or 0 in case of failure
     */
    double scaleWeighting(double baseline_weight) {
        const auto& scale_coord = dev->scales->coordinate;
        const Vec3D scale_top_coord = {scale_coord[0], scale_coord[1],
                                       dev->safe_height};

        rails->move(scale_top_coord);
        rails->move(scale_coord);
        auto weight = scales->getWeight();
        rails->move(scale_top_coord);
        return weight.value_or(0) - baseline_weight;
    }

    /**
     * Measure object weight submerged in cup
     * In case cup is not filled enough - it will request filling and update
     * baseline_weight accordingly
     *
     * @returns measured weight or 0 in case of failure
     */
    double submergedWeighting(double& baseline_weight) {
        const auto& cup_coord = dev->scales->cup->coordinate;
        const Vec3D cup_top_coord = {cup_coord[0], cup_coord[1],
                                     dev->safe_height};

        const double desired_weight = dev->scales->cup->desired_weight;
        if (baseline_weight < desired_weight) {
            mqtt->publish("PawnShop/cmd",
                          json{{"cmd", "FillCup"},
                               {"Value", desired_weight - baseline_weight}}
                              .dump());
            this_thread::sleep_for(1s);
            baseline_weight = scales->getWeight().value_or(0);
        }

        rails->move(cup_top_coord);
        rails->move(cup_coord);
        auto weight = scales->getWeight();
        rails->move(cup_top_coord);
        return weight.value_or(0) - baseline_weight;
    }

public:
    Controller(shared_ptr<Config> config, shared_ptr<mqtt::async_client> mqtt,
               shared_ptr<MqttHandler::MessageQueue> incoming_messages,
               shared_ptr<atomic<bool>> interrupted)
        : mqtt(mqtt),
          incoming_messages(incoming_messages),
          interrupted(interrupted) {
        scales = make_unique<Scales>(move(config->scales));
        rails = make_unique<Rails>(move(config->rails));
        dev = move(config->devices);

        db = make_unique<Db>(move(config->db));

        receiver = make_unique<thread>(&Controller::recieveMsg, this);
        state_cv = make_unique<condition_variable>();

        calibrate();
    }

    ~Controller() {
        state.store(IDLE);
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

    auto config = make_shared<Config>();

    auto mqtt = make_shared<mqtt::async_client>(config->mqtt->broker_url,
                                                config->mqtt->client_id);
    auto mqtt_options = mqtt::connect_options_builder()
                            .user_name(config->mqtt->username)
                            .password(config->mqtt->password)
                            .finalize();
    auto incoming_messages = make_shared<MqttHandler::MessageQueue>();
    MqttHandler mqtt_handler(mqtt, mqtt_options, shutdown_requested,
                             shutdown_cv, incoming_messages);
    mqtt->set_callback(mqtt_handler);
    mqtt->connect(mqtt_options, nullptr, mqtt_handler);

    Controller controller(config, mqtt, incoming_messages, shutdown_requested);

    int signum = 0;
    sigwait(&sigset, &signum);
    shutdown_requested->store(true);
    shutdown_cv->notify_all();
    spdlog::info("Recieved signal, terminating");
}
