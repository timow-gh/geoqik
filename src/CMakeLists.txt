set(project_export_set_name "${PROJECT_NAME}TargetsExportSet")

add_subdirectory(core)
add_subdirectory(renderer)
add_subdirectory(geoqik)

if (${PROJECT_NAME}_INSTALL)
    include(setup_export_set_installation)
    setup_export_set_installation(
            ${PROJECT_NAME}
            ${project_export_set_name}
            Config.cmake.in
            ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH}
    )
endif ()