CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY "nlohmann/json"
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
        OPTIONS
        "JSON_BuildTests OFF"
)
