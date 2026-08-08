CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG 31433470fc4f8d46f23e88350ff017ec16072a69
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)
