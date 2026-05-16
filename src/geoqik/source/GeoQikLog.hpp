#ifndef GEOQIKLOG_HPP
#define GEOQIKLOG_HPP

#include "Color.hpp"
#include "Core/UUID.hpp"
#include "GeoQikMessages.hpp"
#include <filesystem>
#include <optional>
#include <span>
#include <vector>
#include <variant>

namespace geoqik
{

struct GeoQikLogCommonData
{
  core::UUID geometryId;
  core::UUID idempotencyId;
  std::vector<float> rgba;

  [[nodiscard]] bool operator==(const GeoQikLogCommonData&) const = default;
};

struct GeoQikLogAddPointWithOpts
{
  float x, y, z;
  GeoQikLogCommonData commonData;

  [[nodiscard]] bool operator==(const GeoQikLogAddPointWithOpts&) const = default;
};

struct GeoQikLogAddPointsWithOpts
{
  std::vector<float> points;
  GeoQikLogCommonData commonData;

  [[nodiscard]] bool operator==(const GeoQikLogAddPointsWithOpts&) const = default;
};

struct GeoQikLogRemovePoint
{
  core::UUID handle;

  [[nodiscard]] bool operator==(const GeoQikLogRemovePoint&) const = default;
};

struct GeoQikLogSetPointSize
{
  float size;

  [[nodiscard]] bool operator==(const GeoQikLogSetPointSize&) const = default;
};

struct GeoQikLogSetPointColor
{
  Color color;

  [[nodiscard]] bool operator==(const GeoQikLogSetPointColor&) const = default;
};

struct GeoQikLogAddLineWithOpts
{
  float x1, y1, z1;
  float x2, y2, z2;
  GeoQikLogCommonData commonData;

  [[nodiscard]] bool operator==(const GeoQikLogAddLineWithOpts&) const = default;
};

struct GeoQikLogAddLinesWithOpts
{
  std::vector<float> lines;
  GeoQikLogCommonData commonData;

  [[nodiscard]] bool operator==(const GeoQikLogAddLinesWithOpts&) const = default;
};

struct GeoQikLogRemoveLine
{
  core::UUID handle;

  [[nodiscard]] bool operator==(const GeoQikLogRemoveLine&) const = default;
};

struct GeoQikLogSetLineWidth
{
  float width;

  [[nodiscard]] bool operator==(const GeoQikLogSetLineWidth&) const = default;
};

struct GeoQikLogSetLineColor
{
  Color color;

  [[nodiscard]] bool operator==(const GeoQikLogSetLineColor&) const = default;
};

struct GeoQikLogRemoveAllGeometry
{
  [[nodiscard]] bool operator==(const GeoQikLogRemoveAllGeometry&) const = default;
};

struct GeoQikLogTranslateGeometry
{
  core::UUID handle;
  float dx, dy, dz;

  [[nodiscard]] bool operator==(const GeoQikLogTranslateGeometry&) const = default;
};

struct GeoQikLogRotateGeometry
{
  core::UUID handle;
  float centerX, centerY, centerZ;
  float axisX, axisY, axisZ;
  float angle;

  [[nodiscard]] bool operator==(const GeoQikLogRotateGeometry&) const = default;
};

using GeoQikLogEntry = std::variant<GeoQikLogAddPointWithOpts,
                                    GeoQikLogAddPointsWithOpts,
                                    GeoQikLogRemovePoint,
                                    GeoQikLogSetPointSize,
                                    GeoQikLogSetPointColor,
                                    GeoQikLogAddLineWithOpts,
                                    GeoQikLogAddLinesWithOpts,
                                    GeoQikLogRemoveLine,
                                    GeoQikLogSetLineWidth,
                                    GeoQikLogSetLineColor,
                                    GeoQikLogRemoveAllGeometry,
                                    GeoQikLogTranslateGeometry,
                                    GeoQikLogRotateGeometry>;

[[nodiscard]] std::optional<GeoQikLogEntry> create_log_entry(const GeoQikMessage& message);

void save_log_binary(const std::filesystem::path& path, std::span<const GeoQikLogEntry> entries);
[[nodiscard]] std::vector<GeoQikLogEntry> load_log_binary(const std::filesystem::path& path);

} // namespace geoqik

#endif // GEOQIKLOG_HPP
