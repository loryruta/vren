include_guard()

find_package(Vulkan)

function (compile_shader _SHADERS IN_PATH OUT_PATH)
    if (!Vulkan_glslc_FOUND)
        message(FATAL_ERROR "Could not compile shader: glslc could not be found")
    endif()

    # Definitions
    set(OPTIONS_ ${ARGV3} ${ARGV4}) # TODO interpret arguments from 3 on as OPTIONS


    string(REPLACE ";" " " USER_OPTIONS "${ARGN}")

    add_custom_command(
            OUTPUT
                ${OUT_PATH}
                ${OUT_PATH}__enforce_run # *__enforce_run is a fake output file that won't be created and is here to ensure the command is always run
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.2 -I "${VREN_HOME}/vren/resources/shaders" ${ARGN} -g -o ${OUT_PATH} ${IN_PATH}
            MAIN_DEPENDENCY ${IN_PATH}
            WORKING_DIRECTORY ${VREN_HOME}
            COMMENT "${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.2 -I \"${VREN_HOME}/vren/resources/shaders\" ${USER_OPTIONS} -g -o ${OUT_PATH} ${IN_PATH}"
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

    #add_custom_target(
    #        vren_create_shaders_dir
    #        COMMAND ${CMAKE_COMMAND} -E make_directory ${VREN_SHADERS_DIR}
    #        # TODO WORKING_DIR
    #        COMMENT "Creating directory: ${VREN_SHADERS_DIR}"
    #)

    set(SHADERS "")

    # Pipeline
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/depth_buffer_copy.comp" "${VREN_SHADERS_DIR}/depth_buffer_copy.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/depth_buffer_reduce.comp" "${VREN_SHADERS_DIR}/depth_buffer_reduce.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/basic_draw.vert" "${VREN_SHADERS_DIR}/basic_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/debug_draw.vert" "${VREN_SHADERS_DIR}/debug_draw.vert.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/debug_draw.frag" "${VREN_SHADERS_DIR}/debug_draw.frag.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.mesh" "${VREN_SHADERS_DIR}/draw.mesh.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/draw.task" "${VREN_SHADERS_DIR}/draw.task.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/pbr_draw.frag" "${VREN_SHADERS_DIR}/pbr_draw.frag.spv")
    
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/build_bvh.comp" "${VREN_SHADERS_DIR}/build_bvh.comp.spv")

    # Reduce
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_uint_add.comp.spv" "-D_VREN_DATA_TYPE=uint" "-D_VREN_OPERATION(a,b)=a+b")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_uint_min.comp.spv" "-D_VREN_DATA_TYPE=uint" "-D_VREN_OPERATION(a,b)=min(a,b)")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_uint_max.comp.spv" "-D_VREN_DATA_TYPE=uint" "-D_VREN_OPERATION(a,b)=max(a,b)")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_vec3_add.comp.spv" "-D_VREN_DATA_TYPE=vec3" "-D_VREN_OPERATION(a,b)=a+b")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_vec3_min.comp.spv" "-D_VREN_DATA_TYPE=vec3" "-D_VREN_OPERATION(a,b)=min(a,b)")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/reduce.comp" "${VREN_SHADERS_DIR}/reduce_vec3_max.comp.spv" "-D_VREN_DATA_TYPE=vec3" "-D_VREN_OPERATION(a,b)=max(a,b)")

    # Blelloch scan
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/blelloch_scan_downsweep.comp" "${VREN_SHADERS_DIR}/blelloch_scan_downsweep.comp.spv" "-D_VREN_DOWNSWEEP_ENTRYPOINT")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/blelloch_scan_downsweep.comp" "${VREN_SHADERS_DIR}/blelloch_scan_workgroup_downsweep.comp.spv" "-D_VREN_WORKGROUP_DOWNSWEEP_ENTRYPOINT")

    # Radix sort
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/radix_sort_local_count.comp" "${VREN_SHADERS_DIR}/radix_sort_local_count.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/radix_sort_global_offset.comp" "${VREN_SHADERS_DIR}/radix_sort_global_offset.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/radix_sort_reorder.comp" "${VREN_SHADERS_DIR}/radix_sort_reorder.comp.spv")

    # Bucket sort
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/bucket_sort_count.comp" "${VREN_SHADERS_DIR}/bucket_sort_count.comp.spv")
    compile_shader(SHADERS "${VREN_HOME}/vren/resources/shaders/bucket_sort_write.comp" "${VREN_SHADERS_DIR}/bucket_sort_write.comp.spv")

    add_custom_target(vren_${TARGET}_shaders DEPENDS ${SHADERS})

    add_dependencies(${TARGET} vren_${TARGET}_shaders)

endfunction()
