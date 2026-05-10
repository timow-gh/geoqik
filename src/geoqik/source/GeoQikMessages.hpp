#ifndef GEOQIKMESSAGES_HPP
#define GEOQIKMESSAGES_HPP

#include "Color.hpp"
#include "Core/UUID.hpp"

#include <array>
#include <functional>
#include <future>
#include <optional>

namespace geoqik
{

class Context;

enum class GeoQikMessageType
{
  ADD_POINT_WITH_OPTS,
  ADD_POINTS_WITH_OPTS,
  REMOVE_POINT,
  GET_POINT_SIZE,
  SET_POINT_SIZE,
  GET_POINT_COLOR,
  SET_POINT_COLOR,
  ADD_LINE_WITH_OPTS,
  ADD_LINES_WITH_OPTS,
  REMOVE_LINE,
  GET_LINE_WIDTH,
  SET_LINE_WIDTH,
  GET_LINE_COLOR,
  SET_LINE_COLOR,
  DRAW,
  STOP_DRAW,
  REMOVE_ALL_GEOMETRY,
  TRANSLATE_GEOMETRY,
  ROTATE_GEOMETRY,
  CLEANUP
};

[[nodiscard]] inline bool is_geometry_message(GeoQikMessageType type)
{
  return type == GeoQikMessageType::ADD_POINT_WITH_OPTS ||
         type == GeoQikMessageType::ADD_POINTS_WITH_OPTS ||
         type == GeoQikMessageType::REMOVE_POINT ||
         type == GeoQikMessageType::SET_POINT_SIZE ||
         type == GeoQikMessageType::SET_POINT_COLOR ||
         type == GeoQikMessageType::ADD_LINE_WITH_OPTS ||
         type == GeoQikMessageType::ADD_LINES_WITH_OPTS ||
         type == GeoQikMessageType::REMOVE_LINE ||
         type == GeoQikMessageType::SET_LINE_WIDTH ||
         type == GeoQikMessageType::SET_LINE_COLOR ||
         type == GeoQikMessageType::REMOVE_ALL_GEOMETRY ||
         type == GeoQikMessageType::TRANSLATE_GEOMETRY ||
         type == GeoQikMessageType::ROTATE_GEOMETRY;
}

union GeoQikMessageData
{
  struct CommonMessageData
  {
    core::UUID geometryId;
    core::UUID idempotencyId;
    float* rgba = nullptr;
    std::size_t rgbaCount = 0;
  };  

  struct PointWitOpts
  {
    float x, y, z;
    CommonMessageData commonData;
  };

  struct PointsWithOpts
  {
    // PointsWithOps has ownerswip of the memory pointed to by `points`
    float* points;
    size_t size;
    CommonMessageData commonData;
  };

  struct RemovePoint
  {
    core::UUID handle;
  };

  struct LineWithOpts
  {
    float x1, y1, z1;
    float x2, y2, z2;
    CommonMessageData commonData;
  };

  struct LinesWithOpts
  {
    // LinesWithOpts has ownership of the memory pointed to by `lines`
    float* lines;
    size_t size;
    CommonMessageData commonData;
  };

  struct RemoveLine
  {
    core::UUID handle;
  };

  struct TranslateGeometry
  {
    core::UUID handle;
    float dx, dy, dz;
  };
  
   struct RotateGeometry
  {
    core::UUID handle;
    float centerX, centerY, centerZ;
    float axisX, axisY, axisZ;
    float angle;
  };

  struct PointSize
  {
    float size;
  };

  struct LineWidth
  {
    float width;
  };

  PointWitOpts pointWithOpts;
  PointsWithOpts pointsWithOpts;
  RemovePoint removePoint;

  LineWithOpts lineWithOpts;
  LinesWithOpts linesWithOpts;
  RemoveLine removeLine;

  PointSize pointSize;
  LineWidth lineWidth;
  geoqik::Color color;

  TranslateGeometry translateGeometry;
  RotateGeometry rotateGeometry;
};

struct GeoQikMessage
{
  GeoQikMessageType type;
  GeoQikMessageData data;
  std::function<void(Context& context)> callback; // Workaround for now, used for GET operations
};

} // namespace geoqik

#endif // GEOQIKMESSAGES_HPP
