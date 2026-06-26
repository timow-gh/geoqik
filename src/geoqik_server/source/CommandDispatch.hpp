#pragma once

#ifdef _WIN32
#include <boost/asio/io_context.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#else
#include <boost/asio/local/stream_protocol.hpp>
#endif

namespace geoqik::server::dispatch {

#ifdef _WIN32
using PipeStream = boost::asio::windows::basic_stream_handle<boost::asio::io_context::executor_type>;
#else
using PipeStream = boost::asio::local::stream_protocol::socket;
#endif

// Reads commands from stream in a loop until the connection closes or
// WaitForExit/Cleanup is received (both call std::exit).
void handle_connection(PipeStream& stream);

} // namespace geoqik::server::dispatch
