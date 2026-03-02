#include "Origin.hpp"
#include <GeoQik/GeoQik.hpp>
#include "sleep_helper.hpp"

using namespace geoqik::examples;

int main()
{
  geoqik_init();

  geoqik_set_point_size(5.0f);
  geoqik_set_line_width(2.0f);

  geoqik_draw();

  geoqik_set_point_color(1.0f, 0.0f, 0.0f); // Red
  geoqik_uuid_t pointIdA;
  geoqik_add_point_with_id(5.0, 5.0, 0.0, &pointIdA, NULL);
  
  geoqik_set_point_color(0.0f, 1.0f, 0.0f); // Green  
  geoqik_uuid_t pointIdB;
  geoqik_add_point_with_id(5.0, 0.0, 0.0, &pointIdB, NULL);
  
  geoqik_set_point_color(0.0f, 0.0f, 1.0f); // Blue
  geoqik_uuid_t pointIdC;
  geoqik_add_point_with_id(0.0, 5.0, 0.0, &pointIdC, NULL);

  sleep_for_seconds(0.5);

  geoqik_uuid_t lineIdA;
  geoqik_add_line_with_id(5.0, 5.0, 0.0, 5.0, 0.0, 0.0, &lineIdA, NULL);
  geoqik_uuid_t lineIdB;
  geoqik_add_line_with_id(5.0, 0.0, 0.0, 0.0, 5.0, 0.0, &lineIdB, NULL);
  geoqik_uuid_t lineIdC;
  geoqik_add_line_with_id(0.0, 5.0, 0.0, 5.0, 5.0, 0.0, &lineIdC, NULL);

  sleep_for_seconds(0.5);

  double pointDelta = 1.0;

  geoqik_translate_geometry(&pointIdA, pointDelta, pointDelta, 0.0);
  geoqik_translate_geometry(&pointIdB, pointDelta, pointDelta, 0.0);
  geoqik_translate_geometry(&pointIdC, pointDelta, pointDelta, 0.0);

  sleep_for_seconds(0.5);

  double lineDelta = 3.0;

  geoqik_translate_geometry(&lineIdA, lineDelta, 0.0, 0.0);
  geoqik_translate_geometry(&lineIdB, 0.0, lineDelta, 0.0);
  geoqik_translate_geometry(&lineIdC, 0.0, 0.0, lineDelta);

  for (int i = 0; i < 25; ++i)
  {
    double angle = static_cast<float>(i) * 0.01f;
    geoqik_rotate_geometry(&lineIdA, 0.5, 0.5, 0.0, 0.0, 0.0, 1.0, angle);
    geoqik_rotate_geometry(&pointIdA, 0.5, 0.5, 0.0, 0.0, 0.0, -1.0, angle);
    sleep_for_seconds(0.05);
  }

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}