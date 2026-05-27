cmake_minimum_required(VERSION 3.25)

project(hyprdeck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYPRLAND REQUIRED hyprland)

set(HYPRDECK_SOURCES
    src/main.cpp
    src/confirmation.cpp
    src/config.cpp
    src/env.cpp
    src/input.cpp
    src/keyboard.cpp
    src/layout.cpp
    src/monitors.cpp
    src/naming.cpp
    src/naming_render.cpp
    src/navigation.cpp
    src/overview.cpp
    src/overview_controller.cpp
    src/overview_interaction.cpp
    src/overlays.cpp
    src/rendering.cpp
    src/selection.cpp
    src/shortcut_catalog.cpp
    src/shortcuts.cpp
    src/shortcuts_menu.cpp
    src/shortcuts_render.cpp
    src/state.cpp
    src/strings.cpp
    src/textinput.cpp
    src/textinput_render.cpp
    src/textinput_repeat.cpp
    src/ui.cpp
    src/workspace_filter.cpp
    src/workspace_filter_match.cpp
    src/workspace_filter_render.cpp
    src/workspaces.cpp
)

add_library(hyprdeck SHARED ${HYPRDECK_SOURCES})

target_include_directories(hyprdeck PRIVATE ${HYPRLAND_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_directories(hyprdeck PRIVATE ${HYPRLAND_LIBRARY_DIRS})
target_link_libraries(hyprdeck PRIVATE ${HYPRLAND_LIBRARIES})
target_compile_options(hyprdeck PRIVATE ${HYPRLAND_CFLAGS_OTHER} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)

set_target_properties(hyprdeck PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hyprdeck"
)
