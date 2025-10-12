function(embed_assets TARGET)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets")
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    set(REGISTRY_FILE "${GENERATED_DIR}/EmbeddedAssetRegistry.generated.hpp")

    target_include_directories(${TARGET} PRIVATE ${GENERATED_DIR})

    set(ASSET_ARGS "")
    set(ASSET_DEPENDS "")

    math(EXPR ARGC_MINUS_ONE "${ARGC} - 1")
    set(ASSET_INDEX 0)

    while(ASSET_INDEX LESS ARGC_MINUS_ONE)
        math(EXPR FILE_INDEX "${ASSET_INDEX} + 1")
        math(EXPR NAME_INDEX "${ASSET_INDEX} + 2")

        set(ASSET_FILE "${ARGV${FILE_INDEX}}")
        set(ASSET_NAME "${ARGV${NAME_INDEX}}")

        get_filename_component(ASSET_FILE_ABS ${ASSET_FILE} ABSOLUTE)

        if(EXISTS ${ASSET_FILE_ABS})
            list(APPEND ASSET_ARGS "${ASSET_FILE_ABS}:${ASSET_NAME}")
            list(APPEND ASSET_DEPENDS ${ASSET_FILE_ABS})

            string(MAKE_C_IDENTIFIER ${ASSET_NAME} ASSET_IDENTIFIER)
            list(APPEND GENERATED_HEADERS "${GENERATED_DIR}/${ASSET_IDENTIFIER}.hpp")
        else()
            message(WARNING "Asset file not found: ${ASSET_FILE_ABS} (skipping)")
        endif()

        math(EXPR ASSET_INDEX "${ASSET_INDEX} + 2")
    endwhile()

    if(ASSET_ARGS)
        set(ASSET_LIST_FILE "${GENERATED_DIR}/asset_list.txt")
        string(REPLACE ";" "\n" ASSET_LIST_CONTENT "${ASSET_ARGS}")
        file(WRITE ${ASSET_LIST_FILE} "${ASSET_LIST_CONTENT}")

        add_custom_command(
            OUTPUT ${REGISTRY_FILE} ${GENERATED_HEADERS}
            COMMAND embed_asset_tool ${GENERATED_DIR} @${ASSET_LIST_FILE}
            DEPENDS embed_asset_tool ${ASSET_DEPENDS} ${ASSET_LIST_FILE}
            COMMENT "Embedding assets into C++ headers..."
            VERBATIM
        )
    else()
        file(WRITE ${REGISTRY_FILE}
"#pragma once
#include <unordered_map>
#include <string_view>
#include <span>
#include <cstddef>

namespace PiiXeL::EmbeddedAssets {

inline const std::unordered_map<std::string_view, std::span<const std::byte>> Registry = {};

}
")
    endif()

    target_sources(${TARGET} PRIVATE ${REGISTRY_FILE})
endfunction()
