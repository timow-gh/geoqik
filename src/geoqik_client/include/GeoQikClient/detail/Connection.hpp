#ifndef GEOQIK_CLIENT_DETAIL_CONNECTION_HPP
#define GEOQIK_CLIENT_DETAIL_CONNECTION_HPP

#include <GeoQikClient/detail/Protocol.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <unistd.h>
#endif

namespace geoqik::client::detail {

class IpcConnectionError : public std::runtime_error {
public:
    explicit IpcConnectionError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class IpcWriteError : public std::runtime_error {
public:
    explicit IpcWriteError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class IpcReadError : public std::runtime_error {
public:
    explicit IpcReadError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ProtocolError : public std::runtime_error {
public:
    explicit ProtocolError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class Connection {
public:
    Connection() = default;
    ~Connection() { disconnect(); }

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
            if (try_connect(pipeName)) {
                return;
            }
            std::this_thread::sleep_for(retryInterval);
        }
        throw IpcConnectionError(
            "geoqik: timed out connecting to server pipe: " + pipeName);
    }

    [[nodiscard]]
    geoqik::protocol::ResponseFrame send_recv(
        geoqik::protocol::CommandId cmd,
        const std::vector<std::uint8_t>& payload)
    {
        namespace proto = geoqik::protocol;

        try {
            proto::FrameHeader header{};
            header.commandId = static_cast<std::uint32_t>(cmd);
            header.payloadBytes = proto::payload_byte_count(payload.size());
            write_exact(&header, sizeof(header));

            if (!payload.empty()) {
                write_exact(payload.data(), payload.size());
            }
        } catch (const std::exception& e) {
            disconnect();
            throw IpcWriteError(e.what());
        }

        proto::ResponseHeader responseHeader{};
        try {
            read_exact(&responseHeader, sizeof(responseHeader));

            std::vector<std::uint8_t> diagnosticPayload(responseHeader.diagnosticBytes);
            if (responseHeader.diagnosticBytes > 0) {
                read_exact(diagnosticPayload.data(), diagnosticPayload.size());
            }

            proto::ResponseFrame resp{};
            resp.errorCode = responseHeader.errorCode;
            resp.uuid = responseHeader.uuid;
            resp.diagnostic = proto::decode_diagnostic(diagnosticPayload);
            return resp;
        } catch (const std::out_of_range& e) {
            disconnect();
            throw ProtocolError(e.what());
        } catch (const std::exception& e) {
            disconnect();
            throw IpcReadError(e.what());
        }
    }

    [[nodiscard]] bool connected() const noexcept {
#ifdef _WIN32
        return pipeHandle_ != INVALID_HANDLE_VALUE;
#else
        return socketFd_ >= 0;
#endif
    }

    void disconnect() noexcept {
#ifdef _WIN32
        if (pipeHandle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(pipeHandle_);
            pipeHandle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (socketFd_ >= 0) {
            ::close(socketFd_);
            socketFd_ = -1;
        }
#endif
    }

private:
    [[nodiscard]] bool try_connect(const std::string& pipeName) {
#ifdef _WIN32
        HANDLE handle = ::CreateFileA(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            nullptr);

        if (handle != INVALID_HANDLE_VALUE) {
            pipeHandle_ = handle;
            return true;
        }

        if (::GetLastError() == ERROR_PIPE_BUSY) {
            ::WaitNamedPipeA(pipeName.c_str(), 10);
        }
        return false;
#else
        const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            return false;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        const std::size_t maxPathBytes = sizeof(addr.sun_path);
        if (pipeName.size() > maxPathBytes) {
            ::close(fd);
            throw IpcConnectionError("geoqik: socket name is too long");
        }
        std::memcpy(addr.sun_path, pipeName.data(), pipeName.size());

        const auto addrLen = static_cast<socklen_t>(
            offsetof(sockaddr_un, sun_path) + pipeName.size());
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), addrLen) == 0) {
            socketFd_ = fd;
            return true;
        }

        ::close(fd);
        return false;
#endif
    }

    void write_exact(const void* data, std::size_t byteCount) {
        const auto* cursor = static_cast<const std::uint8_t*>(data);
        std::size_t remaining = byteCount;
        while (remaining > 0) {
#ifdef _WIN32
            const DWORD written = write_overlapped(cursor, remaining);
            cursor += written;
            remaining -= written;
#else
            const ssize_t written = ::write(socketFd_, cursor, remaining);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error("geoqik: socket write failed: " + std::string(std::strerror(errno)));
            }
            if (written == 0) {
                throw std::runtime_error("geoqik: socket write returned zero bytes");
            }
            cursor += static_cast<std::size_t>(written);
            remaining -= static_cast<std::size_t>(written);
#endif
        }
    }

    void read_exact(void* data, std::size_t byteCount) {
        auto* cursor = static_cast<std::uint8_t*>(data);
        std::size_t remaining = byteCount;
        while (remaining > 0) {
#ifdef _WIN32
            const DWORD read = read_overlapped(cursor, remaining);
            cursor += read;
            remaining -= read;
#else
            const ssize_t bytesRead = ::read(socketFd_, cursor, remaining);
            if (bytesRead < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::runtime_error("geoqik: socket read failed: " + std::string(std::strerror(errno)));
            }
            if (bytesRead == 0) {
                throw std::runtime_error("geoqik: socket closed while reading");
            }
            cursor += static_cast<std::size_t>(bytesRead);
            remaining -= static_cast<std::size_t>(bytesRead);
#endif
        }
    }

#ifdef _WIN32
    [[nodiscard]] DWORD write_overlapped(const std::uint8_t* data, std::size_t byteCount) {
        return transfer_overlapped(
            [&](OVERLAPPED& overlapped, DWORD chunk) {
                return ::WriteFile(pipeHandle_, data, chunk, nullptr, &overlapped);
            },
            byteCount,
            "geoqik: pipe write failed");
    }

    [[nodiscard]] DWORD read_overlapped(std::uint8_t* data, std::size_t byteCount) {
        return transfer_overlapped(
            [&](OVERLAPPED& overlapped, DWORD chunk) {
                return ::ReadFile(pipeHandle_, data, chunk, nullptr, &overlapped);
            },
            byteCount,
            "geoqik: pipe read failed");
    }

    template<typename TransferFunc>
    [[nodiscard]] DWORD transfer_overlapped(
        TransferFunc&& transfer,
        std::size_t byteCount,
        const char* failureMessage)
    {
        OVERLAPPED overlapped{};
        overlapped.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (overlapped.hEvent == nullptr) {
            throw std::runtime_error("geoqik: could not create pipe event");
        }

        const DWORD chunk = byteCount > static_cast<std::size_t>(MAXDWORD)
            ? MAXDWORD
            : static_cast<DWORD>(byteCount);

        const BOOL started = transfer(overlapped, chunk);
        DWORD transferred = 0;
        if (started == FALSE) {
            const DWORD error = ::GetLastError();
            if (error != ERROR_IO_PENDING) {
                ::CloseHandle(overlapped.hEvent);
                throw std::runtime_error(
                    std::string(failureMessage) + " with error " + std::to_string(error));
            }
            if (::WaitForSingleObject(overlapped.hEvent, INFINITE) != WAIT_OBJECT_0) {
                ::CloseHandle(overlapped.hEvent);
                throw std::runtime_error(failureMessage);
            }
        }

        if (::GetOverlappedResult(pipeHandle_, &overlapped, &transferred, FALSE) == FALSE
            || transferred == 0) {
            const DWORD error = ::GetLastError();
            ::CloseHandle(overlapped.hEvent);
            throw std::runtime_error(
                std::string(failureMessage) + " with error " + std::to_string(error));
        }

        ::CloseHandle(overlapped.hEvent);
        return transferred;
    }
#endif

#ifdef _WIN32
    HANDLE pipeHandle_ = INVALID_HANDLE_VALUE;
#else
    int socketFd_ = -1;
#endif
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

inline void disconnect_thread_connection() noexcept {
    thread_connection().disconnect();
}

} // namespace geoqik::client::detail

#endif // GEOQIK_CLIENT_DETAIL_CONNECTION_HPP
