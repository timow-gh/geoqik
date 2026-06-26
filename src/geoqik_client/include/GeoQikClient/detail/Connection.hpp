#pragma once

#include <GeoQikProtocol/Protocol.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <optional>

#ifdef _WIN32
#  include <boost/asio/windows/stream_handle.hpp>
#  include <windows.h>
#else
#  include <boost/asio/local/stream_protocol.hpp>
#endif

namespace geoqik::client::detail {

class Connection {
public:
#ifdef _WIN32
    using StreamType = boost::asio::windows::basic_stream_handle<boost::asio::io_context::executor_type>;
#else
    using StreamType = boost::asio::basic_stream_socket<
        boost::asio::local::stream_protocol,
        boost::asio::io_context::executor_type>;
#endif

    void connect(const std::string& pipeName) {
        using namespace std::chrono_literals;
        constexpr auto kTimeout  = 5000ms;
        constexpr auto kInterval =   10ms;

        auto deadline = std::chrono::steady_clock::now() + kTimeout;

        while (std::chrono::steady_clock::now() < deadline) {
#ifdef _WIN32
            HANDLE h = ::CreateFileA(
                pipeName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr);

            if (h != INVALID_HANDLE_VALUE) {
                stream_.emplace(io_.get_executor(), h);
                return;
            }
#else
            boost::system::error_code ec;
            StreamType sock(io_.get_executor());
            sock.connect(
                boost::asio::local::stream_protocol::endpoint(pipeName), ec);
            if (!ec) {
                stream_.emplace(std::move(sock));
                return;
            }
#endif
            std::this_thread::sleep_for(kInterval);
        }
        throw std::runtime_error(
            "geoqik: timed out connecting to server pipe: " + pipeName);
    }

    geoqik::protocol::ResponseFrame send_recv(
        geoqik::protocol::CommandId cmd,
        const std::vector<uint8_t>& payload)
    {
        namespace proto = geoqik::protocol;

        proto::FrameHeader hdr{};
        hdr.commandId    = static_cast<uint32_t>(cmd);
        hdr.payloadBytes = static_cast<uint32_t>(payload.size());
        boost::asio::write(*stream_,
            boost::asio::buffer(&hdr, sizeof(hdr)));

        if (!payload.empty()) {
            boost::asio::write(*stream_,
                boost::asio::buffer(payload.data(), payload.size()));
        }

        proto::ResponseFrame resp{};
        boost::asio::read(*stream_,
            boost::asio::buffer(&resp, sizeof(resp)));
        return resp;
    }

    bool connected() const { return stream_.has_value(); }

private:
    boost::asio::io_context io_;

#ifdef _WIN32
    std::optional<StreamType> stream_;
#else
    std::optional<StreamType> stream_;
#endif
};

inline Connection& thread_connection() {
    thread_local Connection conn;
    return conn;
}

inline void ensure_connected(const std::string& pipeName) {
    auto& conn = thread_connection();
    if (!conn.connected()) {
        conn.connect(pipeName);
    }
}

} // namespace geoqik::client::detail
