#include "IpcServer.hpp"
#include <GeoQik/GeoQik.hpp>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string pipeName;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--pipe-name") {
            pipeName = argv[i + 1];
            break;
        }
    }
    if (pipeName.empty()) {
        std::cerr << "geoqik_server: --pipe-name <name> is required\n";
        return 1;
    }

    auto err = geoqik_init();
    if (err != GEOQIK_SUCCESS) {
        std::cerr << "geoqik_init failed: " << geoqik_get_error_string(err) << '\n';
        return 1;
    }

    geoqik::server::run(pipeName);

    return 0;
}
