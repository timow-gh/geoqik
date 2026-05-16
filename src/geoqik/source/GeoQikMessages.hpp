#ifndef GEOQIKMESSAGES_HPP
#define GEOQIKMESSAGES_HPP

#include "Color.hpp"
#include "Core/UUID.hpp"

#include <functional>
#include <variant>
#include <vector>

namespace geoqik
{

class Context;

struct GeoQikMessageCommonData
{
  core::UUID geometryId;
  core::UUID idempotencyId;
  std::vector<float> rgba;

  [[nodiscard]] bool operator==(const GeoQikMessageCommonData&) const = default;
};

struct AddPointWithOpts
{
  float x, y, z;
  GeoQikMessageCommonData commonData;

  [[nodiscard]] bool operator==(const AddPointWithOpts&) const = default;
};

struct AddPointsWithOpts
{
  std::vector<float> points;
  GeoQikMessageCommonData commonData;

  [[nodiscard]] bool operator==(const AddPointsWithOpts&) const = default;
};

struct RemovePoint
{
  core::UUID handle;

  [[nodiscard]] bool operator==(const RemovePoint&) const = default;
};

struct SetPointSize
{
  float size;

  [[nodiscard]] bool operator==(const SetPointSize&) const = default;
};

struct SetPointColor
{
  Color color;

  [[nodiscard]] bool operator==(const SetPointColor&) const = default;
};

struct AddLineWithOpts
{
  float x1, y1, z1;
  float x2, y2, z2;
  GeoQikMessageCommonData commonData;

  [[nodiscard]] bool operator==(const AddLineWithOpts&) const = default;
};

struct AddLinesWithOpts
{
  std::vector<float> lines;
  GeoQikMessageCommonData commonData;

  [[nodiscard]] bool operator==(const AddLinesWithOpts&) const = default;
};

struct RemoveLine
{
  core::UUID handle;

  [[nodiscard]] bool operator==(const RemoveLine&) const = default;
};

struct SetLineWidth
{
  float width;

  [[nodiscard]] bool operator==(const SetLineWidth&) const = default;
};

struct SetLineColor
{
  Color color;

  [[nodiscard]] bool operator==(const SetLineColor&) const = default;
};

struct RemoveAllGeometry
{
  [[nodiscard]] bool operator==(const RemoveAllGeometry&) const = default;
};

struct TranslateGeometry
{
  core::UUID handle;
  float dx, dy, dz;

  [[nodiscard]] bool operator==(const TranslateGeometry&) const = default;
};

struct RotateGeometry
{
  core::UUID handle;
  float centerX, centerY, centerZ;
  float axisX, axisY, axisZ;
  float angle;

  [[nodiscard]] bool operator==(const RotateGeometry&) const = default;
};

struct Draw
{
};

struct StopDraw
{
};

struct SaveLog
{
  std::function<void(Context& context)> callback;
};

struct LoadLog
{
  std::function<void(Context& context)> callback;
};

struct ReplayLog
{
  std::function<void(Context& context)> callback;
};

struct ReplayCurrentLog
{
  std::function<void(Context& context)> callback;
};

struct PauseReplay
{
};

struct ResumeReplay
{
};

struct StepReplay
{
  std::size_t count;
};

struct StepReplayBackward
{
  std::size_t count;
};

struct GetReplayState
{
  std::function<void(Context& context)> callback;
};

struct GetReplayProgress
{
  std::function<void(Context& context)> callback;
};

struct GetPointSize
{
  std::function<void(Context& context)> callback;
};

struct GetPointColor
{
  std::function<void(Context& context)> callback;
};

struct GetLineWidth
{
  std::function<void(Context& context)> callback;
};

struct GetLineColor
{
  std::function<void(Context& context)> callback;
};

struct Cleanup
{
};

using GeoQikLogEntry = std::variant<AddPointWithOpts,
                                    AddPointsWithOpts,
                                    RemovePoint,
                                    SetPointSize,
                                    SetPointColor,
                                    AddLineWithOpts,
                                    AddLinesWithOpts,
                                    RemoveLine,
                                    SetLineWidth,
                                    SetLineColor,
                                    RemoveAllGeometry,
                                    TranslateGeometry,
                                    RotateGeometry>;

using GeoQikMessage = std::variant<AddPointWithOpts,
                                   AddPointsWithOpts,
                                   RemovePoint,
                                   SetPointSize,
                                   SetPointColor,
                                   AddLineWithOpts,
                                   AddLinesWithOpts,
                                   RemoveLine,
                                   SetLineWidth,
                                   SetLineColor,
                                   RemoveAllGeometry,
                                   TranslateGeometry,
                                   RotateGeometry,
                                   Draw,
                                   StopDraw,
                                   SaveLog,
                                   LoadLog,
                                   ReplayLog,
                                   ReplayCurrentLog,
                                   PauseReplay,
                                   ResumeReplay,
                                   StepReplay,
                                   StepReplayBackward,
                                   GetReplayState,
                                   GetReplayProgress,
                                   GetPointSize,
                                   GetPointColor,
                                   GetLineWidth,
                                   GetLineColor,
                                   Cleanup>;

} // namespace geoqik

#endif // GEOQIKMESSAGES_HPP
