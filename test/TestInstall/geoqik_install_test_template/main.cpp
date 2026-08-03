#include <GeoQik/GeoQik.hpp>

int main() {
    if (geoqik_init() != GEOQIK_SUCCESS) {
        return 1;
    }

    geoqik_set_point_size(5.0f);
    geoqik_set_line_width(2.0f);
    return geoqik_cleanup() == GEOQIK_SUCCESS ? 0 : 1;
}
