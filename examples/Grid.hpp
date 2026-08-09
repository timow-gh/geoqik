#ifndef EXAMPLES_GRID_HPP
#define EXAMPLES_GRID_HPP

#include "GeoQik/GeoQik.hpp"

#include <cassert>
#include <iterator>
#include <vector>

namespace geoqik::examples {

struct Line {
    double x1, y1, z1, x2, y2, z2;
};

struct GridPoint {
    double x, y, z;
};

struct Grid {
    std::vector<GridPoint> points;
    std::vector<Line> lines;
};

Grid create_grid(double size, double step) {
    assert(size > 0.0 && step > 0.0);

    Grid grid;

    for (double i = -size; i <= size; i += step) {
        grid.lines.emplace_back(i, -size, 0.0, i, size, 0.0);
        grid.lines.emplace_back(-size, i, 0.0, size, i, 0.0);
    }
    for (double x = -size; x <= size; x += step) {
        for (double y = -size; y <= size; y += step) {
            grid.points.push_back({x, y, 0.0});
        }
    }

    return grid;
}

struct GridGeometryIds {
    geoqik_uuid_t pointsId;
    geoqik_uuid_t lineIds;
    geoqik_uuid_t screenPointsId;
    geoqik_uuid_t stripId;
};

GridGeometryIds add_grid(const Grid& grid, float lineWidthPixels = 2.0F, float pointRadiusWorld = 1.0F) {
    assert(lineWidthPixels > 0.0F);
    assert(pointRadiusWorld > 0.0F);
    bool initialized;
    geoqik_is_api_initialized(&initialized);
    assert(initialized);

    assert(!grid.lines.empty());

    std::vector<double> lineCoords;
    lineCoords.reserve(grid.lines.size() * 6);
    for (const auto& line: grid.lines) {
        lineCoords.insert(lineCoords.end(), {line.x1, line.y1, line.z1, line.x2, line.y2, line.z2});
    }

    const float gridColor[] = {0.5F, 0.5F, 0.5F, 1.0F};
    const float dashPattern[] = {0.6F, 0.25F};
    geoqik_add_line_opts_t lineOptions{};
    lineOptions.color = gridColor;
    lineOptions.colorCount = 4;
    lineOptions.styleSet = 1;
    lineOptions.style.lineWidth = lineWidthPixels;
    lineOptions.style.cap = GEOQIK_LINE_CAP_ROUND;
    lineOptions.style.join = GEOQIK_LINE_JOIN_ROUND;
    lineOptions.style.dashPattern = dashPattern;
    lineOptions.style.dashPatternCount = 2;
    lineOptions.style.dashSpace = GEOQIK_DASH_SPACE_WORLD;
    lineOptions.lineType = GEOQIK_LINE_TYPE_LINES;
    geoqik_result_t lineRes = geoqik_add_lines_opts(lineCoords.data(), lineCoords.size(), &lineOptions);
    assert(lineRes.err == GEOQIK_SUCCESS);

    std::vector<double> pointCoords;
    pointCoords.reserve(grid.points.size() * 3);
    assert(!grid.points.empty());
    for (const auto& point: grid.points) {
        // geoqik_add_point(point.x, point.y, point.z);
        pointCoords.insert(pointCoords.end(), {point.x, point.y, point.z});
    }

    geoqik_add_points_options_t pointOptions{};
    pointOptions.color = gridColor;
    pointOptions.colorCount = 4;
    pointOptions.radii = &pointRadiusWorld;
    pointOptions.radiusCount = 1;
    pointOptions.sizeSpace = GEOQIK_SPHERE_SIZE_SPACE_WORLD;
    geoqik_result_t pointRes = geoqik_add_points_opts(pointCoords.data(), pointCoords.size(), &pointOptions);
    assert(pointRes.err == GEOQIK_SUCCESS);

    const auto& first = grid.points.front();
    const auto& middle = grid.points[grid.points.size() / 2];
    const auto& last = grid.points.back();
    const double markerCoords[] = {first.x, first.y, first.z, middle.x, middle.y, middle.z, last.x, last.y, last.z};
    const float markerSize = 10.0F;
    const float markerColor[] = {1.0F, 0.65F, 0.1F, 1.0F};
    geoqik_add_points_options_t markerOptions{};
    markerOptions.color = markerColor;
    markerOptions.colorCount = 4;
    markerOptions.radii = &markerSize;
    markerOptions.radiusCount = 1;
    markerOptions.sizeSpace = GEOQIK_SPHERE_SIZE_SPACE_SCREEN;
    const geoqik_result_t markerRes = geoqik_add_points_opts(markerCoords, std::size(markerCoords), &markerOptions);
    assert(markerRes.err == GEOQIK_SUCCESS);

    // The public bulk-line representation remains endpoint pairs. For a strip, adjacent pairs
    // share endpoints: first->middle, middle->last.
    const double stripCoords[] =
        {first.x, first.y, first.z, middle.x, middle.y, middle.z, middle.x, middle.y, middle.z, last.x, last.y, last.z};
    const float stripDash[] = {8.0F, 4.0F};
    geoqik_add_line_opts_t stripOptions{};
    stripOptions.color = markerColor;
    stripOptions.colorCount = 4;
    stripOptions.styleSet = 1;
    stripOptions.style.lineWidth = lineWidthPixels * 1.5F;
    stripOptions.style.cap = GEOQIK_LINE_CAP_ROUND;
    stripOptions.style.join = GEOQIK_LINE_JOIN_ROUND;
    stripOptions.style.dashPattern = stripDash;
    stripOptions.style.dashPatternCount = 2;
    stripOptions.style.dashSpace = GEOQIK_DASH_SPACE_SCREEN;
    stripOptions.lineType = GEOQIK_LINE_TYPE_LINE_STRIP;
    const geoqik_result_t stripRes = geoqik_add_lines_opts(stripCoords, std::size(stripCoords), &stripOptions);
    assert(stripRes.err == GEOQIK_SUCCESS);

    return GridGeometryIds{pointRes.geometryId, lineRes.geometryId, markerRes.geometryId, stripRes.geometryId};
}

GridGeometryIds add_grid(double size, double step, float lineWidthPixels = 2.0F, float pointRadiusWorld = 1.0F) {
    auto grid = create_grid(size, step);
    return add_grid(grid, lineWidthPixels, pointRadiusWorld);
}

} // namespace geoqik::examples

#endif // EXAMPLES_GRID_HPP
