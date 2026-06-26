#ifndef GEOQIKREPLAYUNDO_HPP
#define GEOQIKREPLAYUNDO_HPP

#include "GeoQikMessages.hpp"
#include "IdempotencyData.hpp"
#include "Scene.hpp"
#include <Core/UUID.hpp>
#include <variant>

namespace geoqik
{

struct ReplayUndoFrame
{
  struct RestoreScene
  {
    SceneSnapshot scene;
  };

  std::variant<std::monostate, GeoQikLogEntry, RestoreScene> action;
  core::UUID idempotencyKeyToErase;
};

struct ReplayUndoContext
{
  const Scene& scene;
  const IdempotencySet& idempotencySet;

  [[nodiscard]] bool has_known_idempotency_key(const core::UUID& key) const;
};

[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const AddPointWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const AddPointsWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const UpdatePointWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const UpdatePointsWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const RemovePoint& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetPointSize& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetPointColor& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const AddLineWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const AddLinesWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const UpdateLineWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const UpdateLinesWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const RemoveLine& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetLineWidth& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetLineColor& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetMeshColor& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const RemoveAllGeometry& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const TranslateGeometry& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const RotateGeometry& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const AddMeshWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const RemoveMesh& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const UpdateMeshWithOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetMeshOverlayOpts& message, const ReplayUndoContext& context);
[[nodiscard]] ReplayUndoFrame make_replay_undo_frame(const SetMeshRenderingOpts& message, const ReplayUndoContext& context);

[[nodiscard]] ReplayUndoFrame create_replay_undo_frame(const GeoQikLogEntry& entry, const ReplayUndoContext& context);

} // namespace geoqik

#endif // GEOQIKREPLAYUNDO_HPP
