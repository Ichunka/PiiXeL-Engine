function(add_windows_icon TARGET_NAME ICON_PATH)
    if(WIN32)
        if(EXISTS ${ICON_PATH})
            get_filename_component(ICON_EXT ${ICON_PATH} EXT)
            string(TOLOWER ${ICON_EXT} ICON_EXT_LOWER)

            if(ICON_EXT_LOWER STREQUAL ".ico")
                set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_icon.rc")

                file(WRITE ${RC_FILE} "IDI_ICON1 ICON DISCARDABLE \"${ICON_PATH}\"\n")

                target_sources(${TARGET_NAME} PRIVATE ${RC_FILE})

                message(STATUS "Added Windows icon to ${TARGET_NAME}: ${ICON_PATH}")
            else()
                message(WARNING "Icon file must be .ico format for Windows executables: ${ICON_PATH}")
            endif()
        else()
            message(WARNING "Icon file not found: ${ICON_PATH}")
        endif()
    endif()
endfunction()
