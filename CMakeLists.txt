cmake_minimum_required(VERSION 3.16)

# Use vcpkg for libraries
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(
	Pawnshop
	VERSION 0.1.0
	DESCRIPTION "Controlls rails and scales in automated pawnshop"
	LANGUAGES CXX)

# True if this project is main and not included with add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	# Provides IDE's with information about folders
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

	find_package(Doxygen)
	if(Doxygen_FOUND)
		add_subdirectory(docs)
	else()
		message(STATUS "Doxygen not found, docs build skipped")
	endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Threads REQUIRED)
find_package(Gpiod REQUIRED)
find_package(PahoMqttCpp REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(src)
add_subdirectory(apps)
