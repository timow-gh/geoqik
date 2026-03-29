#include "Grid.hpp"
#include "Origin.hpp"
#include <GeoQik/GeoQik.hpp>
#include <linal/linal.hpp>
#include <cassert>
#include <array>
#include <algorithm>

void add_float3_with_color(const linal::float3& point)
{
  geoqik_add_point_with_color(point[0], point[1], point[2], 0.0f, 1.0f, 0.0f);
}

void add_line_with_color(const linal::float3& start, const linal::float3& end)
{
  geoqik_add_line_with_color(start[0], start[1], start[2], end[0], end[1], end[2], 0.0f, 1.0f, 0.0f);
}

void draw_curve_points(const std::array<linal::float3, 4>& startPoints, std::size_t approxSteps)
{
  bool initialized;
  geoqik_is_api_initialized(&initialized);
  assert(initialized);

  std::for_each(startPoints.begin(), startPoints.end(), add_float3_with_color);

  linal::float3 translationA{15.0f, 0.0f, 0.0f};
  linal::float3 translationB{0.0f, 0.0f, 8.0f};
  float angle = linal::degrees_to_radians(260.0f);
  std::array<linal::float3, 4> prevPoints = startPoints;
  for (std::size_t i = 1; i < approxSteps; ++i)
  {
    std::array<linal::float3, 4> transformedPoint = startPoints;

    linal::float3 translationStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * translationA;
    for (auto& point : transformedPoint)
    {
      point += translationStep;
    }

    translationStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * translationB;
    for (auto& point : transformedPoint)
    {
      point += translationStep;
    }

    linal::float33 rot;
    float angleStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * angle;
    linal::rot_z(rot, angleStep);
    for (auto& point : transformedPoint)
    {
      point = rot * point;
    }

    for (std::size_t j = 0; j < prevPoints.size(); ++j)
    {
      add_line_with_color(prevPoints[j], transformedPoint[j]);
    }
    prevPoints = transformedPoint;
  }
}

int main()
{
  geoqik_init();

  geoqik::examples::draw_origin(1.0);
  geoqik::examples::add_grid(10.0, 1.0);

  geoqik_draw();

  std::array<linal::float3, 4> startPoints = {
    linal::float3{0.0f, 0.0f, 0.0f},
    linal::float3{1.0f, 0.0f, 0.0f},
    linal::float3{1.0f, 0.0f, 1.0f},
    linal::float3{0.0f, 0.0f, 1.0f}
  };
  draw_curve_points(startPoints, 600);

  for (auto& point : startPoints)
  {
    point += linal::float3{15.0f, 0.0f, 0.0f};
  }
  draw_curve_points(startPoints, 100);

  geoqik_wait_for_exit_and_cleanup();
  
  return 0;
}