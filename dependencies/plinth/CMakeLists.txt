CPMAddPackage(
        NAME plinth
        GITHUB_REPOSITORY "timow-gh/plinth"
        GIT_TAG 67674d874e26cca8614495df7c0678855ee038b2
        # GIT_SHALLOW is intentionally omitted: this pin is a full commit SHA, and
        # GitHub cannot shallow-fetch a SHA that is no longer the tip of its branch
        # ("fatal: unable to read tree"). That breaks CI as soon as plinth/main
        # advances past the pinned commit. A full clone can fetch any reachable
        # SHA, so the build stays reproducible regardless of upstream pushes.
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "plinth_TESTS OFF"
        "plinth_EXAMPLES OFF"
        "plinth_INSTALL OFF"
        "plinth_PACK OFF"
        "plinth_BUILD_DOCUMENTATION OFF"
)
