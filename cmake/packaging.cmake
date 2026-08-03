include_guard(GLOBAL)

set(PKG_DEB_RUNTIME_NAME "lib${PROJECT_NAME}${PROJECT_VERSION_MAJOR}")
set(PKG_DEB_DEV_NAME "lib${PROJECT_NAME}-dev")

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "GeoQik")
set(CPACK_PACKAGE_CONTACT "GeoQik maintainers <39085531+timow-gh@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A lightweight C++ library for visualizing 3D geometry during debugging")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/timow-gh/geoqik")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/License")
set(CPACK_PACKAGE_CHECKSUM SHA256)

# Warn if packaging debug builds
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    message(WARNING "Creating packages from ${CMAKE_BUILD_TYPE} build. ")
endif()

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-windows-x64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-ubuntu-24.04-${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Component configuration
set(CPACK_COMPONENTS_ALL runtime dev)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "The GeoQik shared library and server executable")
set(CPACK_COMPONENT_DEV_DISPLAY_NAME "Development Files")
set(CPACK_COMPONENT_DEV_DESCRIPTION "Headers, import libraries, and CMake package files")
set(CPACK_COMPONENT_DEV_DEPENDS runtime)

# Portable archives are complete SDKs rather than separate component archives.
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

# DEB (Debian/Ubuntu) Configuration
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")

set(CPACK_DEBIAN_DEV_PACKAGE_NAME "${PKG_DEB_DEV_NAME}")
set(CPACK_DEBIAN_DEV_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "${PKG_DEB_RUNTIME_NAME}")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_RUNTIME_PACKAGE_SHLIBDEPS ON)

if(WIN32)
    set(CPACK_NSIS_PACKAGE_NAME "GeoQik ${PROJECT_VERSION}")
    set(CPACK_NSIS_DISPLAY_NAME "GeoQik ${PROJECT_VERSION}")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_HELP_LINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
endif()

# Generator Selection
# Default to TGZ if no generator specified (works everywhere)
# CMake presets can override this via CPACK_GENERATOR cache variable
if(NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR "TGZ")
endif()

# Load CPack module
include(CPack)

# Status Messages
message(STATUS "CPack: Packaging enabled for ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "CPack: Supported generators: DEB, TGZ, ZIP, NSIS")
message(STATUS "CPack: Default generator: ${CPACK_GENERATOR}")
message(STATUS "CPack: Components: runtime (shared library and server), dev (headers and CMake package)")
message(STATUS "CPack: DEB packages: ${PKG_DEB_RUNTIME_NAME}, ${PKG_DEB_DEV_NAME}")
message(STATUS "CPack: Run 'cpack -G <generator>' to create packages")
