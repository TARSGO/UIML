
function(target_link_uiml)
    set(options OPTIONAL FAST)
    set(oneValueArgs STRICT_MODE)
    set(multiValueArgs TARGET_NAME CPU_TYPE)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

    if(NOT ARG_TARGET_NAME)
        message(FATAL_ERROR "UIML: No target name specified")
    endif()

    if(NOT ARG_CPU_TYPE)
        message(FATAL_ERROR "UIML: No CPU type specified")
    endif()

    # Append include directories of the target to UIML
    get_target_property(TARGET_INCL ${ARG_TARGET_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(uiml_static PRIVATE ${TARGET_INCL})
    target_include_directories(uiml_static PUBLIC  ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/inc/serviceapi)

    # And its compile definitions
    get_target_property(TARGET_CDEFS ${ARG_TARGET_NAME} COMPILE_DEFINITIONS)
    target_compile_options(uiml_static PUBLIC -mcpu=${ARG_CPU_TYPE})
    while(TARGET_CDEFS)
        list(POP_FRONT TARGET_CDEFS CDEF)
        target_compile_options(uiml_static PRIVATE -D${CDEF})
    endwhile()

    # Check UIML "Strict Mode"
    if(ARG_STRICT_MODE)
        message(STATUS "UIML: Strict Mode enabled")
        target_compile_options(uiml_static PRIVATE "-DUIML_STRICT_MODE")
        target_compile_options(${ARG_TARGET_NAME} PRIVATE "-DUIML_STRICT_MODE")
    endif()

    # Link it
    target_link_libraries(${ARG_TARGET_NAME} PRIVATE uiml_static)
endfunction()
