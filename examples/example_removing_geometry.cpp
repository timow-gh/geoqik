#include "Origin.hpp"
#include <GeoQik/GeoQik.hpp>
#include <chrono>
#include <thread>

void sleep_for_seconds(double seconds)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(seconds * 1000)));
}

static geoqik_uuid_t add_point_with_delay(double x, double y, double z, double delaySeconds)
{
  geoqik_uuid_t pointId;
  geoqik_add_point_with_id(x, y, z, &pointId, NULL);
  sleep_for_seconds(delaySeconds);
  return pointId;
}

static geoqik_uuid_t add_line_with_delay(double x1, double y1, double z1, double x2, double y2, double z2, double delaySeconds)
{
  geoqik_uuid_t lineId;
  geoqik_add_line_with_id(x1, y1, z1, x2, y2, z2, &lineId, NULL);
  sleep_for_seconds(delaySeconds);
  return lineId;
}

static void remove_line_with_delay(const geoqik_uuid_t* lineId, double delaySeconds)
{
  geoqik_remove_line(lineId);
  sleep_for_seconds(delaySeconds);
}

int main()
{
  geoqik_init();

  geoqik_set_point_size(5.0f);
  geoqik_set_line_width(2.0f);

  geoqik_draw();

  constexpr double delaySeconds = 0.3;

  geoqik_set_point_color(1.0f, 0.0f, 0.0f); // Red
  geoqik_uuid_t pointIdA = add_point_with_delay(5.0, 5.0, 0.0, delaySeconds);
  
  geoqik_set_point_color(0.0f, 1.0f, 0.0f); // Green  
  geoqik_uuid_t pointIdB = add_point_with_delay(5.0, 0.0, 0.0, delaySeconds);
  
  geoqik_set_point_color(0.0f, 0.0f, 1.0f); // Blue
  geoqik_uuid_t pointIdC = add_point_with_delay(0.0, 5.0, 0.0, delaySeconds);

  geoqik_uuid_t lineIdA = add_line_with_delay(5.0, 5.0, 0.0, 5.0, 0.0, 0.0, delaySeconds);
  geoqik_uuid_t lineIdB = add_line_with_delay(5.0, 0.0, 0.0, 0.0, 5.0, 0.0, delaySeconds);
  geoqik_uuid_t lineIdC = add_line_with_delay(0.0, 5.0, 0.0, 5.0, 5.0, 0.0, delaySeconds);

  sleep_for_seconds(1.0);

  remove_line_with_delay(&lineIdA, delaySeconds);
  remove_line_with_delay(&lineIdB, delaySeconds);
  remove_line_with_delay(&lineIdC, delaySeconds);

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}