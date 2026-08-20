cmake_minimum_required(VERSION 3.16)
project(keyboard-quack VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
endif()

set(ENGINE_SOURCES
    src/engine/telex.c
)

set(CAPTURE_SOURCES
    src/capture/evdev_capture.c
)

set(INJECT_SOURCES
    src/inject/uinput_inject.c
)

set(CONFIG_SOURCES
    src/config/config.c
)

add_library(telex_engine STATIC ${ENGINE_SOURCES})
target_include_directories(telex_engine PUBLIC src/engine)

add_library(capture STATIC ${CAPTURE_SOURCES})
target_include_directories(capture PUBLIC src/capture)

add_library(inject STATIC ${INJECT_SOURCES})
target_include_directories(inject PUBLIC src/inject)

add_library(config STATIC ${CONFIG_SOURCES})
target_include_directories(config PUBLIC src/config)

add_executable(quack
    src/main.c
)
target_link_libraries(quack PRIVATE telex_engine capture inject config m dl)

add_executable(quack-config
    src/ui/main_window.cpp
)
if(Qt6_FOUND)
    target_link_libraries(quack-config PRIVATE Qt6::Widgets config)
else()
    target_link_libraries(quack-config PRIVATE Qt5::Widgets config)
endif()

install(TARGETS quack quack-config
    RUNTIME DESTINATION bin
)
install(FILES config/default.toml
    DESTINATION etc/keyboard-quack
)
