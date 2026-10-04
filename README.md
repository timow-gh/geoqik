# GeoQik

**See your geometry while you debug it.** GeoQik is a tiny C++ library that opens a live 3D window and draws the points, lines and meshes your code produces — from any thread, while your program runs. No viewer to write, no files to export.

https://github.com/user-attachments/assets/aed8a220-aeaf-4044-b47a-1a9df9edd9c9

## Example

```cpp
#include <GeoQik/GeoQik.hpp>
#include <cmath>

int main()
{
    geoqik_init();
    geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f); // red
    geoqik_draw();                                  // open the window

    double px = 1, py = 0, pz = 0;
    for (int i = 1; i <= 200; ++i) {
        double a = i * 0.1, x = std::cos(a), y = std::sin(a), z = i * 0.02;
        geoqik_add_line(px, py, pz, x, y, z);       // appears live
        geoqik_add_point(x, y, z);
        px = x; py = y; pz = z;
    }

    geoqik_wait_for_exit_and_cleanup();             // blocks until the window is closed
}
```

Meshes work the same way (`geoqik_add_mesh_opts`) — see `GeoQik.hpp`.

## Record and replay

Every command you send to GeoQik is recorded. Save the session to a file, and replay it later to see exactly what your code did — one command at a time. Play it back, pause, and step forward or backward through each point, line and mesh (arrow keys or `A`/`D`). That makes it easy to find the exact call where your geometry went wrong, even in a run you can no longer reproduce.

```cpp
geoqik_save_log("session.geoqik", GEOQIK_LOG_FORMAT_BINARY);        // at the end of a run

// later, in any program:
geoqik_init();
geoqik_draw();
geoqik_replay_log("session.geoqik", GEOQIK_LOG_FORMAT_BINARY, NULL); // NULL = default options
geoqik_wait_for_exit_and_cleanup();
```

## Two ways to use it

**Link the library.** `geoqik::geoqik` is a shared library with no further dependencies. Your program renders in-process.

**Drop in the header-only client.** `GeoQikClient.hpp` has the same API, but instead of linking a library it starts the `geoqik_server` executable and talks to it over IPC. The server opens the window, so you only need GeoQik *installed* — then copy the single header into your own project and build. That makes it ideal for debugging geometry code in a library that shouldn't depend on GeoQik: swap the include and your calls are unchanged.

```cpp
#include <GeoQikClient/GeoQikClient.hpp>   // instead of <GeoQik/GeoQik.hpp>
```

## Integration

Download a package from [GitHub Releases](https://github.com/timow-gh/geoqik/releases) (Windows `.msi` / `.zip`, Ubuntu 24.04 `.deb` / `.tar.gz`, each with a `.sha256`), then:

```cmake
find_package(geoqik CONFIG REQUIRED)                       # cmake -DCMAKE_PREFIX_PATH=/path/to/geoqik ...
target_link_libraries(your_target PRIVATE geoqik::geoqik)  # link the library
target_link_libraries(your_target PRIVATE geoqik::client)  # or: header-only client
```

The client finds `geoqik_server` through `PATH` or `GEOQIK_EXE_PATH`. The MSI and DEB packages set this up; for archives, add `<archive>/bin` to `PATH`.

Or build it with FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(geoqik
    GIT_REPOSITORY https://github.com/timow-gh/geoqik.git
    GIT_TAG        v0.3.1)
set(geoqik_INSTALL OFF)
FetchContent_MakeAvailable(geoqik)
target_link_libraries(your_target PRIVATE geoqik::geoqik)
```

## Building from source

Needs CMake 3.28+, a C++20 compiler, and [vcpkg](https://vcpkg.io) (`VCPKG_ROOT` set). On Ubuntu also `sudo apt install libgl1-mesa-dev xorg-dev`.

```
cmake --workflow --preset workflow-test-msvc-release   # or -gcc- / -clang-
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for conventions. Public domain — see [License](License).
