add_library(geoqik_protocol INTERFACE) add_library(geoqik::protocol ALIAS geoqik_protocol)

    target_include_directories(geoqik_protocol INTERFACE $<BUILD_INTERFACE : ${CMAKE_CURRENT_SOURCE_DIR} / include>
                                   $<INSTALL_INTERFACE : include>)

        target_compile_features(geoqik_protocol INTERFACE cxx_std_23)

            if (${PROJECT_NAME} _INSTALL)
                install(DIRECTORY "include/GeoQikProtocol" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" COMPONENT dev)

                    install(TARGETS geoqik_protocol EXPORT ${project_export_set_name} INCLUDES DESTINATION ${
                        CMAKE_INSTALL_INCLUDEDIR}) endif()
