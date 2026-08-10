CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG 67674d874e26cca8614495df7c0678855ee038b2
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)
