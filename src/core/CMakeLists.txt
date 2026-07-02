set(core_HEADERS
        include/Core/Assert.hpp
        include/Core/Compiler.hpp
        include/Core/DLLWarnings.hpp
        include/Core/FmtIncludeHelper.hpp
        include/Core/Handle.hpp
        include/Core/ScopedAction.hpp
        include/Core/UUID.hpp
        include/Core/Warnings.hpp
)

set(core_SOURCES
        source/Assert.cpp
)

set(TARGET_NAME geoqik_core)
add_library(${TARGET_NAME} STATIC ${core_SOURCES} ${core_HEADERS})
add_library(${PROJECT_NAME}::core ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_compile_definitions(${TARGET_NAME} PUBLIC CORE_STATIC_DEFINE)

target_link_libraries(${TARGET_NAME}
        PUBLIC
        stduuid
        fmt::fmt
)

include(GenerateExportHeader)
set(EXPORT_HEADER_BASE_NAME "core")
generate_export_header(${TARGET_NAME} BASE_NAME ${EXPORT_HEADER_BASE_NAME})
string(TOLOWER "${EXPORT_HEADER_BASE_NAME}_export.h" GENERATED_TARGET_EXPORTS_FILE_NAME)
file(COPY
        ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME}
        DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/Core)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME})

set(TARGET_EXPORT_NAME ${TARGET_NAME})
set(${TARGET_NAME}_MAJOR 0)
set(${TARGET_NAME}_MINOR 1)
set(${TARGET_NAME}_PATCH 0)
message(STATUS "Configuring ${TARGET_NAME} ${${TARGET_NAME}_MAJOR}.${${TARGET_NAME}_MINOR}.${${TARGET_NAME}_PATCH}")

set_target_properties(${TARGET_NAME}
        PROPERTIES
        LANGUAGE CXX
        POSITION_INDEPENDENT_CODE ON
        DEBUG_POSTFIX d
        )

add_warnings_and_compile_options(${TARGET_NAME} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
if (${PROJECT_NAME}_STATIC_ANALYSIS)
    enable_static_analysis(${TARGET_NAME} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
endif ()








