// Proof-of-concept: calls geoqik API from multiple threads via IPC.
// Links geoqik::client only; geoqik.dll is NOT linked.
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
#include <iostream>
#include <thread>

int main() {
    [[maybe_unused]] auto err = geoqik_init();
    assert(err == GEOQIK_SUCCESS && "geoqik_init failed; is GEOQIK_EXE_PATH set?");

    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    constexpr std::size_t threadCount = 4;
    constexpr std::size_t stepsPerThread = 250;

    std::barrier sync(static_cast<std::ptrdiff_t>(threadCount));
    std::array<std::thread, threadCount> threads;

    for (std::size_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
        threads[threadIndex] = std::thread([threadIndex, &sync]() {
            sync.arrive_and_wait();

            const double offset = static_cast<double>(threadIndex) * 3.0;
            for (std::size_t stepIndex = 0; stepIndex < stepsPerThread; ++stepIndex) {
                const double angle = static_cast<double>(stepIndex) * 0.1;
                const double x = offset + std::cos(angle);
                const double y = std::sin(angle);
                const double z = static_cast<double>(stepIndex) * 0.008;

                [[maybe_unused]] const auto pointResult = geoqik_add_point(x, y, z);
                assert(pointResult.err == GEOQIK_SUCCESS);

                [[maybe_unused]] const auto lineResult = geoqik_add_line(offset, 0.0, z * 0.35, x, y, z);
                assert(lineResult == GEOQIK_SUCCESS);
            }
        });
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << "PoC complete: " << threadCount * stepsPerThread * 2
              << " points and lines added from " << threadCount << " threads.\n";

    err = geoqik_wait_for_exit_and_cleanup();
    assert(err == GEOQIK_SUCCESS);

    return 0;
}
