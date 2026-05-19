#include "CameraAutoFit.hpp"
#include <gtest/gtest.h>

namespace
{

geoqik::GeoQikSettings make_settings()
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 8;
  settings.initialLineCapacity = 8;
  return settings;
}

geoqik::Scene make_scene()
{
  return geoqik::Scene::create(make_settings());
}

geoqik::CameraAutoFitInput make_input()
{
  geoqik::CameraAutoFitInput input;
  input.position = linal::double3{0.0, 0.0, 10.0};
  input.target = linal::double3{0.0, 0.0, 0.0};
  input.vertical = linal::double3{0.0, 1.0, 0.0};
  input.aspectRatio = 1.0;
  input.verticalFovDegrees = 30.0;
  input.nearPlane = 0.01;
  input.farPlaneMultiplier = 3.0;
  return input;
}

} // namespace

TEST(CameraAutoFitTest, ZoomsOutWhenGeometryIsOutsideHorizontalFrustum)
{
  auto scene = make_scene();
  scene.add_line(-100.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f);

  const geoqik::CameraAutoFitInput input = make_input();
  const geoqik::CameraAutoFitResult result = geoqik::calculate_camera_auto_fit(scene, input);

  EXPECT_TRUE(result.hasGeometry);
  EXPECT_TRUE(result.zoomedOut);
  EXPECT_TRUE(result.changed);
  EXPECT_GT(result.position[2], input.position[2]);
  EXPECT_GT(result.farPlane, input.nearPlane);
}

TEST(CameraAutoFitTest, RepositionsSafelyWhenGeometryStartsBehindCamera)
{
  auto scene = make_scene();
  scene.add_point(0.0f, 0.0f, 20.0f);

  const geoqik::CameraAutoFitInput input = make_input();
  const geoqik::CameraAutoFitResult result = geoqik::calculate_camera_auto_fit(scene, input);

  EXPECT_TRUE(result.hasGeometry);
  EXPECT_TRUE(result.zoomedOut);
  EXPECT_TRUE(result.changed);
  EXPECT_GT(result.position[2], 20.0);
  EXPECT_NE(result.position, result.target);
}

TEST(CameraAutoFitTest, ZoomsInWhenSceneOccupiesTooLittleViewport)
{
  auto scene = make_scene();
  scene.add_line(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

  geoqik::CameraAutoFitInput input = make_input();
  input.position = linal::double3{0.0, 0.0, 100.0};
  input.target = linal::double3{0.0, 0.0, 0.0};

  const geoqik::CameraAutoFitResult result = geoqik::calculate_camera_auto_fit(scene, input);

  EXPECT_TRUE(result.hasGeometry);
  EXPECT_TRUE(result.zoomedIn);
  EXPECT_TRUE(result.changed);
  EXPECT_LT(result.position[2], input.position[2]);
}

TEST(CameraAutoFitTest, SuppressesZoomInAfterRecentCameraInteraction)
{
  auto scene = make_scene();
  scene.add_line(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

  geoqik::CameraAutoFitInput input = make_input();
  input.position = linal::double3{0.0, 0.0, 100.0};
  input.target = linal::double3{0.0, 0.0, 0.0};
  input.suppressZoomIn = true;

  const geoqik::CameraAutoFitResult result = geoqik::calculate_camera_auto_fit(scene, input);

  EXPECT_TRUE(result.hasGeometry);
  EXPECT_FALSE(result.zoomedIn);
  EXPECT_FALSE(result.changed);
}

TEST(CameraAutoFitTest, SuppressionDoesNotBlockZoomOut)
{
  auto scene = make_scene();
  scene.add_line(-100.0f, 0.0f, 0.0f, 100.0f, 0.0f, 0.0f);

  geoqik::CameraAutoFitInput input = make_input();
  input.suppressZoomIn = true;

  const geoqik::CameraAutoFitResult result = geoqik::calculate_camera_auto_fit(scene, input);

  EXPECT_TRUE(result.hasGeometry);
  EXPECT_TRUE(result.zoomedOut);
  EXPECT_TRUE(result.changed);
}
