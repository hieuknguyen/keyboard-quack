cmake_minimum_required(VERSION 3.16)

set(_quack_version_header "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h")
file(STRINGS "${_quack_version_header}" _quack_version_line
     REGEX "^#define QUACK_VERSION [0-9]+\\.[0-9]+\\.[0-9]+$")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+$" _quack_version
       "${_quack_version_line}")
if(_quack_version STREQUAL "")
    message(FATAL_ERROR "Could not read QUACK_VERSION from src/version.h")
endif()
project(keyboard-quack VERSION "${_quack_version}" LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets QUIET)
endif()

set(ENGINE_SOURCES
    src/engine/telex.c
)

set(CONFIG_SOURCES
    src/config/config.c
    src/platform/platform.c
)

if(WIN32)
    set(CAPTURE_SOURCES
        src/capture/win32_capture.c
        src/capture/win32_uia.c
        src/windows/win_update.c
    )
    set(INJECT_SOURCES
        src/inject/win32_inject.c
    )
else()
    set(CAPTURE_SOURCES
        src/capture/evdev_capture.c
    )
    set(INJECT_SOURCES
        src/inject/uinput_inject.c
    )
endif()

add_library(telex_engine STATIC ${ENGINE_SOURCES})
target_include_directories(telex_engine PUBLIC src/engine)

add_library(capture STATIC ${CAPTURE_SOURCES})
target_include_directories(capture PUBLIC src/capture src/engine src/inject src/platform)

add_library(inject STATIC ${INJECT_SOURCES})
target_include_directories(inject PUBLIC src/inject)

add_library(config STATIC ${CONFIG_SOURCES})
target_include_directories(config PUBLIC src/config src/platform)

if(WIN32)
    add_executable(quack WIN32
        src/main.c
    )
    target_link_libraries(quack PRIVATE telex_engine capture inject config user32 shell32 advapi32 uiautomationcore ole32 oleaut32 uuid winhttp bcrypt)
else()
    add_executable(quack
        src/main.c
    )
    target_link_libraries(quack PRIVATE telex_engine capture inject config m dl)
endif()

if(TARGET Qt6::Widgets OR TARGET Qt5::Widgets)
    add_executable(quack-config
        src/ui/main_window.cpp
    )
    if(TARGET Qt6::Widgets)
        target_link_libraries(quack-config PRIVATE Qt6::Widgets config)
    else()
        target_link_libraries(quack-config PRIVATE Qt5::Widgets config)
    endif()

    install(TARGETS quack-config RUNTIME DESTINATION bin)
endif()

install(TARGETS quack
    RUNTIME DESTINATION bin
)

if(UNIX)
    install(FILES config/default.toml
        DESTINATION etc/keyboard-quack
    )
endif()
