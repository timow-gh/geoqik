#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <vector>

class GeoQikTestApi : public ::testing::Test
{
};

namespace
{

void init_hidden_geoqik()
{
  geoqik_window_settings_t windowSettings;
  geoqik_init_default_window_settings(&windowSettings);
  windowSettings.visible = 0;

  geoqik_settings_t geoSettings;
  geoqik_create_default_settings(&geoSettings);

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init_with_settings(&geoSettings, &windowSettings));
}

std::vector<char> read_binary_file(const std::filesystem::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  return std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

template <typename T>
T read_binary_value(std::istream& stream)
{
  T value{};
  stream.read(reinterpret_cast<char*>(&value), sizeof(T));
  return value;
}

std::vector<float> read_add_point_x_values(const std::filesystem::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  stream.seekg(8); // magic
  (void)read_binary_value<std::uint32_t>(stream);
  const std::uint64_t entryCount = read_binary_value<std::uint64_t>(stream);

  std::vector<float> values;
  values.reserve(static_cast<std::size_t>(entryCount));
  for (std::uint64_t i = 0; i < entryCount; ++i)
  {
    const std::uint32_t messageType = read_binary_value<std::uint32_t>(stream);
    if (messageType != 1)
    {
      return {};
    }

    const float x = read_binary_value<float>(stream);
    (void)read_binary_value<float>(stream);
    (void)read_binary_value<float>(stream);
    stream.seekg(32, std::ios::cur); // geometry + idempotency UUIDs

    const std::uint64_t colorCount = read_binary_value<std::uint64_t>(stream);
    stream.seekg(static_cast<std::streamoff>(colorCount * sizeof(float)), std::ios::cur);
    values.push_back(x);
  }

  return values;
}

} // namespace

TEST_F(GeoQikTestApi, CustomWindowSettings)
{
  geoqik_window_settings_t settings;
  geoqik_init_default_window_settings(&settings);
  settings.title = "Custom GeoQik Test";
  settings.width = 1024;
  settings.height = 768;
  settings.samples = 4;
  settings.visible = 0; // Make window invisible for testing

  geoqik_settings_t geoSettings;
  geoqik_create_default_settings(&geoSettings);

  geoqik_init_with_settings(&geoSettings, &settings);
  // Can't easily verify the settings took effect without accessing internals
  // This test primarily checks that custom initialization doesn't crash
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, generate_uuid)
{
  geoqik_uuid_t uuid1;
  geoqik_uuid_t uuid2;
  geoqik_error_code_t res1 = geoqik_generate_uuid(&uuid1);
  geoqik_error_code_t res2 = geoqik_generate_uuid(&uuid2);

  ASSERT_EQ(GEOQIK_SUCCESS, res1);
  ASSERT_EQ(GEOQIK_SUCCESS, res2);

  bool areEqual = true;
  for (size_t i = 0; i < 16; ++i)
  {
    if (uuid1.value[i] != uuid2.value[i])
    {
      areEqual = false;
      break;
    }
  }
  EXPECT_FALSE(areEqual);
}

TEST_F(GeoQikTestApi, CombinedOperations)
{
  geoqik_init();

  geoqik_set_point_size(4.0f);
  geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f); // Red

  geoqik_add_point(-1.0, -1.0, -1.0);
  geoqik_add_point(-1.0, 1.0, 1.0);

  geoqik_set_line_width(2.0f);
  geoqik_set_line_color(0.0f, 0.0f, 1.0f, 1.0f); // Blue

  geoqik_add_line(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  geoqik_add_line(1.0, -1.0, -1.0, -1.0, 1.0, 1.0);

  // Verify attributes were set correctly
  float pointSize;
  geoqik_get_point_size(&pointSize);
  EXPECT_FLOAT_EQ(pointSize, 4.0f);

  float pointR;
  float pointG;
  float pointB;
  float pointA;
  geoqik_get_point_color(&pointR, &pointG, &pointB, &pointA);
  EXPECT_FLOAT_EQ(pointR, 1.0f);
  EXPECT_FLOAT_EQ(pointG, 0.0f);
  EXPECT_FLOAT_EQ(pointB, 0.0f);
  EXPECT_FLOAT_EQ(pointA, 1.0f);

  float lineWidth;
  geoqik_get_line_width(&lineWidth);
  EXPECT_FLOAT_EQ(lineWidth, 2.0f);

  float lineR;
  float lineG;
  float lineB;
  float lineA;
  geoqik_get_line_color(&lineR, &lineG, &lineB, &lineA);
  EXPECT_FLOAT_EQ(lineR, 0.0f);
  EXPECT_FLOAT_EQ(lineG, 0.0f);
  EXPECT_FLOAT_EQ(lineB, 1.0f);
  EXPECT_FLOAT_EQ(lineA, 1.0f);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, InitDestroySequence)
{
  for (int i = 0; i < 3; i++)
  {
    geoqik_init();
    geoqik_add_point(0.0, 0.0, 0.0);
    geoqik_draw();
    geoqik_cleanup();
  }
}

TEST_F(GeoQikTestApi, Draw)
{
  geoqik_init();

  geoqik_add_point(0.0, 0.0, 0.0);
  geoqik_add_line(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  // Test that draw can be called multiple times without issues
  geoqik_draw();
  geoqik_draw();
  geoqik_stop_drawing();
  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, SaveLoadLogRoundTrip)
{
  const std::filesystem::path firstPath = std::filesystem::temp_directory_path() / "geoqik_api_log_roundtrip_first.gqklog";
  const std::filesystem::path secondPath = std::filesystem::temp_directory_path() / "geoqik_api_log_roundtrip_second.gqklog";
  std::filesystem::remove(firstPath);
  std::filesystem::remove(secondPath);

  init_hidden_geoqik();

  geoqik_set_point_size(6.0f);
  geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f);
  geoqik_result_t point = geoqik_add_point(1.0, 2.0, 3.0);
  ASSERT_EQ(GEOQIK_SUCCESS, point.err);

  const double points[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  const float pointColors[] = {0.0f, 1.0f, 0.0f, 1.0f};
  geoqik_add_points_options_t pointOptions{};
  pointOptions.color = pointColors;
  pointOptions.colorCount = 4;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_points_opts(points, sizeof(points) / sizeof(points[0]), &pointOptions).err);

  geoqik_set_line_width(2.0f);
  geoqik_set_line_color(0.0f, 0.0f, 1.0f, 1.0f);
  geoqik_result_t line = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, nullptr);
  ASSERT_EQ(GEOQIK_SUCCESS, line.err);

  const double lines[] = {0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_lines_opts(lines, sizeof(lines) / sizeof(lines[0]), nullptr).err);

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_translate_geometry(&point.geometryId, 1.0, 2.0, 3.0));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_rotate_geometry(&line.geometryId, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 45.0));

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(firstPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_remove_all_geometry());
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_load_log(firstPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(secondPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  EXPECT_EQ(read_binary_file(firstPath), read_binary_file(secondPath));

  std::filesystem::remove(firstPath);
  std::filesystem::remove(secondPath);
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, SaveLoadLogValidation)
{
  const std::filesystem::path validPath = std::filesystem::temp_directory_path() / "geoqik_api_validation_log.gqklog";
  const std::filesystem::path missingPath = std::filesystem::temp_directory_path() / "geoqik_api_missing_log.gqklog";
  std::filesystem::remove(validPath);
  std::filesystem::remove(missingPath);

  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_save_log(nullptr, GEOQIK_LOG_FORMAT_BINARY));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_load_log("", GEOQIK_LOG_FORMAT_BINARY));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_save_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_load_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  init_hidden_geoqik();

  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_save_log("", GEOQIK_LOG_FORMAT_BINARY));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_save_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42)));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_load_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42)));
  EXPECT_EQ(GEOQIK_ERROR_UNKNOWN, geoqik_load_log(missingPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, ReplayLogValidation)
{
  const std::filesystem::path validPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_validation_log.gqklog";
  const std::filesystem::path missingPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_missing_log.gqklog";
  std::filesystem::remove(validPath);
  std::filesystem::remove(missingPath);

  geoqik_replay_options_t invalidOptions{};
  invalidOptions.entriesPerSecond = -1.0;

  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(nullptr, GEOQIK_LOG_FORMAT_BINARY, nullptr));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log("", GEOQIK_LOG_FORMAT_BINARY, nullptr));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42), nullptr));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, nullptr));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_replay_current_log(nullptr));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_cancel_replay());

  init_hidden_geoqik();

  EXPECT_EQ(GEOQIK_ERROR_UNKNOWN, geoqik_replay_log(missingPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, nullptr));

  invalidOptions.entriesPerSecond = 0.0;
  invalidOptions.speedMultiplier = -1.0;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_current_log(&invalidOptions));

  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, ReplayLogRoundTrip)
{
  const std::filesystem::path firstPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_roundtrip_first.gqklog";
  const std::filesystem::path secondPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_roundtrip_second.gqklog";
  std::filesystem::remove(firstPath);
  std::filesystem::remove(secondPath);

  init_hidden_geoqik();

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_set_point_size(5.0f));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(1.0, 2.0, 3.0).err);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_line(0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(firstPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_remove_all_geometry());

  geoqik_replay_options_t options{};
  options.entriesPerSecond = 100000.0;
  options.maxEntriesPerFrame = 100000;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_log(firstPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &options));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(secondPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  EXPECT_EQ(read_binary_file(firstPath), read_binary_file(secondPath));

  std::filesystem::remove(firstPath);
  std::filesystem::remove(secondPath);
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, ReplayCurrentLogRoundTrip)
{
  const std::filesystem::path beforePath = std::filesystem::temp_directory_path() / "geoqik_api_replay_current_before.gqklog";
  const std::filesystem::path afterPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_current_after.gqklog";
  std::filesystem::remove(beforePath);
  std::filesystem::remove(afterPath);

  init_hidden_geoqik();

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_set_line_width(3.0f));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_line(0.0, 0.0, 0.0, 1.0, 0.0, 0.0));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(2.0, 3.0, 4.0).err);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(beforePath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  geoqik_replay_options_t options{};
  options.entriesPerSecond = 100000.0;
  options.maxEntriesPerFrame = 100000;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_current_log(&options));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(afterPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  EXPECT_EQ(read_binary_file(beforePath), read_binary_file(afterPath));

  std::filesystem::remove(beforePath);
  std::filesystem::remove(afterPath);
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, ReplayDefersQueuedMessagesUntilDone)
{
  const std::filesystem::path replayPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_deferred_source.gqklog";
  const std::filesystem::path actualPath = std::filesystem::temp_directory_path() / "geoqik_api_replay_deferred_actual.gqklog";
  std::filesystem::remove(replayPath);
  std::filesystem::remove(actualPath);

  init_hidden_geoqik();

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(1.0, 0.0, 0.0).err);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(replayPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  geoqik_replay_options_t options{};
  options.entriesPerSecond = 1.0;
  options.maxEntriesPerFrame = 1;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_log(replayPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &options));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(2.0, 0.0, 0.0).err);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(actualPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  const std::vector<float> xValues = read_add_point_x_values(actualPath);
  ASSERT_EQ(2, xValues.size());
  EXPECT_FLOAT_EQ(1.0f, xValues[0]);
  EXPECT_FLOAT_EQ(2.0f, xValues[1]);

  std::filesystem::remove(replayPath);
  std::filesystem::remove(actualPath);
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, CancelReplayAllowsSubsequentCallsToComplete)
{
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "geoqik_api_replay_cancel.gqklog";
  std::filesystem::remove(path);

  init_hidden_geoqik();

  for (int i = 0; i < 8; ++i)
  {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(static_cast<double>(i), 0.0, 0.0).err);
  }
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  geoqik_replay_options_t options{};
  options.entriesPerSecond = 1.0;
  options.maxEntriesPerFrame = 1;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &options));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(99.0, 0.0, 0.0).err);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  std::filesystem::remove(path);
  geoqik_cleanup();
}
