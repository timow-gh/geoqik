#include "Grid.hpp"
#include "Origin.hpp"
#include "sleep_helper.hpp"
#include <GeoQik/GeoQik.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <linal/linal.hpp>

namespace
{

constexpr std::size_t curveControlPointCount = 4;
constexpr std::size_t curveStepCount = 600;
constexpr std::size_t translatedCurveStepCount = 100;
constexpr std::size_t maxReplayEntriesPerFrame = 64;
constexpr double replayEntriesPerSecond = 240.0;
constexpr double replaySpeedMultiplier = 4.0;
constexpr double gridExtent = 10.0;
constexpr double replayDelaySeconds = 1.0;
constexpr float translationX = 15.0F;
constexpr float translationZ = 8.0F;
constexpr float rotationAngleDegrees = 260.0F;
constexpr float opaque = 1.0F;

} // namespace

void add_float3_with_color(const linal::float3& point)
{
  geoqik_add_point_with_color(
    static_cast<double>(point[0]), static_cast<double>(point[1]), static_cast<double>(point[2]), 0.0F, opaque, 0.0F, opaque);
}

void add_line_with_color(const linal::float3& start, const linal::float3& end)
{
  geoqik_add_line_with_color(
    static_cast<double>(start[0]),
    static_cast<double>(start[1]),
    static_cast<double>(start[2]),
    static_cast<double>(end[0]),
    static_cast<double>(end[1]),
    static_cast<double>(end[2]),
    0.0F,
    opaque,
    0.0F,
    opaque);
}

void draw_curve_points(const std::array<linal::float3, curveControlPointCount>& startPoints, std::size_t approxSteps)
{
  bool initialized = false;
  geoqik_is_api_initialized(&initialized);
  assert(initialized);

  std::for_each(startPoints.begin(), startPoints.end(), add_float3_with_color);

  const linal::float3 translationA{translationX, 0.0F, 0.0F};
  const linal::float3 translationB{0.0F, 0.0F, translationZ};
  const float angle = linal::degrees_to_radians(rotationAngleDegrees);
  std::array<linal::float3, curveControlPointCount> prevPoints{};
  prevPoints = startPoints;
  for (std::size_t i = 1; i < approxSteps; ++i)
  {
    std::array<linal::float3, curveControlPointCount> transformedPoint{};
    transformedPoint = startPoints;

    auto translationStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * translationA;
    for (auto& point : transformedPoint)
    {
      point += translationStep;
    }

    translationStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * translationB;
    for (auto& point : transformedPoint)
    {
      point += translationStep;
    }

    linal::float33 rot{};
    const auto angleStep = static_cast<float>(i) / static_cast<float>(approxSteps - 1) * angle;
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

void replay_current_log()
{
  geoqik_replay_options_t replayOptions{};
  replayOptions.entriesPerSecond = replayEntriesPerSecond;
  replayOptions.speedMultiplier = replaySpeedMultiplier;
  replayOptions.maxEntriesPerFrame = maxReplayEntriesPerFrame;
  replayOptions.startPaused = 1;

  geoqik_error_code_t replayResult = geoqik_replay_current_log(&replayOptions);
  if (replayResult != GEOQIK_SUCCESS)
  {
    std::cerr << "Failed to start replay: " << geoqik_get_error_string(replayResult) << '\n';
  }
}

int main()
{
  geoqik_init();

  geoqik::examples::draw_origin(1.0);
  geoqik::examples::add_grid(gridExtent, 1.0);

  geoqik_draw();

  std::array<linal::float3, curveControlPointCount> startPoints{
    linal::float3{0.0F, 0.0F, 0.0F},
    linal::float3{opaque, 0.0F, 0.0F},
    linal::float3{opaque, 0.0F, opaque},
    linal::float3{0.0F, 0.0F, opaque}
  };
  draw_curve_points(startPoints, curveStepCount);

  for (auto& point : startPoints)
  {
    point += linal::float3{translationX, 0.0F, 0.0F};
  }
  draw_curve_points(startPoints, translatedCurveStepCount);

  geoqik::examples::sleep_for_seconds(replayDelaySeconds);

  replay_current_log();

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}
