include_guard()

function (compile_shader _SHADERS IN_PATH OUT_PATH)
    add_custom_command(
            OUTPUT
                ${OUT_PATH}
                ${OUT_PATH}__enforce_run # *__enforce_run is a fake output file that won't be created and is here to ensure the command is always run
            COMMAND glslangValidator --target-env vulkan1.2 -g -o ${OUT_PATH} ${IN_PATH}  # TODO SPECIFY glslangValidator FULL PATH
            MAIN_DEPENDENCY ${IN_PATH}
            WORKING_DIRECTORY ${VREN_HOME}
            COMMENT "glslangValidator --target-env vulkan1.2 -g -o ${OUT_PATH} ${IN_PATH}"
    )
    set(SUPER_VAR ${${_SHADERS}})
    list(APPEND SUPER_VAR ${OUT_PATH})
    set(${_SHADERS} ${SUPER_VAR} PARENT_SCOPE)
endfunction()

function (copy_resources TARGET_NAME FROM TO)
    set(FROM "${CMAKE_CURRENT_SOURCE_DIR}/${FROM}")
    set(TO "${CMAKE_CURRENT_BINARY_DIR}/${TO}")
    add_custom_command(
            OUTPUT ${TO}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${FROM} ${TO}
            MAIN_DEPENDENCY ${FROM}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Copying resources from `${FROM}` to: `${TO}`"
    )
    target_sources(${TARGET_NAME} PRIVATE ${TO})
endfunction()

function (setup_resources TARGET)
    # Shaders
    set(VREN_SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/.vren/resources/shaders")

    add_custom_target(
            vren_create_shaders_dir
            COMMAND ${CMAKE_COMMAND} -E make_directory ${VREN_SHADERS_DIR}
            # TODO WORKING_DIR
            COMMENT "Creating directory: ${VREN_SHADERS_DIR}"
    )

    set(SHADERS "")

    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/depth_buffer_copy.comp.glsl" "${VREN_SHADERS_DIR}/depth_buffer_copy.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/depth_buffer_reduce.comp.glsl" "${VREN_SHADERS_DIR}/depth_buffer_reduce.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/basic_draw.vert.glsl" "${VREN_SHADERS_DIR}/basic_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/debug_draw.vert.glsl" "${VREN_SHADERS_DIR}/debug_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/debug_draw.frag.glsl" "${VREN_SHADERS_DIR}/debug_draw.frag.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.mesh.glsl" "${VREN_SHADERS_DIR}/draw.mesh.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.task.glsl" "${VREN_SHADERS_DIR}/draw.task.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/pbr_draw.frag.glsl" "${VREN_SHADERS_DIR}/pbr_draw.frag.spv")

    add_custom_target(vren_shaders DEPENDS vren_create_shaders_dir ${SHADERS})

    add_dependencies(${TARGET} vren_shaders)

endfunction()
