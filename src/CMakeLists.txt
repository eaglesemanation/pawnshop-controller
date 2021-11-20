file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${Pawnshop_SOURCE_DIR}/include/pawnshop/*.hpp")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")
set(PRIVATE_DEPS_LIST
    atomic
    doctest::doctest
    fmt::fmt
    spdlog::spdlog
    ${CMAKE_THREAD_LIBS_INIT})

set(PUBLIC_DEPS_LIST
    Gpiod::Gpiod
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    PahoMqttCpp::paho-mqttpp3-static
    readerwriterqueue::readerwriterqueue)

add_library(pawnshop ${SOURCE_LIST} ${HEADER_LIST})

target_include_directories(pawnshop PUBLIC ../include)

target_link_libraries(pawnshop
    PRIVATE ${PRIVATE_DEPS_LIST}
    PUBLIC ${PUBLIC_DEPS_LIST})

target_compile_definitions(pawnshop PRIVATE DOCTEST_CONFIG_DISABLE)

set_target_properties(pawnshop PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST})

if(doctest_FOUND)
    add_executable(pawnshop_test tests/catch_main.cpp ${SOURCE_LIST})
    target_include_directories(pawnshop_test PUBLIC ../include)
    target_link_libraries(pawnshop_test PRIVATE ${PRIVATE_DEPS_LIST} ${PUBLIC_DEPS_LIST})
    set_target_properties(pawnshop_test PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    doctest_discover_tests(pawnshop_test
        ADD_LABELS YES)
endif()
