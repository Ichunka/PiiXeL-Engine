cmake_minimum_required(VERSION 3.23)
project(MyFirstGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE GAME_SCRIPTS src/GameScripts.cpp)

if(BUILD_EDITOR)
    add_executable(editor ${CMAKE_SOURCE_DIR}/engine/src/Editor/editor_main.cpp ${GAME_SCRIPTS})
    target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(editor PRIVATE piixel_engine)
else()
    add_executable(game src/game_main.cpp ${GAME_SCRIPTS})
    target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(game PRIVATE piixel_engine)
endif()
