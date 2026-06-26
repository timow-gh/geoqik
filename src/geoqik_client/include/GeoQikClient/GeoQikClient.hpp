#pragma once

#ifdef __cplusplus
#  include <cstddef>
#  include <cstdint>
#else
#  include <stdbool.h>
#  include <stddef.h>
#  include <stdint.h>
#endif

#ifdef __cplusplus
typedef struct { std::uint8_t value[16]; } geoqik_uuid_t; // NOLINT(modernize-avoid-c-arrays)
#else
typedef struct { uint8_t value[16]; } geoqik_uuid_t;
#endif

typedef enum {
    GEOQIK_SUCCESS                   = 0,
    GEOQIK_ERROR_NOT_INITIALIZED     = 1,
    GEOQIK_ERROR_ALREADY_INITIALIZED = 2,
    GEOQIK_ERROR_INVALID_PARAMETER   = 3,
    GEOQIK_ERROR_WRONG_COLOR_SIZE    = 4,
    GEOQIK_ERROR_MEMORY_ALLOCATION   = 5,
    GEOQIK_ERROR_UNKNOWN             = 6
} geoqik_error_code_t;

typedef struct {
    geoqik_error_code_t err;
    geoqik_uuid_t       geometryId;
} geoqik_result_t;

#ifdef __cplusplus

#include <GeoQikClient/detail/ProcessManager.hpp>
#include <GeoQikClient/detail/Connection.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace geoqik_client_impl {

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

} // namespace geoqik_client_impl

[[nodiscard]] inline geoqik_error_code_t geoqik_init() {
    try {
        auto& pm = geoqik::client::detail::ProcessManager::instance();
        pm.start();
        geoqik::client::detail::ensure_connected(pm.pipe_name());
        return GEOQIK_SUCCESS;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

[[nodiscard]] inline geoqik_error_code_t geoqik_draw() {
    namespace proto = geoqik::protocol;
    const auto resp = geoqik_client_impl::call(proto::CommandId::Draw);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

[[nodiscard]] inline geoqik_result_t geoqik_add_point(double x, double y, double z) {
    namespace proto = geoqik::protocol;
    std::vector<std::uint8_t> payload;
    payload.reserve(proto::pointPayloadByteCount);
    proto::write_pod(payload, x);
    proto::write_pod(payload, y);
    proto::write_pod(payload, z);

    const auto resp = geoqik_client_impl::call(proto::CommandId::AddPoint, payload);

    geoqik_result_t result{};
    result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
    std::copy(resp.uuid.begin(), resp.uuid.end(), result.geometryId.value);
    return result;
}

[[nodiscard]] inline geoqik_error_code_t geoqik_add_line(
    double x1, double y1, double z1,
    double x2, double y2, double z2)
{
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
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

[[nodiscard]] inline geoqik_error_code_t geoqik_wait_for_exit_and_cleanup() {
    namespace proto = geoqik::protocol;
    const auto resp = geoqik_client_impl::call(proto::CommandId::WaitForExit);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

[[nodiscard]] inline geoqik_error_code_t geoqik_cleanup() {
    namespace proto = geoqik::protocol;
    const auto resp = geoqik_client_impl::call(proto::CommandId::Cleanup);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

#endif // __cplusplus
