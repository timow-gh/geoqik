cmake_minimum_required(VERSION 3.28) # CMakePresets are supported since version 3.21

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(detect_ninja)

# No C++20/23 modules are used; disable scanning to avoid clang-scan-deps
# lookup during compiler detection (project()) and the GCC + CMake 3.28
# incompatibility where -MD and -fdeps-format=p1689r5 are passed together.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

project(geoqik
        VERSION 0.3.0
        LANGUAGES CXX)

include(warnings)
include(static_analysis)

include(cmake/CPM.cmake)
if (NOT CPM_SOURCE_CACHE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CPMSourceVariable.cmake")
    include(CPMSourceVariable.cmake)
endif ()

if (NOT CPM_SOURCE_CACHE)
    message(AUTHOR_WARNING "CPM_SOURCE_CACHE not set. The source files are downloaded for every build. This is not recommended.
        Set the CPM_SOURCE_CACHE variable to a directory of your choice to cache the cloned git repositories.
        Set the CPM_SOURCE_CACHE variable in CPMSourceVariable.cmake to keep your paths private.")
endif ()

include(project_preamble)
project_preamble()
include(GNUInstallDirs)

if ((CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR))
    set(IS_TOP_LEVEL_PROJECT TRUE)
else ()
    set(IS_TOP_LEVEL_PROJECT FALSE)
endif ()

if (IS_TOP_LEVEL_PROJECT)
    option(${PROJECT_NAME}_USE_SCCACHE "Use sccache" ON)
    option(${PROJECT_NAME}_USE_WARNINGS "Compile using warnings." ON)
    option(${PROJECT_NAME}_WARNINGS_AS_ERRORS "Treat compiler warnings as errors " ON)
    option(${PROJECT_NAME}_STATIC_ANALYSIS "" ON)
    option(${PROJECT_NAME}_USE_CPPCHECK "" OFF)
    option(${PROJECT_NAME}_TESTS "" ON)
    option(${PROJECT_NAME}_EXAMPLES "" ON)
    option(${PROJECT_NAME}_PACK "Create release packages" ON)
else ()
    option(${PROJECT_NAME}_USE_SCCACHE "Use sccache" OFF)
    option(${PROJECT_NAME}_USE_WARNINGS "Compile using warnings." OFF)
    option(${PROJECT_NAME}_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
    option(${PROJECT_NAME}_STATIC_ANALYSIS "" OFF)
    option(${PROJECT_NAME}_USE_CPPCHECK "" OFF)
    option(${PROJECT_NAME}_TESTS "" OFF)
    option(${PROJECT_NAME}_EXAMPLES "" OFF)
    option(${PROJECT_NAME}_PACK "Create release packages" OFF)
endif ()
# It is possible to exclude this project from installation by setting the following option to OFF.
# This is useful if you want to use this project as a subproject in another project.
option(${PROJECT_NAME}_INSTALL "Install project targets" ON)
option(${PROJECT_NAME}_BUILD_DOCUMENTATION "Build documentation" OFF)

if (${PROJECT_NAME}_USE_SCCACHE)
    include(setup_sccache)
    enable_sccache()
endif ()

if (${PROJECT_NAME}_USE_CPPCHECK)
    include(cppcheck)
    create_cppcheck_target(${TARGET_NAME})
endif ()

# Internal libs are static, use a common runtime for all libs when compiling for windows 
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(dependencies)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(stduuid CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(boost_process CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(src)

if (${PROJECT_NAME}_EXAMPLES)
    add_subdirectory(examples)
endif()

if (${PROJECT_NAME}_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)

    # Include coverage report generation if coverage flags are enabled
    if(CMAKE_CXX_FLAGS MATCHES "--coverage")
        message(STATUS "Coverage enabled - adding coverage-report target")
        include(${PROJECT_SOURCE_DIR}/cmake/coverage.cmake)
    endif()
endif ()

if (${PROJECT_NAME}_INSTALL)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/License"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}"
            COMPONENT runtime)

    if (${PROJECT_NAME}_PACK)
        include(packaging)
    endif ()
endif()
