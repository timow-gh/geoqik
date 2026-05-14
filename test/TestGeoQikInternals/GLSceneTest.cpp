#include "GLScene.hpp"
#include <gtest/gtest.h>

namespace
{

geoqik::GeoQikSettings make_scene_test_settings()
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 16;
  settings.initialLineCapacity = 16;
  settings.defaultPointSize = 7.0f;
  settings.defaultLineWidth = 3.0f;
  settings.defaultPointColor = geoqik::Color{0.1f, 0.2f, 0.3f, 0.4f};
  settings.defaultLineColor = geoqik::Color{0.5f, 0.6f, 0.7f, 0.8f};
  return settings;
}

} // namespace

TEST(GLSceneTest, UsesSettingsForDefaultSizesAndColors)
{
  auto settings = make_scene_test_settings();
  auto scene = geoqik::GLScene::create(settings, nullptr, nullptr);

  EXPECT_FLOAT_EQ(scene.get_point_size(), settings.defaultPointSize);
  EXPECT_FLOAT_EQ(scene.get_line_width(), settings.defaultLineWidth);
  EXPECT_EQ(scene.get_default_point_color(), settings.defaultPointColor);
  EXPECT_EQ(scene.get_default_line_color(), settings.defaultLineColor);
}

TEST(GLSceneTest, CalculatesBoundingSphereAcrossPointsAndLines)
{
  auto scene = geoqik::GLScene::create(make_scene_test_settings(), nullptr, nullptr);

  scene.add_point(3.0f, 4.0f, 0.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 12.0f);

  const auto sphere = scene.calc_bounding_sphere(linal::float3{0.0f, 0.0f, 0.0f});

  EXPECT_FLOAT_EQ(sphere.get_radius(), 12.0f);
}

TEST(GLSceneTest, ClearRemovesPointsAndLinesFromBoundingSphere)
{
  auto scene = geoqik::GLScene::create(make_scene_test_settings(), nullptr, nullptr);

  scene.add_point(10.0f, 0.0f, 0.0f);
  scene.add_line(0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f);

  scene.clear();
  const auto sphere = scene.calc_bounding_sphere(linal::float3{0.0f, 0.0f, 0.0f});

  EXPECT_FLOAT_EQ(sphere.get_radius(), 0.0f);
}

TEST(GLSceneTest, TranslatesLineGeometry)
{
  auto scene = geoqik::GLScene::create(make_scene_test_settings(), nullptr, nullptr);
  const auto lineId = core::UUID::generate();

  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &lineId);
  scene.translate_geometry(lineId, 1.0f, 2.0f, 3.0f);

  const auto lines = scene.get_geometry_buffer().get_lines();
  ASSERT_EQ(lines.size(), 6);
  EXPECT_FLOAT_EQ(lines[0], 1.0f);
  EXPECT_FLOAT_EQ(lines[1], 2.0f);
  EXPECT_FLOAT_EQ(lines[2], 3.0f);
  EXPECT_FLOAT_EQ(lines[3], 2.0f);
  EXPECT_FLOAT_EQ(lines[4], 3.0f);
  EXPECT_FLOAT_EQ(lines[5], 4.0f);
}
