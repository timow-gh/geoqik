set(geoqik_HEADER_FILES
        include/GeoQik/GeoQik.hpp
)

set(geoqik_SOURCE_FILES
        source/ConcurrentQueue/ConcurrentQueue.hpp
        source/Context.hpp
        source/Context.cpp
        source/GeometryHelpers.hpp
        source/GeometryBuffers/LineBuffer.hpp
        source/GeometryBuffers/PointBuffer.hpp
        source/GeoQik.cpp
        source/GeoQikLog.cpp
        source/GeoQikLog.hpp
        source/GeoQikMessageSerialization.cpp
        source/GeoQikMessageSerialization.hpp
        source/GeoQikMessages.hpp
        source/GeoQikReplayUndo.cpp
        source/GeoQikReplayUndo.hpp
        source/GeoQikSettings.hpp
        source/IdempotencyData.hpp
        source/Rendering/GeoQikSceneRenderer.cpp
        source/Rendering/GeoQikSceneRenderer.hpp
        source/Scene.cpp
        source/Scene.hpp
)

set(TARGET_NAME geoqik)
add_library(${TARGET_NAME} SHARED ${geoqik_SOURCE_FILES} ${geoqik_HEADER_FILES})

include(GenerateExportHeader)
set(EXPORT_HEADER_BASE_NAME "geoqik")
generate_export_header(${TARGET_NAME} BASE_NAME ${EXPORT_HEADER_BASE_NAME})
string(TOLOWER "${EXPORT_HEADER_BASE_NAME}_export.h" GENERATED_TARGET_EXPORTS_FILE_NAME)
file(COPY
        ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME}
        DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/GeoQik)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME})

target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export_header_includes>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/source>
)

add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

set(${TARGET_NAME}_MAJOR 0)
set(${TARGET_NAME}_MINOR 1)
set(${TARGET_NAME}_PATCH 0)
message(STATUS "Configuring ${TARGET_NAME} ${${TARGET_NAME}_MAJOR}.${${TARGET_NAME}_MINOR}.${${TARGET_NAME}_PATCH}")

set_target_properties(${TARGET_NAME}
        PROPERTIES
        LANGUAGE CXX
        LINKER_LANGUAGE CXX
        DEBUG_POSTFIX d
        VERSION ${${TARGET_NAME}_MAJOR}.${${TARGET_NAME}_MINOR}.${${TARGET_NAME}_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR})

add_warnings_and_compile_options(${TARGET_NAME} ON)
if (${PROJECT_NAME}_STATIC_ANALYSIS)
    enable_static_analysis(${TARGET_NAME} ON)
endif ()

target_link_libraries(${TARGET_NAME}
        PRIVATE
        $<BUILD_LOCAL_INTERFACE:geoqik::core>
        $<BUILD_LOCAL_INTERFACE:renderer::renderer>
        $<BUILD_LOCAL_INTERFACE:imgui::imgui>
        $<BUILD_LOCAL_INTERFACE:fmt::fmt>
        $<BUILD_LOCAL_INTERFACE:linal::linal>
        $<BUILD_LOCAL_INTERFACE:Geometry::Geometry>
)

if (${PROJECT_NAME}_INSTALL)

    install(DIRECTORY "include/GeoQik" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT dev)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/GeoQik/${GENERATED_TARGET_EXPORTS_FILE_NAME}"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/GeoQik"
            COMPONENT dev)

        # Use a single isntall commnd. Multiple install commands can lead to issues with CMake's install script generation. It may only 
        # use the last install command.
    install(TARGETS
            ${TARGET_NAME}
            EXPORT ${project_export_set_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT runtime
            LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT runtime
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT dev
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            COMPONENT dev)
endif ()

# Only to test the library, not to be installed.
add_library(geoqik_internals INTERFACE)
# This following include dir paths need to be paths with the project as its root, since the test projects rely on them.
target_include_directories(geoqik_internals INTERFACE ${CMAKE_SOURCE_DIR}/src/geoqik/source)
target_sources(geoqik_internals INTERFACE
        ${CMAKE_SOURCE_DIR}/src/geoqik/source/Scene.cpp
        ${CMAKE_SOURCE_DIR}/src/geoqik/source/GeoQikLog.cpp
        ${CMAKE_SOURCE_DIR}/src/geoqik/source/GeoQikMessageSerialization.cpp
        ${CMAKE_SOURCE_DIR}/src/geoqik/source/GeoQikReplayUndo.cpp)
target_link_libraries(geoqik_internals INTERFACE
        stduuid linal::linal Geometry::Geometry geoqik::core renderer::renderer)
target_include_directories(geoqik_internals INTERFACE
        $<TARGET_PROPERTY:geoqik_core,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:renderer_opengl,INTERFACE_INCLUDE_DIRECTORIES>)
add_library(geoqik::geoqik_internals ALIAS geoqik_internals)
