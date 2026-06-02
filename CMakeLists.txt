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
    src/core/hyprland_facade.cpp
    src/core/hook_registry.cpp
    src/core/plugin.cpp
    src/core/strings.cpp
    src/config/config.cpp
    src/config/env.cpp
    src/modals/confirmation.cpp
    src/modals/naming.cpp
    src/modals/naming_render.cpp
    src/modals/shortcut_catalog.cpp
    src/modals/shortcuts.cpp
    src/modals/shortcuts_menu.cpp
    src/modals/shortcuts_render.cpp
    src/modals/workspace_filter.cpp
    src/modals/workspace_filter_match.cpp
    src/modals/workspace_filter_render.cpp
    src/overview/input.cpp
    src/overview/overview.cpp
    src/overview/overview_controller.cpp
    src/overview/overview_interaction.cpp
    src/overlays/overlays.cpp
    src/render/animations.cpp
    src/render/render_services.cpp
    src/render/rendering.cpp
    src/render/workspace_preview_renderer.cpp
    src/text/textinput.cpp
    src/text/textinput_render.cpp
    src/text/textinput_repeat.cpp
    src/workspace/layout.cpp
    src/workspace/navigation.cpp
    src/workspace/selection.cpp
    src/workspace/workspaces.cpp
)

add_library(hyprdeck SHARED ${HYPRDECK_SOURCES})

target_include_directories(hyprdeck PRIVATE
    ${HYPRLAND_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modals
    ${CMAKE_CURRENT_SOURCE_DIR}/src/overview
    ${CMAKE_CURRENT_SOURCE_DIR}/src/overlays
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render
    ${CMAKE_CURRENT_SOURCE_DIR}/src/text
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
)
target_link_directories(hyprdeck PRIVATE ${HYPRLAND_LIBRARY_DIRS})
target_link_libraries(hyprdeck PRIVATE ${HYPRLAND_LIBRARIES})
target_compile_options(hyprdeck PRIVATE ${HYPRLAND_CFLAGS_OTHER} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)

set_target_properties(hyprdeck PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hyprdeck"
)
