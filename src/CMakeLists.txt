file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${Pawnshop_SOURCE_DIR}/include/pawnshop/*.hpp")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")

add_library(pawnshop ${SOURCE_LIST} ${HEADER_LIST})

target_include_directories(pawnshop PUBLIC ../include)

target_link_libraries(pawnshop PRIVATE Gpiod::Gpiod atomic ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(pawnshop PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST})
