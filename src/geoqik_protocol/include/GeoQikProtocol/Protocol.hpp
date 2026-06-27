#ifndef GEOQIK_PROTOCOL_PROTOCOL_HPP
#define GEOQIK_PROTOCOL_PROTOCOL_HPP

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace geoqik::protocol {

inline constexpr std::size_t uuidByteCount = 16;
inline constexpr std::size_t frameHeaderByteCount = 8;
inline constexpr std::size_t responseHeaderByteCount = 24;
inline constexpr std::size_t pointPayloadByteCount = 3 * sizeof(double);
inline constexpr std::size_t linePayloadByteCount = 6 * sizeof(double);

enum class CommandId : std::uint32_t { // NOLINT(performance-enum-size): wire protocol uses 32-bit command IDs.
    Draw        = 1,
    AddPoint    = 2,
    AddLine     = 3,
    WaitForExit = 4,
    Cleanup     = 5,
};

struct FrameHeader {
    std::uint32_t commandId{};
    std::uint32_t payloadBytes{};
};
static_assert(sizeof(FrameHeader) == frameHeaderByteCount);

struct ResponseHeader {
    std::uint32_t errorCode{};
    std::uint32_t diagnosticBytes{};
    std::array<std::uint8_t, uuidByteCount> uuid{};
};
static_assert(sizeof(ResponseHeader) == responseHeaderByteCount);

struct DiagnosticText {
    std::string operation;
    std::string what;
    std::string why;
    std::string action;
    std::string details;
};

struct ResponseFrame {
    std::uint32_t errorCode{};
    std::array<std::uint8_t, uuidByteCount> uuid{};
    DiagnosticText diagnostic;
};

// On Linux, a leading null byte places the socket in the kernel's abstract
// namespace rather than the filesystem. This eliminates the TOCTOU race where a
// local attacker could watch for the server's unlink(), then race to place a
// symlink at /tmp/geoqik-<pid>.sock before bind() runs — potentially redirecting
// the bind to an arbitrary file or causing a denial-of-service. An abstract
// socket has no filesystem entry to race against and vanishes when the server
// exits.
[[nodiscard]] inline std::string make_pipe_name(std::uint64_t pid) {
#ifdef _WIN32
    return R"(\\.\pipe\geoqik-)" + std::to_string(pid);
#else
    return std::string("\0geoqik-", 8) + std::to_string(pid);
#endif
}

[[nodiscard]] inline std::string make_pipe_name_argument(std::uint64_t pid) {
#ifdef _WIN32
    return make_pipe_name(pid);
#else
    return "geoqik-" + std::to_string(pid);
#endif
}

[[nodiscard]] inline std::string make_pipe_name_from_argument(const std::string& argument) {
#ifdef _WIN32
    return argument;
#else
    return std::string("\0", 1) + argument;
#endif
}

[[nodiscard]] inline std::uint32_t payload_byte_count(std::size_t payloadSize) {
    constexpr auto maxPayloadSize = static_cast<std::size_t>(
        std::numeric_limits<std::uint32_t>::max());
    if (payloadSize > maxPayloadSize) {
        throw std::length_error("geoqik protocol payload is too large");
    }

    return static_cast<std::uint32_t>(payloadSize);
}

// Append sizeof(T) bytes of val into buf.
template<typename T>
void write_pod(std::vector<std::uint8_t>& buf, const T& val) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(val);
    buf.insert(buf.end(), bytes.begin(), bytes.end());
}

// Read sizeof(T) bytes from data into a T value.
template<typename T>
[[nodiscard]] T read_pod(const std::vector<std::uint8_t>& data, std::size_t offset) {
    static_assert(std::is_trivially_copyable_v<T>);
    if ((offset > data.size()) || (data.size() - offset < sizeof(T))) {
        throw std::out_of_range("geoqik protocol payload is too small");
    }

    T val{};
    std::memcpy(&val, data.data() + offset, sizeof(T));
    return val;
}

inline void write_string(std::vector<std::uint8_t>& buf, const std::string& value) {
    write_pod(buf, payload_byte_count(value.size()));
    buf.insert(buf.end(), value.begin(), value.end());
}

[[nodiscard]] inline std::string read_string(const std::vector<std::uint8_t>& data, std::size_t& offset) {
    const std::uint32_t size = read_pod<std::uint32_t>(data, offset);
    offset += sizeof(std::uint32_t);
    if ((offset > data.size()) || (data.size() - offset < size)) {
        throw std::out_of_range("geoqik protocol diagnostic payload is too small");
    }

    std::string value(reinterpret_cast<const char*>(data.data() + offset), size);
    offset += size;
    return value;
}

[[nodiscard]] inline std::vector<std::uint8_t> encode_diagnostic(const DiagnosticText& diagnostic) {
    std::vector<std::uint8_t> payload;
    write_string(payload, diagnostic.operation);
    write_string(payload, diagnostic.what);
    write_string(payload, diagnostic.why);
    write_string(payload, diagnostic.action);
    write_string(payload, diagnostic.details);
    return payload;
}

[[nodiscard]] inline DiagnosticText decode_diagnostic(const std::vector<std::uint8_t>& payload) {
    DiagnosticText diagnostic;
    if (payload.empty()) {
        return diagnostic;
    }

    std::size_t offset = 0;
    diagnostic.operation = read_string(payload, offset);
    diagnostic.what = read_string(payload, offset);
    diagnostic.why = read_string(payload, offset);
    diagnostic.action = read_string(payload, offset);
    diagnostic.details = read_string(payload, offset);
    if (offset != payload.size()) {
        throw std::out_of_range("geoqik protocol diagnostic payload has trailing bytes");
    }
    return diagnostic;
}

} // namespace geoqik::protocol

#endif // GEOQIK_PROTOCOL_PROTOCOL_HPP
