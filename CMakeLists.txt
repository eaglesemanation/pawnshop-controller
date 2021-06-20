cmake_minimum_required(VERSION 3.16)

project(
	Pawnshop
	VERSION 0.1
	DESCRIPTION "Controlls rails and scales in automated pawnshop"
	LANGUAGES CXX)

# True if this project is main and not included with add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	# Use -std=c++xx instead of -std=g++xx
	set(CMAKE_CXX_EXTENSIONS OFF)
	# Provides IDE's with information about folders
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

# TODO: Fix C library compilation upstream
set(PAHO_MQTT_C_LIBRARIES "/usr/local/lib/libpaho-mqtt3a.so")
set(PAHO_MQTT_C_INCLUDE_DIRS "/usr/local/include")
find_package(PahoMqttCpp REQUIRED)

add_subdirectory(src)
add_subdirectory(apps)