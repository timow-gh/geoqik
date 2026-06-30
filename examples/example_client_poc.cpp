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
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {

void print_last_client_error(const char* operation, geoqik_error_code_t err) {
    std::cerr << operation << " failed with API code " << static_cast<int>(err) << '\n';

    geoqik_client_error_info_t info{};
    info.struct_size = sizeof(info);
    if (geoqik_client_get_last_error_info(&info) != GEOQIK_SUCCESS) {
        return;
    }

    if (info.client_code != GEOQIK_CLIENT_SUCCESS) {
        std::cerr << "client code: " << static_cast<int>(info.client_code) << '\n';
    }
    if (info.operation != nullptr && info.operation[0] != '\0') {
        std::cerr << "operation: " << info.operation << '\n';
    }
    if (info.what != nullptr && info.what[0] != '\0') {
        std::cerr << "what: " << info.what << '\n';
    }
    if (info.why != nullptr && info.why[0] != '\0') {
        std::cerr << "why: " << info.why << '\n';
    }
    if (info.action != nullptr && info.action[0] != '\0') {
        std::cerr << "action: " << info.action << '\n';
    }
    if (info.details != nullptr && info.details[0] != '\0') {
        std::cerr << "details: " << info.details << '\n';
    }
}

bool check_success(geoqik_error_code_t err, const char* operation) {
    if (err == GEOQIK_SUCCESS) {
        return true;
    }

    print_last_client_error(operation, err);
    return false;
}

} // namespace

int main() {
    auto err = geoqik_init();
    if (!check_success(err, "geoqik_init")) {
        return EXIT_FAILURE;
    }

    err = geoqik_draw();
    if (!check_success(err, "geoqik_draw")) {
        return EXIT_FAILURE;
    }

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
    if (!check_success(err, "geoqik_wait_for_exit_and_cleanup")) {
        return EXIT_FAILURE;
    }

    return 0;
}
