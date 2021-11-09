# Modified FindSQLite3.cmake provided with CMake

find_path(Gpiod_INCLUDE_DIR NAMES gpiod.hpp)
mark_as_advanced(Gpiod_INCLUDE_DIR)

find_library(Gpiod_LIBRARY NAMES gpiodcxx)
mark_as_advanced(Gpiod_LIBRARY)

find_package(PackageHandleStandardArgs REQUIRED)
find_package_handle_standard_args(Gpiod
    REQUIRED_VARS Gpiod_INCLUDE_DIR Gpiod_LIBRARY
)

if(Gpiod_FOUND)
    set(Gpiod_INCLUDE_DIRS ${Gpiod_INCLUDE_DIR})
    set(Gpiod_LIBRARIES ${Gpiod_LIBRARY})
    if(NOT TARGET Gpiod::Gpiod)
        add_library(Gpiod::Gpiod UNKNOWN IMPORTED)
        set_target_properties(Gpiod::Gpiod PROPERTIES
            IMPORTED_LOCATION             "${Gpiod_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Gpiod_INCLUDE_DIRS}"
        )
    endif()
endif()