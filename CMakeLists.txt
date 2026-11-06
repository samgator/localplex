# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.22)

project(localplex
    VERSION 0.1.0
    DESCRIPTION "Fast, lightweight desktop media library and video player"
    HOMEPAGE_URL "https://github.com/samgator/localplex"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Qml
    QuickControls2
    OpenGL
    Sql
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

find_package(SQLite3 REQUIRED)

set(LOCALPLEX_SOURCES
    src/main.cpp
    src/player/mpv_player_item.h
    src/player/mpv_player_item.cpp
    resources.qrc
)

add_executable(localplex ${LOCALPLEX_SOURCES})

target_compile_features(localplex PRIVATE cxx_std_20)

target_compile_options(localplex PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_include_directories(localplex PRIVATE
    src
)

target_link_libraries(localplex PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::OpenGL
    Qt6::Sql
    PkgConfig::MPV
    SQLite3::SQLite3
)

set_target_properties(localplex PROPERTIES
    OUTPUT_NAME "localplex"
)

# Testing configuration
enable_testing()
add_test(NAME ColdStartBenchmark
    COMMAND ${CMAKE_COMMAND} -E env LOCALPLEX_BENCH_EXIT=1 $<TARGET_FILE:localplex> --benchmark-mode
)
