#include "CommandDispatch.hpp"
#include <GeoQikProtocol/Protocol.hpp>
#include <GeoQik/GeoQik.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>

namespace geoqik::server::dispatch {

namespace proto = geoqik::protocol;

static void send_response(PipeStream& stream,
                           geoqik_error_code_t err,
                           const geoqik_uuid_t* uuid = nullptr)
{
    proto::ResponseFrame resp{};
    resp.errorCode = static_cast<uint32_t>(err);
    if (uuid) {
        std::memcpy(resp.uuid, uuid->value, 16);
    }
    boost::asio::write(stream,
        boost::asio::buffer(&resp, sizeof(resp)));
}

void handle_connection(PipeStream& stream) {
    for (;;) {
        proto::FrameHeader header{};
        boost::system::error_code ec;
        boost::asio::read(stream,
            boost::asio::buffer(&header, sizeof(header)), ec);
        if (ec) return;

        std::vector<uint8_t> payload(header.payloadBytes);
        if (header.payloadBytes > 0) {
            boost::asio::read(stream,
                boost::asio::buffer(payload.data(), payload.size()), ec);
            if (ec) return;
        }

        const auto cmd = static_cast<proto::CommandId>(header.commandId);
        switch (cmd) {

        case proto::CommandId::Draw: {
            auto err = geoqik_draw();
            send_response(stream, err);
            break;
        }

        case proto::CommandId::AddPoint: {
            const uint8_t* p = payload.data();
            double x = proto::read_pod<double>(p);      p += 8;
            double y = proto::read_pod<double>(p);      p += 8;
            double z = proto::read_pod<double>(p);
            auto result = geoqik_add_point(x, y, z);
            send_response(stream, result.err, &result.geometryId);
            break;
        }

        case proto::CommandId::AddLine: {
            const uint8_t* p = payload.data();
            double x1 = proto::read_pod<double>(p);  p += 8;
            double y1 = proto::read_pod<double>(p);  p += 8;
            double z1 = proto::read_pod<double>(p);  p += 8;
            double x2 = proto::read_pod<double>(p);  p += 8;
            double y2 = proto::read_pod<double>(p);  p += 8;
            double z2 = proto::read_pod<double>(p);
            auto err = geoqik_add_line(x1, y1, z1, x2, y2, z2);
            send_response(stream, err);
            break;
        }

        case proto::CommandId::WaitForExit: {
            auto err = geoqik_wait_for_exit_and_cleanup();
            send_response(stream, err);
            std::exit(0); // NOLINT(concurrency-mt-unsafe)
            break;
        }

        case proto::CommandId::Cleanup: {
            auto err = geoqik_cleanup();
            send_response(stream, err);
            std::exit(0); // NOLINT(concurrency-mt-unsafe)
            break;
        }

        default:
            send_response(stream, GEOQIK_ERROR_UNKNOWN);
            return;
        }
    }
}

} // namespace geoqik::server::dispatch
