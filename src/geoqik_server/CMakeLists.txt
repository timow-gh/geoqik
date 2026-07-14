set(TARGET_NAME geoqik_server)

if (MSVC)
    add_executable(${TARGET_NAME} WIN32
        source/main.cpp
        source/IpcServer.cpp
        source/CommandDispatch.cpp)
else()
    add_executable(${TARGET_NAME}
        source/main.cpp
        source/IpcServer.cpp
        source/CommandDispatch.cpp)
endif()

add_executable(geoqik::server ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PRIVATE source)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)

set_target_properties(${TARGET_NAME} PROPERTIES
    LANGUAGE CXX
    LINKER_LANGUAGE CXX
    DEBUG_POSTFIX d)

if (WIN32 AND MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

find_package(Boost REQUIRED)

target_link_libraries(${TARGET_NAME} PRIVATE
    geoqik::geoqik
    geoqik::protocol
    Boost::headers
    Threads::Threads)

# Boost.Asio on Windows needs these system libs
if (WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(${TARGET_NAME} PRIVATE ws2_32 mswsock advapi32)
endif()

add_warnings_and_compile_options(${TARGET_NAME} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")

if (${PROJECT_NAME}_INSTALL)
    install(TARGETS ${TARGET_NAME}
            EXPORT ${project_export_set_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT runtime)
endif()
