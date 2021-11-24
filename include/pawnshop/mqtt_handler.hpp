#pragma once

#include <mqtt/async_client.h>
#include <mqtt/callback.h>
#include <mqtt/connect_options.h>
#include <readerwriterqueue/readerwriterqueue.h>
#include <toml++/toml_table.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace pawnshop {

struct MqttConfig {
    std::string broker_url;
    std::string client_id;
    std::string username;
    std::string password;

    MqttConfig(const toml::table& table);
};

struct MqttMessage {
    std::string topic;
    nlohmann::json payload;
};

class MqttHandler : public mqtt::callback, public mqtt::iaction_listener {
public:
    typedef moodycamel::BlockingReaderWriterQueue<MqttMessage> MessageQueue;

    MqttHandler(std::shared_ptr<mqtt::async_client> mqtt,
                mqtt::connect_options mqtt_options,
                std::shared_ptr<std::atomic<bool>> interupted,
                std::shared_ptr<std::condition_variable> interupt_cv,
                std::shared_ptr<MessageQueue> in)
        : mqtt(mqtt),
          mqtt_options(std::move(mqtt_options)),
          interupted(interupted),
          interupt_cv(interupt_cv),
          in(in) {}

private:
    // MQTT QOS config defines how broker should deliver messages:
    // QOS 0 - Tries to send message once, doesn't check if it was delivered
    // QOS 1 - Ensures that message is delivered, but it may deliver it multiple
    // times QOS 2 - Ensures that message is delivered and only once
    const int QOS = 2;

    std::shared_ptr<mqtt::async_client> mqtt;
    mqtt::connect_options mqtt_options;

    std::shared_ptr<std::atomic<bool>> interupted;
    std::shared_ptr<std::condition_variable> interupt_cv;
    mutable std::mutex interupt_cv_m;

    std::shared_ptr<MessageQueue> in;

    void reconnect();
    void send();

    // iaction_listener
    void on_failure(const mqtt::token& token) override;
    void on_success(const mqtt::token& token) override;

    // callback
    void connection_lost(const std::string& cause) override;
    void connected(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
};

}  // namespace pawnshop
