cmake_minimum_required(VERSION 3.23)
project(PiiXeLEngine LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cpp include/*.hpp)
if(ENGINE_SRC)
    add_library(piixel_engine STATIC ${ENGINE_SRC})
else()
    file(WRITE ${CMAKE_BINARY_DIR}/engine_dummy.cpp "void __piix_engine_dummy(){}")
    add_library(piixel_engine STATIC ${CMAKE_BINARY_DIR}/engine_dummy.cpp)
endif()

target_include_directories(piixel_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_EDITOR)
    target_link_libraries(piixel_engine PUBLIC raylib EnTT::EnTT box2d nlohmann_json::nlohmann_json rlImGui imgui)
    target_compile_definitions(piixel_engine PUBLIC BUILD_WITH_EDITOR)
else()
    target_link_libraries(piixel_engine PUBLIC raylib EnTT::EnTT box2d nlohmann_json::nlohmann_json)
endif()

if(MSVC)
    target_compile_options(piixel_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(piixel_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
