#ifndef GEOQIKCLIENT_HPP
#define GEOQIKCLIENT_HPP

#ifdef __cplusplus
#  include <array>
#  include <cstddef>
#  include <cstdint>
#else
#  include <stdbool.h>
#  include <stddef.h>
#  include <stdint.h>
#endif

#ifdef __cplusplus
inline constexpr std::size_t geoqikUuidByteCount = 16;

struct geoqik_uuid_t { // NOLINT(readability-identifier-naming)
    std::array<std::uint8_t, geoqikUuidByteCount> value{};
};
#else
typedef struct { uint8_t value[16]; } geoqik_uuid_t;
#endif

#ifdef __cplusplus
enum geoqik_error_code_t : int { // NOLINT(performance-enum-size,readability-identifier-naming): matches the C API error-code ABI.
    GEOQIK_SUCCESS                   = 0,
    GEOQIK_ERROR_NOT_INITIALIZED     = 1,
    GEOQIK_ERROR_ALREADY_INITIALIZED = 2,
    GEOQIK_ERROR_INVALID_PARAMETER   = 3,
    GEOQIK_ERROR_WRONG_COLOR_SIZE    = 4,
    GEOQIK_ERROR_MEMORY_ALLOCATION   = 5,
    GEOQIK_ERROR_UNKNOWN             = 6,
    GEOQIK_ERROR_RENDERER_INIT_FAILED = 7,
    GEOQIK_ERROR_IO                  = 8,
    GEOQIK_ERROR_UNSUPPORTED_FORMAT  = 9,
    GEOQIK_ERROR_INVALID_STATE       = 10
};
#else
typedef enum {
    GEOQIK_SUCCESS                   = 0,
    GEOQIK_ERROR_NOT_INITIALIZED     = 1,
    GEOQIK_ERROR_ALREADY_INITIALIZED = 2,
    GEOQIK_ERROR_INVALID_PARAMETER   = 3,
    GEOQIK_ERROR_WRONG_COLOR_SIZE    = 4,
    GEOQIK_ERROR_MEMORY_ALLOCATION   = 5,
    GEOQIK_ERROR_UNKNOWN             = 6,
    GEOQIK_ERROR_RENDERER_INIT_FAILED = 7,
    GEOQIK_ERROR_IO                  = 8,
    GEOQIK_ERROR_UNSUPPORTED_FORMAT  = 9,
    GEOQIK_ERROR_INVALID_STATE       = 10
} geoqik_error_code_t;
#endif

#ifdef __cplusplus
enum geoqik_client_error_code_t : int { // NOLINT(performance-enum-size,readability-identifier-naming)
    GEOQIK_CLIENT_SUCCESS = 0,
    GEOQIK_CLIENT_ERROR_SERVER_EXECUTABLE_NOT_FOUND = 1,
    GEOQIK_CLIENT_ERROR_SERVER_START_FAILED = 2,
    GEOQIK_CLIENT_ERROR_IPC_CONNECTION = 3,
    GEOQIK_CLIENT_ERROR_IPC_WRITE = 4,
    GEOQIK_CLIENT_ERROR_IPC_READ = 5,
    GEOQIK_CLIENT_ERROR_PROTOCOL = 6,
    GEOQIK_CLIENT_ERROR_UNKNOWN = 7
};
#else
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
#endif

#ifdef __cplusplus
struct geoqik_client_error_info_t { // NOLINT(readability-identifier-naming)
    std::size_t struct_size{};
    geoqik_client_error_code_t client_code{};
    geoqik_error_code_t code{};
    const char* operation{};
    const char* what{};
    const char* why{};
    const char* action{};
    const char* details{};
};
#else
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
#endif

#ifdef __cplusplus
struct geoqik_result_t { // NOLINT(readability-identifier-naming)
    geoqik_error_code_t err{};
    geoqik_uuid_t       geometryId{};
};
#else
typedef struct {
    geoqik_error_code_t err;
    geoqik_uuid_t       geometryId;
} geoqik_result_t;
#endif

#ifdef __cplusplus

#include <GeoQikClient/detail/Connection.hpp>
#include <GeoQikClient/detail/ProcessManager.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <algorithm>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>

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
        return GEOQIK_SUCCESS;
    }
    const auto resp = call(cmd);
    geoqik::client::detail::disconnect_thread_connection();
    set_server_response_error(resp, operation);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
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
        std::copy(resp.uuid.begin(), resp.uuid.end(), result.geometryId.value.begin());
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

#endif // __cplusplus

#endif // GEOQIKCLIENT_HPP
