cmake_minimum_required(VERSION 3.23)
project(MyFirstGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_SOURCE_DIR}/cmake/AddWindowsIcon.cmake)

file(GLOB_RECURSE GAME_SCRIPTS src/GameScripts.cpp)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/game.config.json)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/game.config.json GAME_CONFIG_JSON)
    string(JSON GAME_ICON_PATH ERROR_VARIABLE JSON_ERROR GET ${GAME_CONFIG_JSON} window icon)
    if(NOT JSON_ERROR AND GAME_ICON_PATH)
        set(GAME_ICON_FULL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${GAME_ICON_PATH}")
    endif()
endif()

if(BUILD_EDITOR)
    add_executable(editor ${CMAKE_SOURCE_DIR}/engine/src/Editor/editor_main.cpp ${GAME_SCRIPTS})
    target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(editor PRIVATE piixel_engine)

    add_custom_command(TARGET editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/assets
        ${CMAKE_BINARY_DIR}/engine/assets
    )
else()
    add_executable(game src/game_main.cpp ${GAME_SCRIPTS})
    target_include_directories(game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(game PRIVATE piixel_engine)

    if(GAME_ICON_FULL_PATH)
        add_windows_icon(game ${GAME_ICON_FULL_PATH})
    endif()

    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/assets
        ${CMAKE_BINARY_DIR}/engine/assets
    )
endif()
