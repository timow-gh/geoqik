#ifndef GEOMETRYBUFFER_HPP
#define GEOMETRYBUFFER_HPP

#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "GeoQikSettings.hpp"
#include "linal/linal.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <unordered_map>

namespace geoqik
{

// Holds the index of a point in the point buffer
struct PointGeoBufferIndex
{
  std::size_t pointIndex;

  [[nodiscard]] std::strong_ordering operator<=>(const PointGeoBufferIndex& other) const = default;
};

struct PointsGeoBufferIndex
{
  std::size_t pointStartIndex;
  std::size_t pointEndIndex; // inclusive, this index is part of the range.

  [[nodiscard]] std::strong_ordering operator<=>(const PointsGeoBufferIndex& other) const = default;
};

// Holds the index of a line in the line buffer
struct LineGeoBufferIndex
{
  std::size_t lineIndex;

  [[nodiscard]] std::strong_ordering operator<=>(const LineGeoBufferIndex& other) const = default;
};

struct LinesGeoBufferIndex
{
  std::size_t lineStartIndex;
  std::size_t lineEndIndex; // inclusive, this index is part of the range.

  [[nodiscard]] std::strong_ordering operator<=>(const LinesGeoBufferIndex& other) const = default;
};

class PointBuffer
{
  static constexpr std::int32_t m_pointDimension = 3; // x, y, z
  static constexpr std::int32_t m_colorDimension = 3; // r, g, b

  std::array<float, 3> m_currentPointColor{1.0f, 1.0f, 1.0f};
  Buffer<float> m_points;
  Buffer<float> m_pointColors;
  Buffer<std::uint32_t> m_pointIndices;

  bool m_pointsHaveChanged{false};

  std::unordered_map<core::UUID, PointGeoBufferIndex> m_handleToPointIndexMapping;
  std::unordered_map<core::UUID, PointsGeoBufferIndex> m_handleToPointsIndexMapping;

public:
  [[nodiscard]] static std::unique_ptr<PointBuffer> create() { return std::unique_ptr<PointBuffer>(new PointBuffer()); }

  [[nodiscard]] static std::unique_ptr<PointBuffer> create(const GeoQikSettings& settings)
  {
    return std::unique_ptr<PointBuffer>(new PointBuffer(settings));
  }

  [[nodiscard]] static std::unique_ptr<PointBuffer> create_from(const PointBuffer& other, std::size_t growthFactor)
  {
    auto newBuffer = PointBuffer::create();

    newBuffer->m_currentPointColor = other.m_currentPointColor;
    if (!other.m_points.is_empty())
    {
      newBuffer->m_points = Buffer<float>::create_from(other.m_points, other.m_points.capacity() * growthFactor);
      newBuffer->m_pointColors = Buffer<float>::create_from(other.m_pointColors, other.m_pointColors.capacity() * growthFactor);
      newBuffer->m_pointIndices = Buffer<std::uint32_t>::create_from(other.m_pointIndices, other.m_pointIndices.capacity() * growthFactor);
      newBuffer->m_handleToPointIndexMapping = std::move(other.m_handleToPointIndexMapping);
      newBuffer->m_pointsHaveChanged = true;
    }

    return newBuffer;
  }

  PointBuffer(const PointBuffer&) = delete;
  PointBuffer& operator=(const PointBuffer&) = delete;
  PointBuffer(PointBuffer&&) = default;
  PointBuffer& operator=(PointBuffer&&) = default;
  ~PointBuffer() = default;

  [[nodiscard]] bool has_changed() const { return m_pointsHaveChanged; }
  void reset_changed_flag() { m_pointsHaveChanged = false; }

  [[nodiscard]] bool points_have_changed() const { return m_pointsHaveChanged; }
  void reset_points_have_changed() { m_pointsHaveChanged = false; }

  [[nodiscard]] bool empty() const { return m_points.is_empty(); }

  void clear()
  {
    m_points.reset();
    m_pointColors.reset();
    m_pointIndices.reset();
    m_handleToPointIndexMapping.clear();
    m_handleToPointsIndexMapping.clear();
    m_pointsHaveChanged = true;
  }

  [[nodiscard]] static constexpr std::int32_t get_point_dimension() { return m_pointDimension; }
  [[nodiscard]] static constexpr std::int32_t get_color_dimension() { return m_colorDimension; }

  [[nodiscard]] std::array<float, 3> get_point_color() const { return m_currentPointColor; }

  // clang-format off
  [[nodiscard]] std::span<const float> get_points() const { return m_points.get_as_span(); }
  [[nodiscard]] std::span<const float> get_point_colors() const { return m_pointColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_point_indices() const { return m_pointIndices.get_as_span(); }
  // clang-format on

  [[nodiscard]] std::size_t get_point_capacity() const { return m_points.capacity() / m_pointDimension; }
  [[nodiscard]] std::size_t get_free_point_capacity() const { return m_points.free_capacity() / m_pointDimension; }

  bool has_space_for_points(std::size_t count) const { return m_points.free_capacity() >= count * m_pointDimension; }

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr)
  {
    add_point(x, y, z, m_currentPointColor[0], m_currentPointColor[1], m_currentPointColor[2], handle);
  }

  void add_point(float x, float y, float z, float r, float g, float b, const core::UUID* handle = nullptr)
  {
    if (handle && handle->is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to add a point with a nil handle");
    }
    if (has_space_for_points(1) == false)
    {
      throw std::runtime_error("GeometryBuffer: Not enough space for points");
    }

    std::size_t currentPointIndex = m_points.size() / m_pointDimension;

    m_points.push_back(x);
    m_points.push_back(y);
    m_points.push_back(z);

    m_pointColors.push_back(r);
    m_pointColors.push_back(g);
    m_pointColors.push_back(b);

    CORE_ASSERT(currentPointIndex <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()));
    m_pointIndices.push_back(static_cast<std::uint32_t>(currentPointIndex));

    if (handle != nullptr && !handle->is_nil())
    {
      m_handleToPointIndexMapping.emplace(*handle, PointGeoBufferIndex{currentPointIndex});
    }

    m_pointsHaveChanged = true;
  }

  void remove_point(core::UUID handle)
  {
    if (handle.is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to remove a point with a nil handle");
    }

    if (remove_single_point(handle))
    {
      return;
    }

    if (remove_points(handle))
    {
      return;
    }
  }

  void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    if (points.empty() && colors.empty())
    {
      return;
    }
    if (handle && handle->is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to add points with a nil handle");
    }
    if (points.size() % m_pointDimension != 0)
    {
      throw std::runtime_error("GeometryBuffer: The size of the points span must be a multiple of the point dimension.");
    }
    std::size_t pointCount = points.size() / m_pointDimension;
    if (has_space_for_points(pointCount) == false)
    {
      throw std::runtime_error("GeometryBuffer: Not enough space for points");
    }

    std::size_t currentPointIndex = m_points.size() / m_pointDimension;

    if (colors.size() == 0)
    {
      for (std::size_t i = 0; i < points.size(); ++i)
      {
        m_points.push_back(points[i]);
        m_pointColors.push_back(m_currentPointColor[i % m_colorDimension]);
      }
    }
    else if (colors.size() == 3 && colors[0] >= 0.0f && colors[1] >= 0.0f && colors[2] >= 0.0f)
    {
      for (std::size_t i = 0; i < points.size(); ++i)
      {
        m_points.push_back(points[i]);
        m_pointColors.push_back(colors[i % m_colorDimension]);
      }
    }
    else if (colors.size() == points.size())
    {
      for (std::size_t i = 0; i < points.size(); ++i)
      {
        m_points.push_back(points[i]);
        m_pointColors.push_back(colors[i]);
      }
    }
    else
    {
      throw std::runtime_error(
          "GeometryBuffer: Invalid colors span. It must be either empty, have a size of 3 with valid RGB values, or match the size of the "
          "points span.");
    }

    for (std::uint32_t i = static_cast<std::uint32_t>(currentPointIndex); i < static_cast<std::uint32_t>(currentPointIndex + pointCount);
         ++i)
    {
      m_pointIndices.push_back(i);
    }

    if (handle)
    {
      m_handleToPointsIndexMapping.emplace(*handle, PointsGeoBufferIndex{currentPointIndex, currentPointIndex + pointCount - 1});
    }

    m_pointsHaveChanged = true;
  }

  void set_point_color(float r, float g, float b) { m_currentPointColor = {r, g, b}; }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz)
  {
    auto pointIt = m_handleToPointIndexMapping.find(handle);
    if (pointIt != m_handleToPointIndexMapping.end())
    {
      std::size_t pointIndex = pointIt->second.pointIndex;
      std::size_t pointStart = pointIndex * m_pointDimension;
      m_points[pointStart] += dx;
      m_points[pointStart + 1] += dy;
      m_points[pointStart + 2] += dz;
      m_pointsHaveChanged = true;
      return;
    }
  }

  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
  {
    auto pointIt = m_handleToPointIndexMapping.find(handle);
    if (pointIt != m_handleToPointIndexMapping.end())
    {
      std::size_t pointIndex = pointIt->second.pointIndex;
      std::size_t pointStart = pointIndex * m_pointDimension;

      linal::float3 point{m_points[pointStart], m_points[pointStart + 1], m_points[pointStart + 2]};
      linal::float3 center{centerX, centerY, centerZ};
      linal::float3 axis{axisX, axisY, axisZ};
      axis = axis.normalize();

      linal::float33 rotationMatrix;
      linal::rot_axis(rotationMatrix, axis, angle);

      linal::float3 rotatedPoint = rotationMatrix * (point - center) + center;

      m_points[pointStart] = rotatedPoint[0];
      m_points[pointStart + 1] = rotatedPoint[1];
      m_points[pointStart + 2] = rotatedPoint[2];
      m_pointsHaveChanged = true;
      return;
    }
  }

private:
  PointBuffer()
      : PointBuffer(GeoQikSettings{})
  {
  }

  PointBuffer(const GeoQikSettings& settings)
      : m_currentPointColor(settings.defaultPointColor)
      , m_points(settings.initialPointCapacity * m_pointDimension)
      , m_pointColors(settings.initialPointCapacity * m_pointDimension)
      , m_pointIndices(settings.initialPointCapacity)
  {
    assert(m_points.capacity() % m_pointDimension == 0);
    assert(m_pointColors.capacity() % m_pointDimension == 0);
    assert(m_pointIndices.capacity() == settings.initialPointCapacity);
  }

  bool remove_single_point(core::UUID handle)
  {
    auto it = m_handleToPointIndexMapping.find(handle);
    if (it == m_handleToPointIndexMapping.end())
    {
      return false;
    }

    // Iterate the existing indices and decrement those greater than the removed point index.
    // Otherwise they would point to the wrong data after removal of the current point.
    std::size_t pointIndex = it->second.pointIndex;
    for (auto& pairIter: m_handleToPointIndexMapping)
    {
      if (pointIndex < pairIter.second.pointIndex)
      {
        pairIter.second.pointIndex--;
      }
    }

    std::size_t pointStart = pointIndex * m_pointDimension;
    m_points.remove(pointStart, m_pointDimension);
    m_pointColors.remove(pointStart, m_pointDimension);

    // Fix the indices in the index buffer and remove the index of the removed point.
    // The indices after the removed point need to be decremented by one.
    for (std::size_t i = 0; i < m_pointIndices.size(); ++i)
    {
      if (m_pointIndices[i] > pointIndex)
      {
        m_pointIndices[i]--;
      }
    }
    m_pointIndices.remove(pointIndex, 1);
    m_handleToPointIndexMapping.erase(handle);

    m_pointsHaveChanged = true;

    return true;
  }

  bool remove_points(core::UUID handle)
  {
    if (handle.is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to remove points with a nil handle");
    }

    auto it = m_handleToPointsIndexMapping.find(handle);
    if (it == m_handleToPointsIndexMapping.end())
    {
      return false;
    }

    std::size_t pointStartIndex = it->second.pointStartIndex;
    std::size_t pointEndIndex = it->second.pointEndIndex;

    std::size_t numberOfPointsToRemove = pointEndIndex - pointStartIndex + 1;
    std::size_t pointStart = pointStartIndex * m_pointDimension;
    m_points.remove(pointStart, numberOfPointsToRemove * m_pointDimension);
    m_pointColors.remove(pointStart, numberOfPointsToRemove * m_colorDimension);

    // Fix the indices in the index buffer and remove the indices of the removed points.
    // The indices after the removed points need to be decremented by the number of removed points.
    for (std::size_t i = 0; i < m_pointIndices.size(); ++i)
    {
      if (m_pointIndices[i] > pointEndIndex)
      {
        m_pointIndices[i] -= static_cast<std::uint32_t>(numberOfPointsToRemove);
      }
    }
    m_pointIndices.remove(pointStartIndex, numberOfPointsToRemove);

    // Iterate existing indices in the handle to points mapping and decrement those greater than the removed points.
    // Assume the ranges of points do not overlap. This is ensured by the GeometryBuffer.
    for (auto& pairIter: m_handleToPointsIndexMapping)
    {
      if (pointEndIndex < pairIter.second.pointStartIndex)
      {
        pairIter.second.pointStartIndex -= numberOfPointsToRemove;
        pairIter.second.pointEndIndex -= numberOfPointsToRemove;
      }
    }
    m_handleToPointsIndexMapping.erase(handle);

    m_pointsHaveChanged = true;

    return true;
  }
};

class LineBuffer
{
  static constexpr std::int32_t m_pointDimension = 3; // x, y, z
  static constexpr std::int32_t m_colorDimension = 3; // r, g, b

  std::array<float, 3> m_currentLineColor{1.0f, 1.0f, 1.0f};
  Buffer<float> m_lines;
  Buffer<float> m_lineColors;
  Buffer<std::uint32_t> m_lineIndices;

  bool m_linesHaveChanged{false};

  std::unordered_map<core::UUID, LineGeoBufferIndex> m_handleToLineIndexMapping;
  std::unordered_map<core::UUID, LinesGeoBufferIndex> m_handleToLinesIndexMapping;

public:
  [[nodiscard]] static std::unique_ptr<LineBuffer> create() { return std::unique_ptr<LineBuffer>(new LineBuffer()); }

  [[nodiscard]] static std::unique_ptr<LineBuffer> create(const GeoQikSettings& settings)
  {
    return std::unique_ptr<LineBuffer>(new LineBuffer(settings));
  }

  [[nodiscard]] static std::unique_ptr<LineBuffer> create_from(const LineBuffer& other, std::size_t growthFactor)
  {
    auto newBuffer = LineBuffer::create();

    newBuffer->m_currentLineColor = other.m_currentLineColor;

    if (!other.m_lines.is_empty())
    {
      newBuffer->m_lines = Buffer<float>::create_from(other.m_lines, other.m_lines.capacity() * growthFactor);
      newBuffer->m_lineColors = Buffer<float>::create_from(other.m_lineColors, other.m_lineColors.capacity() * growthFactor);
      newBuffer->m_lineIndices = Buffer<std::uint32_t>::create_from(other.m_lineIndices, other.m_lineIndices.capacity() * growthFactor);
      newBuffer->m_handleToLineIndexMapping = std::move(other.m_handleToLineIndexMapping);
      newBuffer->m_handleToLinesIndexMapping = std::move(other.m_handleToLinesIndexMapping);
      newBuffer->m_linesHaveChanged = true;
    }

    return newBuffer;
  }

  LineBuffer(const LineBuffer&) = delete;
  LineBuffer& operator=(const LineBuffer&) = delete;
  LineBuffer(LineBuffer&&) = default;
  LineBuffer& operator=(LineBuffer&&) = default;
  ~LineBuffer() = default;

  [[nodiscard]] bool has_changed() const { return m_linesHaveChanged; }
  void reset_changed_flag() { m_linesHaveChanged = false; }

  [[nodiscard]] bool lines_have_changed() const { return m_linesHaveChanged; }
  void reset_lines_have_changed() { m_linesHaveChanged = false; }

  [[nodiscard]] bool empty() const { return m_lines.is_empty(); }

  void clear()
  {
    m_lines.reset();
    m_lineColors.reset();
    m_lineIndices.reset();
    m_handleToLineIndexMapping.clear();
    m_handleToLinesIndexMapping.clear();
    m_linesHaveChanged = true;
  }

  [[nodiscard]] static constexpr std::int32_t get_point_dimension() { return m_pointDimension; }
  [[nodiscard]] static constexpr std::int32_t get_color_dimension() { return m_colorDimension; }

  [[nodiscard]] std::array<float, 3> get_line_color() const { return m_currentLineColor; }

  // clang-format off
  [[nodiscard]] std::span<const float> get_lines() const { return m_lines.get_as_span(); }
  [[nodiscard]] std::span<const float> get_line_colors() const { return m_lineColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_line_indices() const { return m_lineIndices.get_as_span(); }
  // clang-format on

  [[nodiscard]] std::size_t get_line_capacity() const { return m_lines.capacity() / (2 * m_pointDimension); }
  [[nodiscard]] std::size_t get_free_line_capacity() const { return m_lines.free_capacity() / (2 * m_pointDimension); }

  bool has_space_for_lines(std::size_t count) const { return m_lines.free_capacity() >= count * 2 * m_pointDimension; }

  void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    if (lines.empty() && colors.empty())
    {
      return;
    }
    if (handle && handle->is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to add lines with a nil handle");
    }
    if (lines.size() % (2 * m_pointDimension) != 0)
    {
      throw std::runtime_error("GeometryBuffer: The size of the lines span must be a multiple of 6 (2 * pointDimension).");
    }
    std::size_t lineCount = lines.size() / (2 * m_pointDimension);
    if (has_space_for_lines(lineCount) == false)
    {
      throw std::runtime_error("GeometryBuffer: Not enough space for lines");
    }

    std::size_t firstLineIndex = m_lines.size() / (2 * m_pointDimension);
    std::uint32_t firstVertexIndex = static_cast<std::uint32_t>(m_lines.size() / m_pointDimension);

    if (colors.empty())
    {
      for (std::size_t i = 0; i < lines.size(); ++i)
        m_lines.push_back(lines[i]);
      for (std::size_t i = 0; i < lineCount; ++i)
      {
        m_lineColors.push_back(m_currentLineColor[0]);
        m_lineColors.push_back(m_currentLineColor[1]);
        m_lineColors.push_back(m_currentLineColor[2]);
        m_lineColors.push_back(m_currentLineColor[0]);
        m_lineColors.push_back(m_currentLineColor[1]);
        m_lineColors.push_back(m_currentLineColor[2]);
      }
    }
    else if (colors.size() == 3)
    {
      for (std::size_t i = 0; i < lines.size(); ++i)
        m_lines.push_back(lines[i]);
      for (std::size_t i = 0; i < lineCount; ++i)
      {
        m_lineColors.push_back(colors[0]);
        m_lineColors.push_back(colors[1]);
        m_lineColors.push_back(colors[2]);
        m_lineColors.push_back(colors[0]);
        m_lineColors.push_back(colors[1]);
        m_lineColors.push_back(colors[2]);
      }
    }
    else if (colors.size() * 2 == lines.size()) // one color (3 floats) per line
    {
      for (std::size_t i = 0; i < lineCount; ++i)
      {
        const std::size_t lineBase = i * 2 * m_pointDimension;
        const std::size_t colorBase = i * m_colorDimension;
        m_lines.push_back(lines[lineBase]);
        m_lines.push_back(lines[lineBase + 1]);
        m_lines.push_back(lines[lineBase + 2]);
        m_lines.push_back(lines[lineBase + 3]);
        m_lines.push_back(lines[lineBase + 4]);
        m_lines.push_back(lines[lineBase + 5]);
        m_lineColors.push_back(colors[colorBase]);
        m_lineColors.push_back(colors[colorBase + 1]);
        m_lineColors.push_back(colors[colorBase + 2]);
        m_lineColors.push_back(colors[colorBase]);
        m_lineColors.push_back(colors[colorBase + 1]);
        m_lineColors.push_back(colors[colorBase + 2]);
      }
    }
    else
    {
      throw std::runtime_error(
          "GeometryBuffer: Invalid colors span for add_lines. It must be empty, have a size of 3, or match lineCount * 3.");
    }

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(lineCount); ++i)
    {
      m_lineIndices.push_back(firstVertexIndex + i * 2);
      m_lineIndices.push_back(firstVertexIndex + i * 2 + 1);
    }

    if (handle && !handle->is_nil())
    {
      m_handleToLinesIndexMapping.emplace(*handle, LinesGeoBufferIndex{firstLineIndex, firstLineIndex + lineCount - 1});
    }

    m_linesHaveChanged = true;
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr)
  {
    add_line(x1, y1, z1, x2, y2, z2, m_currentLineColor[0], m_currentLineColor[1], m_currentLineColor[2], handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, const core::UUID* handle = nullptr)
  {
    if (handle && handle->is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to add a line with a nil handle");
    }
    if (has_space_for_lines(1) == false)
    {
      throw std::runtime_error("GeometryBuffer: Not enough space for lines.");
    }

    std::size_t lineIndex = m_lines.size() / (2 * m_pointDimension);

    m_lines.push_back(x1);
    m_lines.push_back(y1);
    m_lines.push_back(z1);
    m_lines.push_back(x2);
    m_lines.push_back(y2);
    m_lines.push_back(z2);

    m_lineColors.push_back(r);
    m_lineColors.push_back(g);
    m_lineColors.push_back(b);
    m_lineColors.push_back(r);
    m_lineColors.push_back(g);
    m_lineColors.push_back(b);

    std::uint32_t firstVertexIndex = static_cast<std::uint32_t>(m_lines.size() / m_pointDimension) - 2;
    std::uint32_t secondVertexIndex = firstVertexIndex + 1;

    m_lineIndices.push_back(firstVertexIndex);
    m_lineIndices.push_back(secondVertexIndex);

    if (handle != nullptr && !handle->is_nil())
    {
      m_handleToLineIndexMapping.emplace(*handle, LineGeoBufferIndex{lineIndex});
    }

    m_linesHaveChanged = true;
  }

  void remove_line(core::UUID handle)
  {
    if (handle.is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to remove a line with a nil handle");
    }

    auto it = m_handleToLineIndexMapping.find(handle);
    if (it == m_handleToLineIndexMapping.end())
    {
      return;
    }

    // Iterate the existing indices and decrement those greater than the removed line index.
    // Otherwise they would point to the wrong data after removal of the current line.
    std::size_t lineIndex = it->second.lineIndex;
    for (auto& pairIter: m_handleToLineIndexMapping)
    {
      if (lineIndex < pairIter.second.lineIndex)
      {
        pairIter.second.lineIndex--;
      }
    }

    std::size_t lineStart = lineIndex * 2 * m_pointDimension;
    m_lines.remove(lineStart, 2 * m_pointDimension);
    m_lineColors.remove(lineStart, 2 * m_pointDimension);

    // Fix the indices in the index buffer and remove the indices of the removed line.
    // The indices after the removed line need to be decremented by two (since each line has 2 vertices).
    std::size_t indicesToRemove = lineIndex * 2;
    for (std::size_t i = 0; i < m_lineIndices.size(); ++i)
    {
      if (m_lineIndices[i] > lineIndex * 2 + 1)
      {
        m_lineIndices[i] -= 2;
      }
    }
    m_lineIndices.remove(indicesToRemove, 2);
    m_handleToLineIndexMapping.erase(handle);

    m_linesHaveChanged = true;
  }

  void set_line_color(float r, float g, float b) { m_currentLineColor = {r, g, b}; }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz)
  {
    auto lineIt = m_handleToLineIndexMapping.find(handle);
    if (lineIt != m_handleToLineIndexMapping.end())
    {
      std::size_t lineIndex = lineIt->second.lineIndex;
      std::size_t lineStart = lineIndex * 2 * m_pointDimension;
      for (std::size_t i = 0; i < 2; ++i)
      {
        m_lines[lineStart + i * m_pointDimension] += dx;
        m_lines[lineStart + i * m_pointDimension + 1] += dy;
        m_lines[lineStart + i * m_pointDimension + 2] += dz;
      }
      m_linesHaveChanged = true;
      return;
    }
  }

  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
  {
    auto lineIt = m_handleToLineIndexMapping.find(handle);
    if (lineIt != m_handleToLineIndexMapping.end())
    {
      std::size_t lineIndex = lineIt->second.lineIndex;
      std::size_t lineStart = lineIndex * 2 * m_pointDimension;
      for (std::size_t i = 0; i < 2; ++i)
      {
        linal::float3 point{m_lines[lineStart + i * m_pointDimension],
                            m_lines[lineStart + i * m_pointDimension + 1],
                            m_lines[lineStart + i * m_pointDimension + 2]};
        linal::float3 center{centerX, centerY, centerZ};
        linal::float3 axis{axisX, axisY, axisZ};
        axis = axis.normalize();

        linal::float33 rotationMatrix;
        linal::rot_axis(rotationMatrix, axis, angle);

        linal::float3 rotatedPoint = rotationMatrix * (point - center) + center;

        m_lines[lineStart + i * m_pointDimension] = rotatedPoint[0];
        m_lines[lineStart + i * m_pointDimension + 1] = rotatedPoint[1];
        m_lines[lineStart + i * m_pointDimension + 2] = rotatedPoint[2];
      }
      m_linesHaveChanged = true;
    }
  }

private:
  LineBuffer()
      : LineBuffer(GeoQikSettings{})
  {
  }

  LineBuffer(const GeoQikSettings& settings)
      : m_currentLineColor(settings.defaultLineColor)
      , m_lines(settings.initialLineCapacity * 2 * m_pointDimension)
      , m_lineColors(settings.initialLineCapacity * 2 * m_pointDimension)
      , m_lineIndices(settings.initialLineCapacity * 2)
  {
    assert(m_lines.capacity() % (2 * m_pointDimension) == 0);
    assert(m_lineColors.capacity() % (2 * m_pointDimension) == 0);
    assert(m_lineIndices.capacity() == settings.initialLineCapacity * 2);
  }
};

} // namespace geoqik

#endif // GEOMETRYBUFFER_HPP