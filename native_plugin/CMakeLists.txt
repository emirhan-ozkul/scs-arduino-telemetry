cmake_minimum_required(VERSION 3.20)

project(ETS2TelemetryPlugin LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This plugin is for the Windows SCS game plugin interface.")
endif()

set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../scs_sdk_1_16" CACHE PATH "Original SCS SDK 1.16 root")
set(SDK_INCLUDE "${SDK_ROOT}/include")

if(NOT EXISTS "${SDK_INCLUDE}/scssdk_telemetry.h")
    message(FATAL_ERROR "SCS SDK telemetry header not found at ${SDK_INCLUDE}/scssdk_telemetry.h")
endif()

add_library(ETS2TelemetryPlugin SHARED
    src/plugin.cpp
    src/logger.cpp
)

add_library(ETS2TelemetryPluginLoggerHeader INTERFACE)

set_target_properties(ETS2TelemetryPlugin PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(ETS2TelemetryPlugin PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${SDK_INCLUDE}"
    "${SDK_INCLUDE}/common"
    "${SDK_INCLUDE}/eurotrucks2"
    "${SDK_INCLUDE}/amtrucks"
)

if(MSVC)
    target_compile_options(ETS2TelemetryPlugin PRIVATE /W4 /permissive- /EHsc)
    target_link_options(ETS2TelemetryPlugin PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/exports.def")
else()
    message(FATAL_ERROR "This CMake project requires MSVC for the Windows SCS plugin interface.")
endif()
