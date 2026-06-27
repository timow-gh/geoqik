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

inline constexpr std::size_t floatPayloadByteCount     = sizeof(float);           // 4
inline constexpr std::size_t colorPayloadByteCount     = 4 * sizeof(float);       // 16
inline constexpr std::size_t uuidPayloadByteCount      = uuidByteCount;           // 16
inline constexpr std::size_t translatePayloadByteCount = uuidByteCount + 3 * sizeof(double); // 40
inline constexpr std::size_t rotatePayloadByteCount    = uuidByteCount + 7 * sizeof(double); // 72

enum class CommandId : std::uint32_t { // NOLINT(performance-enum-size): wire protocol uses 32-bit command IDs.
    Draw        = 1,
    AddPoint    = 2,
    AddLine     = 3,
    WaitForExit = 4,
    Cleanup     = 5,

    IsApiInitialized   = 6,
    StopDrawing        = 7,
    InitWithSettings   = 8,   // no wire IPC; settings passed at server startup via temp file
    SetPointSize       = 9,
    GetPointSize       = 10,
    SetPointColor      = 11,
    GetPointColor      = 12,
    SetLineWidth       = 13,
    GetLineWidth       = 14,
    SetLineColor       = 15,
    GetLineColor       = 16,
    SetMeshColor       = 17,
    GetMeshColor       = 18,
    RemoveAllGeometry  = 19,
    TranslateGeometry  = 20,
    RotateGeometry     = 21,

    AddPointWithColor    = 22,
    AddPointOpts         = 23,
    AddPointsOpts        = 24,
    UpdatePoint          = 25,
    UpdatePointWithColor = 26,
    UpdatePointOpts      = 27,
    UpdatePointsOpts     = 28,
    RemovePoint          = 29,
    AddLineWithColor     = 30,
    AddLineOpts          = 31,
    AddLinesOpts         = 32,
    UpdateLine           = 33,
    UpdateLineWithColor  = 34,
    UpdateLineOpts       = 35,
    UpdateLinesOpts      = 36,
    RemoveLine           = 37,

    AddMeshOpts          = 38,
    RemoveMesh           = 39,
    UpdateMeshOpts       = 40,
    SetMeshOverlayOpts   = 41,
    SetMeshRenderingOpts = 42,
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

template<typename T>
[[nodiscard]] std::array<std::uint8_t, uuidByteCount> pack_return_value(const T& val) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= uuidByteCount);
    std::array<std::uint8_t, uuidByteCount> out{};
    std::memcpy(out.data(), &val, sizeof(T));
    return out;
}

[[nodiscard]] inline std::array<std::uint8_t, uuidByteCount> pack_color_return(
    float r, float g, float b, float a)
{
    std::array<float, 4> rgba{r, g, b, a};
    return pack_return_value(rgba);
}

inline constexpr std::size_t addPointWithColorPayloadByteCount =
    3 * sizeof(double) + 4 * sizeof(float);
inline constexpr std::size_t updatePointPayloadByteCount =
    uuidByteCount + 3 * sizeof(double);
inline constexpr std::size_t updatePointWithColorPayloadByteCount =
    uuidByteCount + 3 * sizeof(double) + 4 * sizeof(float);
inline constexpr std::size_t removeGeometryPayloadByteCount = uuidByteCount;
inline constexpr std::size_t addLineWithColorPayloadByteCount =
    6 * sizeof(double) + 4 * sizeof(float);
inline constexpr std::size_t updateLinePayloadByteCount =
    uuidByteCount + 6 * sizeof(double);
inline constexpr std::size_t updateLineWithColorPayloadByteCount =
    uuidByteCount + 6 * sizeof(double) + 4 * sizeof(float);

inline constexpr std::size_t setMeshOverlayOptsPayloadByteCount =
    uuidByteCount + 2 * sizeof(std::int32_t);
inline constexpr std::size_t setMeshRenderingOptsPayloadByteCount =
    uuidByteCount + sizeof(std::uint32_t) + sizeof(std::int32_t);

inline void write_idempotency_key(std::vector<std::uint8_t>& buf,
                                   const std::array<std::uint8_t, uuidByteCount>& key)
{
    buf.insert(buf.end(), key.begin(), key.end());
}

inline void write_optional_colors(std::vector<std::uint8_t>& buf,
                                   const float* colorData,
                                   std::uint32_t colorCount)
{
    write_pod(buf, colorCount);
    if (colorData != nullptr && colorCount > 0) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(colorData);
        buf.insert(buf.end(), bytes, bytes + colorCount * sizeof(float));
    }
}

[[nodiscard]] inline std::vector<float> read_optional_colors(
    const std::vector<std::uint8_t>& payload,
    std::size_t& offset)
{
    const auto colorCount = read_pod<std::uint32_t>(payload, offset);
    offset += sizeof(std::uint32_t);
    if (colorCount == 0) { return {}; }
    const std::size_t colorBytes = colorCount * sizeof(float);
    if (offset + colorBytes > payload.size()) {
        throw std::out_of_range("geoqik protocol: color payload truncated");
    }
    std::vector<float> colors(colorCount);
    std::memcpy(colors.data(), payload.data() + offset, colorBytes);
    offset += colorBytes;
    return colors;
}

inline void write_optional_single_color(std::vector<std::uint8_t>& buf,
                                         const float* colorPtr,
                                         bool hasColor)
{
    constexpr float kZero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const float* src = (hasColor && colorPtr != nullptr) ? colorPtr : kZero;
    write_pod(buf, src[0]);
    write_pod(buf, src[1]);
    write_pod(buf, src[2]);
    write_pod(buf, src[3]);
}

inline void write_optional_float_array(std::vector<std::uint8_t>& buf,
                                        const float* data,
                                        std::uint32_t count)
{
    write_pod(buf, count);
    if (data != nullptr && count > 0) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
        buf.insert(buf.end(), bytes, bytes + count * sizeof(float));
    }
}

inline void write_optional_uint32_array(std::vector<std::uint8_t>& buf,
                                         const std::uint32_t* data,
                                         std::uint32_t count)
{
    write_pod(buf, count);
    if (data != nullptr && count > 0) {
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
        buf.insert(buf.end(), bytes, bytes + count * sizeof(std::uint32_t));
    }
}

template<typename T>
[[nodiscard]] T unpack_return_value(const std::array<std::uint8_t, uuidByteCount>& uuid) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= uuidByteCount);
    T val{};
    std::memcpy(&val, uuid.data(), sizeof(T));
    return val;
}

} // namespace geoqik::protocol

#endif // GEOQIK_PROTOCOL_PROTOCOL_HPP
