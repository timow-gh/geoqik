#include <GeoQikProtocol/Protocol.hpp>
#include <gtest/gtest.h>

TEST(ProtocolTest, DiagnosticTextRoundTrips) {
    geoqik::protocol::DiagnosticText expected;
    expected.operation = "geoqik_add_point";
    expected.what = "what";
    expected.why = "why";
    expected.action = "action";
    expected.details = "details";

    const std::vector<std::uint8_t> payload = geoqik::protocol::encode_diagnostic(expected);
    const geoqik::protocol::DiagnosticText actual = geoqik::protocol::decode_diagnostic(payload);

    EXPECT_EQ(expected.operation, actual.operation);
    EXPECT_EQ(expected.what, actual.what);
    EXPECT_EQ(expected.why, actual.why);
    EXPECT_EQ(expected.action, actual.action);
    EXPECT_EQ(expected.details, actual.details);
}

TEST(ProtocolTest, EmptyDiagnosticPayloadDecodesToEmptyDiagnostic) {
    const geoqik::protocol::DiagnosticText actual = geoqik::protocol::decode_diagnostic({});

    EXPECT_TRUE(actual.operation.empty());
    EXPECT_TRUE(actual.what.empty());
    EXPECT_TRUE(actual.why.empty());
    EXPECT_TRUE(actual.action.empty());
    EXPECT_TRUE(actual.details.empty());
}

TEST(ProtocolTest, PipeNameArgumentRoundTripsToPipeName) {
    constexpr std::uint64_t pid = 12345;

    const std::string pipeName = geoqik::protocol::make_pipe_name(pid);
    const std::string argument = geoqik::protocol::make_pipe_name_argument(pid);

    EXPECT_EQ(pipeName, geoqik::protocol::make_pipe_name_from_argument(argument));
}

TEST(ProtocolTest, StrokeStyleAndUint8ArrayRoundTrip) {
    namespace proto = geoqik::protocol;
    const float pattern[] = {4.0f, 2.0f, 1.0f};
    const std::uint8_t flags[] = {1, 0, 1, 1};
    std::vector<std::uint8_t> payload;
    proto::write_stroke_style_wire(payload, 5.0f, 2, 1, 6.0f, pattern, 3, 0.25f, 1);
    proto::write_optional_uint8_array(payload, flags, 4);

    std::size_t offset = 0;
    const auto style = proto::read_stroke_style_wire(payload, offset);
    const auto decodedFlags = proto::read_optional_uint8_array(payload, offset);
    EXPECT_FLOAT_EQ(style.lineWidth, 5.0f);
    EXPECT_EQ(style.cap, 2);
    EXPECT_EQ(style.join, 1);
    EXPECT_FLOAT_EQ(style.miterLimit, 6.0f);
    EXPECT_EQ(style.dashPattern, std::vector<float>(std::begin(pattern), std::end(pattern)));
    EXPECT_FLOAT_EQ(style.dashPhase, 0.25f);
    EXPECT_EQ(style.dashSpace, 1);
    EXPECT_EQ(decodedFlags, std::vector<std::uint8_t>(std::begin(flags), std::end(flags)));
    EXPECT_EQ(offset, payload.size());
}

#ifndef _WIN32
TEST(ProtocolTest, UnixPipeNameArgumentDoesNotContainEmbeddedNull) {
    constexpr std::uint64_t pid = 12345;

    const std::string pipeName = geoqik::protocol::make_pipe_name(pid);
    const std::string argument = geoqik::protocol::make_pipe_name_argument(pid);

    ASSERT_FALSE(pipeName.empty());
    EXPECT_EQ('\0', pipeName.front());
    EXPECT_NE(std::string::npos, pipeName.find('\0'));
    EXPECT_EQ(std::string::npos, argument.find('\0'));
}
#endif
