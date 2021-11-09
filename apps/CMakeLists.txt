add_executable(controller controller.cpp)

set_target_properties(controller PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_link_libraries(controller
    PRIVATE
    Gpiod::Gpiod
    PahoMqttCpp::paho-mqttpp3-static
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    pawnshop)
