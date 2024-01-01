
function(_uimlpriv_generate_service_list_include_target SERVICE_LIST_HEADER)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen/uiml_service_list_header.h
                       COMMAND ${CMAKE_COMMAND} -E copy ${SERVICE_LIST_HEADER} ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen/uiml_service_list_header.h
                       DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen ${SERVICE_LIST_HEADER})

    add_custom_target(UimlGenerateServiceList DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen/uiml_service_list_header.h)
    target_include_directories(uiml_static PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/UimlAutogen)
    add_dependencies(uiml_static UimlGenerateServiceList)
endfunction()

function(target_link_uiml)
    set(options OPTIONAL FAST STRICT_MODE)
    set(oneValueArgs SERVICE_LIST)
    set(multiValueArgs TARGET_NAME CPU_TYPE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

    if(NOT ARG_TARGET_NAME)
        message(FATAL_ERROR "UIML: No target name specified")
    endif()

    if(NOT ARG_CPU_TYPE)
        message(FATAL_ERROR "UIML: No CPU type specified")
    endif()

    if(NOT ARG_SERVICE_LIST)
        message(FATAL_ERROR "UIML: No SERVICE_LIST header file specified")
    endif()

    # 给UIML添加上被链接目标的包含目录
    get_target_property(TARGET_INCL ${ARG_TARGET_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(uiml_static PRIVATE ${TARGET_INCL})
    target_include_directories(uiml_static PUBLIC  ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/inc/serviceapi)

    # 以及编译定义
    get_target_property(TARGET_CDEFS ${ARG_TARGET_NAME} COMPILE_DEFINITIONS)
    target_compile_options(uiml_static PUBLIC -mcpu=${ARG_CPU_TYPE})
    while(TARGET_CDEFS)
        list(POP_FRONT TARGET_CDEFS CDEF)
        target_compile_options(uiml_static PRIVATE -D${CDEF})
    endwhile()

    # 检测严格模式
    if(ARG_STRICT_MODE)
        message(STATUS "UIML: Strict Mode enabled")
        target_compile_options(uiml_static PRIVATE "-DUIML_STRICT_MODE")
        target_compile_options(${ARG_TARGET_NAME} PRIVATE "-DUIML_STRICT_MODE")
    endif()

    # 把UIML链接到目标上面
    target_link_libraries(${ARG_TARGET_NAME} PRIVATE uiml_static)

    # 对目标链接AHRS
    # target_link_libraries(${ARG_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/lib/AHRS.lib)

    # 运行复制服务列表头文件的Codegen
    _uimlpriv_generate_service_list_include_target(${ARG_SERVICE_LIST})

    # BSP选项
    if(UIML_ENABLE_BSP_CAN)
    target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_CAN")
    endif()
    if(UIML_ENABLE_BSP_UART)
        target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_UART")
    endif()
    if(UIML_ENABLE_BSP_SPI)
        target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_SPI")
    endif()
    if(UIML_ENABLE_BSP_TIM)
        target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_TIM")
    endif()
    if(UIML_ENABLE_BSP_EXTI)
        target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_EXTI")
    endif()
    if(UIML_ENABLE_BSP_GPIO)
        target_compile_definitions(uiml_static PUBLIC "-DUIML_ENABLE_BSP_GPIO")
    endif()
endfunction()
