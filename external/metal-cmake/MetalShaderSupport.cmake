function(add_metal_shader_library TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 _amsl
            ""
            "STANDARD"
            "")

    add_library(${TARGET} MODULE ${_amsl_UNPARSED_ARGUMENTS})

    set_target_properties(${TARGET} PROPERTIES
            DEBUG_POSTFIX ""
            XCODE_PRODUCT_TYPE com.apple.product-type.metal-library
            XCODE_ATTRIBUTE_MTL_FAST_MATH "YES"
            XCODE_ATTRIBUTE_MTL_ENABLE_DEBUG_INFO[variant=Debug] "INCLUDE_SOURCE"
            XCODE_ATTRIBUTE_MTL_ENABLE_DEBUG_INFO[variant=RelWithDebInfo] "INCLUDE_SOURCE"
            XCODE_ATTRIBUTE_MTL_HEADER_SEARCH_PATHS "$(HEADER_SEARCH_PATHS)"
            LANGUAGE Metal
            LINKER_LANGUAGE Metal)

    if(_amsl_STANDARD AND _amsl_STANDARD MATCHES "metal([0-9]+)\.([0-9]+)")
        target_compile_options(${TARGET}
                PRIVATE "-std=${_amsl_STANDARD}")

        set_target_properties(${TARGET} PROPERTIES
                XCODE_ATTRIBUTE_MTL_LANGUAGE_REVISION "Metal${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
    endif()
endfunction()

function(target_embed_metal_shader_libraries TARGET)
    foreach(SHADERLIB IN LISTS ARGN)
        add_dependencies(${TARGET} ${SHADERLIB})
    endforeach()

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.28 AND ${CMAKE_GENERATOR} STREQUAL "Xcode")
        set_target_properties(${TARGET} PROPERTIES
                XCODE_EMBED_RESOURCES "${ARGN}")
    else()
        foreach(SHADERLIB IN LISTS ARGN)
            add_custom_command(TARGET ${SHADERLIB} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${TARGET}/Resources"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${SHADERLIB}>" "$<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/Resources/$<TARGET_FILE_NAME:${SHADERLIB}>"
                    VERBATIM)
        endforeach()
    endif()
endfunction()

function(target_copy_metal_shader_libraries TARGET)
    foreach (SHADERLIB IN LISTS ARGN)
        add_dependencies(${TARGET} ${SHADERLIB})

        add_custom_command(TARGET ${SHADERLIB} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${SHADERLIB}>" "$<TARGET_FILE_DIR:${TARGET}>"
                VERBATIM)
    endforeach ()
endfunction()