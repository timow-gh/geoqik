#include "Origin.hpp"
#include <GeoQik/GeoQik.hpp>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
struct Point
{
    double x, y, z;
};

namespace
{

constexpr float pointSize = 5.0F;
constexpr float lineWidth = 2.0F;
constexpr float cyanRed = 0.0F;
constexpr float cyanGreen = 0.7F;
constexpr float cyanBlue = 1.0F;
constexpr float opaque = 1.0F;

} // namespace

int
main()
{
    geoqik_init();

    geoqik_set_point_size(pointSize);
    geoqik_set_line_width(lineWidth);

    geoqik_draw();
    geoqik::examples::draw_origin(1.0);
    geoqik_set_point_color(cyanRed, cyanGreen, cyanBlue, opaque); // Cyan

    constexpr double radius = 15.0;
    constexpr int latitudeDivisions = 10;  // Number of horizontal rings
    constexpr int longitudeDivisions = 16; // Number of vertical meridians

    std::vector<Point> points;
    points.reserve((latitudeDivisions - 1) * longitudeDivisions);

    for (int lat = 1; lat < latitudeDivisions; ++lat) {
        double theta = lat * M_PI / latitudeDivisions;
        double sinTheta = std::sin(theta);
        double cosTheta = std::cos(theta);

        for (int lon = 0; lon < longitudeDivisions; ++lon) {
            double phi = lon * 2 * M_PI / longitudeDivisions;
            double sinPhi = std::sin(phi);
            double cosPhi = std::cos(phi);

            double x = radius * sinTheta * cosPhi;
            double y = radius * sinTheta * sinPhi;
            double z = radius * cosTheta;

            if (lat == 1) {
                geoqik_add_line(0.0, 0.0, radius, x, y, z);
            } else if (lat == latitudeDivisions - 1) {
                geoqik_add_line(0.0, 0.0, -radius, x, y, z);
            }

            Point point{x, y, z};
            geoqik_add_point(x, y, z);
            points.push_back(point);
        }
    }

    std::size_t pointsPerLatitudeRing = longitudeDivisions;
    std::size_t numberOfLatitudeRings = latitudeDivisions - 1;
    for (std::size_t latRingIdx = 0; latRingIdx < numberOfLatitudeRings; ++latRingIdx) {
        std::size_t startIdx = latRingIdx * pointsPerLatitudeRing;
        for (std::size_t latIdx = startIdx; latIdx < (startIdx + pointsPerLatitudeRing); ++latIdx) {
            std::size_t nextLatIdx = latIdx + 1;
            if ((latIdx + 1) % pointsPerLatitudeRing == 0) {
                nextLatIdx = startIdx;
            }
            const Point& point = points[latIdx];
            const Point& nextPoint = points[nextLatIdx];
            geoqik_add_line(point.x, point.y, point.z, nextPoint.x, nextPoint.y, nextPoint.z);

            if (latRingIdx + 1 < numberOfLatitudeRings) {
                std::size_t nextLatRingStartIdx = (latRingIdx + 1) * pointsPerLatitudeRing;
                const Point& pointBelow = points[nextLatRingStartIdx + (latIdx % pointsPerLatitudeRing)];
                geoqik_add_line(point.x, point.y, point.z, pointBelow.x, pointBelow.y, pointBelow.z);
            }
        }
    }

    geoqik_wait_for_exit_and_cleanup();

    return 0;
}
