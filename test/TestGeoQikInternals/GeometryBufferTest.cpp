#include "GeometryBuffer.hpp"
#include "Core/UUID.hpp"
#include <cmath>
#include <gtest/gtest.h>

class GeometryBufferTest : public ::testing::Test
{
};

TEST_F(GeometryBufferTest, CreateBuffer)
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 1000;
  settings.initialLineCapacity = 500;

  auto buffer = geoqik::GeometryBuffer::create(settings);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ(buffer->get_points().size(), 0);
  EXPECT_EQ(buffer->get_lines().size(), 0);
}

TEST_F(GeometryBufferTest, AddPoint)
{
  auto buffer = geoqik::GeometryBuffer::create(geoqik::GeoQikSettings{});
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
  auto buffer = geoqik::GeometryBuffer::create(geoqik::GeoQikSettings{});
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
  buffer->add_point(4.0f, 5.0f, 6.0f, &invalidHandle);
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
