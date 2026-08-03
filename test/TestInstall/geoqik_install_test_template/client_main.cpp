#include <GeoQikClient/GeoQikClient.hpp>

int main() {
    if (geoqik_init() != GEOQIK_SUCCESS) {
        return 1;
    }
    return geoqik_cleanup() == GEOQIK_SUCCESS ? 0 : 1;
}
