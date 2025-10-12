function(embed_asset TARGET ASSET_FILE ASSET_NAME)
    get_filename_component(ASSET_FILE_ABS ${ASSET_FILE} ABSOLUTE)

    if(NOT EXISTS ${ASSET_FILE_ABS})
        message(FATAL_ERROR "Asset file not found: ${ASSET_FILE_ABS}")
    endif()

    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets")
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    string(MAKE_C_IDENTIFIER ${ASSET_NAME} ASSET_IDENTIFIER)
    set(OUTPUT_FILE "${GENERATED_DIR}/${ASSET_IDENTIFIER}.hpp")

    file(READ ${ASSET_FILE_ABS} HEX_CONTENT HEX)
    string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
    math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")

    set(BYTE_ARRAY "")
    string(REGEX MATCHALL ".." HEX_BYTES "${HEX_CONTENT}")
    set(COUNTER 0)
    foreach(HEX_BYTE ${HEX_BYTES})
        if(COUNTER GREATER 0)
            string(APPEND BYTE_ARRAY ", ")
        endif()
        if(COUNTER GREATER 0)
            math(EXPR MOD_RESULT "${COUNTER} % 16")
            if(MOD_RESULT EQUAL 0)
                string(APPEND BYTE_ARRAY "\n    ")
            endif()
        endif()
        string(APPEND BYTE_ARRAY "std::byte{0x${HEX_BYTE}}")
        math(EXPR COUNTER "${COUNTER} + 1")
    endforeach()

    file(WRITE ${OUTPUT_FILE}
"#pragma once
#include <cstddef>
#include <array>
#include <string_view>

namespace PiiXeL::EmbeddedAssets {

constexpr std::string_view k_${ASSET_IDENTIFIER}_Name = \"${ASSET_NAME}\";
constexpr std::size_t k_${ASSET_IDENTIFIER}_Size = ${BYTE_COUNT};

inline constexpr std::array<std::byte, ${BYTE_COUNT}> k_${ASSET_IDENTIFIER}_Data = {
    ${BYTE_ARRAY}
};

}
")

    target_sources(${TARGET} PRIVATE ${OUTPUT_FILE})
    target_include_directories(${TARGET} PRIVATE ${GENERATED_DIR})
endfunction()

function(embed_assets TARGET)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded_assets")
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    set(REGISTRY_FILE "${GENERATED_DIR}/EmbeddedAssetRegistry.generated.hpp")

    set(INCLUDES "")
    set(REGISTRY_ENTRIES "")

    math(EXPR ARGC_MINUS_ONE "${ARGC} - 1")
    set(ASSET_INDEX 0)

    while(ASSET_INDEX LESS ARGC_MINUS_ONE)
        math(EXPR FILE_INDEX "${ASSET_INDEX} + 1")
        math(EXPR NAME_INDEX "${ASSET_INDEX} + 2")

        set(ASSET_FILE "${ARGV${FILE_INDEX}}")
        set(ASSET_NAME "${ARGV${NAME_INDEX}}")

        embed_asset(${TARGET} ${ASSET_FILE} ${ASSET_NAME})

        string(MAKE_C_IDENTIFIER ${ASSET_NAME} ASSET_IDENTIFIER)
        string(APPEND INCLUDES "#include \"${ASSET_IDENTIFIER}.hpp\"\n")

        if(ASSET_INDEX GREATER 0)
            string(APPEND REGISTRY_ENTRIES ",\n        ")
        endif()
        string(APPEND REGISTRY_ENTRIES "{ k_${ASSET_IDENTIFIER}_Name, { k_${ASSET_IDENTIFIER}_Data.data(), k_${ASSET_IDENTIFIER}_Size } }")

        math(EXPR ASSET_INDEX "${ASSET_INDEX} + 2")
    endwhile()

    file(WRITE ${REGISTRY_FILE}
"#pragma once
${INCLUDES}
#include <unordered_map>
#include <string_view>
#include <span>
#include <cstddef>

namespace PiiXeL::EmbeddedAssets {

inline const std::unordered_map<std::string_view, std::span<const std::byte>> Registry = {
        ${REGISTRY_ENTRIES}
};

}
")

    target_sources(${TARGET} PRIVATE ${REGISTRY_FILE})
endfunction()
