#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace geoqik::protocol {

// TODO: add protocol version handshake

enum class CommandId : uint32_t {
    Draw        = 1,
    AddPoint    = 2,
    AddLine     = 3,
    WaitForExit = 4,
    Cleanup     = 5,
};

struct FrameHeader {
    uint32_t commandId;
    uint32_t payloadBytes;
};
static_assert(sizeof(FrameHeader) == 8);

struct ResponseFrame {
    uint32_t errorCode;
    uint8_t  uuid[16];
};
static_assert(sizeof(ResponseFrame) == 20);


inline std::string make_pipe_name(int pid) {
#ifdef _WIN32
    return R"(\\.\pipe\geoqik-)" + std::to_string(pid);
#else
    return "/tmp/geoqik-" + std::to_string(pid) + ".sock";
#endif
}


// Append sizeof(T) bytes of val into buf.
template<typename T>
void write_pod(std::vector<uint8_t>& buf, const T& val) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* p = reinterpret_cast<const uint8_t*>(&val);
    buf.insert(buf.end(), p, p + sizeof(T));
}

// Read sizeof(T) bytes from data into a T value.
template<typename T>
T read_pod(const uint8_t* data) {
    static_assert(std::is_trivially_copyable_v<T>);
    T val{};
    std::memcpy(&val, data, sizeof(T));
    return val;
}

} // namespace geoqik::protocol
