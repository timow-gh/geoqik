CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG v3.0.0
        # Pinned to a stable release tag, so the dependency is reproducible and
        # unambiguous. The tag is immutable and shallow-fetchable by GitHub, so
        # this stays reliable as plinth/main advances.
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)
