#include <GeoQik/GeoQik.hpp>
#include <cmath>
#include <gtest/gtest.h>

class GeoQikTest_Lines : public ::testing::Test
{
};


TEST_F(GeoQikTest_Lines, AddLine)
{
  geoqik_init();

  geoqik_add_line(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
  geoqik_add_line(0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  geoqik_add_line(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, CheckMaximumLinesCapacity)
{
  geoqik_settings_t settings;
  geoqik_init_default_settings(&settings);
  settings.initialLineCapacity = 3;

  geoqik_init_with_settings(&settings, nullptr);

  for (int i = 0; i < settings.initialLineCapacity; ++i)
  {
    geoqik_add_line(static_cast<double>(i) / 10.0,
                     static_cast<double>(i) / 20.0,
                     static_cast<double>(i) / 30.0,
                     static_cast<double>(i + 1) / 10.0,
                     static_cast<double>(i + 1) / 20.0,
                     static_cast<double>(i + 1) / 30.0);
  }
  geoqik_add_line(1.0, 1.0, 1.0, 2.0, 2.0, 2.0);
  geoqik_draw();
  geoqik_cleanup();
}


TEST_F(GeoQikTest_Lines, LineWidthGetSet)
{
  geoqik_init();

  float initialWidth;
  geoqik_get_line_width(&initialWidth);

  const float newWidth = 3.0f;
  geoqik_set_line_width(newWidth);

  float retrievedWidth;
  geoqik_get_line_width(&retrievedWidth);
  EXPECT_FLOAT_EQ(newWidth, retrievedWidth);
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, LineColorGetSet)
{
  geoqik_init();

  float initialR;
  float initialG;
  float initialB;
  geoqik_get_line_color(&initialR, &initialG, &initialB);

  const float newR = 0.2f;
  const float newG = 0.3f;
  const float newB = 0.4f;
  geoqik_set_line_color(newR, newG, newB);

  float retrievedR;
  float retrievedG;
  float retrievedB;
  geoqik_get_line_color(&retrievedR, &retrievedG, &retrievedB);

  EXPECT_FLOAT_EQ(newR, retrievedR);
  EXPECT_FLOAT_EQ(newG, retrievedG);
  EXPECT_FLOAT_EQ(newB, retrievedB);
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithColor)
{
  geoqik_init();

  geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f); // Red line
  geoqik_add_line_with_color(1.0, 1.0, 1.0, 2.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f); // Green line

  geoqik_draw();
  geoqik_cleanup();
}