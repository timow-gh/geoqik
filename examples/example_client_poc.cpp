// Proof-of-concept: calls geoqik API from multiple threads via IPC.
// Links geoqik::client only — geoqik.dll is NOT linked.
//
// To run:
//   Set GEOQIK_EXE_PATH to the path of geoqik_serverd.exe (debug build), e.g.:
//   set GEOQIK_EXE_PATH=C:\path\to\out\build\conf-msvc-debug\src\geoqik_server\geoqik_serverd.exe
//   Then run this executable.

#include <GeoQikClient/GeoQikClient.hpp>
#include <array>
#include <barrier>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <thread>

int main() {
    // Spawn geoqik.exe and establish the main thread's connection.
    auto err = geoqik_init();
    assert(err == GEOQIK_SUCCESS && "geoqik_init failed — is GEOQIK_EXE_PATH set?");

    // Tell the server to open its OpenGL window.
    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    // Add a small number of points and lines from multiple threads.
    constexpr std::size_t kThreads = 4;
    constexpr std::size_t kStepsPerThread = 250;

    std::barrier sync(static_cast<std::ptrdiff_t>(kThreads));
    std::array<std::thread, kThreads> threads;

    for (std::size_t t = 0; t < kThreads; ++t) {
        threads[t] = std::thread([t, &sync]() {
            sync.arrive_and_wait(); // all threads start together

            const double offset = static_cast<double>(t) * 3.0;
            for (std::size_t i = 0; i < kStepsPerThread; ++i) {
                const double angle = static_cast<double>(i) * 0.1;
                const double x = offset + std::cos(angle);
                const double y = std::sin(angle);
                const double z = static_cast<double>(i) * 0.008;

                // Add a point at the current position.
                auto pr = geoqik_add_point(x, y, z);
                assert(pr.err == GEOQIK_SUCCESS);

                // Add a line from origin to the point.
                auto le = geoqik_add_line(offset, 0.0, z * 0.35, x, y, z);
                assert(le == GEOQIK_SUCCESS);
            }
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    std::printf("PoC complete: %zu points and lines added from %zu threads.\n",
        kThreads * kStepsPerThread * 2, kThreads);

    // Block until the user closes the geoqik window.
    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
