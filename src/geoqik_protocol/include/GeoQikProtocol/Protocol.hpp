#pragma once

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
inline constexpr std::size_t responseFrameByteCount = 20;
inline constexpr std::size_t pointPayloadByteCount = 3 * sizeof(double);
inline constexpr std::size_t linePayloadByteCount = 6 * sizeof(double);

enum class CommandId : std::uint32_t {
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

struct ResponseFrame {
    std::uint32_t errorCode{};
    std::array<std::uint8_t, uuidByteCount> uuid{};
};
static_assert(sizeof(ResponseFrame) == responseFrameByteCount);

[[nodiscard]] inline std::string make_pipe_name(std::uint64_t pid) {
#ifdef _WIN32
    return R"(\\.\pipe\geoqik-)" + std::to_string(pid);
#else
    return "/tmp/geoqik-" + std::to_string(pid) + ".sock";
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

} // namespace geoqik::protocol
