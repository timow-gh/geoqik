CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG v2.0.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)

# Plinth v2.0.0's internal OpenGL target does not inherit the directory that
# contains the generated plinth_export.h header.  Supply it until the upstream
# package corrects its target usage requirements.
if (TARGET plinth_opengl)
    target_include_directories(plinth_opengl PRIVATE "${plinth_BINARY_DIR}/src/plinth/export_header_includes")
endif ()
