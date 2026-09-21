cmake_minimum_required(VERSION 3.20)

project(system_monitoring_server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.74 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(monitoring_server
    src/main.cpp
    src/server.cpp
    src/session.cpp
    src/monitoring/metrics_generator.cpp
    src/monitoring/metrics_service.cpp
    src/monitoring/shared_state.cpp
    src/monitoring/session_handler.cpp
)

target_include_directories(monitoring_server PRIVATE include)
target_link_libraries(monitoring_server PRIVATE
    Boost::system
    Threads::Threads
    nlohmann_json::nlohmann_json
)
target_compile_definitions(monitoring_server PRIVATE BOOST_ASIO_NO_DEPRECATED)

if (MSVC)
    target_compile_options(monitoring_server PRIVATE /W4 /permissive-)
else()
    target_compile_options(monitoring_server PRIVATE -Wall -Wextra -Wpedantic)
endif()
