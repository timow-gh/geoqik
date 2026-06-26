#include "IpcServer.hpp"
#include "CommandDispatch.hpp"
#include <GeoQikProtocol/Protocol.hpp>

#ifdef _WIN32
#include <windows.h>
#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <thread>
#include <stdexcept>
#include <iostream>

namespace geoqik::server {

void run(const std::string& pipeName) {
    for (;;) {
        HANDLE h = ::CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            /*outBuf=*/65536,
            /*inBuf=*/ 65536,
            /*defaultTimeout=*/0,
            /*security=*/nullptr);

        if (h == INVALID_HANDLE_VALUE) {
            std::cerr << "CreateNamedPipe failed: " << ::GetLastError() << '\n';
            std::exit(1);
        }

        OVERLAPPED ov{};
        ov.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        ::ConnectNamedPipe(h, &ov);
        ::WaitForSingleObject(ov.hEvent, INFINITE);
        ::CloseHandle(ov.hEvent);

        std::thread([h]() {
            boost::asio::io_context io;
            PipeStream stream(io.get_executor(), h);
            dispatch::handle_connection(stream);
        }).detach();
    }
}

} // namespace geoqik::server
#endif // _WIN32

#ifndef _WIN32
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <thread>
#include <filesystem>

namespace asio = boost::asio;

namespace geoqik::server {

void run(const std::string& socketPath) {
    namespace local = asio::local;
    asio::io_context io;

    std::filesystem::remove(socketPath);

    local::stream_protocol::acceptor acceptor(
        io,
        local::stream_protocol::endpoint(socketPath));

    for (;;) {
        local::stream_protocol::socket socket(io);
        acceptor.accept(socket);

        std::thread([s = std::move(socket)]() mutable {
            dispatch::handle_connection(s);
        }).detach();
    }
}

} // namespace geoqik::server
#endif // !_WIN32
