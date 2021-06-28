include_guard()

function (compile_shader _SHADERS IN_PATH OUT_PATH)  # SHADER_0, SHADER_1 ...
    add_custom_command(
            OUTPUT ${OUT_PATH}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${IN_PATH} -o ${OUT_PATH}
            MAIN_DEPENDENCY ${IN_PATH}
            WORKING_DIRECTORY ${VREN_HOME}
            COMMENT "Compiling shader ${IN_PATH} -> ${OUT_PATH}"
    )

    set(SUPER_VAR ${${_SHADERS}})
    list(APPEND SUPER_VAR ${OUT_PATH})
    set(${_SHADERS} ${SUPER_VAR} PARENT_SCOPE)
endfunction()

function (setup_resources TARGET_NAME)
    set(RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/.vren/resources")
    add_custom_target(create_dirs
            COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_DIR}
            COMMENT "Creating resources directory: ${RESOURCES_DIR}"
    )

    # Shaders
    set(SHADERS "")

    compile_shader(SHADERS "${VREN_HOME}/resources/simple_draw.vert" "${RESOURCES_DIR}/simple_draw.vert.bin")
    compile_shader(SHADERS "${VREN_HOME}/resources/simple_draw.frag" "${RESOURCES_DIR}/simple_draw.frag.bin")

    add_custom_target(${TARGET_NAME}_shaders DEPENDS ${SHADERS})

    #
    add_dependencies(${TARGET_NAME} create_dirs ${TARGET_NAME}_shaders)

endfunction()
