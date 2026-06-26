#include "GeoQikReplayUndo.hpp"
#include <array>
#include <gtest/gtest.h>
#include <variant>
#include <vector>

namespace
{

core::UUID make_uuid(std::uint8_t seed)
{
  std::array<std::uint8_t, 16> bytes{};
  for (std::size_t i = 0; i < bytes.size(); ++i)
  {
    bytes[i] = static_cast<std::uint8_t>(seed + i);
  }
  return core::UUID(bytes);
}

geoqik::GeoQikSettings make_scene_settings()
{
  geoqik::GeoQikSettings settings;
  settings.initialPointCapacity = 8;
  settings.initialLineCapacity = 8;
  settings.defaultPointSize = 4.0f;
  settings.defaultLineWidth = 2.0f;
  settings.defaultPointColor = renderer::Color{0.1f, 0.2f, 0.3f, 1.0f};
  settings.defaultLineColor = renderer::Color{0.4f, 0.5f, 0.6f, 1.0f};
  settings.defaultMeshColor = renderer::Color{0.7f, 0.8f, 0.9f, 1.0f};
  return settings;
}

template <typename T>
const T* get_log_action(const geoqik::ReplayUndoFrame& frame)
{
  const auto* entry = std::get_if<geoqik::GeoQikLogEntry>(&frame.action);
  if (!entry)
  {
    return nullptr;
  }
  return std::get_if<T>(entry);
}

} // namespace

TEST(GeoQikReplayUndoTest, AddGeometryUndoRespectsIdempotency)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  geoqik::ReplayUndoContext context{scene, idempotencySet};

  const geoqik::GeoQikMessageCommonData pointCommonData{make_uuid(1), make_uuid(20), {}};
  const auto pointFrame = geoqik::make_replay_undo_frame(geoqik::AddPointWithOpts{1.0f, 2.0f, 3.0f, pointCommonData}, context);

  const auto* removePoint = get_log_action<geoqik::RemovePoint>(pointFrame);
  ASSERT_NE(nullptr, removePoint);
  EXPECT_EQ(pointCommonData.geometryId, removePoint->handle);
  EXPECT_EQ(pointCommonData.idempotencyId, pointFrame.idempotencyKeyToErase);

  idempotencySet.insert(geoqik::IdempotencyData{pointCommonData.idempotencyId, {}});
  const auto knownPointFrame = geoqik::make_replay_undo_frame(geoqik::AddPointsWithOpts{{1.0f, 2.0f, 3.0f}, pointCommonData}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(knownPointFrame.action));
  EXPECT_TRUE(knownPointFrame.idempotencyKeyToErase.is_nil());

  const geoqik::GeoQikMessageCommonData lineCommonData{make_uuid(2), make_uuid(30), {}};
  const auto lineFrame =
      geoqik::make_replay_undo_frame(geoqik::AddLineWithOpts{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, lineCommonData}, context);

  const auto* removeLine = get_log_action<geoqik::RemoveLine>(lineFrame);
  ASSERT_NE(nullptr, removeLine);
  EXPECT_EQ(lineCommonData.geometryId, removeLine->handle);
  EXPECT_EQ(lineCommonData.idempotencyId, lineFrame.idempotencyKeyToErase);

  idempotencySet.insert(geoqik::IdempotencyData{lineCommonData.idempotencyId, {}});
  const auto knownLineFrame =
      geoqik::make_replay_undo_frame(geoqik::AddLinesWithOpts{{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f}, lineCommonData}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(knownLineFrame.action));
}

TEST(GeoQikReplayUndoTest, AddGeometryUndoForNilIdempotencyStillCreatesUndo)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  geoqik::ReplayUndoContext context{scene, idempotencySet};

  geoqik::GeoQikMessageCommonData pointCommonData{make_uuid(40), {}, {}};
  const auto pointFrame = geoqik::make_replay_undo_frame(geoqik::AddPointsWithOpts{{1.0f, 2.0f, 3.0f}, pointCommonData}, context);
  const auto* removePoint = get_log_action<geoqik::RemovePoint>(pointFrame);
  ASSERT_NE(nullptr, removePoint);
  EXPECT_EQ(pointCommonData.geometryId, removePoint->handle);
  EXPECT_TRUE(pointFrame.idempotencyKeyToErase.is_nil());

  geoqik::GeoQikMessageCommonData lineCommonData{make_uuid(41), {}, {}};
  const auto lineFrame = geoqik::make_replay_undo_frame(geoqik::AddLinesWithOpts{{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f}, lineCommonData}, context);
  const auto* removeLine = get_log_action<geoqik::RemoveLine>(lineFrame);
  ASSERT_NE(nullptr, removeLine);
  EXPECT_EQ(lineCommonData.geometryId, removeLine->handle);
  EXPECT_TRUE(lineFrame.idempotencyKeyToErase.is_nil());
}

TEST(GeoQikReplayUndoTest, RemoveGeometryUndoCapturesExistingGeometryAndIgnoresMissingGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto pointId = make_uuid(3);
  const std::vector<float> points{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  const std::vector<float> pointColors{0.1f, 0.2f, 0.3f, 1.0f, 0.4f, 0.5f, 0.6f, 1.0f};
  scene.add_points(points, pointColors, &pointId);

  const auto pointFrame = geoqik::make_replay_undo_frame(geoqik::RemovePoint{pointId}, context);
  const auto* addPoints = get_log_action<geoqik::AddPointsWithOpts>(pointFrame);
  ASSERT_NE(nullptr, addPoints);
  EXPECT_EQ(points, addPoints->points);
  EXPECT_EQ(pointColors, addPoints->commonData.rgba);
  EXPECT_EQ(pointId, addPoints->commonData.geometryId);

  const auto missingPointFrame = geoqik::make_replay_undo_frame(geoqik::RemovePoint{make_uuid(4)}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(missingPointFrame.action));

  const auto lineId = make_uuid(5);
  const std::vector<float> lines{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  const std::vector<float> lineColor{0.7f, 0.8f, 0.9f, 1.0f};
  scene.add_lines(lines, lineColor, &lineId);

  const auto lineFrame = geoqik::make_replay_undo_frame(geoqik::RemoveLine{lineId}, context);
  const auto* addLines = get_log_action<geoqik::AddLinesWithOpts>(lineFrame);
  ASSERT_NE(nullptr, addLines);
  EXPECT_EQ(lines, addLines->lines);
  EXPECT_EQ(lineColor, addLines->commonData.rgba);
  EXPECT_EQ(lineId, addLines->commonData.geometryId);

  const auto missingLineFrame = geoqik::make_replay_undo_frame(geoqik::RemoveLine{make_uuid(6)}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(missingLineFrame.action));
}

TEST(GeoQikReplayUndoTest, StyleUndoCapturesCurrentSceneValues)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  scene.set_point_size(9.0f);
  scene.set_default_point_color(0.2f, 0.3f, 0.4f, 1.0f);
  scene.set_line_width(7.0f);
  scene.set_default_line_color(0.5f, 0.6f, 0.7f, 1.0f);
  scene.set_default_mesh_color(0.8f, 0.7f, 0.6f, 1.0f);

  const auto pointSizeFrame = geoqik::make_replay_undo_frame(geoqik::SetPointSize{1.0f}, context);
  ASSERT_NE(nullptr, get_log_action<geoqik::SetPointSize>(pointSizeFrame));
  EXPECT_FLOAT_EQ(9.0f, get_log_action<geoqik::SetPointSize>(pointSizeFrame)->size);

  const auto pointColorFrame = geoqik::make_replay_undo_frame(geoqik::SetPointColor{}, context);
  const auto* pointColor = get_log_action<geoqik::SetPointColor>(pointColorFrame);
  ASSERT_NE(nullptr, pointColor);
  EXPECT_EQ((renderer::Color{0.2f, 0.3f, 0.4f, 1.0f}), pointColor->color);

  const auto lineWidthFrame = geoqik::make_replay_undo_frame(geoqik::SetLineWidth{}, context);
  const auto* lineWidth = get_log_action<geoqik::SetLineWidth>(lineWidthFrame);
  ASSERT_NE(nullptr, lineWidth);
  EXPECT_FLOAT_EQ(7.0f, lineWidth->width);

  const auto lineColorFrame = geoqik::make_replay_undo_frame(geoqik::SetLineColor{}, context);
  const auto* lineColor = get_log_action<geoqik::SetLineColor>(lineColorFrame);
  ASSERT_NE(nullptr, lineColor);
  EXPECT_EQ((renderer::Color{0.5f, 0.6f, 0.7f, 1.0f}), lineColor->color);

  const auto meshColorFrame = geoqik::make_replay_undo_frame(geoqik::SetMeshColor{}, context);
  const auto* meshColor = get_log_action<geoqik::SetMeshColor>(meshColorFrame);
  ASSERT_NE(nullptr, meshColor);
  EXPECT_EQ((renderer::Color{0.8f, 0.7f, 0.6f, 1.0f}), meshColor->color);
}

TEST(GeoQikReplayUndoTest, RemoveAllGeometryUndoCapturesSceneSnapshot)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto pointId = make_uuid(7);
  const auto lineId = make_uuid(8);
  scene.add_point(1.0f, 2.0f, 3.0f, &pointId);
  scene.add_line(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, &lineId);
  scene.set_point_size(11.0f);
  scene.set_line_width(12.0f);

  const auto frame = geoqik::make_replay_undo_frame(geoqik::RemoveAllGeometry{}, context);
  const auto* restoreScene = std::get_if<geoqik::ReplayUndoFrame::RestoreScene>(&frame.action);
  ASSERT_NE(nullptr, restoreScene);
  EXPECT_EQ(3, restoreScene->scene.pointBuffer.points.size());
  EXPECT_EQ(6, restoreScene->scene.lineBuffer.lines.size());
  EXPECT_FLOAT_EQ(11.0f, restoreScene->scene.pointSize);
  EXPECT_FLOAT_EQ(12.0f, restoreScene->scene.lineWidth);
}

TEST(GeoQikReplayUndoTest, TransformUndoCreatesInverseEntries)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto geometryId = make_uuid(9);
  const auto translateFrame = geoqik::make_replay_undo_frame(geoqik::TranslateGeometry{geometryId, 1.0f, -2.0f, 3.0f}, context);
  const auto* translate = get_log_action<geoqik::TranslateGeometry>(translateFrame);
  ASSERT_NE(nullptr, translate);
  EXPECT_EQ(geometryId, translate->handle);
  EXPECT_FLOAT_EQ(-1.0f, translate->dx);
  EXPECT_FLOAT_EQ(2.0f, translate->dy);
  EXPECT_FLOAT_EQ(-3.0f, translate->dz);

  const auto rotateFrame = geoqik::make_replay_undo_frame(geoqik::RotateGeometry{geometryId, 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 90.0f}, context);
  const auto* rotate = get_log_action<geoqik::RotateGeometry>(rotateFrame);
  ASSERT_NE(nullptr, rotate);
  EXPECT_EQ(geometryId, rotate->handle);
  EXPECT_FLOAT_EQ(1.0f, rotate->centerX);
  EXPECT_FLOAT_EQ(2.0f, rotate->centerY);
  EXPECT_FLOAT_EQ(3.0f, rotate->centerZ);
  EXPECT_FLOAT_EQ(0.0f, rotate->axisX);
  EXPECT_FLOAT_EQ(0.0f, rotate->axisY);
  EXPECT_FLOAT_EQ(1.0f, rotate->axisZ);
  EXPECT_FLOAT_EQ(-90.0f, rotate->angle);
}

TEST(GeoQikReplayUndoTest, UpdateGeometryUndoCapturesPreviousGeometry)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto pointId = make_uuid(30);
  const std::vector<float> points{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  const std::vector<float> pointColors{0.1f, 0.2f, 0.3f, 1.0f, 0.4f, 0.5f, 0.6f, 1.0f};
  scene.add_points(points, pointColors, &pointId);

  const auto pointFrame = geoqik::make_replay_undo_frame(geoqik::UpdatePointsWithOpts{pointId, {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f}, {}}, context);
  const auto* updatePoints = get_log_action<geoqik::UpdatePointsWithOpts>(pointFrame);
  ASSERT_NE(nullptr, updatePoints);
  EXPECT_EQ(pointId, updatePoints->handle);
  EXPECT_EQ(points, updatePoints->points);
  EXPECT_EQ(pointColors, updatePoints->rgba);

  const auto lineId = make_uuid(31);
  const std::vector<float> lines{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  const std::vector<float> lineColors{0.7f, 0.8f, 0.9f, 1.0f};
  scene.add_lines(lines, lineColors, &lineId);

  const auto lineFrame = geoqik::make_replay_undo_frame(geoqik::UpdateLineWithOpts{lineId, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, {}}, context);
  const auto* updateLines = get_log_action<geoqik::UpdateLinesWithOpts>(lineFrame);
  ASSERT_NE(nullptr, updateLines);
  EXPECT_EQ(lineId, updateLines->handle);
  EXPECT_EQ(lines, updateLines->lines);
  EXPECT_EQ(lineColors, updateLines->rgba);

  const auto updateLinesFrame = geoqik::make_replay_undo_frame(geoqik::UpdateLinesWithOpts{lineId, {8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f}, {}}, context);
  const auto* previousLines = get_log_action<geoqik::UpdateLinesWithOpts>(updateLinesFrame);
  ASSERT_NE(nullptr, previousLines);
  EXPECT_EQ(lineId, previousLines->handle);
  EXPECT_EQ(lines, previousLines->lines);
  EXPECT_EQ(lineColors, previousLines->rgba);

  const auto missingFrame = geoqik::make_replay_undo_frame(geoqik::UpdatePointWithOpts{make_uuid(32), 1.0f, 2.0f, 3.0f, {}}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(missingFrame.action));

  const auto missingLineFrame = geoqik::make_replay_undo_frame(geoqik::UpdateLinesWithOpts{make_uuid(33), {1.0f, 2.0f, 3.0f}, {}}, context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(missingLineFrame.action));
}

TEST(GeoQikReplayUndoTest, CreateReplayUndoFrameDispatchesVariantEntries)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto geometryId = make_uuid(50);
  const geoqik::GeoQikLogEntry entry = geoqik::TranslateGeometry{geometryId, 3.0f, 4.0f, 5.0f};
  const auto frame = geoqik::create_replay_undo_frame(entry, context);
  const auto* translate = get_log_action<geoqik::TranslateGeometry>(frame);

  ASSERT_NE(nullptr, translate);
  EXPECT_EQ(geometryId, translate->handle);
  EXPECT_FLOAT_EQ(-3.0f, translate->dx);
  EXPECT_FLOAT_EQ(-4.0f, translate->dy);
  EXPECT_FLOAT_EQ(-5.0f, translate->dz);
}

TEST(GeoQikReplayUndoTest, AddMeshWithOpts_UndoCreatesRemoveMesh)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  geoqik::ReplayUndoContext context{scene, idempotencySet};

  geoqik::GeoQikMessageCommonData common{make_uuid(60), make_uuid(70), {}};
  const auto frame = geoqik::make_replay_undo_frame(
      geoqik::AddMeshWithOpts{{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}, {}, {}, {0, 1, 2}, common, {}, {}, 1.0f, false, {}},
      context);

  const auto* removeMesh = get_log_action<geoqik::RemoveMesh>(frame);
  ASSERT_NE(nullptr, removeMesh);
  EXPECT_EQ(common.geometryId, removeMesh->handle);
  EXPECT_EQ(common.idempotencyId, frame.idempotencyKeyToErase);

  // Known idempotency key → no-op frame.
  idempotencySet.insert(geoqik::IdempotencyData{common.idempotencyId, {}});
  const auto knownFrame = geoqik::make_replay_undo_frame(
      geoqik::AddMeshWithOpts{{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}, {}, {}, {0, 1, 2}, common, {}, {}, 1.0f, false, {}},
      context);
  EXPECT_TRUE(std::holds_alternative<std::monostate>(knownFrame.action));
}

TEST(GeoQikReplayUndoTest, RemoveMesh_UndoCapturesSceneSnapshot)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto meshId = make_uuid(61);
  const std::vector<float> v = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  const std::vector<std::uint32_t> triIdx = {0, 1, 2};
  scene.add_mesh(v, {}, {}, triIdx, &meshId);

  const auto frame = geoqik::make_replay_undo_frame(geoqik::RemoveMesh{meshId}, context);
  const auto* restoreScene = std::get_if<geoqik::ReplayUndoFrame::RestoreScene>(&frame.action);
  ASSERT_NE(nullptr, restoreScene);
  EXPECT_FALSE(restoreScene->scene.meshBuffer.vertices.empty());
}

TEST(GeoQikReplayUndoTest, UpdateMeshWithOpts_UndoCapturesSceneSnapshot)
{
  auto scene = geoqik::Scene::create(make_scene_settings());
  geoqik::IdempotencySet idempotencySet;
  const geoqik::ReplayUndoContext context{scene, idempotencySet};

  const auto meshId = make_uuid(62);
  const std::vector<float> v = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  const std::vector<std::uint32_t> triIdx = {0, 1, 2};
  scene.add_mesh(v, {}, {}, triIdx, &meshId);

  const auto frame = geoqik::make_replay_undo_frame(
      geoqik::UpdateMeshWithOpts{meshId, v, {}, {}}, context);
  const auto* restoreScene = std::get_if<geoqik::ReplayUndoFrame::RestoreScene>(&frame.action);
  ASSERT_NE(nullptr, restoreScene);
  EXPECT_FALSE(restoreScene->scene.meshBuffer.vertices.empty());
}
