#include "CommandDispatch.hpp"

#include <GeoQik/GeoQik.hpp>
#include <GeoQikProtocol/Protocol.hpp>

#include <algorithm>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace geoqik::server::dispatch {

namespace proto = geoqik::protocol;

namespace {

void send_response(PipeStream& stream,
                   geoqik_error_code_t err,
                   const geoqik_uuid_t* uuid = nullptr)
{
    proto::ResponseFrame resp{};
    resp.errorCode = static_cast<std::uint32_t>(err);
    if (uuid != nullptr) {
        std::copy_n(uuid->value, proto::uuidByteCount, resp.uuid.begin());
    }
    boost::asio::write(stream,
        boost::asio::buffer(&resp, sizeof(resp)));
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
            send_response(stream, GEOQIK_ERROR_INVALID_PARAMETER);
            return;
        }

        switch (cmd) {

        case proto::CommandId::Draw: {
            const auto err = geoqik_draw();
            send_response(stream, err);
            break;
        }

        case proto::CommandId::AddPoint: {
            std::size_t offset = 0;
            const auto x = read_field<double>(payload, offset);
            const auto y = read_field<double>(payload, offset);
            const auto z = read_field<double>(payload, offset);
            const auto result = geoqik_add_point(x, y, z);
            send_response(stream, result.err, &result.geometryId);
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
            send_response(stream, err);
            break;
        }

        case proto::CommandId::WaitForExit: {
            const auto err = geoqik_wait_for_exit_and_cleanup();
            send_response(stream, err);
            exit_success();
        }

        case proto::CommandId::Cleanup: {
            const auto err = geoqik_cleanup();
            send_response(stream, err);
            exit_success();
        }

        default:
            send_response(stream, GEOQIK_ERROR_UNKNOWN);
            return;
        }
    }
}

} // namespace geoqik::server::dispatch
