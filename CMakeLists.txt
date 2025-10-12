cmake_minimum_required(VERSION 3.23)
cmake_policy(SET CMP0169 OLD)
project(PiiXeL LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(BUILD_EDITOR "Build with editor (ImGui)" ON)

if(NOT DEFINED GAME_PROJECT)
    set(GAME_PROJECT "MyFirstGame" CACHE STRING "Game project to build")
endif()

message(STATUS "Building game: ${GAME_PROJECT}")

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(OPT_BUILD_DRM OFF CACHE INTERNAL "")
set(USE_EXTERNAL_GLFW OFF CACHE INTERNAL "")
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

set(ENTT_BUILD_TESTING OFF CACHE INTERNAL "")
set(ENTT_BUILD_DOCS OFF CACHE INTERNAL "")
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(entt)

set(BOX2D_BUILD_UNIT_TESTS OFF CACHE INTERNAL "")
set(BOX2D_BUILD_TESTBED OFF CACHE INTERNAL "")
set(BOX2D_BUILD_DOCS OFF CACHE INTERNAL "")
FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(box2d)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders OFF CACHE INTERNAL "")
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

if(BUILD_EDITOR)
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
        GIT_SHALLOW TRUE
    )
    FetchContent_Populate(imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

    FetchContent_Declare(rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG main
        GIT_SHALLOW TRUE
    )
    FetchContent_Populate(rlimgui)
    add_library(rlImGui STATIC
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
        ${rlimgui_SOURCE_DIR}/imgui_impl_raylib.h
        ${rlimgui_SOURCE_DIR}/rlImGui.h
    )
    target_include_directories(rlImGui PUBLIC ${rlimgui_SOURCE_DIR})
    target_link_libraries(rlImGui PUBLIC raylib imgui)
endif()

add_subdirectory(engine)

add_executable(build_package tools/BuildPackage.cpp)
target_link_libraries(build_package PRIVATE piixel_engine)

if(EXISTS ${CMAKE_SOURCE_DIR}/games/${GAME_PROJECT})
    add_subdirectory(games/${GAME_PROJECT})
else()
    message(FATAL_ERROR "Game project '${GAME_PROJECT}' not found in games/")
endif()

include(GNUInstallDirs)
install(TARGETS piixel_engine
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY engine/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
