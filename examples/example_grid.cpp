#include "Grid.hpp"
#include <GeoQik/GeoQik.hpp>

namespace
{

constexpr float pointSize = 5.0F;
constexpr float lineWidth = 2.0F;
constexpr double gridSize = 500.0;
constexpr double gridSpacing = 10.0;

} // namespace

int
main()
{
    geoqik_init();

    geoqik_set_point_size(pointSize);
    geoqik_set_line_width(lineWidth);

    geoqik_draw();

    geoqik::examples::add_grid(gridSize, gridSpacing);

    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
