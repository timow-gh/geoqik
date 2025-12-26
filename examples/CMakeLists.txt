macro(create_example_exe name)
    if (MSVC)
        add_executable(${name} WIN32 ${name}.cpp)
    else()
        add_executable(${name} ${name}.cpp)
    endif()
    target_link_libraries(${name} PRIVATE geoqik::geoqik)
    set_target_properties(${name}
            PROPERTIES
            LANGUAGE CXX
            LINKER_LANGUAGE CXX
            DEBUG_POSTFIX d)

    if(WIN32 AND MSVC)
        set_target_properties(${name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
    endif()

    add_warnings_and_compile_options(${name} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
    if (${PROJECT_NAME}_STATIC_ANALYSIS)
        enable_static_analysis(${name} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
    endif ()
endmacro()

create_example_exe(example_spiral)
add_custom_target(run-example-spiral
    COMMAND $<TARGET_FILE:example_spiral> ${ARGS}
    DEPENDS example_spiral
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running example_spiral"
    VERBATIM
)

create_example_exe(example_grid)

create_example_exe(example_removing_geometry)
target_link_libraries(example_removing_geometry PRIVATE linal::linal)
add_custom_target(run-example-removing-geometry
    COMMAND $<TARGET_FILE:example_removing_geometry> ${ARGS}
    DEPENDS example_removing_geometry
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running example_removing-geometry"
    VERBATIM
)

create_example_exe(example_curve)
target_link_libraries(example_curve PRIVATE Geometry::Geometry linal::linal)
add_custom_target(run-example-curve
    COMMAND $<TARGET_FILE:example_curve> ${ARGS}
    DEPENDS example_curve
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running example_curve"
    VERBATIM
)

create_example_exe(example_sphere)
target_link_libraries(example_sphere PRIVATE linal::linal)
add_custom_target(run-example-sphere
    COMMAND $<TARGET_FILE:example_sphere> ${ARGS}
    DEPENDS example_sphere
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running example_sphere"
    VERBATIM
)