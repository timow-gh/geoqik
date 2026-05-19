#include "GeoQikLog.hpp"
#include "GeoQikMessageSerialization.hpp"
#include <array>
#include <filesystem>
#include <gtest/gtest.h>
#include <sstream>
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

  const GeoQikMessageCommonData commonData{make_uuid(1), make_uuid(20), {0.1f, 0.2f, 0.3f, 1.0f}};
  const std::vector<GeoQikLogEntry> entries{
      AddPointWithOpts{1.0f, 2.0f, 3.0f, commonData},
      AddPointsWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
      UpdatePointWithOpts{make_uuid(15), 7.0f, 8.0f, 9.0f, {0.2f, 0.3f, 0.4f, 1.0f}},
      UpdatePointsWithOpts{make_uuid(16), {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f}, {0.3f, 0.4f, 0.5f, 1.0f}},
      RemovePoint{make_uuid(2)},
      SetPointSize{7.0f},
      SetPointColor{Color{0.4f, 0.5f, 0.6f, 1.0f}},
      AddLineWithOpts{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, commonData},
      AddLinesWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
      UpdateLineWithOpts{make_uuid(17), 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, {0.6f, 0.7f, 0.8f, 1.0f}},
      UpdateLinesWithOpts{make_uuid(18), {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, {0.7f, 0.8f, 0.9f, 1.0f}},
      RemoveLine{make_uuid(3)},
      SetLineWidth{8.0f},
      SetLineColor{Color{0.7f, 0.8f, 0.9f, 1.0f}},
      RemoveAllGeometry{},
      TranslateGeometry{make_uuid(4), 1.0f, 2.0f, 3.0f},
      RotateGeometry{make_uuid(5), 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 90.0f}};

  const std::filesystem::path path = make_temp_path("geoqik_log_roundtrip.gqklog");
  save_log_binary(path, entries);

  const std::vector<GeoQikLogEntry> loadedEntries = load_log_binary(path);
  EXPECT_EQ(entries, loadedEntries);

  (void)std::filesystem::remove(path);
}

TEST(GeoQikLogTest, MessageReaderWriterRoundTripPreservesRepresentativeEntries)
{
  using namespace geoqik;

  const GeoQikMessageCommonData commonData{make_uuid(10), make_uuid(40), {0.1f, 0.2f, 0.3f, 1.0f}};
  const std::vector<GeoQikLogEntry> entries{
      AddPointWithOpts{1.0f, 2.0f, 3.0f, commonData},
      AddPointsWithOpts{{1.0f, 2.0f, 3.0f}, commonData},
      UpdatePointWithOpts{make_uuid(20), 4.0f, 5.0f, 6.0f, {0.2f, 0.3f, 0.4f, 1.0f}},
      UpdatePointsWithOpts{make_uuid(21), {4.0f, 5.0f, 6.0f}, {0.3f, 0.4f, 0.5f, 1.0f}},
      RemovePoint{make_uuid(11)},
      SetPointSize{9.0f},
      SetPointColor{Color{0.2f, 0.3f, 0.4f, 1.0f}},
      AddLineWithOpts{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, commonData},
      AddLinesWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
      UpdateLineWithOpts{make_uuid(22), 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, {0.4f, 0.5f, 0.6f, 1.0f}},
      UpdateLinesWithOpts{make_uuid(23), {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, {0.5f, 0.6f, 0.7f, 1.0f}},
      RemoveLine{make_uuid(12)},
      SetLineWidth{3.0f},
      SetLineColor{Color{0.5f, 0.6f, 0.7f, 1.0f}},
      RemoveAllGeometry{},
      TranslateGeometry{make_uuid(13), 1.0f, 2.0f, 3.0f},
      RotateGeometry{make_uuid(14), 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 45.0f}};

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  MessageWriter writer(stream);
  for (const GeoQikLogEntry& entry: entries)
  {
    writer.write(entry);
  }

  stream.seekg(0);
  MessageReader reader(stream);
  std::vector<GeoQikLogEntry> loadedEntries;
  loadedEntries.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i)
  {
    loadedEntries.push_back(reader.read());
  }

  EXPECT_EQ(entries, loadedEntries);
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
