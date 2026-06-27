#include "IpcServer.hpp"
#include <GeoQik/GeoQik.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string pipeNameArgument;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--pipe-name") {
            pipeNameArgument = argv[i + 1];
            break;
        }
    }
    if (pipeNameArgument.empty()) {
        std::cerr << "geoqik_server: --pipe-name <name> is required\n";
        return 1;
    }
    const auto pipeName = geoqik::protocol::make_pipe_name_from_argument(pipeNameArgument);

    const auto err = geoqik_init();
    if (err != GEOQIK_SUCCESS) {
        std::cerr << "geoqik_init failed: " << geoqik_get_error_string(err) << '\n';
        return 1;
    }

    geoqik::server::run(pipeName);

    return 0;
}
