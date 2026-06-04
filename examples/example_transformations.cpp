#include "Origin.hpp"
#include "sleep_helper.hpp"
#include <GeoQik/GeoQik.hpp>

using namespace geoqik::examples;

int main()
{
  geoqik_init();

  constexpr float pointSize = 5.0F;
  constexpr float lineWidth = 2.0F;
  geoqik_set_point_size(pointSize);
  geoqik_set_line_width(lineWidth);

  geoqik_draw();

  constexpr float colorMin = 0.0F;
  constexpr float colorMax = 1.0F;
  constexpr double cornerOffset = 5.0;
  geoqik_set_point_color(colorMax, colorMin, colorMin, colorMax); // Red
  geoqik_uuid_t pointIdA = geoqik_add_point(cornerOffset, cornerOffset, 0.0).geometryId;

  geoqik_set_point_color(colorMin, colorMax, colorMin, colorMax); // Green
  geoqik_uuid_t pointIdB = geoqik_add_point(cornerOffset, 0.0, 0.0).geometryId;

  geoqik_set_point_color(colorMin, colorMin, colorMax, colorMax); // Blue
  geoqik_uuid_t pointIdC = geoqik_add_point(0.0, cornerOffset, 0.0).geometryId;

  constexpr double longPauseSeconds = 0.5;
  sleep_for_seconds(longPauseSeconds);

  geoqik_uuid_t lineIdA =
      geoqik_add_line_opts(cornerOffset, cornerOffset, 0.0, cornerOffset, 0.0, 0.0, nullptr).geometryId;
  geoqik_uuid_t lineIdB =
      geoqik_add_line_opts(cornerOffset, 0.0, 0.0, 0.0, cornerOffset, 0.0, nullptr).geometryId;
  geoqik_uuid_t lineIdC =
      geoqik_add_line_opts(0.0, cornerOffset, 0.0, cornerOffset, cornerOffset, 0.0, nullptr).geometryId;

  sleep_for_seconds(longPauseSeconds);

  constexpr double pointDelta = 1.0;

  geoqik_translate_geometry(&pointIdA, pointDelta, pointDelta, 0.0);
  geoqik_translate_geometry(&pointIdB, pointDelta, pointDelta, 0.0);
  geoqik_translate_geometry(&pointIdC, pointDelta, pointDelta, 0.0);

  sleep_for_seconds(longPauseSeconds);

  constexpr double lineDelta = 3.0;

  geoqik_translate_geometry(&lineIdA, lineDelta, 0.0, 0.0);
  geoqik_translate_geometry(&lineIdB, 0.0, lineDelta, 0.0);
  geoqik_translate_geometry(&lineIdC, 0.0, 0.0, lineDelta);

  constexpr int animationStepCount = 25;
  constexpr double angleStep = 0.01;
  constexpr double rotationCenter = 0.5;
  constexpr double shortPauseSeconds = 0.05;
  for (int i = 0; i < animationStepCount; ++i)
  {
    const double angle = static_cast<double>(i) * angleStep;
    geoqik_rotate_geometry(&lineIdA, rotationCenter, rotationCenter, 0.0, 0.0, 0.0, 1.0, angle);
    geoqik_rotate_geometry(&pointIdA, rotationCenter, rotationCenter, 0.0, 0.0, 0.0, -1.0, angle);
    sleep_for_seconds(shortPauseSeconds);
  }

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}
