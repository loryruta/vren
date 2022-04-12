include_guard()

function (compile_shader _SHADERS IN_PATH OUT_PATH)  # SHADER_0, SHADER_1 ...
    add_custom_command(
            OUTPUT ${OUT_PATH}
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

function (setup_resources TARGET_NAME)
    set(VREN_SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/.vren/resources/shaders")

    add_custom_target(create_vren_dirs  COMMAND ${CMAKE_COMMAND} -E make_directory ${VREN_SHADERS_DIR})

    # Shaders
    set(SHADERS "")

    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/dbg_draw.vert.glsl" "${VREN_SHADERS_DIR}/dbg_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/dbg_draw.frag.glsl" "${VREN_SHADERS_DIR}/dbg_draw.frag.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/pbr_draw.vert.glsl" "${VREN_SHADERS_DIR}/pbr_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/pbr_draw.frag.glsl" "${VREN_SHADERS_DIR}/pbr_draw.frag.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.task.glsl" "${VREN_SHADERS_DIR}/draw.task.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.mesh.glsl" "${VREN_SHADERS_DIR}/draw.mesh.spv")

    add_custom_target(vren_shaders DEPENDS ${SHADERS})

    add_dependencies(vren_shaders create_vren_dirs)
    add_dependencies(${TARGET_NAME} vren_shaders)
endfunction()
