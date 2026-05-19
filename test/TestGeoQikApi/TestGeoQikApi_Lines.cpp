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
  geoqik_create_default_settings(&settings);
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
  float initialA;
  geoqik_get_line_color(&initialR, &initialG, &initialB, &initialA);

  const float newR = 0.2f;
  const float newG = 0.3f;
  const float newB = 0.4f;
  const float newA = 0.5f;
  geoqik_set_line_color(newR, newG, newB, newA);

  float retrievedR;
  float retrievedG;
  float retrievedB;
  float retrievedA;
  geoqik_get_line_color(&retrievedR, &retrievedG, &retrievedB, &retrievedA);

  EXPECT_FLOAT_EQ(newR, retrievedR);
  EXPECT_FLOAT_EQ(newG, retrievedG);
  EXPECT_FLOAT_EQ(newB, retrievedB);
  EXPECT_FLOAT_EQ(newA, retrievedA);
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithColor)
{
  geoqik_init();

  geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f); // Red line
  geoqik_add_line_with_color(1.0, 1.0, 1.0, 2.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f, 1.0f); // Green line

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithIdempotency)
{
  geoqik_init();

  geoqik_uuid_t key;
  geoqik_generate_uuid(&key);

  geoqik_add_line_opts_t opts{};
  opts.idempotencyKey = key;

  geoqik_result_t first = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, &opts);
  EXPECT_EQ(first.err, GEOQIK_SUCCESS);

  geoqik_result_t second = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, &opts);
  EXPECT_EQ(second.err, GEOQIK_SUCCESS);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithColorOpts)
{
  geoqik_init();

  const float color[4] = {0.0f, 0.0f, 1.0f, 1.0f}; // Blue
  geoqik_add_line_opts_t opts{};
  opts.color = color;
  opts.colorCount = 4;

  geoqik_result_t result = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, &opts);
  EXPECT_EQ(result.err, GEOQIK_SUCCESS);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLinesWithOpts)
{
  geoqik_init();

  // Three lines, each with 6 doubles (x1,y1,z1, x2,y2,z2)
  const double lines[] = {
      0.0, 0.0, 0.0,  1.0, 0.0, 0.0,
      0.0, 0.0, 0.0,  0.0, 1.0, 0.0,
      0.0, 0.0, 0.0,  0.0, 0.0, 1.0,
  };
  // One color per line (RGBA * 3 lines = 12 floats)
  const float colors[] = {
      1.0f, 0.0f, 0.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
  };

  geoqik_add_line_opts_t opts{};
  opts.color = colors;
  opts.colorCount = 12;

  geoqik_result_t result = geoqik_add_lines_opts(lines, sizeof(lines) / sizeof(lines[0]), &opts);
  EXPECT_EQ(result.err, GEOQIK_SUCCESS);

  geoqik_draw();
  geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, UpdateLineValidation)
{
  geoqik_uuid_t id{};

  EXPECT_EQ(geoqik_update_line(nullptr, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0), GEOQIK_ERROR_INVALID_PARAMETER);
  EXPECT_EQ(geoqik_update_line(&id, 0.0, 0.0, 0.0, NAN, 1.0, 1.0), GEOQIK_ERROR_INVALID_PARAMETER);

  const double lines[] = {0.0, 0.0, 0.0, 1.0, 1.0};
  EXPECT_EQ(geoqik_update_lines_opts(&id, lines, 5, nullptr), GEOQIK_ERROR_INVALID_PARAMETER);

  const double validLines[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  const float invalidColors[] = {0.0f, 1.0f, 0.0f};
  geoqik_update_line_opts_t opts{};
  opts.color = invalidColors;
  opts.colorCount = 3;
  EXPECT_EQ(geoqik_update_lines_opts(&id, validLines, 6, &opts), GEOQIK_ERROR_WRONG_COLOR_SIZE);
}

TEST_F(GeoQikTest_Lines, UpdateLineEnqueues)
{
  geoqik_init();

  geoqik_result_t result = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, nullptr);
  ASSERT_EQ(result.err, GEOQIK_SUCCESS);

  EXPECT_EQ(geoqik_update_line(&result.geometryId, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0), GEOQIK_SUCCESS);
  EXPECT_EQ(geoqik_update_line_with_color(&result.geometryId, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 0.1f, 0.2f, 0.3f, 1.0f), GEOQIK_SUCCESS);

  geoqik_cleanup();
}
