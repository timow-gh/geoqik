#include "IpcServer.hpp"
#include "CommandDispatch.hpp"
#include <GeoQikProtocol/Protocol.hpp>

#ifdef _WIN32
#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <windows.h>

namespace geoqik::server {

void run(const std::string& pipeName) {
    constexpr DWORD pipeBufferSize = 65536;

    for (;;) {
        HANDLE pipeHandle = ::CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            pipeBufferSize,
            pipeBufferSize,
            /*defaultTimeout=*/0,
            /*security=*/nullptr);

        if (pipeHandle == INVALID_HANDLE_VALUE) {
            std::cerr << "CreateNamedPipe failed: " << ::GetLastError() << '\n';
            std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
        }

        OVERLAPPED overlapped{};
        overlapped.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (overlapped.hEvent == nullptr) {
            std::cerr << "CreateEvent failed: " << ::GetLastError() << '\n';
            ::CloseHandle(pipeHandle);
            std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
        }

        const BOOL connected = ::ConnectNamedPipe(pipeHandle, &overlapped);
        const DWORD connectError = (connected == FALSE) ? ::GetLastError() : ERROR_SUCCESS;
        if ((connectError != ERROR_SUCCESS) && (connectError != ERROR_IO_PENDING)
            && (connectError != ERROR_PIPE_CONNECTED)) {
            std::cerr << "ConnectNamedPipe failed: " << connectError << '\n';
            ::CloseHandle(overlapped.hEvent);
            ::CloseHandle(pipeHandle);
            std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
        }

        if (connectError == ERROR_IO_PENDING) {
            ::WaitForSingleObject(overlapped.hEvent, INFINITE);
        }
        ::CloseHandle(overlapped.hEvent);

        std::thread([pipeHandle]() mutable {
            boost::asio::io_context io;
            dispatch::PipeStream stream(io.get_executor(), pipeHandle);
            dispatch::handle_connection(stream);
        }).detach();
    }
}

} // namespace geoqik::server
#endif // _WIN32

#ifndef _WIN32
#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <thread>

namespace asio = boost::asio;

namespace geoqik::server {

void run(const std::string& socketPath) {
    namespace local = asio::local;
    asio::io_context io;

    std::error_code removeError;
    std::filesystem::remove(socketPath, removeError);
    if (removeError) {
        std::cerr << "Removing existing socket failed: " << removeError.message() << '\n';
        std::exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe)
    }

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
