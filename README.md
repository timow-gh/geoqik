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
    GIT_REPOSITORY https://github.com/your-org/geoqik.git
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

    geoqik_draw(); // open the window; geometry added before this call is rendered on the first frame

    geoqik_add_point(1.0, 0.0, 0.0);  // rendered immediately — window is already open
    geoqik_add_line(0.0, 0.0, 0.0,  1.0, 0.0, 0.0);

    geoqik_wait_for_exit_and_cleanup(); // blocks until the window is closed
}
```

All API functions except `geoqik_init()` are thread-safe and can be called concurrently.

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

## License

Public domain — see [License](License).
