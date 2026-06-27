#include "CommandDispatch.hpp"

#include <GeoQik/GeoQik.hpp>
#include <GeoQikProtocol/Protocol.hpp>

#include <algorithm>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstdlib>
#include <string>
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

[[nodiscard]] const ServerDiagnosticEntry& server_entry(ServerDiagnosticId id) {
    for (const ServerDiagnosticEntry& entry : serverDiagnosticCatalog) {
        if (entry.id == id) {
            return entry;
        }
    }
    return serverDiagnosticCatalog[0];
}

[[nodiscard]] proto::DiagnosticText server_diagnostic(ServerDiagnosticId id, const std::string& details = {}) {
    const ServerDiagnosticEntry& entry = server_entry(id);
    return proto::DiagnosticText{
        entry.operation,
        entry.what,
        entry.why,
        entry.action,
        details};
}

[[nodiscard]] proto::DiagnosticText last_api_diagnostic() {
    geoqik_error_info_t info{};
    info.struct_size = sizeof(info);
    if (geoqik_get_last_error_info(&info) != GEOQIK_SUCCESS || info.code == GEOQIK_SUCCESS) {
        return {};
    }

    return proto::DiagnosticText{
        info.operation != nullptr ? info.operation : "",
        info.what != nullptr ? info.what : "",
        info.why != nullptr ? info.why : "",
        info.action != nullptr ? info.action : "",
        info.details != nullptr ? info.details : ""};
}

void send_response(PipeStream& stream,
                   geoqik_error_code_t err,
                   const geoqik_uuid_t* uuid = nullptr,
                   const proto::DiagnosticText& diagnostic = {})
{
    const std::vector<std::uint8_t> diagnosticPayload = proto::encode_diagnostic(diagnostic);

    proto::ResponseHeader resp{};
    resp.errorCode = static_cast<std::uint32_t>(err);
    resp.diagnosticBytes = proto::payload_byte_count(diagnosticPayload.size());
    if (uuid != nullptr) {
        std::copy_n(uuid->value, proto::uuidByteCount, resp.uuid.begin());
    }

    boost::system::error_code ec;
    boost::asio::write(stream,
        boost::asio::buffer(&resp, sizeof(resp)), ec);
    if (ec) {
        return;
    }
    if (!diagnosticPayload.empty()) {
        boost::asio::write(stream,
            boost::asio::buffer(diagnosticPayload.data(), diagnosticPayload.size()), ec);
    }
}

void send_api_response(PipeStream& stream,
                       geoqik_error_code_t err,
                       const geoqik_uuid_t* uuid = nullptr)
{
    send_response(stream, err, uuid, err == GEOQIK_SUCCESS ? proto::DiagnosticText{} : last_api_diagnostic());
}

[[nodiscard]] bool payload_size_matches(proto::CommandId commandId, std::size_t payloadSize) {
    switch (commandId) {
    case proto::CommandId::Draw:
    case proto::CommandId::WaitForExit:
    case proto::CommandId::Cleanup:
        return payloadSize == 0;
    case proto::CommandId::AddPoint:
        return payloadSize == proto::pointPayloadByteCount;
    case proto::CommandId::AddLine:
        return payloadSize == proto::linePayloadByteCount;
    default:
        return false;
    }
}

template<typename T>
[[nodiscard]] T read_field(const std::vector<std::uint8_t>& payload, std::size_t& offset) {
    auto value = proto::read_pod<T>(payload, offset);
    offset += sizeof(T);
    return value;
}

[[noreturn]] void exit_success() {
    std::exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe)
}

} // namespace

void handle_connection(PipeStream& stream) {
    for (;;) {
        proto::FrameHeader header{};
        boost::system::error_code ec;
        boost::asio::read(stream,
            boost::asio::buffer(&header, sizeof(header)), ec);
        if (ec) {
            return;
        }

        std::vector<std::uint8_t> payload(header.payloadBytes);
        if (header.payloadBytes > 0) {
            boost::asio::read(stream,
                boost::asio::buffer(payload.data(), payload.size()), ec);
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

        default:
            {
                const ServerDiagnosticEntry& entry = server_entry(ServerDiagnosticId::UnknownCommand);
                send_response(stream, entry.responseCode, nullptr, server_diagnostic(ServerDiagnosticId::UnknownCommand));
            }
            return;
        }
    }
}

} // namespace geoqik::server::dispatch
