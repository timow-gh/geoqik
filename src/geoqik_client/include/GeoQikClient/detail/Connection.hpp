#pragma once

#include <GeoQikProtocol/Protocol.hpp>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

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

    Connection() = default;
    ~Connection() = default;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;

    void connect(const std::string& pipeName) {
        using namespace std::chrono_literals;
        constexpr auto connectionTimeout = 5000ms;
        constexpr auto retryInterval = 10ms;

        const auto deadline = std::chrono::steady_clock::now() + connectionTimeout;

        while (std::chrono::steady_clock::now() < deadline) {
#ifdef _WIN32
            const HANDLE pipeHandle = ::CreateFileA(
                pipeName.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr);

            if (pipeHandle != INVALID_HANDLE_VALUE) {
                stream_.emplace(io_.get_executor(), pipeHandle);
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
            std::this_thread::sleep_for(retryInterval);
        }
        throw std::runtime_error(
            "geoqik: timed out connecting to server pipe: " + pipeName);
    }

    [[nodiscard]]
    geoqik::protocol::ResponseFrame send_recv(
        geoqik::protocol::CommandId cmd,
        const std::vector<std::uint8_t>& payload)
    {
        namespace proto = geoqik::protocol;

        proto::FrameHeader header{};
        header.commandId = static_cast<std::uint32_t>(cmd);
        header.payloadBytes = proto::payload_byte_count(payload.size());
        boost::asio::write(*stream_,
            boost::asio::buffer(&header, sizeof(header)));

        if (!payload.empty()) {
            boost::asio::write(*stream_,
                boost::asio::buffer(payload.data(), payload.size()));
        }

        proto::ResponseFrame resp{};
        boost::asio::read(*stream_,
            boost::asio::buffer(&resp, sizeof(resp)));
        return resp;
    }

    [[nodiscard]] bool connected() const noexcept { return stream_.has_value(); }

private:
    boost::asio::io_context io_;

    std::optional<StreamType> stream_;
};

inline Connection& thread_connection() {
    thread_local Connection conn; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return conn;
}

inline void ensure_connected(const std::string& pipeName) {
    auto& conn = thread_connection();
    if (!conn.connected()) {
        conn.connect(pipeName);
    }
}

} // namespace geoqik::client::detail
