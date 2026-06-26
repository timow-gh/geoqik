#pragma once
#include <string>

namespace geoqik::server {

// Starts an IPC server on the given pipe name / socket path.
// Blocks until std::exit() is called from a connection handler.
void run(const std::string& pipeName);

} // namespace geoqik::server
