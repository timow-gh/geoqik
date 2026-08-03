CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG 08b6d643831648b6227489a9168a8b8ce66c87a5
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)
