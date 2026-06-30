# GeoQik

A lightweight C++ library for visualizing 3D geometry during debugging. Add points and lines from any thread while your program runs — GeoQik opens an OpenGL window and renders them in real time.

> "A picture is worth a thousand words."

## Requirements

- Windows or Linux
- C99 or later (C++ consumers: any standard)
- CMake 3.21+ (for `find_package` / FetchContent integration)

## Build Requirements

- CMake 3.28+
- A C++20 compiler (MSVC, GCC, Clang)
- [vcpkg](https://vcpkg.io)

Windows builds are tested with the x64 MSVC/MSBuild toolchain. CI runs the MSVC presets on GitHub Actions `windows-latest` after `microsoft/setup-msbuild@v2`; Visual Studio 2022 Build Tools with the MSVC v143 toolset is the expected local setup.

## Integration

GeoQik is distributed as a shared library (`geoqik.dll` / `libgeoqik.so`). Consumers need no additional dependencies — everything is absorbed into the library.

### via installed package

```cmake
find_package(geoqik CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE geoqik::geoqik)
```

Pass the install prefix to CMake:

```
cmake -DCMAKE_PREFIX_PATH=/path/to/geoqik/install ...
```

### via FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    geoqik
    GIT_REPOSITORY https://github.com/timow-gh/geoqik.git
    GIT_TAG        v0.1.0
)
set(geoqik_INSTALL OFF)
FetchContent_MakeAvailable(geoqik)

target_link_libraries(your_target PRIVATE geoqik::geoqik)
```

## Usage

```cpp
#include <GeoQik/GeoQik.hpp>

int main()
{
    geoqik_init();

    geoqik_set_point_size(5.0f);
    geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f); // red
    geoqik_set_line_width(2.0f);

    geoqik_draw(); // geometry added before this call is rendered on the first frame

    geoqik_add_point(1.0, 0.0, 0.0);
    geoqik_add_line(0.0, 0.0, 0.0,  1.0, 0.0, 0.0);

    geoqik_wait_for_exit_and_cleanup(); // blocks until the window is closed
}
```

### Mesh

```cpp
#include <GeoQik/GeoQik.hpp>

// vertices: flat XYZ — {x0,y0,z0, x1,y1,z1, ...}
// triangleIndices: triplets of vertex indices
float vertices[] = { 0,0,0,  1,0,0,  0,1,0 };
uint32_t indices[] = { 0, 1, 2 };

geoqik_add_mesh_opts_t opts{};  // normals auto-computed, default color
geoqik_result_t result = geoqik_add_mesh_opts(vertices, 3, indices, 1, &opts);
```

### Log and replay

GeoQik records every geometry event in memory. You can save this log to disk and replay it later:

```cpp
// Save the current session
geoqik_save_log("session.geoqik", GEOQIK_LOG_FORMAT_BINARY);

// In a later run: load and replay
geoqik_init();
geoqik_draw();
geoqik_replay_log("session.geoqik", GEOQIK_LOG_FORMAT_BINARY, NULL); // NULL = replay with default options
geoqik_wait_for_exit_and_cleanup();
```

## Building from source

The build requires vcpkg — set `VCPKG_ROOT` to your vcpkg installation directory.

**Ubuntu** — install OpenGL and windowing system headers first:
```
sudo apt install libgl1-mesa-dev xorg-dev
```

**Windows (MSVC / Visual Studio 2022 Build Tools)**
```
cmake --workflow --preset workflow-test-msvc-release
```

**Ubuntu (GCC)**
```
cmake --workflow --preset workflow-test-gcc-release
```

**Ubuntu (Clang)**
```
cmake --workflow --preset workflow-test-clang-release
```

These presets configure, build, and run the test suite. All available presets are listed in `CMakePresets.json`.

## Contributing and releases

Commit-message and release-note conventions are documented in [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Public domain — see [License](License).
