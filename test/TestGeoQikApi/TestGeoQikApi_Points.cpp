#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>

class TestGeoQikApi_Points : public ::testing::Test
{
};

namespace
{

geoqik_error_code_t init_hidden_geoqik(const geoqik_settings_t* settings = nullptr)
{
  geoqik_window_settings_t windowSettings;
  geoqik_init_default_window_settings(&windowSettings);
  windowSettings.visible = 0;

  if (settings != nullptr)
  {
    return geoqik_init_with_settings(settings, &windowSettings);
  }

  geoqik_settings_t defaultSettings;
  geoqik_create_default_settings(&defaultSettings);
  return geoqik_init_with_settings(&defaultSettings, &windowSettings);
}

} // namespace

TEST_F(TestGeoQikApi_Points, AddPoint)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  geoqik_result_t result1 = geoqik_add_point(0.0, 0.0, 0.0);
  geoqik_result_t result2 = geoqik_add_point(1.0, 0.0, 0.0);
  geoqik_result_t result3 = geoqik_add_point(0.0, 1.0, 0.0);

  EXPECT_EQ(result1.err, GEOQIK_SUCCESS);
  EXPECT_EQ(result2.err, GEOQIK_SUCCESS);
  EXPECT_EQ(result3.err, GEOQIK_SUCCESS);

  const geoqik_uuid_t nilUuid{};
  EXPECT_NE(0, memcmp(result1.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
  EXPECT_NE(0, memcmp(result2.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
  EXPECT_NE(0, memcmp(result3.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, AddPointWithHandle)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  geoqik_draw();

  geoqik_uuid_t pointId1 = geoqik_add_point(0.0, 0.0, 0.0).geometryId;
  geoqik_uuid_t pointId2 = geoqik_add_point(1.0, 0.0, 0.0).geometryId;
  geoqik_uuid_t pointId3 = geoqik_add_point(0.0, 1.0, 0.0).geometryId;

  // Now remove the second point
  geoqik_remove_point(&pointId2);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, CheckMaximumPointsCapacity)
{
  geoqik_settings_t settings;
  geoqik_create_default_settings(&settings);
  settings.initialPointCapacity = 3;

  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik(&settings));

  for (int i = 0; i < settings.initialPointCapacity; ++i)
  {
    geoqik_add_point(static_cast<double>(i) / 10.0, static_cast<double>(i) / 20.0, static_cast<double>(i) / 30.0);
  }
  geoqik_add_point(1.0, 1.0, 1.0); // Adding one more point exceeds the intial capacity
  geoqik_draw();
  geoqik_cleanup();
}


TEST_F(TestGeoQikApi_Points, PointSizeGetSet)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  float initialSize;
  geoqik_get_point_size(&initialSize);

  const float newSize = 5.0f;
  geoqik_set_point_size(newSize);

  float retrievedSize;
  geoqik_get_point_size(&retrievedSize);
  EXPECT_EQ(newSize, retrievedSize);

  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, PointColorGetSet)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  float initialR;
  float initialG;
  float initialB;
  float initialA;
  geoqik_get_point_color(&initialR, &initialG, &initialB, &initialA);

  const float newR = 0.7f;
  const float newG = 0.8f;
  const float newB = 0.9f;
  const float newA = 0.6f;
  geoqik_set_point_color(newR, newG, newB, newA);

  float retrievedR, retrievedG, retrievedB, retrievedA;
  geoqik_get_point_color(&retrievedR, &retrievedG, &retrievedB, &retrievedA);

  EXPECT_FLOAT_EQ(newR, retrievedR);
  EXPECT_FLOAT_EQ(newG, retrievedG);
  EXPECT_FLOAT_EQ(newB, retrievedB);
  EXPECT_FLOAT_EQ(newA, retrievedA);
  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, AddPointWithColor)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  const float r = 0.5f, g = 0.6f, b = 0.7f, a = 0.8f;

  geoqik_result_t result1 = geoqik_add_point_with_color(0.0, 0.0, 0.0, r, g, b, a);
  geoqik_result_t result2 = geoqik_add_point_with_color(1.0, 1.0, 1.0, r, g, b, a);

  EXPECT_EQ(result1.err, GEOQIK_SUCCESS);
  EXPECT_EQ(result2.err, GEOQIK_SUCCESS);

  const geoqik_uuid_t nilUuid{};
  EXPECT_NE(0, memcmp(result1.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
  EXPECT_NE(0, memcmp(result2.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, UpdatePointValidation)
{
  geoqik_uuid_t id{};

  EXPECT_EQ(geoqik_update_point(nullptr, 1.0, 2.0, 3.0), GEOQIK_ERROR_INVALID_PARAMETER);
  EXPECT_EQ(geoqik_update_point(&id, NAN, 2.0, 3.0), GEOQIK_ERROR_INVALID_PARAMETER);

  const double points[] = {1.0, 2.0};
  EXPECT_EQ(geoqik_update_points_opts(&id, points, 2, nullptr), GEOQIK_ERROR_INVALID_PARAMETER);

  const double validPoints[] = {1.0, 2.0, 3.0};
  const float invalidColors[] = {0.0f, 1.0f, 0.0f};
  geoqik_update_points_options_t opts{};
  opts.color = invalidColors;
  opts.colorCount = 3;
  EXPECT_EQ(geoqik_update_points_opts(&id, validPoints, 3, &opts), GEOQIK_ERROR_WRONG_COLOR_SIZE);

  const float outOfRangeColor[] = {0.0f, 1.1f, 0.0f, 1.0f};
  opts.color = outOfRangeColor;
  opts.colorCount = 4;
  EXPECT_EQ(geoqik_update_points_opts(&id, validPoints, 3, &opts), GEOQIK_ERROR_INVALID_PARAMETER);

  opts.color = nullptr;
  opts.colorCount = 4;
  EXPECT_EQ(geoqik_update_point_opts(&id, 1.0, 2.0, 3.0, &opts), GEOQIK_ERROR_INVALID_PARAMETER);
}

TEST_F(TestGeoQikApi_Points, UpdatePointEnqueues)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  geoqik_result_t result = geoqik_add_point(0.0, 0.0, 0.0);
  ASSERT_EQ(result.err, GEOQIK_SUCCESS);

  EXPECT_EQ(geoqik_update_point(&result.geometryId, 1.0, 2.0, 3.0), GEOQIK_SUCCESS);
  EXPECT_EQ(geoqik_update_point_with_color(&result.geometryId, 4.0, 5.0, 6.0, 0.1f, 0.2f, 0.3f, 1.0f), GEOQIK_SUCCESS);

  geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, UpdatePointsEnqueues)
{
  ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

  const double initialPoints[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  geoqik_result_t result = geoqik_add_points_opts(initialPoints, sizeof(initialPoints) / sizeof(initialPoints[0]), nullptr);
  ASSERT_EQ(result.err, GEOQIK_SUCCESS);

  const double updatedPoints[] = {2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
  const float updatedColors[] = {
      1.0f, 0.0f, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 1.0f,
  };
  geoqik_update_points_options_t opts{};
  opts.color = updatedColors;
  opts.colorCount = sizeof(updatedColors) / sizeof(updatedColors[0]);

  EXPECT_EQ(geoqik_update_points_opts(&result.geometryId, updatedPoints, sizeof(updatedPoints) / sizeof(updatedPoints[0]), &opts), GEOQIK_SUCCESS);

  geoqik_cleanup();
}
