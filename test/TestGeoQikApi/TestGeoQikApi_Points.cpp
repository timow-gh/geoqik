#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>

class GeoQikTest_Points : public ::testing::Test
{
};

TEST_F(GeoQikTest_Points, AddPoint)
{
  geoqik_init();

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

TEST_F(GeoQikTest_Points, AddPointWithHandle)
{
  geoqik_init();

  geoqik_draw();

  geoqik_uuid_t pointId1 = geoqik_add_point_with_id(0.0, 0.0, 0.0, nullptr).geometryId;
  geoqik_uuid_t pointId2 = geoqik_add_point_with_id(1.0, 0.0, 0.0, nullptr).geometryId;
  geoqik_uuid_t pointId3 = geoqik_add_point_with_id(0.0, 1.0, 0.0, nullptr).geometryId;

  // Now remove the second point
  geoqik_remove_point(&pointId2);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Points, CheckMaximumPointsCapacity)
{
  geoqik_settings_t settings;
  geoqik_init_default_settings(&settings);
  settings.initialPointCapacity = 3;

  geoqik_init_with_settings(&settings, nullptr);

  for (int i = 0; i < settings.initialPointCapacity; ++i)
  {
    geoqik_add_point(static_cast<double>(i) / 10.0, static_cast<double>(i) / 20.0, static_cast<double>(i) / 30.0);
  }
  geoqik_add_point(1.0, 1.0, 1.0); // Adding one more point exceeds the intial capacity
  geoqik_draw();
  geoqik_cleanup();
}


TEST_F(GeoQikTest_Points, PointSizeGetSet)
{
  geoqik_init();

  float initialSize;
  geoqik_get_point_size(&initialSize);

  const float newSize = 5.0f;
  geoqik_set_point_size(newSize);

  float retrievedSize;
  geoqik_get_point_size(&retrievedSize);
  EXPECT_EQ(newSize, retrievedSize);

  geoqik_cleanup();
}

TEST_F(GeoQikTest_Points, PointColorGetSet)
{
  geoqik_init();

  float initialR;
  float initialG;
  float initialB;
  geoqik_get_point_color(&initialR, &initialG, &initialB);

  const float newR = 0.7f;
  const float newG = 0.8f;
  const float newB = 0.9f;
  geoqik_set_point_color(newR, newG, newB);

  float retrievedR, retrievedG, retrievedB;
  geoqik_get_point_color(&retrievedR, &retrievedG, &retrievedB);

  EXPECT_FLOAT_EQ(newR, retrievedR);
  EXPECT_FLOAT_EQ(newG, retrievedG);
  EXPECT_FLOAT_EQ(newB, retrievedB);
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Points, AddPointWithColor)
{
  geoqik_init();

  const float r = 0.5f, g = 0.6f, b = 0.7f;

  geoqik_result_t result1 = geoqik_add_point_with_color(0.0, 0.0, 0.0, r, g, b);
  geoqik_result_t result2 = geoqik_add_point_with_color(1.0, 1.0, 1.0, r, g, b);

  EXPECT_EQ(result1.err, GEOQIK_SUCCESS);
  EXPECT_EQ(result2.err, GEOQIK_SUCCESS);

  const geoqik_uuid_t nilUuid{};
  EXPECT_NE(0, memcmp(result1.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
  EXPECT_NE(0, memcmp(result2.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));

  geoqik_draw();
  geoqik_cleanup();
}
