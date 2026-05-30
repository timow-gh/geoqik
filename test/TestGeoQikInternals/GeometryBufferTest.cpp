#include "GeometryBuffers/LineBuffer.hpp"
#include "GeometryBuffers/PointBuffer.hpp"
#include "Core/UUID.hpp"
#include <cmath>
#include <gtest/gtest.h>

class GeometryBufferTest : public ::testing::Test
{
  void SetUp() override
  {
    m_settings.initialPointCapacity = 2;
    m_settings.initialLineCapacity = 2;
  }

  protected:
  geoqik::GeoQikSettings m_settings;
};

TEST_F(GeometryBufferTest, CreatePointBuffer)
{
  m_settings.initialPointCapacity = 1000;
  m_settings.initialLineCapacity = 500;

  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
}

TEST_F(GeometryBufferTest, CreateLineBuffer)
{
  m_settings.initialPointCapacity = 1000;
  m_settings.initialLineCapacity = 500;

  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddPoint)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);

  buffer->add_point(1.0f, 2.0f, 3.0f);
  auto points = buffer->get_points();

  EXPECT_EQ(points.size(), 3);
  EXPECT_EQ(buffer->get_point_colors().size(), 4);

  EXPECT_EQ(points[0], 1.0f);
  EXPECT_EQ(points[1], 2.0f);
  EXPECT_EQ(points[2], 3.0f);

  auto colors = buffer->get_point_colors();
  EXPECT_EQ(colors[0], 1.0f);
  EXPECT_EQ(colors[1], 1.0f);
  EXPECT_EQ(colors[2], 1.0f);
  EXPECT_EQ(colors[3], 1.0f);
}

TEST_F(GeometryBufferTest, RemovePoint)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID pointHandle = core::UUID::generate();
  buffer->add_point(1.0f, 2.0f, 3.0f, &pointHandle);
  std::span<const float> points = buffer->get_points();
  EXPECT_EQ(points.size(), 3);
  EXPECT_EQ(points[0], 1.0f);
  EXPECT_EQ(points[1], 2.0f);
  EXPECT_EQ(points[2], 3.0f);

  buffer->remove_point(pointHandle);
  EXPECT_EQ(buffer->get_points().size(), 0);

  core::UUID invalidHandle; // Nil UUID
  EXPECT_THROW(buffer->add_point(4.0f, 5.0f, 6.0f, &invalidHandle), std::runtime_error);
}

TEST_F(GeometryBufferTest, AddPoints_Empty)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);

  std::vector<float> emptyPoints;
  std::vector<float> emptyColors;
  buffer->add_points(emptyPoints, emptyColors);

  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_point_colors().size(), 0);
}

TEST_F(GeometryBufferTest, AddPoints_One)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);

  std::vector<float> points = {1.0f, 2.0f, 3.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f, 5.0f};
  buffer->add_points(points, colors);

  auto pointsSpan = buffer->get_points();
  EXPECT_EQ(pointsSpan.size(), 3);
  EXPECT_EQ(pointsSpan[0], 1.0f);
  EXPECT_EQ(pointsSpan[1], 2.0f);
  EXPECT_EQ(pointsSpan[2], 3.0f);

  auto colorsSpan = buffer->get_point_colors();
  EXPECT_EQ(colorsSpan.size(), 4);
  EXPECT_EQ(colorsSpan[0], 2.0f);
  EXPECT_EQ(colorsSpan[1], 3.0f);
  EXPECT_EQ(colorsSpan[2], 4.0f);
  EXPECT_EQ(colorsSpan[3], 5.0f);
}

TEST_F(GeometryBufferTest, AddPoints_Multiple)
{
  m_settings.initialPointCapacity = 5;
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);

  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
  std::vector<float> color = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  // Points with wrong size.
  EXPECT_THROW(buffer->add_points(points, color), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_points().size(), 0);
  points.push_back(6.0f);
  // Colors with wrong size.
  EXPECT_THROW(buffer->add_points(points, color), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_points().size(), 0);
  color.push_back(7.0f);
  color.push_back(8.0f);
  color.push_back(9.0f);

  buffer->add_points(points, color);

  auto pointsSpan = buffer->get_points();
  EXPECT_EQ(pointsSpan.size(), 6);
  EXPECT_EQ(pointsSpan[0], 1.0f);
  EXPECT_EQ(pointsSpan[1], 2.0f);
  EXPECT_EQ(pointsSpan[2], 3.0f);
  EXPECT_EQ(pointsSpan[3], 4.0f);
  EXPECT_EQ(pointsSpan[4], 5.0f);
  EXPECT_EQ(pointsSpan[5], 6.0f);

  auto colors = buffer->get_point_colors();
  EXPECT_EQ(colors.size(), 8);
  EXPECT_EQ(colors[0], 2.0f);
  EXPECT_EQ(colors[1], 3.0f);
  EXPECT_EQ(colors[2], 4.0f);
  EXPECT_EQ(colors[3], 5.0f);
  EXPECT_EQ(colors[4], 6.0f);
  EXPECT_EQ(colors[5], 7.0f);
  EXPECT_EQ(colors[6], 8.0f);
  EXPECT_EQ(colors[7], 9.0f);

  EXPECT_TRUE(buffer->points_have_changed());
  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->points_have_changed());

  // Add 3 more points to fill the buffer.
  std::vector<float> morePoints = {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
  std::vector<float> moreColors = {8.0f,  9.0f,  10.0f, 11.0f,
                                   12.0f, 13.0f, 14.0f, 15.0f,
                                   16.0f, 17.0f, 18.0f, 19.0f};
  buffer->add_points(morePoints, moreColors);
  EXPECT_EQ(buffer->get_points().size(), 15);
  EXPECT_EQ(buffer->get_point_colors().size(), 20);
  EXPECT_TRUE(buffer->points_have_changed());

  // Add more points to trigger buffer growth
  morePoints = {7.0f, 8.0f, 9.0f};
  moreColors = {8.0f, 9.0f, 10.0f, 11.0f};
  EXPECT_THROW(buffer->add_points(morePoints, moreColors), std::runtime_error);
}

TEST_F(GeometryBufferTest, RemovePoints_NilHandle)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID invalidHandle = core::UUID::nil();
  EXPECT_THROW(buffer->remove_point(invalidHandle), std::runtime_error);
}

TEST_F(GeometryBufferTest, RemovePoints_Empty)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID pointsHandle = core::UUID::generate();
  buffer->remove_point(pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 0);
}

TEST_F(GeometryBufferTest, RemovePoints_One)
{
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID pointsHandle = core::UUID::generate();
  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
  buffer->add_points(points, colors, &pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 6);

  buffer->remove_point(pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 0);
}

TEST_F(GeometryBufferTest, RemovePoints_Multiple)
{
  m_settings.initialPointCapacity = 6;
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};

  std::array<core::UUID, 3> handles;
  for (std::size_t i = 0; i < 3; ++i)
  {
    if (i > 0)
    {
      std::transform(points.begin(), points.end(), points.begin(), [](float value) { return value + 6.0f; });
      std::transform(colors.begin(), colors.end(), colors.begin(), [](float value) { return value + 6.0f; });
    }
    handles[i] = core::UUID::generate();
    buffer->add_points(points, colors, &handles[i]);
  }

  EXPECT_EQ(buffer->get_points().size(), 18);
  buffer->remove_point(handles[1]);
  EXPECT_TRUE(buffer->has_changed());
  buffer->reset_changed_flag();
  EXPECT_EQ(buffer->get_points().size(), 12);
  auto newPoints = buffer->get_points();
  EXPECT_EQ(newPoints[0], 1.0f);
  EXPECT_EQ(newPoints[1], 2.0f); 
  EXPECT_EQ(newPoints[2], 3.0f);
  EXPECT_EQ(newPoints[3], 4.0f);
  EXPECT_EQ(newPoints[4], 5.0f);
  EXPECT_EQ(newPoints[5], 6.0f);
  EXPECT_EQ(newPoints[6], 13.0f);
  EXPECT_EQ(newPoints[7], 14.0f);
  EXPECT_EQ(newPoints[8], 15.0f);
  EXPECT_EQ(newPoints[9], 16.0f);
  EXPECT_EQ(newPoints[10], 17.0f);
  EXPECT_EQ(newPoints[11], 18.0f);

  auto newColors = buffer->get_point_colors();
  EXPECT_EQ(newColors[0], 2.0f);
  EXPECT_EQ(newColors[1], 3.0f);
  EXPECT_EQ(newColors[2], 4.0f);
  EXPECT_EQ(newColors[3], 5.0f);
  EXPECT_EQ(newColors[4], 6.0f);
  EXPECT_EQ(newColors[5], 7.0f);
  EXPECT_EQ(newColors[6], 8.0f);
  EXPECT_EQ(newColors[7], 9.0f);
  EXPECT_EQ(newColors[8], 14.0f);
  EXPECT_EQ(newColors[9], 15.0f);
  EXPECT_EQ(newColors[10], 16.0f);
  EXPECT_EQ(newColors[11], 17.0f);
  EXPECT_EQ(newColors[12], 18.0f);
  EXPECT_EQ(newColors[13], 19.0f);
  EXPECT_EQ(newColors[14], 20.0f);
  EXPECT_EQ(newColors[15], 21.0f);

  buffer->remove_point(handles[0]);
  EXPECT_TRUE(buffer->has_changed());
  EXPECT_EQ(buffer->get_points().size(), 6);
  newPoints = buffer->get_points(); 
  EXPECT_EQ(newPoints[0], 13.0f);
  EXPECT_EQ(newPoints[1], 14.0f);
  EXPECT_EQ(newPoints[2], 15.0f);
  EXPECT_EQ(newPoints[3], 16.0f);
  EXPECT_EQ(newPoints[4], 17.0f);
  EXPECT_EQ(newPoints[5], 18.0f);

  newColors = buffer->get_point_colors();
  EXPECT_EQ(newColors[0], 14.0f);
  EXPECT_EQ(newColors[1], 15.0f);
  EXPECT_EQ(newColors[2], 16.0f);
  EXPECT_EQ(newColors[3], 17.0f);
  EXPECT_EQ(newColors[4], 18.0f);
  EXPECT_EQ(newColors[5], 19.0f);
  EXPECT_EQ(newColors[6], 20.0f);
  EXPECT_EQ(newColors[7], 21.0f);

  buffer->remove_point(handles[2]);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_point_colors().size(), 0);
}

TEST_F(GeometryBufferTest, AddLine)
{
  auto buffer = geoqik::LineBuffer::create(geoqik::GeoQikSettings{});
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  EXPECT_EQ(buffer->get_lines().size(), 6); // One line added
  EXPECT_EQ(buffer->get_line_colors().size(), 8); // Two colors for one line
  auto lines = buffer->get_lines();
  
  EXPECT_EQ(lines[0], 1.0f);
  EXPECT_EQ(lines[1], 2.0f);
  EXPECT_EQ(lines[2], 3.0f);
  
  EXPECT_EQ(lines[3], 4.0f);
  EXPECT_EQ(lines[4], 5.0f);
  EXPECT_EQ(lines[5], 6.0f);

  auto colors = buffer->get_line_colors();
  EXPECT_EQ(colors[0], 1.0f);
  EXPECT_EQ(colors[1], 1.0f);
  EXPECT_EQ(colors[2], 1.0f);
  EXPECT_EQ(colors[3], 1.0f);
  EXPECT_EQ(colors[4], 1.0f);
  EXPECT_EQ(colors[5], 1.0f);
  EXPECT_EQ(colors[6], 1.0f);
  EXPECT_EQ(colors[7], 1.0f);
}

TEST_F(GeometryBufferTest, IncreasePointBufferSize)
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 1;
  auto buffer = geoqik::PointBuffer::create(settings);
  ASSERT_TRUE(buffer != nullptr);

  // Add initial point
  buffer->add_point(1.0f, 2.0f, 3.0f);
  EXPECT_EQ(buffer->get_points().size(), 3);
  EXPECT_EQ(buffer->get_point_colors().size(), 4);
  EXPECT_EQ(buffer->get_point_indices().size(), 1); // One index for one point
  EXPECT_TRUE(buffer->has_changed());

  // Create a new buffer with additional space
  auto newBuffer = geoqik::PointBuffer::create_from(std::move(*buffer), 10);
  ASSERT_TRUE(newBuffer != nullptr);
  EXPECT_EQ(newBuffer->get_points().size(), 3);
  EXPECT_EQ(newBuffer->get_point_colors().size(), 4);
  EXPECT_EQ(newBuffer->get_point_indices().size(), 1);
  EXPECT_TRUE(newBuffer->has_changed());

  newBuffer->add_point(7.0f, 8.0f, 9.0f);
  EXPECT_EQ(newBuffer->get_points().size(), 6);
  EXPECT_EQ(newBuffer->get_point_colors().size(), 8);
  EXPECT_EQ(newBuffer->get_point_indices().size(), 2);
  EXPECT_TRUE(newBuffer->has_changed());
}

TEST_F(GeometryBufferTest, IncreaseLineBufferSize)
{
  geoqik::GeoQikSettings settings;
  settings.initialLineCapacity = 1;
  auto buffer = geoqik::LineBuffer::create(settings);
  ASSERT_TRUE(buffer != nullptr);

  // Add initial line
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  EXPECT_EQ(buffer->get_lines().size(), 6); 
  EXPECT_EQ(buffer->get_line_colors().size(), 8);
  EXPECT_EQ(buffer->get_line_indices().size(), 2); // Two indices for one line
  EXPECT_TRUE(buffer->has_changed());

  // Create a new buffer with additional space
  auto newBuffer = geoqik::LineBuffer::create_from(std::move(*buffer), 10);
  ASSERT_TRUE(newBuffer != nullptr);
  EXPECT_EQ(newBuffer->get_lines().size(), 6);
  EXPECT_EQ(newBuffer->get_line_colors().size(), 8);
  EXPECT_EQ(newBuffer->get_line_indices().size(), 2);
  EXPECT_TRUE(newBuffer->has_changed());

  newBuffer->add_line(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
  EXPECT_EQ(newBuffer->get_lines().size(), 12); // Two lines now
  EXPECT_EQ(newBuffer->get_line_colors().size(), 16); // Two colors for two lines
  EXPECT_EQ(newBuffer->get_line_indices().size(), 4); // Four indices for two lines
  EXPECT_TRUE(newBuffer->has_changed());
}

// =============================================================================
// add_line tests
// =============================================================================

TEST_F(GeometryBufferTest, AddLine_NilHandle)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID nilHandle = core::UUID::nil();
  EXPECT_THROW(buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, &nilHandle), std::runtime_error);
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddLine_WithHandle)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID lineHandle = core::UUID::generate();
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, &lineHandle);

  EXPECT_EQ(buffer->get_lines().size(), 6);
  EXPECT_EQ(buffer->get_line_colors().size(), 8);
  EXPECT_EQ(buffer->get_line_indices().size(), 2);
  EXPECT_TRUE(buffer->lines_have_changed());

  auto lines = buffer->get_lines();
  EXPECT_EQ(lines[0], 1.0f);
  EXPECT_EQ(lines[1], 2.0f);
  EXPECT_EQ(lines[2], 3.0f);
  EXPECT_EQ(lines[3], 4.0f);
  EXPECT_EQ(lines[4], 5.0f);
  EXPECT_EQ(lines[5], 6.0f);

  auto indices = buffer->get_line_indices();
  EXPECT_EQ(indices[0], 0u);
  EXPECT_EQ(indices[1], 1u);
}

TEST_F(GeometryBufferTest, AddLine_WithCustomColor)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0.5f, 0.6f, 0.7f, 0.8f);

  auto colors = buffer->get_line_colors();
  EXPECT_EQ(colors.size(), 8);
  EXPECT_EQ(colors[0], 0.5f);
  EXPECT_EQ(colors[1], 0.6f);
  EXPECT_EQ(colors[2], 0.7f);
  EXPECT_EQ(colors[3], 0.8f);
  EXPECT_EQ(colors[4], 0.5f);
  EXPECT_EQ(colors[5], 0.6f);
  EXPECT_EQ(colors[6], 0.7f);
  EXPECT_EQ(colors[7], 0.8f);
}

TEST_F(GeometryBufferTest, AddLine_NotEnoughSpace)
{
  m_settings.initialLineCapacity = 1;
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  EXPECT_EQ(buffer->get_lines().size(), 6);

  EXPECT_THROW(buffer->add_line(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f), std::runtime_error);
  EXPECT_EQ(buffer->get_lines().size(), 6);
}

// =============================================================================
// add_lines tests
// =============================================================================

TEST_F(GeometryBufferTest, AddLines_Empty)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> emptyLines;
  std::vector<float> emptyColors;
  buffer->add_lines(emptyLines, emptyColors);

  EXPECT_EQ(buffer->get_lines().size(), 0);
  EXPECT_EQ(buffer->get_line_colors().size(), 0);
  EXPECT_EQ(buffer->get_line_indices().size(), 0);
  EXPECT_FALSE(buffer->has_changed());
}

TEST_F(GeometryBufferTest, AddLines_One)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> lines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors;
  buffer->add_lines(lines, colors);

  auto linesSpan = buffer->get_lines();
  EXPECT_EQ(linesSpan.size(), 6);
  EXPECT_EQ(linesSpan[0], 1.0f);
  EXPECT_EQ(linesSpan[1], 2.0f);
  EXPECT_EQ(linesSpan[2], 3.0f);
  EXPECT_EQ(linesSpan[3], 4.0f);
  EXPECT_EQ(linesSpan[4], 5.0f);
  EXPECT_EQ(linesSpan[5], 6.0f);

  auto colorsSpan = buffer->get_line_colors();
  EXPECT_EQ(colorsSpan.size(), 8);
  EXPECT_EQ(colorsSpan[0], 1.0f); // default white
  EXPECT_EQ(colorsSpan[1], 1.0f);
  EXPECT_EQ(colorsSpan[2], 1.0f);
  EXPECT_EQ(colorsSpan[3], 1.0f);
  EXPECT_EQ(colorsSpan[4], 1.0f);
  EXPECT_EQ(colorsSpan[5], 1.0f);
  EXPECT_EQ(colorsSpan[6], 1.0f);
  EXPECT_EQ(colorsSpan[7], 1.0f);

  auto indicesSpan = buffer->get_line_indices();
  EXPECT_EQ(indicesSpan.size(), 2);
  EXPECT_EQ(indicesSpan[0], 0u);
  EXPECT_EQ(indicesSpan[1], 1u);

  EXPECT_TRUE(buffer->lines_have_changed());
}

TEST_F(GeometryBufferTest, AddLines_WithSingleColor)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> lines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors = {0.5f, 0.6f, 0.7f, 0.8f};
  buffer->add_lines(lines, colors);

  auto colorsSpan = buffer->get_line_colors();
  EXPECT_EQ(colorsSpan.size(), 8);
  EXPECT_EQ(colorsSpan[0], 0.5f);
  EXPECT_EQ(colorsSpan[1], 0.6f);
  EXPECT_EQ(colorsSpan[2], 0.7f);
  EXPECT_EQ(colorsSpan[3], 0.8f);
  EXPECT_EQ(colorsSpan[4], 0.5f);
  EXPECT_EQ(colorsSpan[5], 0.6f);
  EXPECT_EQ(colorsSpan[6], 0.7f);
  EXPECT_EQ(colorsSpan[7], 0.8f);
}

TEST_F(GeometryBufferTest, AddLines_WithPerLineColor)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  // 2 lines (12 floats), 2 RGBA colors (8 floats)
  std::vector<float> lines = {
      1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  // line 0
      7.0f,  8.0f,  9.0f, 10.0f, 11.0f, 12.0f   // line 1
  };
  std::vector<float> colors = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
  buffer->add_lines(lines, colors);

  auto colorsSpan = buffer->get_line_colors();
  EXPECT_EQ(colorsSpan.size(), 16);
  // Line 0: both vertices get {0.1, 0.2, 0.3, 0.4}
  EXPECT_EQ(colorsSpan[0], 0.1f);
  EXPECT_EQ(colorsSpan[1], 0.2f);
  EXPECT_EQ(colorsSpan[2], 0.3f);
  EXPECT_EQ(colorsSpan[3], 0.4f);
  EXPECT_EQ(colorsSpan[4], 0.1f);
  EXPECT_EQ(colorsSpan[5], 0.2f);
  EXPECT_EQ(colorsSpan[6], 0.3f);
  EXPECT_EQ(colorsSpan[7], 0.4f);
  // Line 1: both vertices get {0.5, 0.6, 0.7, 0.8}
  EXPECT_EQ(colorsSpan[8], 0.5f);
  EXPECT_EQ(colorsSpan[9], 0.6f);
  EXPECT_EQ(colorsSpan[10], 0.7f);
  EXPECT_EQ(colorsSpan[11], 0.8f);
  EXPECT_EQ(colorsSpan[12], 0.5f);
  EXPECT_EQ(colorsSpan[13], 0.6f);
  EXPECT_EQ(colorsSpan[14], 0.7f);
  EXPECT_EQ(colorsSpan[15], 0.8f);
}

TEST_F(GeometryBufferTest, AddLines_Multiple)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> lines1 = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors1 = {0.5f, 0.6f, 0.7f, 0.8f};
  buffer->add_lines(lines1, colors1);
  EXPECT_EQ(buffer->get_lines().size(), 6);
  EXPECT_EQ(buffer->get_line_indices().size(), 2);
  EXPECT_TRUE(buffer->lines_have_changed());

  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->lines_have_changed());

  std::vector<float> lines2 = {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  std::vector<float> colors2 = {0.1f, 0.2f, 0.3f, 0.4f};
  buffer->add_lines(lines2, colors2);
  EXPECT_EQ(buffer->get_lines().size(), 12);
  EXPECT_EQ(buffer->get_line_colors().size(), 16);
  EXPECT_EQ(buffer->get_line_indices().size(), 4);
  EXPECT_TRUE(buffer->lines_have_changed());

  auto linesSpan = buffer->get_lines();
  EXPECT_EQ(linesSpan[6], 7.0f);
  EXPECT_EQ(linesSpan[7], 8.0f);
  EXPECT_EQ(linesSpan[8], 9.0f);
  EXPECT_EQ(linesSpan[9], 10.0f);
  EXPECT_EQ(linesSpan[10], 11.0f);
  EXPECT_EQ(linesSpan[11], 12.0f);

  auto indicesSpan = buffer->get_line_indices();
  EXPECT_EQ(indicesSpan[2], 2u);
  EXPECT_EQ(indicesSpan[3], 3u);
}

TEST_F(GeometryBufferTest, AddLines_NilHandle)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> lines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors;
  core::UUID nilHandle = core::UUID::nil();
  EXPECT_THROW(buffer->add_lines(lines, colors, &nilHandle), std::runtime_error);
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddLines_InvalidSize)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  // 5 floats is not a multiple of 6 (2 * pointDimension)
  std::vector<float> invalidLines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
  std::vector<float> colors;
  EXPECT_THROW(buffer->add_lines(invalidLines, colors), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddLines_NotEnoughSpace)
{
  m_settings.initialLineCapacity = 1;
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  // 2 lines require capacity for 2, but only 1 is available
  std::vector<float> twoLines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  std::vector<float> colors;
  EXPECT_THROW(buffer->add_lines(twoLines, colors), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddLines_WithHandle)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID linesHandle = core::UUID::generate();
  std::vector<float> lines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
                               7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  std::vector<float> colors = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
  buffer->add_lines(lines, colors, &linesHandle);

  EXPECT_EQ(buffer->get_lines().size(), 12);
  EXPECT_EQ(buffer->get_line_colors().size(), 16);
  EXPECT_EQ(buffer->get_line_indices().size(), 4);
  EXPECT_TRUE(buffer->lines_have_changed());

  auto linesSpan = buffer->get_lines();
  EXPECT_EQ(linesSpan[0], 1.0f);
  EXPECT_EQ(linesSpan[5], 6.0f);
  EXPECT_EQ(linesSpan[6], 7.0f);
  EXPECT_EQ(linesSpan[11], 12.0f);

  auto indicesSpan = buffer->get_line_indices();
  EXPECT_EQ(indicesSpan[0], 0u);
  EXPECT_EQ(indicesSpan[1], 1u);
  EXPECT_EQ(indicesSpan[2], 2u);
  EXPECT_EQ(indicesSpan[3], 3u);
}

TEST_F(GeometryBufferTest, AddLines_InvalidColors)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  // 2 lines = 12 floats; colors size 5 is non-empty, not 4, and not lineCount * 4
  std::vector<float> lines = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
                               7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  std::vector<float> invalidColors = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
  EXPECT_THROW(buffer->add_lines(lines, invalidColors), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_lines().size(), 0);
  EXPECT_EQ(buffer->get_line_colors().size(), 0);
}

// =============================================================================
// remove_line tests
// =============================================================================

TEST_F(GeometryBufferTest, RemoveLine_NilHandle)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID nilHandle = core::UUID::nil();
  EXPECT_THROW(buffer->remove_line(nilHandle), std::runtime_error);
}

TEST_F(GeometryBufferTest, RemoveLine_Empty)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID lineHandle = core::UUID::generate();
  buffer->remove_line(lineHandle);
  EXPECT_EQ(buffer->get_lines().size(), 0);
  EXPECT_EQ(buffer->get_line_colors().size(), 0);
  EXPECT_EQ(buffer->get_line_indices().size(), 0);
}

TEST_F(GeometryBufferTest, RemoveLine_One)
{
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID lineHandle = core::UUID::generate();
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, &lineHandle);
  EXPECT_EQ(buffer->get_lines().size(), 6);

  buffer->remove_line(lineHandle);
  EXPECT_EQ(buffer->get_lines().size(), 0);
  EXPECT_EQ(buffer->get_line_colors().size(), 0);
  EXPECT_EQ(buffer->get_line_indices().size(), 0);
  EXPECT_TRUE(buffer->has_changed());
}

TEST_F(GeometryBufferTest, RemoveLine_Multiple)
{
  m_settings.initialLineCapacity = 3;
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::array<core::UUID, 3> handles;
  handles[0] = core::UUID::generate();
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 1.0f, 0.0f, 0.0f, 1.0f, &handles[0]);
  handles[1] = core::UUID::generate();
  buffer->add_line(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 0.0f, 1.0f, 0.0f, 1.0f, &handles[1]);
  handles[2] = core::UUID::generate();
  buffer->add_line(13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 0.0f, 0.0f, 1.0f, 1.0f, &handles[2]);

  EXPECT_EQ(buffer->get_lines().size(), 18);
  EXPECT_EQ(buffer->get_line_colors().size(), 24);
  EXPECT_EQ(buffer->get_line_indices().size(), 6);

  // Remove middle line (green)
  buffer->remove_line(handles[1]);
  EXPECT_TRUE(buffer->has_changed());
  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->has_changed());

  EXPECT_EQ(buffer->get_lines().size(), 12);
  auto newLines = buffer->get_lines();
  EXPECT_EQ(newLines[0], 1.0f);
  EXPECT_EQ(newLines[1], 2.0f);
  EXPECT_EQ(newLines[2], 3.0f);
  EXPECT_EQ(newLines[3], 4.0f);
  EXPECT_EQ(newLines[4], 5.0f);
  EXPECT_EQ(newLines[5], 6.0f);
  EXPECT_EQ(newLines[6], 13.0f);
  EXPECT_EQ(newLines[7], 14.0f);
  EXPECT_EQ(newLines[8], 15.0f);
  EXPECT_EQ(newLines[9], 16.0f);
  EXPECT_EQ(newLines[10], 17.0f);
  EXPECT_EQ(newLines[11], 18.0f);

  auto newColors = buffer->get_line_colors();
  EXPECT_EQ(newColors.size(), 16);
  // Line 0 (red): both vertices
  EXPECT_EQ(newColors[0], 1.0f);
  EXPECT_EQ(newColors[1], 0.0f);
  EXPECT_EQ(newColors[2], 0.0f);
  EXPECT_EQ(newColors[3], 1.0f);
  EXPECT_EQ(newColors[4], 1.0f);
  EXPECT_EQ(newColors[5], 0.0f);
  EXPECT_EQ(newColors[6], 0.0f);
  EXPECT_EQ(newColors[7], 1.0f);
  // Line 2 (blue), now compacted to position 1: both vertices
  EXPECT_EQ(newColors[8], 0.0f);
  EXPECT_EQ(newColors[9], 0.0f);
  EXPECT_EQ(newColors[10], 1.0f);
  EXPECT_EQ(newColors[11], 1.0f);
  EXPECT_EQ(newColors[12], 0.0f);
  EXPECT_EQ(newColors[13], 0.0f);
  EXPECT_EQ(newColors[14], 1.0f);
  EXPECT_EQ(newColors[15], 1.0f);

  auto newIndices = buffer->get_line_indices();
  EXPECT_EQ(newIndices.size(), 4);
  EXPECT_EQ(newIndices[0], 0u);
  EXPECT_EQ(newIndices[1], 1u);
  EXPECT_EQ(newIndices[2], 2u);
  EXPECT_EQ(newIndices[3], 3u);

  // Remove first line (red)
  buffer->remove_line(handles[0]);
  EXPECT_TRUE(buffer->has_changed());
  EXPECT_EQ(buffer->get_lines().size(), 6);
  newLines = buffer->get_lines();
  EXPECT_EQ(newLines[0], 13.0f);
  EXPECT_EQ(newLines[1], 14.0f);
  EXPECT_EQ(newLines[2], 15.0f);
  EXPECT_EQ(newLines[3], 16.0f);
  EXPECT_EQ(newLines[4], 17.0f);
  EXPECT_EQ(newLines[5], 18.0f);

  newColors = buffer->get_line_colors();
  EXPECT_EQ(newColors.size(), 8);
  EXPECT_EQ(newColors[0], 0.0f);
  EXPECT_EQ(newColors[1], 0.0f);
  EXPECT_EQ(newColors[2], 1.0f);
  EXPECT_EQ(newColors[3], 1.0f);
  EXPECT_EQ(newColors[4], 0.0f);
  EXPECT_EQ(newColors[5], 0.0f);
  EXPECT_EQ(newColors[6], 1.0f);
  EXPECT_EQ(newColors[7], 1.0f);

  // Remove last remaining line (blue)
  buffer->remove_line(handles[2]);
  EXPECT_EQ(buffer->get_lines().size(), 0);
  EXPECT_EQ(buffer->get_line_colors().size(), 0);
  EXPECT_EQ(buffer->get_line_indices().size(), 0);
}

TEST_F(GeometryBufferTest, UpdateSingleAndBulkPoints)
{
  m_settings.initialPointCapacity = 4;
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  const core::UUID singleId = core::UUID::generate();
  const core::UUID bulkId = core::UUID::generate();
  buffer->add_point(1.0f, 2.0f, 3.0f, &singleId);
  const std::vector<float> addedPoints{4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
  const std::vector<float> noPointColors;
  buffer->add_points(addedPoints, noPointColors, &bulkId);
  buffer->reset_changed_flag();

  const std::vector<float> singleColor{0.1f, 0.2f, 0.3f, 0.4f};
  EXPECT_TRUE(buffer->update_point(singleId, 10.0f, 11.0f, 12.0f, singleColor));
  const std::vector<float> bulkPoints{13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f};
  const std::vector<float> bulkColors{0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 0.1f, 0.2f};
  EXPECT_TRUE(buffer->update_points(bulkId, bulkPoints, bulkColors));

  const auto points = buffer->get_points();
  ASSERT_EQ(points.size(), 9);
  EXPECT_FLOAT_EQ(points[0], 10.0f);
  EXPECT_FLOAT_EQ(points[1], 11.0f);
  EXPECT_FLOAT_EQ(points[2], 12.0f);
  EXPECT_FLOAT_EQ(points[3], 13.0f);
  EXPECT_FLOAT_EQ(points[8], 18.0f);

  const auto colors = buffer->get_point_colors();
  EXPECT_FLOAT_EQ(colors[0], 0.1f);
  EXPECT_FLOAT_EQ(colors[4], 0.5f);
  EXPECT_FLOAT_EQ(colors[8], 0.9f);
  EXPECT_TRUE(buffer->has_changed());
}

TEST_F(GeometryBufferTest, PointUpdateCountMismatchDoesNotMutate)
{
  m_settings.initialPointCapacity = 4;
  auto buffer = geoqik::PointBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  const core::UUID bulkId = core::UUID::generate();
  const std::vector<float> original{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  const std::vector<float> emptyColors;
  buffer->add_points(original, emptyColors, &bulkId);
  buffer->reset_changed_flag();

  const std::vector<float> wrongCount{7.0f, 8.0f, 9.0f};
  const std::vector<float> noColors;
  EXPECT_FALSE(buffer->update_points(bulkId, wrongCount, noColors));
  EXPECT_EQ(std::vector<float>(buffer->get_points().begin(), buffer->get_points().end()), original);
  EXPECT_FALSE(buffer->has_changed());
}

TEST_F(GeometryBufferTest, UpdateSingleAndBulkLines)
{
  m_settings.initialLineCapacity = 4;
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  const core::UUID singleId = core::UUID::generate();
  const core::UUID bulkId = core::UUID::generate();
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, &singleId);
  const std::vector<float> addedLines{7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f};
  const std::vector<float> noLineColors;
  buffer->add_lines(addedLines, noLineColors, &bulkId);
  buffer->reset_changed_flag();

  const std::vector<float> singleColor{0.1f, 0.2f, 0.3f, 0.4f};
  EXPECT_TRUE(buffer->update_line(singleId, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, singleColor));
  const std::vector<float> bulkLines{26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f};
  const std::vector<float> bulkColors{0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 0.1f, 0.2f};
  EXPECT_TRUE(buffer->update_lines(bulkId, bulkLines, bulkColors));

  const auto lines = buffer->get_lines();
  ASSERT_EQ(lines.size(), 18);
  EXPECT_FLOAT_EQ(lines[0], 20.0f);
  EXPECT_FLOAT_EQ(lines[5], 25.0f);
  EXPECT_FLOAT_EQ(lines[6], 26.0f);
  EXPECT_FLOAT_EQ(lines[17], 37.0f);

  const auto colors = buffer->get_line_colors();
  EXPECT_FLOAT_EQ(colors[0], 0.1f);
  EXPECT_FLOAT_EQ(colors[8], 0.5f);
  EXPECT_FLOAT_EQ(colors[16], 0.9f);
  EXPECT_TRUE(buffer->has_changed());
}

TEST_F(GeometryBufferTest, LineUpdateCountMismatchDoesNotMutate)
{
  m_settings.initialLineCapacity = 4;
  auto buffer = geoqik::LineBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  const core::UUID bulkId = core::UUID::generate();
  const std::vector<float> original{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f};
  const std::vector<float> emptyColors;
  buffer->add_lines(original, emptyColors, &bulkId);
  buffer->reset_changed_flag();

  const std::vector<float> wrongCount{13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f};
  const std::vector<float> noColors;
  EXPECT_FALSE(buffer->update_lines(bulkId, wrongCount, noColors));
  EXPECT_EQ(std::vector<float>(buffer->get_lines().begin(), buffer->get_lines().end()), original);
  EXPECT_FALSE(buffer->has_changed());
}
