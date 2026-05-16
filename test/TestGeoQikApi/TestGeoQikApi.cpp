#include <GeoQik/GeoQik.hpp>
#include <cmath>
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
