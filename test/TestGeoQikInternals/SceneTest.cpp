#include "Scene.hpp"
#include "MathConstants.hpp"
#include <gtest/gtest.h>
#include <iterator>

namespace
{

geoqik::GeoQikSettings make_scene_test_settings()
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 2;
  settings.initialLineCapacity = 2;
  settings.capacityGrowthFactor = 5;
  settings.defaultPointSize = 7.0f;
  settings.defaultLineWidth = 3.0f;
  settings.defaultPointColor = geoqik::Color{0.1f, 0.2f, 0.3f, 0.4f};
  settings.defaultLineColor = geoqik::Color{0.5f, 0.6f, 0.7f, 0.8f};
  return settings;
}

} // namespace

TEST(SceneTest, UsesSettingsForDefaultSizesColorsAndCapacities)
{
  const auto settings = make_scene_test_settings();
  auto scene = geoqik::Scene::create(settings);

  EXPECT_FLOAT_EQ(scene.get_point_size(), settings.defaultPointSize);
  EXPECT_FLOAT_EQ(scene.get_line_width(), settings.defaultLineWidth);
  EXPECT_EQ(scene.get_default_point_color(), settings.defaultPointColor);
  EXPECT_EQ(scene.get_default_line_color(), settings.defaultLineColor);
  EXPECT_EQ(scene.get_point_buffer().get_point_capacity(), settings.initialPointCapacity);
  EXPECT_EQ(scene.get_line_buffer().get_line_capacity(), settings.initialLineCapacity);
}

TEST(SceneTest, CapacityGrowthUsesSettingsGrowthFactor)
{
  const auto settings = make_scene_test_settings();
  auto scene = geoqik::Scene::create(settings);

  scene.add_point(0.0f, 0.0f, 0.0f);
  scene.add_point(1.0f, 0.0f, 0.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
  scene.add_line(0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f);

  EXPECT_TRUE(scene.ensure_point_capacity(1));
  EXPECT_TRUE(scene.ensure_line_capacity(1));

  EXPECT_EQ(scene.get_point_buffer().get_point_capacity(), settings.initialPointCapacity * settings.capacityGrowthFactor);
  EXPECT_EQ(scene.get_line_buffer().get_line_capacity(), settings.initialLineCapacity * settings.capacityGrowthFactor);
}

TEST(SceneTest, CalculatesBoundingSphereAcrossPointsAndLines)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());

  scene.add_point(3.0f, 4.0f, 0.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 12.0f);

  const auto sphere = scene.calc_bounding_sphere(linal::float3{0.0f, 0.0f, 0.0f});

  EXPECT_FLOAT_EQ(sphere.get_radius(), 12.0f);
}

TEST(SceneTest, AddsBulkPointsAndLines)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());

  const float points[] = {
      0.0f, 1.0f, 2.0f,
      3.0f, 4.0f, 5.0f,
  };
  const float pointColor[] = {0.9f, 0.8f, 0.7f, 0.6f};
  scene.add_points(points, pointColor);

  const float lines[] = {
      0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
  };
  const float lineColors[] = {
      0.1f, 0.2f, 0.3f, 0.4f,
      0.5f, 0.6f, 0.7f, 0.8f,
  };
  scene.add_lines(lines, lineColors);

  EXPECT_EQ(scene.get_point_buffer().get_points().size(), std::size(points));
  EXPECT_EQ(scene.get_point_buffer().get_point_colors().size(), 2 * geoqik::ColorChannelCount);
  EXPECT_EQ(scene.get_point_buffer().get_point_indices().size(), 2);
  EXPECT_EQ(scene.get_line_buffer().get_lines().size(), std::size(lines));
  EXPECT_EQ(scene.get_line_buffer().get_line_colors().size(), 4 * geoqik::ColorChannelCount);
  EXPECT_EQ(scene.get_line_buffer().get_line_indices().size(), 4);
}

TEST(SceneTest, RemovesPointAndLineGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());
  const auto pointId = core::UUID::generate();
  const auto lineId = core::UUID::generate();

  scene.add_point(0.0f, 0.0f, 0.0f, &pointId);
  scene.add_point(1.0f, 1.0f, 1.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, &lineId);
  scene.add_line(0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f);

  scene.remove_point(pointId);
  scene.remove_line(lineId);

  EXPECT_EQ(scene.get_point_buffer().get_points().size(), 3);
  EXPECT_EQ(scene.get_point_buffer().get_point_indices().size(), 1);
  EXPECT_EQ(scene.get_line_buffer().get_lines().size(), 6);
  EXPECT_EQ(scene.get_line_buffer().get_line_indices().size(), 2);
}

TEST(SceneTest, ClearRemovesPointsAndLinesFromBoundingSphere)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());

  scene.add_point(10.0f, 0.0f, 0.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f);

  scene.clear();
  const auto sphere = scene.calc_bounding_sphere(linal::float3{0.0f, 0.0f, 0.0f});

  EXPECT_FLOAT_EQ(sphere.get_radius(), 0.0f);
}

TEST(SceneTest, TranslatesPointAndLineGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());
  const auto pointId = core::UUID::generate();
  const auto lineId = core::UUID::generate();

  scene.add_point(0.0f, 0.0f, 0.0f, &pointId);
  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &lineId);

  scene.translate_geometry(pointId, 4.0f, 5.0f, 6.0f);
  scene.translate_geometry(lineId, 1.0f, 2.0f, 3.0f);

  const auto points = scene.get_point_buffer().get_points();
  ASSERT_EQ(points.size(), 3);
  EXPECT_FLOAT_EQ(points[0], 4.0f);
  EXPECT_FLOAT_EQ(points[1], 5.0f);
  EXPECT_FLOAT_EQ(points[2], 6.0f);

  const auto lines = scene.get_line_buffer().get_lines();
  ASSERT_EQ(lines.size(), 6);
  EXPECT_FLOAT_EQ(lines[0], 1.0f);
  EXPECT_FLOAT_EQ(lines[1], 2.0f);
  EXPECT_FLOAT_EQ(lines[2], 3.0f);
  EXPECT_FLOAT_EQ(lines[3], 2.0f);
  EXPECT_FLOAT_EQ(lines[4], 3.0f);
  EXPECT_FLOAT_EQ(lines[5], 4.0f);
}

TEST(SceneTest, RotatesPointAndLineGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());
  const auto pointId = core::UUID::generate();
  const auto lineId = core::UUID::generate();

  scene.add_point(1.0f, 0.0f, 0.0f, &pointId);
  scene.add_line(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, &lineId);

  scene.rotate_geometry(pointId, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, geoqik::PI_HALF_F);
  scene.rotate_geometry(lineId, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, geoqik::PI_HALF_F);

  const auto points = scene.get_point_buffer().get_points();
  ASSERT_EQ(points.size(), 3);
  EXPECT_NEAR(points[0], 0.0f, 1.0e-5f);
  EXPECT_NEAR(points[1], 1.0f, 1.0e-5f);
  EXPECT_NEAR(points[2], 0.0f, 1.0e-5f);

  const auto lines = scene.get_line_buffer().get_lines();
  ASSERT_EQ(lines.size(), 6);
  EXPECT_NEAR(lines[0], -1.0f, 1.0e-5f);
  EXPECT_NEAR(lines[1], 0.0f, 1.0e-5f);
  EXPECT_NEAR(lines[2], 0.0f, 1.0e-5f);
  EXPECT_NEAR(lines[3], 0.0f, 1.0e-5f);
  EXPECT_NEAR(lines[4], 1.0f, 1.0e-5f);
  EXPECT_NEAR(lines[5], 0.0f, 1.0e-5f);
}

TEST(SceneTest, UpdatesPointAndLineGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_test_settings());
  const auto pointId = core::UUID::generate();
  const auto lineId = core::UUID::generate();

  scene.add_point(1.0f, 2.0f, 3.0f, &pointId);
  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &lineId);

  const float pointColor[] = {0.2f, 0.3f, 0.4f, 0.5f};
  const float lineColor[] = {0.6f, 0.7f, 0.8f, 0.9f};
  EXPECT_TRUE(scene.update_point(pointId, 4.0f, 5.0f, 6.0f, pointColor));
  EXPECT_TRUE(scene.update_line(lineId, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, lineColor));

  const auto points = scene.get_point_buffer().get_points();
  ASSERT_EQ(points.size(), 3);
  EXPECT_FLOAT_EQ(points[0], 4.0f);
  EXPECT_FLOAT_EQ(points[1], 5.0f);
  EXPECT_FLOAT_EQ(points[2], 6.0f);
  EXPECT_FLOAT_EQ(scene.get_point_buffer().get_point_colors()[0], 0.2f);

  const auto lines = scene.get_line_buffer().get_lines();
  ASSERT_EQ(lines.size(), 6);
  EXPECT_FLOAT_EQ(lines[0], 7.0f);
  EXPECT_FLOAT_EQ(lines[5], 12.0f);
  EXPECT_FLOAT_EQ(scene.get_line_buffer().get_line_colors()[0], 0.6f);
}
