#ifndef GEOQIKMESSAGES_HPP
#define GEOQIKMESSAGES_HPP

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
  ADD_POINT = 0,
  ADD_POINT_WITH_ID,
  REMOVE_POINT,
  ADD_POINT_COLOR,
  GET_POINT_SIZE,
  SET_POINT_SIZE,
  GET_POINT_COLOR,
  SET_POINT_COLOR,
  ADD_LINE,
  ADD_LINE_WITH_ID,
  REMOVE_LINE,
  ADD_LINE_COLOR,
  GET_LINE_WIDTH,
  SET_LINE_WIDTH,
  GET_LINE_COLOR,
  SET_LINE_COLOR,
  DRAW,
  STOP_DRAW,
  REMOVE_ALL_GEOMETRY,
  TRANSLATE_GEOMETRY,
  CLEANUP
};

[[nodiscard]] inline bool is_geometry_message(GeoQikMessageType type)
{
  return type == GeoQikMessageType::ADD_POINT ||
         type == GeoQikMessageType::ADD_POINT_WITH_ID ||
         type == GeoQikMessageType::REMOVE_POINT ||
         type == GeoQikMessageType::ADD_POINT_COLOR ||
         type == GeoQikMessageType::SET_POINT_SIZE ||
         type == GeoQikMessageType::SET_POINT_COLOR ||
         type == GeoQikMessageType::ADD_LINE ||
         type == GeoQikMessageType::ADD_LINE_WITH_ID ||
         type == GeoQikMessageType::REMOVE_LINE ||
         type == GeoQikMessageType::ADD_LINE_COLOR ||
         type == GeoQikMessageType::SET_LINE_WIDTH ||
         type == GeoQikMessageType::SET_LINE_COLOR ||
         type == GeoQikMessageType::REMOVE_ALL_GEOMETRY ||
         type == GeoQikMessageType::TRANSLATE_GEOMETRY;
}

union GeoQikMessageData
{
  struct Point
  {
    float x, y, z;
  };

  struct PointWithId
  {
    float x, y, z;
    core::UUID requestId;
    core::UUID idempotencyId;
  };

  struct RemovePoint
  {
    core::UUID handle;
  };

  struct Line
  {
    float x1, y1, z1;
    float x2, y2, z2;
  };

  struct LineWithId
  {
    float x1, y1, z1;
    float x2, y2, z2;
    core::UUID requestId;
    core::UUID idempotencyId;
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

  struct ColorPoint
  {
    float x, y, z;
    float r, g, b;
  };

  struct ColorLine
  {
    float x1, y1, z1;
    float x2, y2, z2;
    float r, g, b;
  };

  struct PointSize
  {
    float size;
  };

  struct LineWidth
  {
    float width;
  };

  struct Color
  {
    float r, g, b;
  };

  Point point;

  PointWithId pointWithId;
  RemovePoint removePoint;

  Line line;

  LineWithId lineWithId;
  RemoveLine removeLine;

  ColorPoint colorPoint;
  ColorLine colorLine;

  PointSize pointSize;
  LineWidth lineWidth;
  Color color;

  TranslateGeometry translateGeometry;
};

struct GeoQikMessage
{
  GeoQikMessageType type;
  GeoQikMessageData data;
  std::function<void(Context& context)> callback; // Workaround for now, used for GET operations
};

} // namespace geoqik

#endif // GEOQIKMESSAGES_HPP