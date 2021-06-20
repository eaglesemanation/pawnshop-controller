add_executable(controller controller.cpp)

target_compile_features(controller PRIVATE cxx_std_17)
target_link_libraries(controller PRIVATE Gpiod::Gpiod PahoMqttCpp::paho-mqttpp3 pawnshop)