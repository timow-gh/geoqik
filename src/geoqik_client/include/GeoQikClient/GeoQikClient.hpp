#pragma once

#include <stdint.h>
#include <stddef.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif

typedef struct { uint8_t value[16]; } geoqik_uuid_t;

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
#include <vector>
#include <cstring>

namespace geoqik_client_impl {

inline const std::string& pipe_name() {
    return geoqik::client::detail::ProcessManager::instance().pipe_name();
}

inline geoqik::protocol::ResponseFrame call(
    geoqik::protocol::CommandId cmd,
    const std::vector<uint8_t>& payload = {})
{
    geoqik::client::detail::ensure_connected(pipe_name());
    return geoqik::client::detail::thread_connection().send_recv(cmd, payload);
}

} // namespace geoqik_client_impl

inline geoqik_error_code_t geoqik_init() {
    try {
        auto& pm = geoqik::client::detail::ProcessManager::instance();
        pm.start();
        geoqik::client::detail::ensure_connected(pm.pipe_name());
        return GEOQIK_SUCCESS;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

inline geoqik_error_code_t geoqik_draw() {
    namespace proto = geoqik::protocol;
    auto resp = geoqik_client_impl::call(proto::CommandId::Draw);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

inline geoqik_result_t geoqik_add_point(double x, double y, double z) {
    namespace proto = geoqik::protocol;
    std::vector<uint8_t> payload;
    payload.reserve(24);
    proto::write_pod(payload, x);
    proto::write_pod(payload, y);
    proto::write_pod(payload, z);

    auto resp = geoqik_client_impl::call(proto::CommandId::AddPoint, payload);

    geoqik_result_t result{};
    result.err = static_cast<geoqik_error_code_t>(resp.errorCode);
    std::memcpy(result.geometryId.value, resp.uuid, 16);
    return result;
}

inline geoqik_error_code_t geoqik_add_line(
    double x1, double y1, double z1,
    double x2, double y2, double z2)
{
    namespace proto = geoqik::protocol;
    std::vector<uint8_t> payload;
    payload.reserve(48);
    proto::write_pod(payload, x1);
    proto::write_pod(payload, y1);
    proto::write_pod(payload, z1);
    proto::write_pod(payload, x2);
    proto::write_pod(payload, y2);
    proto::write_pod(payload, z2);

    auto resp = geoqik_client_impl::call(proto::CommandId::AddLine, payload);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

inline geoqik_error_code_t geoqik_wait_for_exit_and_cleanup() {
    namespace proto = geoqik::protocol;
    auto resp = geoqik_client_impl::call(proto::CommandId::WaitForExit);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

inline geoqik_error_code_t geoqik_cleanup() {
    namespace proto = geoqik::protocol;
    auto resp = geoqik_client_impl::call(proto::CommandId::Cleanup);
    return static_cast<geoqik_error_code_t>(resp.errorCode);
}

#endif // __cplusplus
