#ifndef GEOQIKCLIENT_HPP
#define GEOQIKCLIENT_HPP

#include <GeoQik/ApiTypes.h>

typedef enum {
    GEOQIK_CLIENT_SUCCESS = 0,
    GEOQIK_CLIENT_ERROR_SERVER_EXECUTABLE_NOT_FOUND = 1,
    GEOQIK_CLIENT_ERROR_SERVER_START_FAILED = 2,
    GEOQIK_CLIENT_ERROR_IPC_CONNECTION = 3,
    GEOQIK_CLIENT_ERROR_IPC_WRITE = 4,
    GEOQIK_CLIENT_ERROR_IPC_READ = 5,
    GEOQIK_CLIENT_ERROR_PROTOCOL = 6,
    GEOQIK_CLIENT_ERROR_UNKNOWN = 7
} geoqik_client_error_code_t;

typedef struct {
    size_t struct_size;
    geoqik_client_error_code_t client_code;
    geoqik_error_code_t code;
    const char* operation;
    const char* what;
    const char* why;
    const char* action;
    const char* details;
} geoqik_client_error_info_t;

#ifdef __cplusplus
#  include <GeoQikClient/detail/ClientTypes.hpp>
#endif

#ifdef __cplusplus

#include <GeoQikClient/detail/Connection.hpp>
#include <GeoQikClient/detail/ProcessManager.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iterator>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

// ── Log format ────────────────────────────────────────────────────────────────
// Mirror of geoqik_log_format_t from GeoQik.hpp — must be kept in sync.
enum geoqik_log_format_t : int { // NOLINT(readability-identifier-naming,performance-enum-size)
    GEOQIK_LOG_FORMAT_BINARY = 0
};

// ── Key enum ──────────────────────────────────────────────────────────────────
// Mirror of geoqik_key_t from GeoQik.hpp — must be kept in sync.
enum geoqik_key_t : int { // NOLINT(readability-identifier-naming,performance-enum-size)
    GEOQIK_KEY_UNKNOWN = -1,
    GEOQIK_KEY_SPACE = 32, GEOQIK_KEY_APOSTROPHE = 39, GEOQIK_KEY_COMMA = 44,
    GEOQIK_KEY_MINUS = 45, GEOQIK_KEY_PERIOD = 46, GEOQIK_KEY_SLASH = 47,
    GEOQIK_KEY_0 = 48, GEOQIK_KEY_1 = 49, GEOQIK_KEY_2 = 50, GEOQIK_KEY_3 = 51,
    GEOQIK_KEY_4 = 52, GEOQIK_KEY_5 = 53, GEOQIK_KEY_6 = 54, GEOQIK_KEY_7 = 55,
    GEOQIK_KEY_8 = 56, GEOQIK_KEY_9 = 57, GEOQIK_KEY_SEMICOLON = 59,
    GEOQIK_KEY_EQUAL = 61,
    GEOQIK_KEY_A = 65, GEOQIK_KEY_B = 66, GEOQIK_KEY_C = 67, GEOQIK_KEY_D = 68,
    GEOQIK_KEY_E = 69, GEOQIK_KEY_F = 70, GEOQIK_KEY_G = 71, GEOQIK_KEY_H = 72,
    GEOQIK_KEY_I = 73, GEOQIK_KEY_J = 74, GEOQIK_KEY_K = 75, GEOQIK_KEY_L = 76,
    GEOQIK_KEY_M = 77, GEOQIK_KEY_N = 78, GEOQIK_KEY_O = 79, GEOQIK_KEY_P = 80,
    GEOQIK_KEY_Q = 81, GEOQIK_KEY_R = 82, GEOQIK_KEY_S = 83, GEOQIK_KEY_T = 84,
    GEOQIK_KEY_U = 85, GEOQIK_KEY_V = 86, GEOQIK_KEY_W = 87, GEOQIK_KEY_X = 88,
    GEOQIK_KEY_Y = 89, GEOQIK_KEY_Z = 90,
    GEOQIK_KEY_LEFT_BRACKET = 91, GEOQIK_KEY_BACKSLASH = 92,
    GEOQIK_KEY_RIGHT_BRACKET = 93, GEOQIK_KEY_GRAVE_ACCENT = 96,
    GEOQIK_KEY_WORLD_1 = 161, GEOQIK_KEY_WORLD_2 = 162,
    GEOQIK_KEY_ESCAPE = 256, GEOQIK_KEY_ENTER = 257, GEOQIK_KEY_TAB = 258,
    GEOQIK_KEY_BACKSPACE = 259, GEOQIK_KEY_INSERT = 260, GEOQIK_KEY_DELETE = 261,
    GEOQIK_KEY_RIGHT = 262, GEOQIK_KEY_LEFT = 263, GEOQIK_KEY_DOWN = 264,
    GEOQIK_KEY_UP = 265, GEOQIK_KEY_PAGE_UP = 266, GEOQIK_KEY_PAGE_DOWN = 267,
    GEOQIK_KEY_HOME = 268, GEOQIK_KEY_END = 269,
    GEOQIK_KEY_CAPS_LOCK = 280, GEOQIK_KEY_SCROLL_LOCK = 281,
    GEOQIK_KEY_NUM_LOCK = 282, GEOQIK_KEY_PRINT_SCREEN = 283, GEOQIK_KEY_PAUSE = 284,
    GEOQIK_KEY_F1  = 290, GEOQIK_KEY_F2  = 291, GEOQIK_KEY_F3  = 292,
    GEOQIK_KEY_F4  = 293, GEOQIK_KEY_F5  = 294, GEOQIK_KEY_F6  = 295,
    GEOQIK_KEY_F7  = 296, GEOQIK_KEY_F8  = 297, GEOQIK_KEY_F9  = 298,
    GEOQIK_KEY_F10 = 299, GEOQIK_KEY_F11 = 300, GEOQIK_KEY_F12 = 301,
    GEOQIK_KEY_F13 = 302, GEOQIK_KEY_F14 = 303, GEOQIK_KEY_F15 = 304,
    GEOQIK_KEY_F16 = 305, GEOQIK_KEY_F17 = 306, GEOQIK_KEY_F18 = 307,
    GEOQIK_KEY_F19 = 308, GEOQIK_KEY_F20 = 309, GEOQIK_KEY_F21 = 310,
    GEOQIK_KEY_F22 = 311, GEOQIK_KEY_F23 = 312, GEOQIK_KEY_F24 = 313,
    GEOQIK_KEY_F25 = 314,
    GEOQIK_KEY_KP_0 = 320, GEOQIK_KEY_KP_1 = 321, GEOQIK_KEY_KP_2 = 322,
    GEOQIK_KEY_KP_3 = 323, GEOQIK_KEY_KP_4 = 324, GEOQIK_KEY_KP_5 = 325,
    GEOQIK_KEY_KP_6 = 326, GEOQIK_KEY_KP_7 = 327, GEOQIK_KEY_KP_8 = 328,
    GEOQIK_KEY_KP_9 = 329, GEOQIK_KEY_KP_DECIMAL = 330, GEOQIK_KEY_KP_DIVIDE = 331,
    GEOQIK_KEY_KP_MULTIPLY = 332, GEOQIK_KEY_KP_SUBTRACT = 333,
    GEOQIK_KEY_KP_ADD = 334, GEOQIK_KEY_KP_ENTER = 335, GEOQIK_KEY_KP_EQUAL = 336,
    GEOQIK_KEY_LEFT_SHIFT = 340, GEOQIK_KEY_LEFT_CONTROL = 341,
    GEOQIK_KEY_LEFT_ALT = 342, GEOQIK_KEY_LEFT_SUPER = 343,
    GEOQIK_KEY_RIGHT_SHIFT = 344, GEOQIK_KEY_RIGHT_CONTROL = 345,
    GEOQIK_KEY_RIGHT_ALT = 346, GEOQIK_KEY_RIGHT_SUPER = 347,
    GEOQIK_KEY_MENU = 348
};

// ── Replay options ────────────────────────────────────────────────────────────
// Mirror of geoqik_replay_options_t from GeoQik.hpp — must be kept in sync.
// Zero values in all fields mean "use DLL default" (same semantics as null pointer).
struct geoqik_replay_options_t { // NOLINT(readability-identifier-naming)
    double              entriesPerSecond{};           // 0 = default 60.0
    double              speedMultiplier{};            // 0 = default 1.0
    std::size_t         maxEntriesPerFrame{};         // 0 = default 1024
    int                 startPaused{};                // 0 = play, non-zero = start paused
    std::size_t         entriesPerStep{};             // 0 = default 1
    const geoqik_key_t* stepKeys{};                  // null/empty = default: RIGHT, D
    std::size_t         stepKeyCount{};
    const geoqik_key_t* backwardStepKeys{};          // null/empty = default: LEFT, A
    std::size_t         backwardStepKeyCount{};
    const geoqik_key_t* resumeKeys{};                // null/empty = default: SPACE
    std::size_t         resumeKeyCount{};
    const geoqik_key_t* pauseKeys{};                 // null/empty = default: SPACE
    std::size_t         pauseKeyCount{};
    const geoqik_key_t* increaseEntriesPerStepKeys{};// null/empty = default: UP, W
    std::size_t         increaseEntriesPerStepKeyCount{};
    const geoqik_key_t* decreaseEntriesPerStepKeys{};// null/empty = default: DOWN, S
    std::size_t         decreaseEntriesPerStepKeyCount{};
};

// ── Replay state ──────────────────────────────────────────────────────────────
// Mirror of geoqik_replay_state_t from GeoQik.hpp — must be kept in sync.
enum geoqik_replay_state_t : int { // NOLINT(readability-identifier-naming,performance-enum-size)
    GEOQIK_REPLAY_INACTIVE = 0,
    GEOQIK_REPLAY_PLAYING  = 1,
    GEOQIK_REPLAY_PAUSED   = 2
};

namespace geoqik_client_impl {

enum class ClientDiagnosticId {
    Success,
    ServerExecutableNotFound,
    ServerStartFailed,
    IpcConnection,
    IpcWrite,
    IpcRead,
    Protocol,
    ServerApiFailure,
    Unknown,
};

struct ClientDiagnosticEntry {
    ClientDiagnosticId id;
    geoqik_client_error_code_t clientCode;
    const char* what;
    const char* why;
    const char* action;
};

inline constexpr ClientDiagnosticEntry clientDiagnosticCatalog[] = {
    {ClientDiagnosticId::Success, GEOQIK_CLIENT_SUCCESS, "", "", ""},
    {ClientDiagnosticId::ServerExecutableNotFound,
     GEOQIK_CLIENT_ERROR_SERVER_EXECUTABLE_NOT_FOUND,
     "GeoQik could not find the server executable.",
     "The client could not resolve GEOQIK_EXE_PATH and could not find geoqik on PATH.",
     "Set GEOQIK_EXE_PATH to the geoqik_server executable or add it to PATH."},
    {ClientDiagnosticId::ServerStartFailed,
     GEOQIK_CLIENT_ERROR_SERVER_START_FAILED,
     "GeoQik could not start the server process.",
     "The server executable was found, but the child process did not remain running.",
     "Check that the executable is valid for this platform and can load its runtime dependencies."},
    {ClientDiagnosticId::IpcConnection,
     GEOQIK_CLIENT_ERROR_IPC_CONNECTION,
     "GeoQik could not connect to the server IPC endpoint.",
     "The server did not accept a connection before the client timeout.",
     "Check that the server process is running and that the pipe/socket name is accessible."},
    {ClientDiagnosticId::IpcWrite,
     GEOQIK_CLIENT_ERROR_IPC_WRITE,
     "GeoQik could not write a command to the server.",
     "The IPC stream failed while sending the request header or payload.",
     "Retry after ensuring the server process is still running."},
    {ClientDiagnosticId::IpcRead,
     GEOQIK_CLIENT_ERROR_IPC_READ,
     "GeoQik could not read the server response.",
     "The IPC stream failed while reading the response header or diagnostic payload.",
     "Retry after ensuring the server process is still running."},
    {ClientDiagnosticId::Protocol,
     GEOQIK_CLIENT_ERROR_PROTOCOL,
     "GeoQik received an invalid server response.",
     "The response payload did not match the client/server protocol.",
     "Use matching GeoQik client and server builds."},
    {ClientDiagnosticId::ServerApiFailure,
     GEOQIK_CLIENT_SUCCESS,
     "The GeoQik server returned an API error.",
     "The command reached the server, but the server-side GeoQik API call failed.",
     "Inspect the returned API code and details, then adjust the API call."},
    {ClientDiagnosticId::Unknown,
     GEOQIK_CLIENT_ERROR_UNKNOWN,
     "GeoQik client failed unexpectedly.",
     "The client caught an exception that is not mapped to a more specific failure.",
     "Check the details field and report the call sequence if this is reproducible."},
};

struct ClientDiagnosticState {
    geoqik_client_error_code_t clientCode = GEOQIK_CLIENT_SUCCESS;
    geoqik_error_code_t code = GEOQIK_SUCCESS;
    std::string operation;
    std::string what;
    std::string why;
    std::string action;
    std::string details;
};

[[nodiscard]] inline const ClientDiagnosticEntry& client_entry(ClientDiagnosticId id) {
    for (const ClientDiagnosticEntry& entry : clientDiagnosticCatalog) {
        if (entry.id == id) {
            return entry;
        }
    }
    return clientDiagnosticCatalog[0];
}

[[nodiscard]] inline ClientDiagnosticState& last_client_error_storage() {
    thread_local ClientDiagnosticState state; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return state;
}

inline void clear_last_client_error() {
    last_client_error_storage() = ClientDiagnosticState{};
}

inline void set_client_error(
    ClientDiagnosticId id,
    const char* operation,
    const char* details = "",
    geoqik_error_code_t apiCode = GEOQIK_ERROR_UNKNOWN)
{
    const ClientDiagnosticEntry& entry = client_entry(id);
    ClientDiagnosticState state;
    state.clientCode = entry.clientCode;
    state.code = apiCode;
    state.operation = operation != nullptr ? operation : "";
    state.what = entry.what;
    state.why = entry.why;
    state.action = entry.action;
    state.details = details != nullptr ? details : "";
    last_client_error_storage() = std::move(state);
}

inline void set_server_response_error(const geoqik::protocol::ResponseFrame& response, const char* fallbackOperation) {
    const auto apiCode = static_cast<geoqik_error_code_t>(response.errorCode);
    if (apiCode == GEOQIK_SUCCESS) {
        clear_last_client_error();
        return;
    }

    ClientDiagnosticState state;
    state.clientCode = GEOQIK_CLIENT_SUCCESS;
    state.code = apiCode;
    state.operation = !response.diagnostic.operation.empty() ? response.diagnostic.operation : (fallbackOperation != nullptr ? fallbackOperation : "");
    state.what = !response.diagnostic.what.empty() ? response.diagnostic.what : client_entry(ClientDiagnosticId::ServerApiFailure).what;
    state.why = !response.diagnostic.why.empty() ? response.diagnostic.why : client_entry(ClientDiagnosticId::ServerApiFailure).why;
    state.action = !response.diagnostic.action.empty() ? response.diagnostic.action : client_entry(ClientDiagnosticId::ServerApiFailure).action;
    state.details = response.diagnostic.details;
    last_client_error_storage() = std::move(state);
}

[[nodiscard]] inline geoqik_error_code_t fallback_api_code(ClientDiagnosticId id) {
    switch (id) {
    case ClientDiagnosticId::ServerExecutableNotFound:
    case ClientDiagnosticId::ServerStartFailed:
    case ClientDiagnosticId::IpcConnection:
    case ClientDiagnosticId::IpcWrite:
    case ClientDiagnosticId::IpcRead:
    case ClientDiagnosticId::Protocol:
    case ClientDiagnosticId::Unknown:
    case ClientDiagnosticId::ServerApiFailure:
    case ClientDiagnosticId::Success:
    default:
        return GEOQIK_ERROR_UNKNOWN;
    }
}

inline geoqik_error_code_t fail_client(ClientDiagnosticId id, const char* operation, const char* details = "") {
    const geoqik_error_code_t code = fallback_api_code(id);
    set_client_error(id, operation, details, code);
    return code;
}

inline geoqik_result_t fail_client_result(ClientDiagnosticId id, const char* operation, const char* details = "") {
    return geoqik_result_t{fail_client(id, operation, details), {}};
}

template<typename Func>
[[nodiscard]] inline auto execute_client_call(const char* operation, Func&& func) -> decltype(func()) {
    try {
        return func();
    } catch (const geoqik::client::detail::ServerExecutableNotFoundError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::ServerExecutableNotFound, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::ServerExecutableNotFound, operation, e.what());
        }
    } catch (const geoqik::client::detail::ServerStartError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::ServerStartFailed, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::ServerStartFailed, operation, e.what());
        }
    } catch (const geoqik::client::detail::IpcConnectionError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::IpcConnection, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::IpcConnection, operation, e.what());
        }
    } catch (const geoqik::client::detail::IpcWriteError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::IpcWrite, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::IpcWrite, operation, e.what());
        }
    } catch (const geoqik::client::detail::ProtocolError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::Protocol, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::Protocol, operation, e.what());
        }
    } catch (const geoqik::client::detail::IpcReadError& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::IpcRead, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::IpcRead, operation, e.what());
        }
    } catch (const std::exception& e) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::Unknown, operation, e.what());
        } else {
            return fail_client(ClientDiagnosticId::Unknown, operation, e.what());
        }
    } catch (...) {
        if constexpr (std::is_same_v<decltype(func()), geoqik_result_t>) {
            return fail_client_result(ClientDiagnosticId::Unknown, operation);
        } else {
            return fail_client(ClientDiagnosticId::Unknown, operation);
        }
    }
}

[[nodiscard]] inline const std::string& pipe_name() {
    return geoqik::client::detail::ProcessManager::instance().pipe_name();
}

[[nodiscard]] inline geoqik::protocol::ResponseFrame call(
    geoqik::protocol::CommandId cmd,
    const std::vector<std::uint8_t>& payload = {})
{
    geoqik::client::detail::ensure_connected(pipe_name());
    return geoqik::client::detail::thread_connection().send_recv(cmd, payload);
}

[[nodiscard]] inline geoqik_error_code_t call_terminating(
    geoqik::protocol::CommandId cmd,
    const char* operation)
{
    auto& pm = geoqik::client::detail::ProcessManager::instance();
    if (!pm.is_running()) {
        geoqik::client::detail::disconnect_thread_connection();
        pm.wait_for_exit();
        return GEOQIK_SUCCESS;
    }
    const auto resp = call(cmd);
    geoqik::client::detail::disconnect_thread_connection();
    // The server exits after acknowledging a terminating command. Block until the
    // process is gone so the next geoqik_init() spawns a fresh server instead of
    // connecting to a leftover pipe instance of the one that is exiting.
    pm.wait_for_exit();
    set_server_response_error(resp, operation);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

inline void write_uuid(std::vector<std::uint8_t>& buf, const geoqik_uuid_t& uuid) {
    buf.insert(buf.end(), std::begin(uuid.value), std::end(uuid.value));
}

inline void append_colors(std::vector<std::uint8_t>& buf,
                           const float* color, std::size_t colorCount)
{
    namespace proto = geoqik::protocol;
    proto::write_optional_colors(buf, color, static_cast<std::uint32_t>(colorCount));
}

inline void encode_add_mesh_opts(std::vector<std::uint8_t>& payload,
                                  const geoqik_add_mesh_opts_t* opts,
                                  const float* vertices,  std::size_t vertexCount,
                                  const std::uint32_t* triangleIndices, std::size_t triangleCount)
{
    namespace proto = geoqik::protocol;
    const geoqik_uuid_t emptyKey{};
    const geoqik_uuid_t& key = (opts != nullptr) ? opts->idempotencyKey : emptyKey;
    write_uuid(payload, key);

    proto::write_pod(payload, static_cast<std::uint64_t>(vertexCount));
    proto::write_pod(payload, static_cast<std::uint64_t>(triangleCount));
    if (vertexCount > 0 && vertices != nullptr) {
        const auto* b = reinterpret_cast<const std::uint8_t*>(vertices);
        payload.insert(payload.end(), b, b + vertexCount * 3 * sizeof(float));
    }
    if (triangleCount > 0 && triangleIndices != nullptr) {
        const auto* b = reinterpret_cast<const std::uint8_t*>(triangleIndices);
        payload.insert(payload.end(), b, b + triangleCount * 3 * sizeof(std::uint32_t));
    }

    const float* normals    = (opts != nullptr) ? opts->normals : nullptr;
    const auto   normCount  = (opts != nullptr) ? static_cast<std::uint32_t>(opts->normalsCount) : 0u;
    proto::write_optional_float_array(payload, normals, normCount);

    const float* color      = (opts != nullptr) ? opts->color : nullptr;
    const auto   colorCount = (opts != nullptr) ? static_cast<std::uint32_t>(opts->colorCount) : 0u;
    proto::write_optional_float_array(payload, color, colorCount);

    const std::uint32_t* segIdx   = (opts != nullptr) ? opts->segmentIndices : nullptr;
    const auto segIdxCount = (opts != nullptr)
        ? static_cast<std::uint32_t>(opts->segmentIndexCount) : 0u;
    proto::write_optional_uint32_array(payload, segIdx, segIdxCount);

    const bool hasSegColor = (opts != nullptr && opts->segmentColor != nullptr);
    proto::write_optional_single_color(payload, (opts != nullptr) ? opts->segmentColor : nullptr,
        hasSegColor);

    const auto showSegs  = (opts != nullptr) ? static_cast<std::int32_t>(opts->showSegments) : 0;
    const auto segWidth  = (opts != nullptr) ? opts->segmentLineWidth : 0.0f;
    proto::write_pod(payload, showSegs);
    proto::write_pod(payload, segWidth);

    const bool hasVtxColor = (opts != nullptr && opts->vertexColor != nullptr);
    proto::write_optional_single_color(payload, (opts != nullptr) ? opts->vertexColor : nullptr,
        hasVtxColor);

    const auto showVerts = (opts != nullptr) ? static_cast<std::int32_t>(opts->showVertices) : 0;
    const auto vtxSize   = (opts != nullptr) ? opts->vertexPointSize : 0.0f;
    proto::write_pod(payload, showVerts);
    proto::write_pod(payload, vtxSize);
}

// Serialize a geoqik_replay_options_t into payload bytes.
// When opts is null, all fields are treated as zero (DLL default semantics).
inline void encode_replay_options(std::vector<std::uint8_t>& payload,
                                   const geoqik_replay_options_t* opts)
{
    namespace proto = geoqik::protocol;
    const geoqik_replay_options_t zero{};
    const geoqik_replay_options_t& o = (opts != nullptr) ? *opts : zero;

    proto::write_pod(payload, o.entriesPerSecond);
    proto::write_pod(payload, o.speedMultiplier);
    proto::write_pod(payload, static_cast<std::uint64_t>(o.maxEntriesPerFrame));
    proto::write_pod(payload, static_cast<std::int32_t>(o.startPaused));
    proto::write_pod(payload, static_cast<std::uint64_t>(o.entriesPerStep));

    auto write_key_array = [&](const geoqik_key_t* keys, std::size_t count) {
        proto::write_pod(payload, static_cast<std::uint32_t>(count));
        for (std::size_t i = 0; i < count; ++i) {
            proto::write_pod(payload, static_cast<std::int32_t>(keys[i]));
        }
    };

    write_key_array(o.stepKeys,                    o.stepKeyCount);
    write_key_array(o.backwardStepKeys,            o.backwardStepKeyCount);
    write_key_array(o.resumeKeys,                  o.resumeKeyCount);
    write_key_array(o.pauseKeys,                   o.pauseKeyCount);
    write_key_array(o.increaseEntriesPerStepKeys,  o.increaseEntriesPerStepKeyCount);
    write_key_array(o.decreaseEntriesPerStepKeys,  o.decreaseEntriesPerStepKeyCount);
}

} // namespace geoqik_client_impl

inline geoqik_error_code_t geoqik_client_get_last_error_info(geoqik_client_error_info_t* info) {
    if (info == nullptr || info->struct_size < sizeof(geoqik_client_error_info_t)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    const geoqik_client_impl::ClientDiagnosticState& state = geoqik_client_impl::last_client_error_storage();
    info->client_code = state.clientCode;
    info->code = state.code;
    info->operation = state.operation.c_str();
    info->what = state.what.c_str();
    info->why = state.why.c_str();
    info->action = state.action.c_str();
    info->details = state.details.c_str();
    return GEOQIK_SUCCESS;
}

inline void geoqik_client_clear_last_error() {
    geoqik_client_impl::clear_last_client_error();
}

[[nodiscard]] inline geoqik_error_code_t geoqik_init() {
    return geoqik_client_impl::execute_client_call("geoqik_init", []() -> geoqik_error_code_t {
        auto& pm = geoqik::client::detail::ProcessManager::instance();
        if (!pm.is_running()) {
            geoqik::client::detail::disconnect_thread_connection();
        }
        pm.start();
        geoqik::client::detail::ensure_connected(pm.pipe_name());
        geoqik_client_impl::clear_last_client_error();
        return GEOQIK_SUCCESS;
    });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_draw() {
    return geoqik_client_impl::execute_client_call("geoqik_draw", []() -> geoqik_error_code_t {
        namespace proto = geoqik::protocol;
        const auto resp = geoqik_client_impl::call(proto::CommandId::Draw);
        geoqik_client_impl::set_server_response_error(resp, "geoqik_draw");
        return static_cast<geoqik_error_code_t>(resp.errorCode);
    });
}

[[nodiscard]] inline geoqik_result_t geoqik_add_point(double x, double y, double z) {
    return geoqik_client_impl::execute_client_call("geoqik_add_point", [&]() -> geoqik_result_t {
        namespace proto = geoqik::protocol;
        std::vector<std::uint8_t> payload;
        payload.reserve(proto::pointPayloadByteCount);
        proto::write_pod(payload, x);
        proto::write_pod(payload, y);
        proto::write_pod(payload, z);

        const auto resp = geoqik_client_impl::call(proto::CommandId::AddPoint, payload);
        geoqik_client_impl::set_server_response_error(resp, "geoqik_add_point");

        geoqik_result_t result{};
        result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
        std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
        return result;
    });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_add_line(
    double x1, double y1, double z1,
    double x2, double y2, double z2)
{
    return geoqik_client_impl::execute_client_call("geoqik_add_line", [&]() -> geoqik_error_code_t {
        namespace proto = geoqik::protocol;
        std::vector<std::uint8_t> payload;
        payload.reserve(proto::linePayloadByteCount);
        proto::write_pod(payload, x1);
        proto::write_pod(payload, y1);
        proto::write_pod(payload, z1);
        proto::write_pod(payload, x2);
        proto::write_pod(payload, y2);
        proto::write_pod(payload, z2);

        const auto resp = geoqik_client_impl::call(proto::CommandId::AddLine, payload);
        geoqik_client_impl::set_server_response_error(resp, "geoqik_add_line");
        return static_cast<geoqik_error_code_t>(resp.errorCode);
    });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_wait_for_exit_and_cleanup() {
    return geoqik_client_impl::execute_client_call("geoqik_wait_for_exit_and_cleanup", []() -> geoqik_error_code_t {
        return geoqik_client_impl::call_terminating(
            geoqik::protocol::CommandId::WaitForExit, "geoqik_wait_for_exit_and_cleanup");
    });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_cleanup() {
    return geoqik_client_impl::execute_client_call("geoqik_cleanup", []() -> geoqik_error_code_t {
        return geoqik_client_impl::call_terminating(
            geoqik::protocol::CommandId::Cleanup, "geoqik_cleanup");
    });
}

inline void geoqik_create_default_settings(geoqik_settings_t* settings) {
    if (settings == nullptr) { return; }
    *settings = geoqik_settings_t{};
    settings->maxMessageQueueSize = 1024;
    settings->initialPointCapacity = 1024;
    settings->initialLineCapacity = 1024;
    settings->initialMeshCapacity = 1024;
    settings->defaultMeshColor[0] = 0.7f; settings->defaultMeshColor[1] = 0.7f;
    settings->defaultMeshColor[2] = 0.7f; settings->defaultMeshColor[3] = 1.0f;
    settings->meshHeadLightColor[0] = 1.0f; settings->meshHeadLightColor[1] = 1.0f;
    settings->meshHeadLightColor[2] = 1.0f;
    settings->meshHeadLightIntensity = 0.8f;
    settings->meshFillLightDirection[0] = 0.0f; settings->meshFillLightDirection[1] = 1.0f;
    settings->meshFillLightDirection[2] = 0.0f;
    settings->meshFillLightColor[0] = 1.0f; settings->meshFillLightColor[1] = 1.0f;
    settings->meshFillLightColor[2] = 1.0f;
    settings->meshFillLightIntensity = 0.3f;
    settings->meshAmbientColor[0] = 1.0f; settings->meshAmbientColor[1] = 1.0f;
    settings->meshAmbientColor[2] = 1.0f;
    settings->meshAmbientIntensity = 0.2f;
    settings->meshShininess = 32.0f;
    settings->capacityGrowthFactor = 2;
    settings->defaultPointSize = 5.0f;
    settings->defaultLineWidth = 1.0f;
    settings->defaultPointColor[0] = 1.0f; settings->defaultPointColor[1] = 1.0f;
    settings->defaultPointColor[2] = 1.0f; settings->defaultPointColor[3] = 1.0f;
    settings->defaultLineColor[0] = 1.0f; settings->defaultLineColor[1] = 1.0f;
    settings->defaultLineColor[2] = 1.0f; settings->defaultLineColor[3] = 1.0f;
    settings->backgroundColor[0] = 0.1f; settings->backgroundColor[1] = 0.1f;
    settings->backgroundColor[2] = 0.1f; settings->backgroundColor[3] = 1.0f;
    settings->cameraFarPlaneMultiplier = 100.0;
    settings->autoFitCameraEnabled = 1;
    settings->autoFitZoomInEnabled = 1;
    settings->autoFitZoomOutPadding = 0.1;
    settings->autoFitMinViewportOccupancy = 0.1;
    settings->autoFitTargetViewportOccupancy = 0.7;
    settings->autoFitSuppressAfterUserCameraInteractionMs = 2000;
    settings->minGeometryProcessingTimeMs = 1;
    settings->maxFrameProcessingTimeMs = 16;
    settings->updateSceneFrequency = 1;
}

inline void geoqik_init_default_window_settings(geoqik_window_settings_t* settings) {
    if (settings == nullptr) { return; }
    *settings = geoqik_window_settings_t{};
    settings->title = "GeoQik";
    settings->width = 1280;
    settings->height = 720;
    settings->red_bits = 8;
    settings->green_bits = 8;
    settings->blue_bits = 8;
    settings->alpha_bits = 8;
    settings->depth_bits = 24;
    settings->stencil_bits = 8;
    settings->accum_red_bits = 0;
    settings->accum_green_bits = 0;
    settings->accum_blue_bits = 0;
    settings->accum_alpha_bits = 0;
    settings->aux_buffers = 0;
    settings->samples = 4;
    settings->refresh_rate = -1;
    settings->stereo = 0;
    settings->srgb_capable = 0;
    settings->double_buffer = 1;
    settings->resizable = 1;
    settings->visible = 1;
    settings->decorated = 1;
    settings->focused = 1;
    settings->auto_iconify = 1;
    settings->floating = 0;
    settings->maximized = 0;
    settings->center_cursor = 1;
    settings->transparent_framebuffer = 0;
    settings->focus_on_show = 1;
    settings->scale_to_monitor = 0;
}

[[nodiscard]] inline const char* geoqik_get_error_string(geoqik_error_code_t result) {
    switch (result) {
    case GEOQIK_SUCCESS:                    return "Success";
    case GEOQIK_ERROR_NOT_INITIALIZED:      return "GeoQik not initialized";
    case GEOQIK_ERROR_ALREADY_INITIALIZED:  return "GeoQik already initialized";
    case GEOQIK_ERROR_INVALID_PARAMETER:    return "Invalid parameter";
    case GEOQIK_ERROR_WRONG_COLOR_SIZE:     return "Wrong RGBA color size";
    case GEOQIK_ERROR_MEMORY_ALLOCATION:    return "Memory allocation error";
    case GEOQIK_ERROR_UNKNOWN:              return "Unknown error";
    case GEOQIK_ERROR_RENDERER_INIT_FAILED: return "Renderer initialization failed";
    case GEOQIK_ERROR_IO:                   return "I/O error";
    case GEOQIK_ERROR_UNSUPPORTED_FORMAT:   return "Unsupported format";
    case GEOQIK_ERROR_INVALID_STATE:        return "Invalid state";
    default:                                return "Invalid error code";
    }
}

[[nodiscard]] inline geoqik_error_code_t geoqik_generate_uuid(geoqik_uuid_t* uuid) {
    if (uuid == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    // Generate a pseudo-random UUID v4.
    thread_local std::mt19937_64 rng{std::random_device{}()}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::uniform_int_distribution<std::uint32_t> dist(0, 255);
    for (auto& byte : uuid->value) { byte = static_cast<std::uint8_t>(dist(rng)); }
    // Set version 4 and variant bits
    uuid->value[6] = (uuid->value[6] & 0x0F) | 0x40;
    uuid->value[8] = (uuid->value[8] & 0x3F) | 0x80;
    return GEOQIK_SUCCESS;
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_last_error_info(geoqik_error_info_t* info) {
    if (info == nullptr || info->struct_size < sizeof(geoqik_error_info_t)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    const geoqik_client_impl::ClientDiagnosticState& state = geoqik_client_impl::last_client_error_storage();
    info->code = state.code;
    info->operation = state.operation.c_str();
    info->what = state.what.c_str();
    info->why = state.why.c_str();
    info->action = state.action.c_str();
    info->details = state.details.c_str();
    return GEOQIK_SUCCESS;
}

inline void geoqik_clear_last_error() {
    geoqik_client_impl::clear_last_client_error();
}

[[nodiscard]] inline geoqik_error_code_t geoqik_init_with_settings(
    const geoqik_settings_t* settings,
    const geoqik_window_settings_t* windowSettings)
{
    return geoqik_client_impl::execute_client_call("geoqik_init_with_settings",
        [&]() -> geoqik_error_code_t {
            auto& pm = geoqik::client::detail::ProcessManager::instance();
            if (!pm.is_running()) {
                geoqik::client::detail::disconnect_thread_connection();
            }
            if (settings != nullptr && windowSettings != nullptr) {
                pm.set_settings(*settings, *windowSettings);
            }
            pm.start();
            geoqik::client::detail::ensure_connected(pm.pipe_name());
            geoqik_client_impl::clear_last_client_error();
            return GEOQIK_SUCCESS;
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized) {
    return geoqik_client_impl::execute_client_call("geoqik_is_api_initialized",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::IsApiInitialized);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_is_api_initialized");
            if (isInitialized != nullptr && resp.errorCode == GEOQIK_SUCCESS) {
                *isInitialized = (proto::unpack_return_value<std::uint8_t>(resp.uuid) != 0);
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_stop_drawing() {
    return geoqik_client_impl::execute_client_call("geoqik_stop_drawing",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::StopDrawing);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_stop_drawing");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_point_size(float pointSize) {
    return geoqik_client_impl::execute_client_call("geoqik_set_point_size",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::floatPayloadByteCount);
            proto::write_pod(payload, pointSize);
            const auto resp = geoqik_client_impl::call(proto::CommandId::SetPointSize, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_point_size");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_point_size(float* pointSize) {
    return geoqik_client_impl::execute_client_call("geoqik_get_point_size",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload(proto::floatPayloadByteCount, 0);
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetPointSize, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_point_size");
            if (pointSize != nullptr && resp.errorCode == GEOQIK_SUCCESS) {
                *pointSize = proto::unpack_return_value<float>(resp.uuid);
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_point_color(float r, float g, float b, float a) {
    return geoqik_client_impl::execute_client_call("geoqik_set_point_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::colorPayloadByteCount);
            proto::write_pod(payload, r);
            proto::write_pod(payload, g);
            proto::write_pod(payload, b);
            proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(proto::CommandId::SetPointColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_point_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_point_color(float* r, float* g, float* b, float* a) {
    return geoqik_client_impl::execute_client_call("geoqik_get_point_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload(proto::colorPayloadByteCount, 0);
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetPointColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_point_color");
            if (resp.errorCode == GEOQIK_SUCCESS) {
                std::array<float, 4> rgba{};
                std::memcpy(rgba.data(), resp.uuid.data(), sizeof(rgba));
                if (r) *r = rgba[0];
                if (g) *g = rgba[1];
                if (b) *b = rgba[2];
                if (a) *a = rgba[3];
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_line_width(float lineWidth) {
    return geoqik_client_impl::execute_client_call("geoqik_set_line_width",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::floatPayloadByteCount);
            proto::write_pod(payload, lineWidth);
            const auto resp = geoqik_client_impl::call(proto::CommandId::SetLineWidth, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_line_width");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_line_width(float* lineWidth) {
    return geoqik_client_impl::execute_client_call("geoqik_get_line_width",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload(proto::floatPayloadByteCount, 0);
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetLineWidth, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_line_width");
            if (lineWidth != nullptr && resp.errorCode == GEOQIK_SUCCESS) {
                *lineWidth = proto::unpack_return_value<float>(resp.uuid);
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_line_color(float r, float g, float b, float a) {
    return geoqik_client_impl::execute_client_call("geoqik_set_line_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::colorPayloadByteCount);
            proto::write_pod(payload, r);
            proto::write_pod(payload, g);
            proto::write_pod(payload, b);
            proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(proto::CommandId::SetLineColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_line_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_line_color(float* r, float* g, float* b, float* a) {
    return geoqik_client_impl::execute_client_call("geoqik_get_line_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload(proto::colorPayloadByteCount, 0);
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetLineColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_line_color");
            if (resp.errorCode == GEOQIK_SUCCESS) {
                std::array<float, 4> rgba{};
                std::memcpy(rgba.data(), resp.uuid.data(), sizeof(rgba));
                if (r) *r = rgba[0];
                if (g) *g = rgba[1];
                if (b) *b = rgba[2];
                if (a) *a = rgba[3];
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_mesh_color(float r, float g, float b, float a) {
    return geoqik_client_impl::execute_client_call("geoqik_set_mesh_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::colorPayloadByteCount);
            proto::write_pod(payload, r);
            proto::write_pod(payload, g);
            proto::write_pod(payload, b);
            proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(proto::CommandId::SetMeshColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_mesh_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_mesh_color(float* r, float* g, float* b, float* a) {
    return geoqik_client_impl::execute_client_call("geoqik_get_mesh_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload(proto::colorPayloadByteCount, 0);
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetMeshColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_mesh_color");
            if (resp.errorCode == GEOQIK_SUCCESS) {
                std::array<float, 4> rgba{};
                std::memcpy(rgba.data(), resp.uuid.data(), sizeof(rgba));
                if (r) *r = rgba[0];
                if (g) *g = rgba[1];
                if (b) *b = rgba[2];
                if (a) *a = rgba[3];
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_remove_all_geometry() {
    return geoqik_client_impl::execute_client_call("geoqik_remove_all_geometry",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::RemoveAllGeometry);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_remove_all_geometry");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_translate_geometry(
    const geoqik_uuid_t* geometryId, double dx, double dy, double dz)
{
    return geoqik_client_impl::execute_client_call("geoqik_translate_geometry",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::translatePayloadByteCount);
            if (geometryId != nullptr) {
                geoqik_client_impl::write_uuid(payload, *geometryId);
            } else {
                payload.insert(payload.end(), proto::uuidByteCount, 0);
            }
            proto::write_pod(payload, dx);
            proto::write_pod(payload, dy);
            proto::write_pod(payload, dz);
            const auto resp = geoqik_client_impl::call(proto::CommandId::TranslateGeometry, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_translate_geometry");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_rotate_geometry(
    const geoqik_uuid_t* geometryId,
    double centerX, double centerY, double centerZ,
    double axisX, double axisY, double axisZ,
    double angle)
{
    return geoqik_client_impl::execute_client_call("geoqik_rotate_geometry",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::rotatePayloadByteCount);
            if (geometryId != nullptr) {
                geoqik_client_impl::write_uuid(payload, *geometryId);
            } else {
                payload.insert(payload.end(), proto::uuidByteCount, 0);
            }
            proto::write_pod(payload, centerX);
            proto::write_pod(payload, centerY);
            proto::write_pod(payload, centerZ);
            proto::write_pod(payload, axisX);
            proto::write_pod(payload, axisY);
            proto::write_pod(payload, axisZ);
            proto::write_pod(payload, angle);
            const auto resp = geoqik_client_impl::call(proto::CommandId::RotateGeometry, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_rotate_geometry");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

// ── Extended point API ────────────────────────────────────────────────────────

[[nodiscard]] inline geoqik_result_t geoqik_add_point_with_color(
    double x, double y, double z, float r, float g, float b, float a)
{
    return geoqik_client_impl::execute_client_call("geoqik_add_point_with_color",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::addPointWithColorPayloadByteCount);
            proto::write_pod(payload, x); proto::write_pod(payload, y); proto::write_pod(payload, z);
            proto::write_pod(payload, r); proto::write_pod(payload, g);
            proto::write_pod(payload, b); proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddPointWithColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_point_with_color");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_result_t geoqik_add_point_opts(
    double x, double y, double z, geoqik_add_points_options_t* options)
{
    return geoqik_client_impl::execute_client_call("geoqik_add_point_opts",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            const geoqik_uuid_t emptyKey{};
            const geoqik_uuid_t& key = (options != nullptr) ? options->idempotencyKey : emptyKey;
            geoqik_client_impl::write_uuid(payload, key);
            proto::write_pod(payload, x); proto::write_pod(payload, y); proto::write_pod(payload, z);
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddPointOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_point_opts");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_result_t geoqik_add_points_opts(
    const double* points, std::size_t size, geoqik_add_points_options_t* options)
{
    if (points == nullptr || size == 0 || size % 3 != 0) {
        return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    return geoqik_client_impl::execute_client_call("geoqik_add_points_opts",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            const std::uint64_t pointCount = static_cast<std::uint64_t>(size / 3);
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::uuidByteCount + sizeof(std::uint64_t)
                            + size * sizeof(double) + sizeof(std::uint32_t));
            const geoqik_uuid_t emptyKey{};
            const geoqik_uuid_t& key = (options != nullptr) ? options->idempotencyKey : emptyKey;
            geoqik_client_impl::write_uuid(payload, key);
            proto::write_pod(payload, pointCount);
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(points);
            payload.insert(payload.end(), bytes, bytes + size * sizeof(double));
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddPointsOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_points_opts");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_point(
    const geoqik_uuid_t* geometryId, double x, double y, double z)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_point",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::updatePointPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x); proto::write_pod(payload, y); proto::write_pod(payload, z);
            const auto resp = geoqik_client_impl::call(proto::CommandId::UpdatePoint, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_point");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_point_with_color(
    const geoqik_uuid_t* geometryId,
    double x, double y, double z,
    float r, float g, float b, float a)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_point_with_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::updatePointWithColorPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x); proto::write_pod(payload, y); proto::write_pod(payload, z);
            proto::write_pod(payload, r); proto::write_pod(payload, g);
            proto::write_pod(payload, b); proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::UpdatePointWithColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_point_with_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_point_opts(
    const geoqik_uuid_t* geometryId, double x, double y, double z,
    geoqik_update_points_options_t* options)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_point_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x); proto::write_pod(payload, y); proto::write_pod(payload, z);
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::UpdatePointOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_point_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_points_opts(
    const geoqik_uuid_t* geometryId, const double* points, std::size_t size,
    geoqik_update_points_options_t* options)
{
    if (geometryId == nullptr || points == nullptr || size == 0 || size % 3 != 0) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    return geoqik_client_impl::execute_client_call("geoqik_update_points_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const std::uint64_t pointCount = static_cast<std::uint64_t>(size / 3);
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, pointCount);
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(points);
            payload.insert(payload.end(), bytes, bytes + size * sizeof(double));
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::UpdatePointsOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_points_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_remove_point",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::removeGeometryPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            const auto resp = geoqik_client_impl::call(proto::CommandId::RemovePoint, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_remove_point");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

// ── Extended line API ─────────────────────────────────────────────────────────

[[nodiscard]] inline geoqik_error_code_t geoqik_add_line_with_color(
    double x1, double y1, double z1, double x2, double y2, double z2,
    float r, float g, float b, float a)
{
    return geoqik_client_impl::execute_client_call("geoqik_add_line_with_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::addLineWithColorPayloadByteCount);
            proto::write_pod(payload, x1); proto::write_pod(payload, y1); proto::write_pod(payload, z1);
            proto::write_pod(payload, x2); proto::write_pod(payload, y2); proto::write_pod(payload, z2);
            proto::write_pod(payload, r);  proto::write_pod(payload, g);
            proto::write_pod(payload, b);  proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddLineWithColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_line_with_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_result_t geoqik_add_line_opts(
    double x1, double y1, double z1, double x2, double y2, double z2,
    geoqik_add_line_opts_t* options)
{
    return geoqik_client_impl::execute_client_call("geoqik_add_line_opts",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            const geoqik_uuid_t emptyKey{};
            const geoqik_uuid_t& key = (options != nullptr) ? options->idempotencyKey : emptyKey;
            geoqik_client_impl::write_uuid(payload, key);
            proto::write_pod(payload, x1); proto::write_pod(payload, y1); proto::write_pod(payload, z1);
            proto::write_pod(payload, x2); proto::write_pod(payload, y2); proto::write_pod(payload, z2);
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddLineOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_line_opts");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_result_t geoqik_add_lines_opts(
    const double* lines, std::size_t size, geoqik_add_line_opts_t* options)
{
    if (lines == nullptr || size == 0 || size % 6 != 0) {
        return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    return geoqik_client_impl::execute_client_call("geoqik_add_lines_opts",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            const std::uint64_t lineCount = static_cast<std::uint64_t>(size / 6);
            std::vector<std::uint8_t> payload;
            const geoqik_uuid_t emptyKey{};
            const geoqik_uuid_t& key = (options != nullptr) ? options->idempotencyKey : emptyKey;
            geoqik_client_impl::write_uuid(payload, key);
            proto::write_pod(payload, lineCount);
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(lines);
            payload.insert(payload.end(), bytes, bytes + size * sizeof(double));
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddLinesOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_lines_opts");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_line(
    const geoqik_uuid_t* geometryId,
    double x1, double y1, double z1, double x2, double y2, double z2)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_line",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::updateLinePayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x1); proto::write_pod(payload, y1); proto::write_pod(payload, z1);
            proto::write_pod(payload, x2); proto::write_pod(payload, y2); proto::write_pod(payload, z2);
            const auto resp = geoqik_client_impl::call(proto::CommandId::UpdateLine, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_line");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_line_with_color(
    const geoqik_uuid_t* geometryId,
    double x1, double y1, double z1, double x2, double y2, double z2,
    float r, float g, float b, float a)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_line_with_color",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::updateLineWithColorPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x1); proto::write_pod(payload, y1); proto::write_pod(payload, z1);
            proto::write_pod(payload, x2); proto::write_pod(payload, y2); proto::write_pod(payload, z2);
            proto::write_pod(payload, r);  proto::write_pod(payload, g);
            proto::write_pod(payload, b);  proto::write_pod(payload, a);
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::UpdateLineWithColor, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_line_with_color");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_line_opts(
    const geoqik_uuid_t* geometryId,
    double x1, double y1, double z1, double x2, double y2, double z2,
    geoqik_update_line_opts_t* options)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_line_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, x1); proto::write_pod(payload, y1); proto::write_pod(payload, z1);
            proto::write_pod(payload, x2); proto::write_pod(payload, y2); proto::write_pod(payload, z2);
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::UpdateLineOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_line_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_lines_opts(
    const geoqik_uuid_t* geometryId, const double* lines, std::size_t size,
    geoqik_update_line_opts_t* options)
{
    if (geometryId == nullptr || lines == nullptr || size == 0 || size % 6 != 0) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    return geoqik_client_impl::execute_client_call("geoqik_update_lines_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const std::uint64_t lineCount = static_cast<std::uint64_t>(size / 6);
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, lineCount);
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(lines);
            payload.insert(payload.end(), bytes, bytes + size * sizeof(double));
            const float* col    = (options != nullptr) ? options->color      : nullptr;
            const auto colCount = (options != nullptr) ? options->colorCount : 0;
            geoqik_client_impl::append_colors(payload, col, colCount);
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::UpdateLinesOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_lines_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_remove_line",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::removeGeometryPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            const auto resp = geoqik_client_impl::call(proto::CommandId::RemoveLine, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_remove_line");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

// ── Mesh API ──────────────────────────────────────────────────────────────────

[[nodiscard]] inline geoqik_result_t geoqik_add_mesh_opts(
    const float* vertices, std::size_t vertexCount,
    const std::uint32_t* triangleIndices, std::size_t triangleCount,
    geoqik_add_mesh_opts_t* options)
{
    if (vertices == nullptr && vertexCount > 0) {
        return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    if (triangleIndices == nullptr && triangleCount > 0) {
        return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    return geoqik_client_impl::execute_client_call("geoqik_add_mesh_opts",
        [&]() -> geoqik_result_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::encode_add_mesh_opts(payload, options,
                vertices, vertexCount, triangleIndices, triangleCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::AddMeshOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_add_mesh_opts");
            geoqik_result_t result{};
            result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
            std::copy(resp.uuid.begin(), resp.uuid.end(), std::begin(result.geometryId.value));
            return result;
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_remove_mesh(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_remove_mesh",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::uuidByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            const auto resp = geoqik_client_impl::call(proto::CommandId::RemoveMesh, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_remove_mesh");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_update_mesh_opts(
    const geoqik_uuid_t* geometryId,
    const float* vertices, std::size_t vertexCount,
    geoqik_update_mesh_opts_t* options)
{
    if (geometryId == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    if (vertices == nullptr && vertexCount > 0) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_update_mesh_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, static_cast<std::uint64_t>(vertexCount));
            if (vertexCount > 0 && vertices != nullptr) {
                const auto* b = reinterpret_cast<const std::uint8_t*>(vertices);
                payload.insert(payload.end(), b, b + vertexCount * 3 * sizeof(float));
            }
            const float* normals   = (options != nullptr) ? options->normals : nullptr;
            const auto normCount   = (options != nullptr)
                ? static_cast<std::uint32_t>(options->normalsCount) : 0u;
            proto::write_optional_float_array(payload, normals, normCount);
            const float* color     = (options != nullptr) ? options->color : nullptr;
            const auto colorCount  = (options != nullptr)
                ? static_cast<std::uint32_t>(options->colorCount) : 0u;
            proto::write_optional_float_array(payload, color, colorCount);
            const auto resp = geoqik_client_impl::call(proto::CommandId::UpdateMeshOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_update_mesh_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_mesh_overlay_opts(
    const geoqik_uuid_t* geometryId, const geoqik_mesh_overlay_opts_t* opts)
{
    if (geometryId == nullptr || opts == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_set_mesh_overlay_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::setMeshOverlayOptsPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, static_cast<std::int32_t>(opts->showSegments));
            proto::write_pod(payload, static_cast<std::int32_t>(opts->showVertices));
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::SetMeshOverlayOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_mesh_overlay_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_set_mesh_rendering_opts(
    const geoqik_uuid_t* geometryId, const geoqik_mesh_rendering_opts_t* options)
{
    if (geometryId == nullptr || options == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_set_mesh_rendering_opts",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::setMeshRenderingOptsPayloadByteCount);
            geoqik_client_impl::write_uuid(payload, *geometryId);
            proto::write_pod(payload, static_cast<std::uint32_t>(options->cullMode));
            proto::write_pod(payload, static_cast<std::int32_t>(options->surfaceVisible));
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::SetMeshRenderingOpts, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_set_mesh_rendering_opts");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

// ── Log & replay API ──────────────────────────────────────────────────────────

[[nodiscard]] inline geoqik_error_code_t geoqik_save_log(
    const char* path, geoqik_log_format_t format)
{
    if (path == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_save_log",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            proto::write_string(payload, std::string(path));
            proto::write_pod(payload, static_cast<std::int32_t>(format));
            const auto resp = geoqik_client_impl::call(proto::CommandId::SaveLog, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_save_log");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_load_log(
    const char* path, geoqik_log_format_t format)
{
    if (path == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_load_log",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            proto::write_string(payload, std::string(path));
            proto::write_pod(payload, static_cast<std::int32_t>(format));
            const auto resp = geoqik_client_impl::call(proto::CommandId::LoadLog, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_load_log");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_replay_log(
    const char* path, geoqik_log_format_t format,
    const geoqik_replay_options_t* options)
{
    if (path == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_replay_log",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            proto::write_string(payload, std::string(path));
            proto::write_pod(payload, static_cast<std::int32_t>(format));
            geoqik_client_impl::encode_replay_options(payload, options);
            const auto resp = geoqik_client_impl::call(proto::CommandId::ReplayLog, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_replay_log");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_replay_current_log(
    const geoqik_replay_options_t* options)
{
    return geoqik_client_impl::execute_client_call("geoqik_replay_current_log",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            geoqik_client_impl::encode_replay_options(payload, options);
            const auto resp = geoqik_client_impl::call(proto::CommandId::ReplayCurrentLog, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_replay_current_log");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_cancel_replay() {
    return geoqik_client_impl::execute_client_call("geoqik_cancel_replay",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::CancelReplay);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_cancel_replay");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_pause_replay() {
    return geoqik_client_impl::execute_client_call("geoqik_pause_replay",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::PauseReplay);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_pause_replay");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_resume_replay() {
    return geoqik_client_impl::execute_client_call("geoqik_resume_replay",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::ResumeReplay);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_resume_replay");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_step_replay() {
    return geoqik_client_impl::execute_client_call("geoqik_step_replay",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::StepReplay);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_step_replay");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_step_replay_n(std::size_t count) {
    return geoqik_client_impl::execute_client_call("geoqik_step_replay_n",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::stepReplayNPayloadByteCount);
            proto::write_pod(payload, static_cast<std::uint64_t>(count));
            const auto resp = geoqik_client_impl::call(proto::CommandId::StepReplayN, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_step_replay_n");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_step_replay_backward() {
    return geoqik_client_impl::execute_client_call("geoqik_step_replay_backward",
        []() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::StepReplayBackward);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_step_replay_backward");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_step_replay_backward_n(std::size_t count) {
    return geoqik_client_impl::execute_client_call("geoqik_step_replay_backward_n",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            std::vector<std::uint8_t> payload;
            payload.reserve(proto::stepReplayNPayloadByteCount);
            proto::write_pod(payload, static_cast<std::uint64_t>(count));
            const auto resp = geoqik_client_impl::call(
                proto::CommandId::StepReplayBackwardN, payload);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_step_replay_backward_n");
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_replay_state(
    geoqik_replay_state_t* state)
{
    if (state == nullptr) { return GEOQIK_ERROR_INVALID_PARAMETER; }
    return geoqik_client_impl::execute_client_call("geoqik_get_replay_state",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetReplayState);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_replay_state");
            if (resp.errorCode == GEOQIK_SUCCESS) {
                *state = static_cast<geoqik_replay_state_t>(
                    proto::unpack_return_value<std::uint32_t>(resp.uuid));
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

[[nodiscard]] inline geoqik_error_code_t geoqik_get_replay_progress(
    std::size_t* currentEntry, std::size_t* totalEntries)
{
    if (currentEntry == nullptr || totalEntries == nullptr) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    return geoqik_client_impl::execute_client_call("geoqik_get_replay_progress",
        [&]() -> geoqik_error_code_t {
            namespace proto = geoqik::protocol;
            const auto resp = geoqik_client_impl::call(proto::CommandId::GetReplayProgress);
            geoqik_client_impl::set_server_response_error(resp, "geoqik_get_replay_progress");
            if (resp.errorCode == GEOQIK_SUCCESS) {
                std::uint64_t curU = 0, totU = 0;
                std::memcpy(&curU, resp.uuid.data(),     sizeof(curU));
                std::memcpy(&totU, resp.uuid.data() + 8, sizeof(totU));
                *currentEntry = static_cast<std::size_t>(curU);
                *totalEntries = static_cast<std::size_t>(totU);
            }
            return static_cast<geoqik_error_code_t>(resp.errorCode);
        });
}

#endif // __cplusplus

#endif // GEOQIKCLIENT_HPP
