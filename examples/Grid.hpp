#ifndef EXAMPLES_GRID_HPP
#define EXAMPLES_GRID_HPP

#include "GeoQik/GeoQik.hpp"
#include <cassert>
#include <vector>

namespace geoqik::examples
{

struct Line
{
  double x1, y1, z1, x2, y2, z2;
};

struct GridPoint
{
  double x, y, z;
};

struct Grid
{
  std::vector<GridPoint> points;
  std::vector<Line> lines;
};

Grid create_grid(double size, double step)
{
  assert(size > 0.0 && step > 0.0);

  Grid grid;

  for (double i = -size; i <= size; i += step)
  {
    grid.lines.emplace_back(i, -size, 0.0, i, size, 0.0);
    grid.lines.emplace_back(-size, i, 0.0, size, i, 0.0);
  }
  for (double x = -size; x <= size; x += step)
  {
    for (double y = -size; y <= size; y += step)
    {
      grid.points.push_back({x, y, 0.0});
    }
  }
  
  return grid;
}

void draw_grid(const Grid& grid, float lineWidth = 1.0f, float pointSize = 5.0f)
{
  assert(lineWidth > 0.0f);
  assert(pointSize > 0.0f);
  assert(pointSize > lineWidth); // Ensure points are visible over lines
  bool initialized;
  geoqik_is_api_initialized(&initialized);
  assert(initialized);

  geoqik_set_line_width(lineWidth);
  geoqik_set_point_size(pointSize);
  geoqik_set_line_color(0.5f, 0.5f, 0.5f);
  assert(!grid.lines.empty());
  for (const auto& line : grid.lines)
  {
    geoqik_add_line(line.x1, line.y1, line.z1, line.x2, line.y2, line.z2);
  }

  geoqik_set_point_color(0.5f, 0.5f, 0.5f);
  assert(!grid.points.empty());
  for (const auto& point : grid.points)
  {
    geoqik_add_point(point.x, point.y, point.z);
  }
}

void draw_grid(double size, double step)
{
  auto grid = create_grid(size, step);
  draw_grid(grid);
}

} // namespace geoqik::examples

#endif // EXAMPLES_GRID_HPP