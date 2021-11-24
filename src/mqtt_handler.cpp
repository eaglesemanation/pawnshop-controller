#include "pawnshop/mqtt_handler.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

using namespace std;
using namespace std::chrono_literals;
using namespace moodycamel;
using json = nlohmann::json;

namespace pawnshop {

MqttConfig::MqttConfig(const toml::table& table) {
    broker_url = table["broker_url"].value<string>().value();
    client_id = table["client_id"].value<string>().value();
    username = table["username"].value<string>().value();
    password = table["password"].value<string>().value();
}

// Tries to reconnect with delay, unless interupted
void MqttHandler::reconnect() {
    unique_lock lk(interupt_cv_m);
    interupt_cv->wait_for(lk, 2s, [this]() { return interupted->load(); });
    if (!interupted->load()) {
        try {
            mqtt->connect(mqtt_options, nullptr, *this);
        } catch (mqtt::exception e) {
            spdlog::info("Failed to reconnect to MQTT broker: {}", e.what());
        }
    }
}

// Called when connection attempt failed
void MqttHandler::on_failure(const mqtt::token& token) {
    spdlog::debug("Connection to MQTT broker failed, reconnecting");
    reconnect();
}

void MqttHandler::on_success(const mqtt::token& token) {}

// Called when working connection lost
void MqttHandler::connection_lost(const std::string& cause) {
    spdlog::info("Connection to MQTT broker lost, reconnecting");
    reconnect();
}

// Called when successfuly connected
// NOTE: Add subscriptions here
void MqttHandler::connected(const std::string& cause) {
    spdlog::info("Connected to MQTT broker");
    mqtt->subscribe("PawnShop/controller/measure", QOS);
    mqtt->subscribe("PawnShop/controller/move", QOS);
    mqtt->subscribe("PawnShop/controller/calibrate", QOS);
    mqtt->subscribe("PawnShop/controller/calibration/accept", QOS);
}

// Called on arrival of message on any of topics we subscribed to
void MqttHandler::message_arrived(mqtt::const_message_ptr msg) {
    MqttMessage parsed_msg;

    parsed_msg.topic = msg->get_topic();
    spdlog::debug("Recieved message on \"{}\"", parsed_msg.topic);

    // Any payload should be serialized as JSON, otherwise message will be
    // ignored
    try {
        parsed_msg.payload = json::parse(msg->get_payload_str());
    } catch (json::parse_error e) {
        spdlog::warn("Recieved non JSON payload on topic \"{}\": {}",
                     parsed_msg.topic, e.what());
        return;
    }

    in->enqueue(parsed_msg);
}

}  // namespace pawnshop
