cmake_minimum_required(VERSION 3.10)
project(keyboard-server CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(keyboard-server
    src/main.cpp
    src/ConfigManager.cpp
    src/DeviceManager.cpp
    src/HidrawManager.cpp
    src/UInputEmitter.cpp
    src/BragiK100.cpp
    src/ApiServer.cpp
)

target_include_directories(keyboard-server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(keyboard-server
    pthread
)
