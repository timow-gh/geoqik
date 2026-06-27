function(add_geoqik_example target)
    set(options CLIENT NO_TIDY)
    set(oneValueArgs)
    set(multiValueArgs LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (MSVC)
        add_executable(${target} WIN32 ${target}.cpp)
    else()
        add_executable(${target} ${target}.cpp)
    endif()

    target_link_libraries(${target} PRIVATE ${ARG_LIBRARIES})
    set_target_properties(${target}
        PROPERTIES
        LANGUAGE CXX
        LINKER_LANGUAGE CXX
        DEBUG_POSTFIX d)

    if (WIN32 AND MSVC)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
    endif()

    if (ARG_CLIENT)
        target_compile_features(${target} PRIVATE cxx_std_23)
        if (NOT MSVC)
            target_compile_options(${target} PRIVATE -Wno-null-dereference)
        endif()
    endif()

    add_warnings_and_compile_options(${target} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
    if (${PROJECT_NAME}_STATIC_ANALYSIS)
        enable_static_analysis(${target} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
    endif()

    if (ARG_NO_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "")
    endif()
endfunction()

function(add_geoqik_example_run_target target)
    string(REGEX REPLACE "^example_" "" run_suffix "${target}")
    string(REPLACE "_" "-" run_suffix "${run_suffix}")

    add_custom_target(run-example-${run_suffix}
        COMMAND $<TARGET_FILE:${target}> ${ARGS}
        DEPENDS ${target}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ${target}"
        VERBATIM)
endfunction()

set(geoqik_examples
    example_spiral
    example_transformations
    example_grid
    example_removing_geometry
    example_sphere)

foreach(example IN LISTS geoqik_examples)
    add_geoqik_example(${example} LIBRARIES geoqik::geoqik linal::linal)
endforeach()

add_geoqik_example(example_curve LIBRARIES geoqik::geoqik linal::linal Geometry::Geometry)
add_geoqik_example(example_mesh NO_TIDY LIBRARIES geoqik::geoqik linal::linal)
add_geoqik_example(example_renderer_standalone NO_TIDY LIBRARIES renderer::renderer linal::linal)

set(geoqik_run_examples
    example_spiral
    example_transformations
    example_removing_geometry
    example_curve
    example_sphere
    example_mesh
    example_renderer_standalone)

foreach(example IN LISTS geoqik_run_examples)
    add_geoqik_example_run_target(${example})
endforeach()

set(geoqik_client_examples
    example_client_poc
    example_client_settings
    example_client_points_lines
    example_client_mesh)

foreach(example IN LISTS geoqik_client_examples)
    add_geoqik_example(${example} CLIENT LIBRARIES geoqik::client)
endforeach()
