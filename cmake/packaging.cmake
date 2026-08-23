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
    # WiX/MSI installer. The Windows Installer engine edits the PATH registry
    # value through the OS API (see cmake/wix_patch.xml), avoiding the 1024-char
    # string-buffer limit that made the old NSIS installer fail on machines with
    # a long system PATH.
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "GeoQik")

    # Stable upgrade GUID. Windows Installer uses this to recognize that a new
    # package is an upgrade of a previously installed GeoQik rather than a
    # separate product. It MUST NEVER CHANGE once released: changing it makes
    # future versions install side-by-side instead of upgrading in place.
    # CPACK_WIX_PRODUCT_GUID is intentionally left unset so CPack regenerates it
    # per version, which together with the stable upgrade GUID gives clean
    # major-upgrade (uninstall-old-then-install-new) behavior.
    set(CPACK_WIX_UPGRADE_GUID "BA2A4EDA-3DED-41C4-8B20-11E7C6F0B62B")

    # Add/Remove Programs links (carried over from the old NSIS help/about links).
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")

    # WiX requires the license shown in the installer UI to be real RTF with an
    # .rtf/.txt extension. The repo License is extensionless plain text (used by
    # every other generator via CPACK_RESOURCE_FILE_LICENSE), so point WiX at an
    # RTF rendering of the same Unlicense text instead.
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_LIST_DIR}/License.rtf")

    # Custom WiX template using the WixUI_Advanced dialog set so the installer
    # offers a per-user vs all-users choice, matching the old NSIS behavior.
    set(CPACK_WIX_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/wix_template.wxs")
    set(CPACK_WIX_UI_REF "WixUI_Advanced")

    # Re-implement "add bin to PATH": CPack's WiX generator has no built-in
    # equivalent of CPACK_NSIS_MODIFY_PATH, so an <Environment> element is
    # injected via this patch fragment.
    set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_LIST_DIR}/wix_patch.xml")
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
message(STATUS "CPack: Supported generators: DEB, TGZ, ZIP, WIX")
message(STATUS "CPack: Default generator: ${CPACK_GENERATOR}")
message(STATUS "CPack: Components: runtime (shared library and server), dev (headers and CMake package)")
message(STATUS "CPack: DEB packages: ${PKG_DEB_RUNTIME_NAME}, ${PKG_DEB_DEV_NAME}")
message(STATUS "CPack: Run 'cpack -G <generator>' to create packages")
