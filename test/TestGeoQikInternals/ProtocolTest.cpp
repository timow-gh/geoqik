#include <GeoQikProtocol/Protocol.hpp>
#include <gtest/gtest.h>

TEST(ProtocolTest, DiagnosticTextRoundTrips)
{
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

TEST(ProtocolTest, EmptyDiagnosticPayloadDecodesToEmptyDiagnostic)
{
  const geoqik::protocol::DiagnosticText actual = geoqik::protocol::decode_diagnostic({});

  EXPECT_TRUE(actual.operation.empty());
  EXPECT_TRUE(actual.what.empty());
  EXPECT_TRUE(actual.why.empty());
  EXPECT_TRUE(actual.action.empty());
  EXPECT_TRUE(actual.details.empty());
}
