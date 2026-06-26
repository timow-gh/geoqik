#pragma once

#ifdef _WIN32
#include <boost/asio/windows/stream_handle.hpp>
using PipeStream = boost::asio::windows::stream_handle;
#else
#include <boost/asio/local/stream_protocol.hpp>
using PipeStream = boost::asio::local::stream_protocol::socket;
#endif

namespace geoqik::server::dispatch {

// Reads commands from stream in a loop until the connection closes or
// WaitForExit/Cleanup is received (both call std::exit).
void handle_connection(PipeStream& stream);

} // namespace geoqik::server::dispatch
