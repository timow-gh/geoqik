add_library(geoqik_api_types INTERFACE)
add_library(geoqik::api_types ALIAS geoqik_api_types)

target_include_directories(geoqik_api_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

if (${PROJECT_NAME}_INSTALL)
    install(DIRECTORY "include/GeoQik"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT dev)

    install(TARGETS geoqik_api_types
            EXPORT ${project_export_set_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
