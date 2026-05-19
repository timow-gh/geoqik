#include "GeoQikReplayUndo.hpp"
#include <utility>

namespace geoqik
{

bool ReplayUndoContext::has_known_idempotency_key(const core::UUID& key) const
{
  if (key.is_nil())
  {
    return false;
  }
  return idempotencySet.find(IdempotencyData{key, {}}) != idempotencySet.end();
}

ReplayUndoFrame make_replay_undo_frame(const AddPointWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (context.has_known_idempotency_key(message.commonData.idempotencyId))
  {
    return frame;
  }

  frame.action = GeoQikLogEntry{RemovePoint{message.commonData.geometryId}};
  frame.idempotencyKeyToErase = message.commonData.idempotencyId;
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const AddPointsWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (context.has_known_idempotency_key(message.commonData.idempotencyId))
  {
    return frame;
  }

  frame.action = GeoQikLogEntry{RemovePoint{message.commonData.geometryId}};
  frame.idempotencyKeyToErase = message.commonData.idempotencyId;
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const UpdatePointWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_point_geometry(message.handle))
  {
    frame.action = GeoQikLogEntry{UpdatePointsWithOpts{message.handle, std::move(geometry->points), std::move(geometry->colors)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const UpdatePointsWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_point_geometry(message.handle))
  {
    frame.action = GeoQikLogEntry{UpdatePointsWithOpts{message.handle, std::move(geometry->points), std::move(geometry->colors)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const RemovePoint& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_point_geometry(message.handle))
  {
    GeoQikMessageCommonData commonData;
    commonData.geometryId = message.handle;
    commonData.rgba = std::move(geometry->colors);
    frame.action = GeoQikLogEntry{AddPointsWithOpts{std::move(geometry->points), std::move(commonData)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame([[maybe_unused]] const SetPointSize& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{SetPointSize{context.scene.get_point_size()}};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame([[maybe_unused]] const SetPointColor& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{SetPointColor{context.scene.get_default_point_color()}};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const AddLineWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (context.has_known_idempotency_key(message.commonData.idempotencyId))
  {
    return frame;
  }

  frame.action = GeoQikLogEntry{RemoveLine{message.commonData.geometryId}};
  frame.idempotencyKeyToErase = message.commonData.idempotencyId;
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const AddLinesWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (context.has_known_idempotency_key(message.commonData.idempotencyId))
  {
    return frame;
  }

  frame.action = GeoQikLogEntry{RemoveLine{message.commonData.geometryId}};
  frame.idempotencyKeyToErase = message.commonData.idempotencyId;
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const UpdateLineWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_line_geometry(message.handle))
  {
    frame.action = GeoQikLogEntry{UpdateLinesWithOpts{message.handle, std::move(geometry->lines), std::move(geometry->colors)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const UpdateLinesWithOpts& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_line_geometry(message.handle))
  {
    frame.action = GeoQikLogEntry{UpdateLinesWithOpts{message.handle, std::move(geometry->lines), std::move(geometry->colors)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const RemoveLine& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  if (auto geometry = context.scene.get_line_geometry(message.handle))
  {
    GeoQikMessageCommonData commonData;
    commonData.geometryId = message.handle;
    commonData.rgba = std::move(geometry->colors);
    frame.action = GeoQikLogEntry{AddLinesWithOpts{std::move(geometry->lines), std::move(commonData)}};
  }
  return frame;
}

ReplayUndoFrame make_replay_undo_frame([[maybe_unused]] const SetLineWidth& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{SetLineWidth{context.scene.get_line_width()}};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame([[maybe_unused]] const SetLineColor& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{SetLineColor{context.scene.get_default_line_color()}};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame([[maybe_unused]] const RemoveAllGeometry& message, const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = ReplayUndoFrame::RestoreScene{context.scene.create_snapshot()};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const TranslateGeometry& message, [[maybe_unused]] const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{TranslateGeometry{message.handle, -message.dx, -message.dy, -message.dz}};
  return frame;
}

ReplayUndoFrame make_replay_undo_frame(const RotateGeometry& message, [[maybe_unused]] const ReplayUndoContext& context)
{
  ReplayUndoFrame frame;
  frame.action = GeoQikLogEntry{RotateGeometry{message.handle,
                                               message.centerX,
                                               message.centerY,
                                               message.centerZ,
                                               message.axisX,
                                               message.axisY,
                                               message.axisZ,
                                               -message.angle}};
  return frame;
}

ReplayUndoFrame create_replay_undo_frame(const GeoQikLogEntry& entry, const ReplayUndoContext& context)
{
  return std::visit(
      [&context](const auto& message)
      {
        return make_replay_undo_frame(message, context);
      },
      entry);
}

} // namespace geoqik
