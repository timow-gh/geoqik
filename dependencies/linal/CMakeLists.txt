CPMAddPackage(
        NAME linal
        GITHUB_REPOSITORY "timow-gh/LinAl"
        GIT_TAG bbde45afca1e4b1a867f2cf9cfb66fdec6d48d5c
        # GIT_SHALLOW is intentionally omitted: this pin is a full commit SHA, and
        # GitHub cannot shallow-fetch a SHA that is no longer the tip of its branch
        # ("fatal: unable to read tree"), which would break CI once LinAl/main
        # advances past the pinned commit. A full clone fetches any reachable SHA.
        OPTIONS "LinAl_INSTALL OFF"
)
