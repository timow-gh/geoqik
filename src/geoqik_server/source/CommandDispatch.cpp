#include "CommandDispatch.hpp"
#include <GeoQik/GeoQik.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <algorithm>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace geoqik::server::dispatch {

namespace proto = geoqik::protocol;

namespace {

enum class ServerDiagnosticId {
    InvalidPayload,
    UnknownCommand,
};

struct ServerDiagnosticEntry {
    ServerDiagnosticId id;
    geoqik_error_code_t responseCode;
    const char* operation;
    const char* what;
    const char* why;
    const char* action;
};

constexpr ServerDiagnosticEntry serverDiagnosticCatalog[] = {
    {ServerDiagnosticId::InvalidPayload,
     GEOQIK_ERROR_INVALID_PARAMETER,
     "geoqik_server_dispatch",
     "The IPC command payload is invalid.",
     "The command ID and payload byte count do not match the GeoQik protocol.",
     "Use a matching GeoQik client and server build."},
    {ServerDiagnosticId::UnknownCommand,
     GEOQIK_ERROR_UNKNOWN,
     "geoqik_server_dispatch",
     "The IPC command is unknown.",
     "The server received a command ID that this build does not implement.",
     "Use a matching GeoQik client and server build."},
};

[[nodiscard]]
const ServerDiagnosticEntry& server_entry(ServerDiagnosticId id) {
    for (const ServerDiagnosticEntry& entry: serverDiagnosticCatalog) {
        if (entry.id == id) {
            return entry;
        }
    }
    return serverDiagnosticCatalog[0];
}

[[nodiscard]]
proto::DiagnosticText server_diagnostic(ServerDiagnosticId id, const std::string& details = {}) {
    const ServerDiagnosticEntry& entry = server_entry(id);
    return proto::DiagnosticText{entry.operation, entry.what, entry.why, entry.action, details};
}

[[nodiscard]]
proto::DiagnosticText last_api_diagnostic() {
    geoqik_error_info_t info{};
    info.struct_size = sizeof(info);
    if (geoqik_get_last_error_info(&info) != GEOQIK_SUCCESS || info.code == GEOQIK_SUCCESS) {
        return {};
    }

    return proto::DiagnosticText{info.operation != nullptr ? info.operation : "",
                                 info.what != nullptr ? info.what : "",
                                 info.why != nullptr ? info.why : "",
                                 info.action != nullptr ? info.action : "",
                                 info.details != nullptr ? info.details : ""};
}

void send_response(PipeStream& stream,
                   geoqik_error_code_t err,
                   const geoqik_uuid_t* uuid = nullptr,
                   const proto::DiagnosticText& diagnostic = {}) {
    const std::vector<std::uint8_t> diagnosticPayload = proto::encode_diagnostic(diagnostic);

    proto::ResponseHeader resp{};
    resp.errorCode = static_cast<std::uint32_t>(err);
    resp.diagnosticBytes = proto::payload_byte_count(diagnosticPayload.size());
    if (uuid != nullptr) {
        std::copy_n(uuid->value, proto::uuidByteCount, resp.uuid.begin());
    }

    boost::system::error_code ec;
    boost::asio::write(stream, boost::asio::buffer(&resp, sizeof(resp)), ec);
    if (ec) {
        return;
    }
    if (!diagnosticPayload.empty()) {
        boost::asio::write(stream, boost::asio::buffer(diagnosticPayload.data(), diagnosticPayload.size()), ec);
    }
}

void send_api_response(PipeStream& stream, geoqik_error_code_t err, const geoqik_uuid_t* uuid = nullptr) {
    send_response(stream, err, uuid, err == GEOQIK_SUCCESS ? proto::DiagnosticText{} : last_api_diagnostic());
}

[[nodiscard]]
inline bool would_overflow_size_t(std::uint64_t a, std::uint64_t b) {
    return b != 0 && a > std::numeric_limits<std::size_t>::max() / b;
}

[[nodiscard]]
inline std::size_t wire_count_to_size(std::uint64_t value) {
    if constexpr (std::is_same_v<std::uint64_t, std::size_t>) {
        return value;
    } else {
        return static_cast<std::size_t>(value);
    }
}

[[nodiscard]]
inline std::uint64_t size_to_wire_count(std::size_t value) {
    if constexpr (std::is_same_v<std::size_t, std::uint64_t>) {
        return value;
    } else {
        return static_cast<std::uint64_t>(value);
    }
}

[[nodiscard]]
bool payload_size_matches(proto::CommandId commandId, std::size_t payloadSize) {
    switch (commandId) {
    case proto::CommandId::Draw:
    case proto::CommandId::WaitForExit:
    case proto::CommandId::Cleanup:              return payloadSize == 0;
    case proto::CommandId::AddPoint:             return payloadSize == proto::pointPayloadByteCount;
    case proto::CommandId::AddLine:              return payloadSize == proto::linePayloadByteCount;
    case proto::CommandId::IsApiInitialized:
    case proto::CommandId::StopDrawing:
    case proto::CommandId::RemoveAllGeometry:    return payloadSize == 0;
    case proto::CommandId::SetPointSize:
    case proto::CommandId::GetPointSize:
    case proto::CommandId::SetLineWidth:
    case proto::CommandId::GetLineWidth:         return payloadSize == proto::floatPayloadByteCount;
    case proto::CommandId::SetPointColor:
    case proto::CommandId::GetPointColor:
    case proto::CommandId::SetLineColor:
    case proto::CommandId::GetLineColor:
    case proto::CommandId::SetMeshColor:
    case proto::CommandId::GetMeshColor:         return payloadSize == proto::colorPayloadByteCount;
    case proto::CommandId::TranslateGeometry:    return payloadSize == proto::translatePayloadByteCount;
    case proto::CommandId::RotateGeometry:       return payloadSize == proto::rotatePayloadByteCount;
    case proto::CommandId::AddPointWithColor:    return payloadSize == proto::addPointWithColorPayloadByteCount;
    case proto::CommandId::UpdatePoint:          return payloadSize == proto::updatePointPayloadByteCount;
    case proto::CommandId::UpdatePointWithColor: return payloadSize == proto::updatePointWithColorPayloadByteCount;
    case proto::CommandId::RemovePoint:
    case proto::CommandId::RemoveLine:           return payloadSize == proto::removeGeometryPayloadByteCount;
    case proto::CommandId::AddLineWithColor:     return payloadSize == proto::addLineWithColorPayloadByteCount;
    case proto::CommandId::UpdateLine:           return payloadSize == proto::updateLinePayloadByteCount;
    case proto::CommandId::UpdateLineWithColor:  return payloadSize == proto::updateLineWithColorPayloadByteCount;
    case proto::CommandId::AddPointOpts:
    case proto::CommandId::AddPointsOpts:
    case proto::CommandId::UpdatePointOpts:
    case proto::CommandId::UpdatePointsOpts:
    case proto::CommandId::AddLineOpts:
    case proto::CommandId::AddLinesOpts:
    case proto::CommandId::UpdateLineOpts:
    case proto::CommandId::UpdateLinesOpts:      return payloadSize >= proto::uuidByteCount + sizeof(std::uint64_t);
    case proto::CommandId::RemoveMesh:           return payloadSize == proto::uuidByteCount;
    case proto::CommandId::SetMeshOverlayOpts:   return payloadSize == proto::setMeshOverlayOptsPayloadByteCount;
    case proto::CommandId::SetMeshRenderingOpts: return payloadSize == proto::setMeshRenderingOptsPayloadByteCount;
    case proto::CommandId::AddMeshOpts:
    case proto::CommandId::UpdateMeshOpts:       return payloadSize >= proto::uuidByteCount + sizeof(std::uint64_t);

    // Fixed-size log / replay commands
    case proto::CommandId::CancelReplay:
    case proto::CommandId::PauseReplay:
    case proto::CommandId::ResumeReplay:
    case proto::CommandId::StepReplay:
    case proto::CommandId::StepReplayBackward:
    case proto::CommandId::GetReplayState:
    case proto::CommandId::GetReplayProgress:   return payloadSize == 0;
    case proto::CommandId::StepReplayN:
    case proto::CommandId::StepReplayBackwardN: return payloadSize == proto::stepReplayNPayloadByteCount;

    // Variable-size log / replay commands ΓÇö dispatch cases validate inline.
    case proto::CommandId::SaveLog:
    case proto::CommandId::LoadLog:
        // pathLen (uint32) + format (int32)
        return payloadSize >= sizeof(std::uint32_t) + sizeof(std::int32_t);
    case proto::CommandId::ReplayLog:
        // pathLen (uint32) + format (int32) + minimum options block
        return payloadSize >= sizeof(std::uint32_t) + sizeof(std::int32_t) + proto::replayOptionsMinByteCount;
    case proto::CommandId::ReplayCurrentLog: return payloadSize >= proto::replayOptionsMinByteCount;

    default: return false;
    }
}

template <typename T>
[[nodiscard]]
T read_field(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    auto value = proto::read_pod<T>(payload, offset);
    offset += sizeof(T);
    return value;
}

[[nodiscard]]
geoqik_uuid_t read_uuid(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    geoqik_uuid_t uuid{};
    for (std::size_t i = 0; i < proto::uuidByteCount; ++i) {
        uuid.value[i] = read_field<std::uint8_t>(payload, offset);
    }
    return uuid;
}

[[nodiscard]]
bool uuid_is_zero(const geoqik_uuid_t& uuid) {
    for (const auto byte: uuid.value) {
        if (byte != 0) {
            return false;
        }
    }
    return true;
}

[[nodiscard]]
std::vector<float> read_float_array(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    const auto count = proto::read_pod<std::uint32_t>(payload, offset);
    offset += sizeof(std::uint32_t);
    if (count == 0) {
        return {};
    }
    const std::size_t byteLen = static_cast<std::size_t>(count) * sizeof(float);
    if (offset + byteLen > payload.size()) {
        throw std::out_of_range("geoqik protocol: float array payload truncated");
    }
    std::vector<float> arr(count);
    std::memcpy(arr.data(), payload.data() + offset, byteLen);
    offset += byteLen;
    return arr;
}

[[nodiscard]]
std::vector<std::uint32_t> read_uint32_array(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    const auto count = proto::read_pod<std::uint32_t>(payload, offset);
    offset += sizeof(std::uint32_t);
    if (count == 0) {
        return {};
    }
    const std::size_t byteLen = static_cast<std::size_t>(count) * sizeof(std::uint32_t);
    if (offset + byteLen > payload.size()) {
        throw std::out_of_range("geoqik protocol: uint32 array payload truncated");
    }
    std::vector<std::uint32_t> arr(count);
    std::memcpy(arr.data(), payload.data() + offset, byteLen);
    offset += byteLen;
    return arr;
}

struct ColorSlot {
    float rgba[4];
    bool isZero() const { return rgba[0] == 0.0f && rgba[1] == 0.0f && rgba[2] == 0.0f && rgba[3] == 0.0f; }
};

[[nodiscard]]
ColorSlot read_color_slot(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    ColorSlot s{};
    for (float& f: s.rgba) {
        f = read_field<float>(payload, offset);
    }
    return s;
}

// Owns the key vectors whose lifetimes must outlive the geoqik_replay_options_t that points into them.
struct ReplayOptionsData {
    std::vector<geoqik_key_t> stepKeys;
    std::vector<geoqik_key_t> backwardStepKeys;
    std::vector<geoqik_key_t> resumeKeys;
    std::vector<geoqik_key_t> pauseKeys;
    std::vector<geoqik_key_t> increaseKeys;
    std::vector<geoqik_key_t> decreaseKeys;
    geoqik_replay_options_t opts{};
};

// Deserialize a geoqik_replay_options_t from the wire payload.
// The returned ReplayOptionsData owns all key vectors; opts.xxxKeys point into them.
// Never move opts out of its ReplayOptionsData ΓÇö the pointers will dangle.
[[nodiscard]]
ReplayOptionsData read_replay_options(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    ReplayOptionsData data{};
    auto& opts = data.opts;

    opts.entriesPerSecond = read_field<double>(payload, offset);
    opts.speedMultiplier = read_field<double>(payload, offset);
    opts.maxEntriesPerFrame = wire_count_to_size(read_field<std::uint64_t>(payload, offset));
    opts.startPaused = read_field<std::int32_t>(payload, offset);
    opts.entriesPerStep = wire_count_to_size(read_field<std::uint64_t>(payload, offset));

    auto read_key_array = [&](std::vector<geoqik_key_t>& keys, const geoqik_key_t*& optsField, std::size_t& optsCount) {
        const auto count = read_field<std::uint32_t>(payload, offset);
        if (count == 0) {
            optsField = nullptr;
            optsCount = 0;
        } else {
            keys.resize(count);
            for (auto& k: keys) {
                k = static_cast<geoqik_key_t>(read_field<std::int32_t>(payload, offset));
            }
            optsField = keys.data();
            optsCount = keys.size();
        }
    };

    read_key_array(data.stepKeys, opts.stepKeys, opts.stepKeyCount);
    read_key_array(data.backwardStepKeys, opts.backwardStepKeys, opts.backwardStepKeyCount);
    read_key_array(data.resumeKeys, opts.resumeKeys, opts.resumeKeyCount);
    read_key_array(data.pauseKeys, opts.pauseKeys, opts.pauseKeyCount);
    read_key_array(data.increaseKeys, opts.increaseEntriesPerStepKeys, opts.increaseEntriesPerStepKeyCount);
    read_key_array(data.decreaseKeys, opts.decreaseEntriesPerStepKeys, opts.decreaseEntriesPerStepKeyCount);

    return data;
}

[[noreturn]]
void exit_success() {
    std::exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe)
}

} // namespace

void handle_connection(PipeStream& stream) {
    for (;;) {
        proto::FrameHeader header{};
        boost::system::error_code ec;
        boost::asio::read(stream, boost::asio::buffer(&header, sizeof(header)), ec);
        if (ec) {
            return;
        }

        constexpr std::uint32_t maxPayloadBytes = 64u * 1024u * 1024u; // 64 MiB
        if (header.payloadBytes > maxPayloadBytes) {
            send_response(stream,
                          GEOQIK_ERROR_INVALID_PARAMETER,
                          nullptr,
                          server_diagnostic(ServerDiagnosticId::InvalidPayload, "payload exceeds 64 MiB limit"));
            return;
        }
        std::vector<std::uint8_t> payload(header.payloadBytes);
        if (header.payloadBytes > 0) {
            boost::asio::read(stream, boost::asio::buffer(payload.data(), payload.size()), ec);
            if (ec) {
                return;
            }
        }

        const auto cmd = static_cast<proto::CommandId>(header.commandId);
        if (!payload_size_matches(cmd, payload.size())) {
            const ServerDiagnosticEntry& entry = server_entry(ServerDiagnosticId::InvalidPayload);
            send_response(stream, entry.responseCode, nullptr, server_diagnostic(ServerDiagnosticId::InvalidPayload));
            return;
        }

        switch (cmd) {

        case proto::CommandId::Draw: {
            const auto err = geoqik_draw();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::AddPoint: {
            std::size_t offset = 0;
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto result = geoqik_add_point(x, y, z);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::AddLine: {
            std::size_t offset = 0;
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto err = geoqik_add_line(x1, y1, z1, x2, y2, z2);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::WaitForExit: {
            const auto err = geoqik_wait_for_exit_and_cleanup();
            send_api_response(stream, err);
            exit_success();
        }

        case proto::CommandId::Cleanup: {
            const auto err = geoqik_cleanup();
            send_api_response(stream, err);
            exit_success();
        }

        case proto::CommandId::IsApiInitialized: {
            bool initialized = false;
            const auto err = geoqik_is_api_initialized(&initialized);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_return_value(static_cast<std::uint8_t>(initialized ? 1 : 0));
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::StopDrawing: {
            const auto err = geoqik_stop_drawing();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::RemoveAllGeometry: {
            const auto err = geoqik_remove_all_geometry();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::SetPointSize: {
            std::size_t offset = 0;
            const auto size = read_field<float>(payload, offset);
            const auto err = geoqik_set_point_size(size);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetPointSize: {
            float size = 0.0f;
            const auto err = geoqik_get_point_size(&size);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_return_value(size);
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::SetPointColor: {
            std::size_t offset = 0;
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_set_point_color(r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetPointColor: {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            const auto err = geoqik_get_point_color(&r, &g, &b, &a);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_color_return(r, g, b, a);
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::SetLineWidth: {
            std::size_t offset = 0;
            const auto width = read_field<float>(payload, offset);
            const auto err = geoqik_set_line_width(width);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetLineWidth: {
            float width = 0.0f;
            const auto err = geoqik_get_line_width(&width);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_return_value(width);
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::SetLineColor: {
            std::size_t offset = 0;
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_set_line_color(r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetLineColor: {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            const auto err = geoqik_get_line_color(&r, &g, &b, &a);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_color_return(r, g, b, a);
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::SetMeshColor: {
            std::size_t offset = 0;
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_set_mesh_color(r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetMeshColor: {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            const auto err = geoqik_get_mesh_color(&r, &g, &b, &a);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_color_return(r, g, b, a);
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::TranslateGeometry: {
            std::size_t offset = 0;
            geoqik_uuid_t geometryId{};
            for (auto& byte: geometryId.value) {
                byte = read_field<std::uint8_t>(payload, offset);
            }
            const auto dx = read_field<double>(payload, offset);
            const auto dy = read_field<double>(payload, offset);
            const auto dz = read_field<double>(payload, offset);
            const auto err = geoqik_translate_geometry(&geometryId, dx, dy, dz);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::RotateGeometry: {
            std::size_t offset = 0;
            geoqik_uuid_t geometryId{};
            for (auto& byte: geometryId.value) {
                byte = read_field<std::uint8_t>(payload, offset);
            }
            const auto cx = read_field<double>(payload, offset);
            const auto cy = read_field<double>(payload, offset);
            const auto cz = read_field<double>(payload, offset);
            const auto ax = read_field<double>(payload, offset);
            const auto ay = read_field<double>(payload, offset);
            const auto az = read_field<double>(payload, offset);
            const auto angle = read_field<double>(payload, offset);
            const auto err = geoqik_rotate_geometry(&geometryId, cx, cy, cz, ax, ay, az, angle);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::AddPointWithColor: {
            std::size_t offset = 0;
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto result = geoqik_add_point_with_color(x, y, z, r, g, b, a);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::AddPointOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t key = read_uuid(payload, offset);
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_add_points_options_t opts{};
            if (!uuid_is_zero(key)) {
                opts.idempotencyKey = key;
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto result = geoqik_add_point_opts(x, y, z, &opts);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::AddPointsOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t key = read_uuid(payload, offset);
            const auto count = read_field<std::uint64_t>(payload, offset);
            if (would_overflow_size_t(count, 3 * sizeof(double))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t coordBytes = wire_count_to_size(count) * 3 * sizeof(double);
            if (offset + coordBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const double* points = reinterpret_cast<const double*>(payload.data() + offset);
            offset += coordBytes;
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_add_points_options_t opts{};
            if (!uuid_is_zero(key)) {
                opts.idempotencyKey = key;
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto result = geoqik_add_points_opts(points, wire_count_to_size(count) * 3, &opts);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::UpdatePoint: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto err = geoqik_update_point(&id, x, y, z);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdatePointWithColor: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_update_point_with_color(&id, x, y, z, r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdatePointOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_update_points_options_t opts{};
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto err = geoqik_update_point_opts(&id, x, y, z, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdatePointsOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto count = read_field<std::uint64_t>(payload, offset);
            if (would_overflow_size_t(count, 3 * sizeof(double))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t coordBytes = wire_count_to_size(count) * 3 * sizeof(double);
            if (offset + coordBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const double* points = reinterpret_cast<const double*>(payload.data() + offset);
            offset += coordBytes;
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_update_points_options_t opts{};
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto err = geoqik_update_points_opts(&id, points, wire_count_to_size(count) * 3, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::RemovePoint: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto err = geoqik_remove_point(&id);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::AddLineWithColor: {
            std::size_t offset = 0;
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_add_line_with_color(x1, y1, z1, x2, y2, z2, r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::AddLineOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t key = read_uuid(payload, offset);
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_add_line_opts_t opts{};
            if (!uuid_is_zero(key)) {
                opts.idempotencyKey = key;
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto result = geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, &opts);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::AddLinesOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t key = read_uuid(payload, offset);
            const auto count = read_field<std::uint64_t>(payload, offset);
            if (would_overflow_size_t(count, 6 * sizeof(double))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t coordBytes = wire_count_to_size(count) * 6 * sizeof(double);
            if (offset + coordBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const double* lines = reinterpret_cast<const double*>(payload.data() + offset);
            offset += coordBytes;
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_add_line_opts_t opts{};
            if (!uuid_is_zero(key)) {
                opts.idempotencyKey = key;
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto result = geoqik_add_lines_opts(lines, wire_count_to_size(count) * 6, &opts);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::UpdateLine: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto err = geoqik_update_line(&id, x1, y1, z1, x2, y2, z2);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdateLineWithColor: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto r = read_field<float>(payload, offset);
            const auto g = read_field<float>(payload, offset);
            const auto b = read_field<float>(payload, offset);
            const auto a = read_field<float>(payload, offset);
            const auto err = geoqik_update_line_with_color(&id, x1, y1, z1, x2, y2, z2, r, g, b, a);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdateLineOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto x1 = read_field<double>(payload, offset);
            const auto y1 = read_field<double>(payload, offset);
            const auto z1 = read_field<double>(payload, offset);
            const auto x2 = read_field<double>(payload, offset);
            const auto y2 = read_field<double>(payload, offset);
            const auto z2 = read_field<double>(payload, offset);
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_update_line_opts_t opts{};
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto err = geoqik_update_line_opts(&id, x1, y1, z1, x2, y2, z2, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdateLinesOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto count = read_field<std::uint64_t>(payload, offset);
            if (would_overflow_size_t(count, 6 * sizeof(double))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t coordBytes = wire_count_to_size(count) * 6 * sizeof(double);
            if (offset + coordBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const double* lines = reinterpret_cast<const double*>(payload.data() + offset);
            offset += coordBytes;
            const auto colors = proto::read_optional_colors(payload, offset);

            geoqik_update_line_opts_t opts{};
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            const auto err = geoqik_update_lines_opts(&id, lines, wire_count_to_size(count) * 6, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::RemoveLine: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto err = geoqik_remove_line(&id);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::AddMeshOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t key = read_uuid(payload, offset);

            const auto vertexCount = read_field<std::uint64_t>(payload, offset);
            const auto triangleCount = read_field<std::uint64_t>(payload, offset);

            if (would_overflow_size_t(vertexCount, 3 * sizeof(float)) ||
                would_overflow_size_t(triangleCount, 3 * sizeof(std::uint32_t))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t vertBytes = wire_count_to_size(vertexCount) * 3 * sizeof(float);
            const std::size_t triBytes = wire_count_to_size(triangleCount) * 3 * sizeof(std::uint32_t);
            if (vertBytes > payload.size() || triBytes > payload.size() - vertBytes ||
                offset + vertBytes + triBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const float* vertices = reinterpret_cast<const float*>(payload.data() + offset);
            offset += vertBytes;
            const std::uint32_t* triIdx = reinterpret_cast<const std::uint32_t*>(payload.data() + offset);
            offset += triBytes;

            const auto normals = read_float_array(payload, offset);
            const auto colors = read_float_array(payload, offset);
            const auto segIdx = read_uint32_array(payload, offset);
            const auto segColor = read_color_slot(payload, offset);
            const auto showSegs = read_field<std::int32_t>(payload, offset);
            const auto segWidth = read_field<float>(payload, offset);
            const auto vtxColor = read_color_slot(payload, offset);
            const auto showVerts = read_field<std::int32_t>(payload, offset);
            const auto vtxSize = read_field<float>(payload, offset);

            geoqik_add_mesh_opts_t opts{};
            if (!uuid_is_zero(key)) {
                opts.idempotencyKey = key;
            }
            if (!normals.empty()) {
                opts.normals = normals.data();
                opts.normalsCount = normals.size();
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }
            if (!segIdx.empty()) {
                opts.segmentIndices = segIdx.data();
                opts.segmentIndexCount = segIdx.size();
            }
            opts.segmentColor = segColor.isZero() ? nullptr : segColor.rgba;
            opts.showSegments = showSegs;
            opts.segmentLineWidth = segWidth;
            opts.vertexColor = vtxColor.isZero() ? nullptr : vtxColor.rgba;
            opts.showVertices = showVerts;
            opts.vertexPointSize = vtxSize;

            const auto result = geoqik_add_mesh_opts(vertices,
                                                     wire_count_to_size(vertexCount),
                                                     triIdx,
                                                     wire_count_to_size(triangleCount),
                                                     &opts);
            send_api_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::RemoveMesh: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto err = geoqik_remove_mesh(&id);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::UpdateMeshOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);

            const auto vertexCount = read_field<std::uint64_t>(payload, offset);
            if (would_overflow_size_t(vertexCount, 3 * sizeof(float))) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const std::size_t vertBytes = wire_count_to_size(vertexCount) * 3 * sizeof(float);
            if (offset + vertBytes > payload.size()) {
                send_response(stream,
                              GEOQIK_ERROR_INVALID_PARAMETER,
                              nullptr,
                              server_diagnostic(ServerDiagnosticId::InvalidPayload));
                return;
            }
            const float* vertices = reinterpret_cast<const float*>(payload.data() + offset);
            offset += vertBytes;

            const auto normals = read_float_array(payload, offset);
            const auto colors = read_float_array(payload, offset);

            geoqik_update_mesh_opts_t opts{};
            if (!normals.empty()) {
                opts.normals = normals.data();
                opts.normalsCount = normals.size();
            }
            if (!colors.empty()) {
                opts.color = colors.data();
                opts.colorCount = colors.size();
            }

            const auto err = geoqik_update_mesh_opts(&id, vertices, wire_count_to_size(vertexCount), &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::SetMeshOverlayOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto showSegments = read_field<std::int32_t>(payload, offset);
            const auto showVertices = read_field<std::int32_t>(payload, offset);
            geoqik_mesh_overlay_opts_t opts{showSegments, showVertices};
            const auto err = geoqik_set_mesh_overlay_opts(&id, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::SetMeshRenderingOpts: {
            std::size_t offset = 0;
            const geoqik_uuid_t id = read_uuid(payload, offset);
            const auto cullModeRaw = read_field<std::uint32_t>(payload, offset);
            const auto surfaceVis = read_field<std::int32_t>(payload, offset);
            geoqik_mesh_rendering_opts_t opts{static_cast<geoqik_mesh_cull_mode_t>(cullModeRaw), surfaceVis};
            const auto err = geoqik_set_mesh_rendering_opts(&id, &opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::SaveLog: {
            std::size_t offset = 0;
            const std::string path = proto::read_string(payload, offset);
            const auto fmt = static_cast<geoqik_log_format_t>(read_field<std::int32_t>(payload, offset));
            const auto err = geoqik_save_log(path.empty() ? nullptr : path.c_str(), fmt);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::LoadLog: {
            std::size_t offset = 0;
            const std::string path = proto::read_string(payload, offset);
            const auto fmt = static_cast<geoqik_log_format_t>(read_field<std::int32_t>(payload, offset));
            const auto err = geoqik_load_log(path.empty() ? nullptr : path.c_str(), fmt);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::ReplayLog: {
            std::size_t offset = 0;
            const std::string path = proto::read_string(payload, offset);
            const auto fmt = static_cast<geoqik_log_format_t>(read_field<std::int32_t>(payload, offset));
            const auto data = read_replay_options(payload, offset);
            const auto err = geoqik_replay_log(path.empty() ? nullptr : path.c_str(), fmt, &data.opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::ReplayCurrentLog: {
            std::size_t offset = 0;
            const auto data = read_replay_options(payload, offset);
            const auto err = geoqik_replay_current_log(&data.opts);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::CancelReplay: {
            const auto err = geoqik_cancel_replay();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::PauseReplay: {
            const auto err = geoqik_pause_replay();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::ResumeReplay: {
            const auto err = geoqik_resume_replay();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::StepReplay: {
            const auto err = geoqik_step_replay();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::StepReplayN: {
            std::size_t offset = 0;
            const auto count = wire_count_to_size(read_field<std::uint64_t>(payload, offset));
            const auto err = geoqik_step_replay_n(count);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::StepReplayBackward: {
            const auto err = geoqik_step_replay_backward();
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::StepReplayBackwardN: {
            std::size_t offset = 0;
            const auto count = wire_count_to_size(read_field<std::uint64_t>(payload, offset));
            const auto err = geoqik_step_replay_backward_n(count);
            send_api_response(stream, err);
            break;
        }

        case proto::CommandId::GetReplayState: {
            geoqik_replay_state_t state = GEOQIK_REPLAY_INACTIVE;
            const auto err = geoqik_get_replay_state(&state);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const auto packed = proto::pack_return_value(static_cast<std::uint32_t>(state));
                std::copy(packed.begin(), packed.end(), fakeUuid.value);
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        case proto::CommandId::GetReplayProgress: {
            std::size_t current = 0, total = 0;
            const auto err = geoqik_get_replay_progress(&current, &total);
            geoqik_uuid_t fakeUuid{};
            if (err == GEOQIK_SUCCESS) {
                const std::uint64_t curU = size_to_wire_count(current);
                const std::uint64_t totU = size_to_wire_count(total);
                std::memcpy(fakeUuid.value, &curU, sizeof(curU));
                std::memcpy(fakeUuid.value + 8, &totU, sizeof(totU));
            }
            send_api_response(stream, err, &fakeUuid);
            break;
        }

        default: {
            const ServerDiagnosticEntry& entry = server_entry(ServerDiagnosticId::UnknownCommand);
            send_response(stream, entry.responseCode, nullptr, server_diagnostic(ServerDiagnosticId::UnknownCommand));
        }
            return;
        }
    }
}

} // namespace geoqik::server::dispatch
