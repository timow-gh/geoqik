#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <gtest/gtest.h>

class GeoQikTestApi : public ::testing::Test
{
};

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
  geoqik_init_default_settings(&geoSettings);

  geoqik_init_with_settings(&geoSettings, &settings);
  // Can't easily verify the settings took effect without accessing internals
  // This test primarily checks that custom initialization doesn't crash
  geoqik_cleanup();
}

TEST_F(GeoQikTestApi, generate_uuid)
{
  geoqik_uuid_t uuid1;
  geoqik_uuid_t uuid2;
  geoqik_result_t res1 = geoqik_generate_uuid(&uuid1);
  geoqik_result_t res2 = geoqik_generate_uuid(&uuid2);

  ASSERT_EQ(GEOQIK_SUCCESS, res1.err);
  ASSERT_EQ(GEOQIK_SUCCESS, res2.err);

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
  geoqik_set_point_color(1.0f, 0.0f, 0.0f); // Red

  geoqik_add_point(-1.0, -1.0, -1.0);
  geoqik_add_point(-1.0, 1.0, 1.0);

  geoqik_set_line_width(2.0f);
  geoqik_set_line_color(0.0f, 0.0f, 1.0f); // Blue

  geoqik_add_line(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0);
  geoqik_add_line(1.0, -1.0, -1.0, -1.0, 1.0, 1.0);

  // Verify attributes were set correctly
  float pointSize;
  geoqik_get_point_size(&pointSize);
  EXPECT_FLOAT_EQ(pointSize, 4.0f);

  float pointR;
  float pointG;
  float pointB;
  geoqik_get_point_color(&pointR, &pointG, &pointB);
  EXPECT_FLOAT_EQ(pointR, 1.0f);
  EXPECT_FLOAT_EQ(pointG, 0.0f);
  EXPECT_FLOAT_EQ(pointB, 0.0f);

  float lineWidth;
  geoqik_get_line_width(&lineWidth);
  EXPECT_FLOAT_EQ(lineWidth, 2.0f);

  float lineR;
  float lineG;
  float lineB;
  geoqik_get_line_color(&lineR, &lineG, &lineB);
  EXPECT_FLOAT_EQ(lineR, 0.0f);
  EXPECT_FLOAT_EQ(lineG, 0.0f);
  EXPECT_FLOAT_EQ(lineB, 1.0f);

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
