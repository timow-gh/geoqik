#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <thread>
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

TEST_F(GeoQikTestApi, InitializationStatusAndErrorStrings)
{
  bool isInitialized = true;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_is_api_initialized(nullptr));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_is_api_initialized(&isInitialized));
  EXPECT_FALSE(isInitialized);

  EXPECT_STREQ("Success", geoqik_get_error_string(GEOQIK_SUCCESS));
  EXPECT_STREQ("GeoQik not initialized", geoqik_get_error_string(GEOQIK_ERROR_NOT_INITIALIZED));
  EXPECT_STREQ("GeoQik already initialized", geoqik_get_error_string(GEOQIK_ERROR_ALREADY_INITIALIZED));
  EXPECT_STREQ("Invalid parameter", geoqik_get_error_string(GEOQIK_ERROR_INVALID_PARAMETER));
  EXPECT_STREQ("Wrong RGBA color size", geoqik_get_error_string(GEOQIK_ERROR_WRONG_COLOR_SIZE));
  EXPECT_STREQ("Memory allocation error", geoqik_get_error_string(GEOQIK_ERROR_MEMORY_ALLOCATION));
  EXPECT_STREQ("Unknown error", geoqik_get_error_string(GEOQIK_ERROR_UNKNOWN));
  EXPECT_STREQ("Renderer initialization failed", geoqik_get_error_string(GEOQIK_ERROR_RENDERER_INIT_FAILED));
  EXPECT_STREQ("I/O error", geoqik_get_error_string(GEOQIK_ERROR_IO));
  EXPECT_STREQ("Unsupported format", geoqik_get_error_string(GEOQIK_ERROR_UNSUPPORTED_FORMAT));
  EXPECT_STREQ("Invalid state", geoqik_get_error_string(GEOQIK_ERROR_INVALID_STATE));
  EXPECT_STREQ("Invalid error code", geoqik_get_error_string(static_cast<geoqik_error_code_t>(42)));

  init_hidden_geoqik();
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_is_api_initialized(&isInitialized));
  EXPECT_TRUE(isInitialized);
  geoqik_cleanup();

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_is_api_initialized(&isInitialized));
  EXPECT_FALSE(isInitialized);
}

TEST_F(GeoQikTestApi, LastErrorInfoReportsDetailsAndClearsOnSuccess)
{
  geoqik_clear_last_error();

  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_set_point_size(0.0f));

  geoqik_error_info_t info{};
  info.struct_size = sizeof(info);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_last_error_info(&info));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, info.code);
  EXPECT_STREQ("geoqik_set_point_size", info.operation);
  EXPECT_NE(nullptr, info.what);
  EXPECT_NE(nullptr, info.why);
  EXPECT_NE(nullptr, info.action);
  EXPECT_NE(std::string(info.what).find("parameters"), std::string::npos);
  EXPECT_STREQ("parameter: pointSize; expected > 0", info.details);

  geoqik_uuid_t uuid{};
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_generate_uuid(&uuid));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_last_error_info(&info));
  EXPECT_EQ(GEOQIK_SUCCESS, info.code);
}

TEST_F(GeoQikTestApi, LastErrorInfoIsThreadLocal)
{
  geoqik_clear_last_error();

  std::thread worker([] {
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_get_point_size(nullptr));
    geoqik_error_info_t info{};
    info.struct_size = sizeof(info);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_last_error_info(&info));
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, info.code);
    EXPECT_STREQ("geoqik_get_point_size", info.operation);
  });
  worker.join();

  geoqik_error_info_t info{};
  info.struct_size = sizeof(info);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_last_error_info(&info));
  EXPECT_EQ(GEOQIK_SUCCESS, info.code);
}

TEST_F(GeoQikTestApi, InitWhileInitializedReturnsAlreadyInitialized)
{
  init_hidden_geoqik();

  geoqik_settings_t geoSettings;
  geoqik_create_default_settings(&geoSettings);
  geoqik_window_settings_t windowSettings;
  geoqik_init_default_window_settings(&windowSettings);
  windowSettings.visible = 0;

  EXPECT_EQ(GEOQIK_ERROR_ALREADY_INITIALIZED, geoqik_init_with_settings(&geoSettings, &windowSettings));
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, CombinedOperations)
{
  init_hidden_geoqik();

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
    init_hidden_geoqik();
    geoqik_add_point(0.0, 0.0, 0.0);
    geoqik_draw();
    geoqik_cleanup();
  }
}

TEST_F(GeoQikTestApi, Draw)
{
  init_hidden_geoqik();

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
  EXPECT_EQ(GEOQIK_ERROR_UNSUPPORTED_FORMAT, geoqik_save_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42)));
  EXPECT_EQ(GEOQIK_ERROR_UNSUPPORTED_FORMAT, geoqik_load_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42)));
  EXPECT_EQ(GEOQIK_ERROR_IO, geoqik_load_log(missingPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

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
  EXPECT_EQ(GEOQIK_ERROR_UNSUPPORTED_FORMAT, geoqik_replay_log(validPath.string().c_str(), static_cast<geoqik_log_format_t>(42), nullptr));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));
  invalidOptions.entriesPerSecond = INFINITY;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));
  invalidOptions.entriesPerSecond = 0.0;
  invalidOptions.speedMultiplier = INFINITY;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));
  invalidOptions.speedMultiplier = 1.0;
  invalidOptions.stepKeys = nullptr;
  invalidOptions.stepKeyCount = 1;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));

  geoqik_key_t invalidKey = GEOQIK_KEY_UNKNOWN;
  invalidOptions.stepKeys = &invalidKey;
  invalidOptions.stepKeyCount = 1;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &invalidOptions));
  invalidOptions.stepKeys = nullptr;
  invalidOptions.stepKeyCount = 0;

  geoqik_key_t customStepKey = GEOQIK_KEY_J;
  geoqik_key_t customBackwardStepKey = GEOQIK_KEY_K;
  geoqik_key_t customResumeKey = GEOQIK_KEY_L;
  geoqik_key_t customPauseKey = GEOQIK_KEY_M;
  geoqik_key_t customIncreaseKey = GEOQIK_KEY_N;
  geoqik_key_t customDecreaseKey = GEOQIK_KEY_O;
  geoqik_replay_options_t customKeyOptions{};
  customKeyOptions.entriesPerSecond = 30.0;
  customKeyOptions.speedMultiplier = 2.0;
  customKeyOptions.maxEntriesPerFrame = 3;
  customKeyOptions.startPaused = 1;
  customKeyOptions.entriesPerStep = 2;
  customKeyOptions.stepKeys = &customStepKey;
  customKeyOptions.stepKeyCount = 1;
  customKeyOptions.backwardStepKeys = &customBackwardStepKey;
  customKeyOptions.backwardStepKeyCount = 1;
  customKeyOptions.resumeKeys = &customResumeKey;
  customKeyOptions.resumeKeyCount = 1;
  customKeyOptions.pauseKeys = &customPauseKey;
  customKeyOptions.pauseKeyCount = 1;
  customKeyOptions.increaseEntriesPerStepKeys = &customIncreaseKey;
  customKeyOptions.increaseEntriesPerStepKeyCount = 1;
  customKeyOptions.decreaseEntriesPerStepKeys = &customDecreaseKey;
  customKeyOptions.decreaseEntriesPerStepKeyCount = 1;
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &customKeyOptions));

  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_replay_log(validPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, nullptr));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_replay_current_log(nullptr));
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_cancel_replay());
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_pause_replay());
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_resume_replay());
  EXPECT_EQ(GEOQIK_ERROR_NOT_INITIALIZED, geoqik_step_replay());

  init_hidden_geoqik();

  EXPECT_EQ(GEOQIK_ERROR_IO, geoqik_replay_log(missingPath.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, nullptr));

  invalidOptions.entriesPerSecond = 0.0;
  invalidOptions.speedMultiplier = -1.0;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_replay_current_log(&invalidOptions));

  geoqik_replay_state_t state = GEOQIK_REPLAY_PLAYING;
  std::size_t currentEntry = 42;
  std::size_t totalEntries = 42;
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_step_replay_n(0));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_get_replay_state(nullptr));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_get_replay_progress(nullptr, &totalEntries));
  EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_get_replay_progress(&currentEntry, nullptr));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
  EXPECT_EQ(GEOQIK_REPLAY_INACTIVE, state);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_progress(&currentEntry, &totalEntries));
  EXPECT_EQ(0, currentEntry);
  EXPECT_EQ(0, totalEntries);

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

TEST_F(GeoQikTestApi, PauseStepAndResumeReplayExposeStateAndProgress)
{
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "geoqik_api_replay_controls.gqklog";
  std::filesystem::remove(path);

  init_hidden_geoqik();

  for (int i = 0; i < 5; ++i)
  {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(static_cast<double>(i), 0.0, 0.0).err);
  }
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

  geoqik_replay_options_t options{};
  options.entriesPerSecond = 1.0;
  options.maxEntriesPerFrame = 1;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &options));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_pause_replay());

  geoqik_replay_state_t state = GEOQIK_REPLAY_INACTIVE;
  std::size_t currentEntry = 0;
  std::size_t totalEntries = 0;
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
  EXPECT_EQ(GEOQIK_REPLAY_PAUSED, state);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_progress(&currentEntry, &totalEntries));
  EXPECT_LE(currentEntry, totalEntries);
  EXPECT_EQ(5, totalEntries);

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_step_replay_n(2));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
  EXPECT_EQ(GEOQIK_REPLAY_PAUSED, state);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_progress(&currentEntry, &totalEntries));
  EXPECT_EQ(5, totalEntries);
  EXPECT_GE(currentEntry, 2);
  const std::size_t progressAfterForwardStep = currentEntry;

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_step_replay_backward_n(1));
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
  EXPECT_EQ(GEOQIK_REPLAY_PAUSED, state);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_progress(&currentEntry, &totalEntries));
  EXPECT_EQ(5, totalEntries);
  EXPECT_LT(currentEntry, progressAfterForwardStep);

  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_resume_replay());
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
  EXPECT_EQ(GEOQIK_REPLAY_PLAYING, state);
  ASSERT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());

  std::filesystem::remove(path);
  geoqik_cleanup();
}
