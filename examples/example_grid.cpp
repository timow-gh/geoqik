#include "Grid.hpp"

#include <GeoQik/GeoQik.hpp>

namespace {

constexpr float gridPointRadiusWorld = 1.0F;
constexpr float gridLineWidthPixels = 2.0F;
constexpr double gridSize = 500.0;
constexpr double gridSpacing = 10.0;

} // namespace

int main() {
    geoqik_init();

    geoqik_draw();

    geoqik::examples::add_grid(gridSize, gridSpacing, gridLineWidthPixels, gridPointRadiusWorld);

    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
