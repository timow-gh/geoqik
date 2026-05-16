#include "GeoQikLog.hpp"
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

namespace
{

core::UUID make_uuid(std::uint8_t seed)
{
  std::array<std::uint8_t, 16> bytes{};
  for (std::size_t i = 0; i < bytes.size(); ++i)
  {
    bytes[i] = static_cast<std::uint8_t>(seed + i);
  }
  return core::UUID(bytes);
}

std::filesystem::path make_temp_path(const char* name)
{
  return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST(GeoQikLogTest, BinaryRoundTripPreservesRepresentativeEntries)
{
  using namespace geoqik;

  const GeoQikLogCommonData commonData{make_uuid(1), make_uuid(20), {0.1f, 0.2f, 0.3f, 1.0f}};
  const std::vector<GeoQikLogEntry> entries{
      GeoQikLogAddPointWithOpts{1.0f, 2.0f, 3.0f, commonData},
      GeoQikLogAddPointsWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
      GeoQikLogRemovePoint{make_uuid(2)},
      GeoQikLogSetPointSize{7.0f},
      GeoQikLogSetPointColor{Color{0.4f, 0.5f, 0.6f, 1.0f}},
      GeoQikLogAddLineWithOpts{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, commonData},
      GeoQikLogAddLinesWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
      GeoQikLogRemoveLine{make_uuid(3)},
      GeoQikLogSetLineWidth{8.0f},
      GeoQikLogSetLineColor{Color{0.7f, 0.8f, 0.9f, 1.0f}},
      GeoQikLogRemoveAllGeometry{},
      GeoQikLogTranslateGeometry{make_uuid(4), 1.0f, 2.0f, 3.0f},
      GeoQikLogRotateGeometry{make_uuid(5), 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 90.0f}};

  const std::filesystem::path path = make_temp_path("geoqik_log_roundtrip.gqklog");
  save_log_binary(path, entries);

  const std::vector<GeoQikLogEntry> loadedEntries = load_log_binary(path);
  EXPECT_EQ(entries, loadedEntries);

  (void)std::filesystem::remove(path);
}

TEST(GeoQikLogTest, BinaryLoadRejectsMissingFile)
{
  EXPECT_THROW(
      {
        auto entries = geoqik::load_log_binary(make_temp_path("geoqik_missing_log_file.gqklog"));
        (void)entries;
      },
      std::runtime_error);
}
