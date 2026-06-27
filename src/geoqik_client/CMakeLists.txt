add_library(geoqik_client INTERFACE)
add_library(geoqik::client ALIAS geoqik_client)

target_include_directories(geoqik_client INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(geoqik_client INTERFACE cxx_std_23)
target_compile_definitions(geoqik_client INTERFACE BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

find_package(Boost REQUIRED)
find_package(boost_process CONFIG REQUIRED)

target_link_libraries(geoqik_client INTERFACE
    geoqik::api_types
    geoqik::protocol
    Boost::process)

if (WIN32)
    target_compile_definitions(geoqik_client INTERFACE _WIN32_WINNT=0x0601)
    target_link_libraries(geoqik_client INTERFACE ws2_32 mswsock)
endif()

if (${PROJECT_NAME}_INSTALL)
    install(DIRECTORY "include/GeoQikClient"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT dev)

    install(TARGETS geoqik_client
            EXPORT ${project_export_set_name}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
