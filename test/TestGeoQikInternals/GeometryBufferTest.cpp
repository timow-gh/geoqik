#include "GeometryBuffer.hpp"
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

TEST_F(GeometryBufferTest, CreateBuffer)
{
  m_settings.initialPointCapacity = 1000;
  m_settings.initialLineCapacity = 500;

  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddPoint)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  buffer->add_point(1.0f, 2.0f, 3.0f);
  auto points = buffer->get_points();

  EXPECT_EQ(points.size(), 3);
  EXPECT_EQ(buffer->get_point_colors().size(), 3);

  EXPECT_EQ(points[0], 1.0f);
  EXPECT_EQ(points[1], 2.0f);
  EXPECT_EQ(points[2], 3.0f);

  auto colors = buffer->get_point_colors();
  EXPECT_EQ(colors[0], 1.0f);
  EXPECT_EQ(colors[1], 1.0f);
  EXPECT_EQ(colors[2], 1.0f);
}

TEST_F(GeometryBufferTest, RemovePoint)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
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
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  std::vector<float> emptyPoints;
  std::vector<float> emptyColors;
  buffer->add_points(emptyPoints, emptyColors);

  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_point_colors().size(), 0);
}

TEST_F(GeometryBufferTest, AddPoints_One)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  std::vector<float> points = {1.0f, 2.0f, 3.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f};
  buffer->add_points(points, colors);

  auto pointsSpan = buffer->get_points();
  EXPECT_EQ(pointsSpan.size(), 3);
  EXPECT_EQ(pointsSpan[0], 1.0f);
  EXPECT_EQ(pointsSpan[1], 2.0f);
  EXPECT_EQ(pointsSpan[2], 3.0f);

  auto colorsSpan = buffer->get_point_colors();
  EXPECT_EQ(colorsSpan.size(), 3);
  EXPECT_EQ(colorsSpan[0], 2.0f);
  EXPECT_EQ(colorsSpan[1], 3.0f);
  EXPECT_EQ(colorsSpan[2], 4.0f);
}

TEST_F(GeometryBufferTest, AddPoints_Multiple)
{
  m_settings.initialPointCapacity = 5;
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
  std::vector<float> color = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  // Points with wrong size.
  EXPECT_THROW(buffer->add_points(points, color), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);
  points.push_back(6.0f);
  // Colors with wrong size.
  EXPECT_THROW(buffer->add_points(points, color), std::runtime_error);
  EXPECT_FALSE(buffer->has_changed());
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);
  color.push_back(7.0f);

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
  EXPECT_EQ(colors.size(), 6);
  EXPECT_EQ(colors[0], 2.0f);
  EXPECT_EQ(colors[1], 3.0f);
  EXPECT_EQ(colors[2], 4.0f);
  EXPECT_EQ(colors[3], 5.0f);
  EXPECT_EQ(colors[4], 6.0f);
  EXPECT_EQ(colors[5], 7.0f);

  EXPECT_TRUE(buffer->points_have_changed());
  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->points_have_changed());

  // Add 3 more points to fill the buffer.
  std::vector<float> morePoints = {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
  std::vector<float> moreColors = {8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
  buffer->add_points(morePoints, moreColors);
  EXPECT_EQ(buffer->get_points().size(), 15);
  EXPECT_EQ(buffer->get_point_colors().size(), 15);
  EXPECT_TRUE(buffer->points_have_changed());

  // Add more points to trigger buffer growth
  morePoints = {7.0f, 8.0f, 9.0f};
  moreColors = {8.0f, 9.0f, 10.0f};
  EXPECT_THROW(buffer->add_points(morePoints, moreColors), std::runtime_error);
}

TEST_F(GeometryBufferTest, RemovePoints_NilHandle)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID invalidHandle = core::UUID::nil();
  EXPECT_THROW(buffer->remove_point(invalidHandle), std::runtime_error);
}

TEST_F(GeometryBufferTest, RemovePoints_Empty)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID pointsHandle = core::UUID::generate();
  buffer->remove_point(pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 0);
}

TEST_F(GeometryBufferTest, RemovePoints_One)
{
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  core::UUID pointsHandle = core::UUID::generate();
  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
  buffer->add_points(points, colors, &pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 6);

  buffer->remove_point(pointsHandle);
  EXPECT_EQ(buffer->get_points().size(), 0);
}

TEST_F(GeometryBufferTest, RemovePoints_Multiple)
{
  m_settings.initialPointCapacity = 6;
  auto buffer = geoqik::GeometryBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> points = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> colors = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};

  std::array<core::UUID, 3> handles;
  for (int i = 0; i < 3; ++i)
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
  EXPECT_EQ(newColors[6], 14.0f);
  EXPECT_EQ(newColors[7], 15.0f);
  EXPECT_EQ(newColors[8], 16.0f);
  EXPECT_EQ(newColors[9], 17.0f);
  EXPECT_EQ(newColors[10], 18.0f);
  EXPECT_EQ(newColors[11], 19.0f);

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

  buffer->remove_point(handles[2]);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_point_colors().size(), 0);
}

TEST_F(GeometryBufferTest, AddLine)
{
  auto buffer = geoqik::GeometryBuffer::create(geoqik::GeoQikSettings{});
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);

  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  EXPECT_EQ(buffer->get_lines().size(), 6); // One line added
  EXPECT_EQ(buffer->get_line_colors().size(), 6); // Two colors for one line
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
}

TEST_F(GeometryBufferTest, IncreaseBufferSize)
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 1;
  settings.initialLineCapacity = 1;
  auto buffer = geoqik::GeometryBuffer::create(settings);
  ASSERT_TRUE(buffer != nullptr);

  // Add initial point and line
  buffer->add_point(1.0f, 2.0f, 3.0f);
  buffer->add_line(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
  EXPECT_EQ(buffer->get_points().size(), 3);
  EXPECT_EQ(buffer->get_lines().size(), 6); 
  EXPECT_EQ(buffer->get_point_colors().size(), 3);
  EXPECT_EQ(buffer->get_line_colors().size(), 6); 
  EXPECT_EQ(buffer->get_line_indices().size(), 2); // Two indices for one line
  EXPECT_EQ(buffer->get_point_indices().size(), 1); // One index for one point
  EXPECT_TRUE(buffer->has_changed());

  // Create a new buffer with additional space
  auto newBuffer = geoqik::GeometryBuffer::create_from(std::move(*buffer), 10);
  ASSERT_TRUE(newBuffer != nullptr);
  EXPECT_EQ(newBuffer->get_points().size(), 3);
  EXPECT_EQ(newBuffer->get_lines().size(), 6);
  EXPECT_EQ(newBuffer->get_point_colors().size(), 3);
  EXPECT_EQ(newBuffer->get_line_colors().size(), 6);
  EXPECT_EQ(newBuffer->get_line_indices().size(), 2);
  EXPECT_EQ(newBuffer->get_point_indices().size(), 1);
  EXPECT_TRUE(newBuffer->has_changed());

  newBuffer->add_point(7.0f, 8.0f, 9.0f);
  newBuffer->add_line(7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f);
  EXPECT_EQ(newBuffer->get_points().size(), 6);
  EXPECT_EQ(newBuffer->get_lines().size(), 12); // Two lines now
  EXPECT_EQ(newBuffer->get_point_colors().size(), 6);
  EXPECT_EQ(newBuffer->get_line_colors().size(), 12); // Two colors for two lines
  EXPECT_EQ(newBuffer->get_line_indices().size(), 4); // Four indices for two lines
  EXPECT_EQ(newBuffer->get_point_indices().size(), 2);
  EXPECT_TRUE(newBuffer->has_changed());
}
