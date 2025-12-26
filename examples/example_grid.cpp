#include "Grid.hpp"
#include <GeoQik/GeoQik.hpp>

int main()
{
  geoqik_init();

  geoqik_set_point_size(5.0f);
  geoqik_set_line_width(2.0f);

  geoqik_draw();

  geoqik::examples::draw_grid(500.0, 10.0);

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}