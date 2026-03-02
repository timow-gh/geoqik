#ifndef GEOMETRYBUFFER_HPP
#define GEOMETRYBUFFER_HPP

#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "GeoQikSettings.hpp"
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

// Holds the index of a line in the line buffer
struct LineGeoBufferIndex
{
  std::size_t lineIndex;

  [[nodiscard]] std::strong_ordering operator<=>(const LineGeoBufferIndex& other) const = default;
};

/** \brief Geometry buffer storing geometry data that can be used for rendering.
 *
 * This class manages the storage of points and lines, including their colors and indices to render them.
 * The buffers are directly used in the shaders and cannot be resized dynamically. When the buffers run out of capacity,
 * a new GeometryBuffer must be created with larger capacity using the create_from() method.
 */
class GeometryBuffer
{
  static constexpr std::int32_t m_pointDimension = 3; // x, y, z
  static constexpr std::int32_t m_colorDimension = 3; // r, g, b

  std::array<float, 3> m_currentPointColor{1.0f, 1.0f, 1.0f};
  Buffer<float> m_points;
  Buffer<float> m_pointColors;
  Buffer<std::uint32_t> m_pointIndices;

  std::array<float, 3> m_currentLineColor{1.0f, 1.0f, 1.0f};
  Buffer<float> m_lines;
  Buffer<float> m_lineColors;
  Buffer<std::uint32_t> m_lineIndices;

  bool m_pointsHaveChanged{false};
  bool m_linesHaveChanged{false};

  std::unordered_map<core::UUID, PointGeoBufferIndex> m_handleToPointIndexMapping;
  std::unordered_map<core::UUID, LineGeoBufferIndex> m_handleToLineIndexMapping;

public:
  [[nodiscard]] static std::unique_ptr<GeometryBuffer> create()
  {
    return std::unique_ptr<GeometryBuffer>(new GeometryBuffer());
  }

  [[nodiscard]] static std::unique_ptr<GeometryBuffer> create(const GeoQikSettings& settings)
  {
    return std::unique_ptr<GeometryBuffer>(new GeometryBuffer(settings));
  }

  [[nodiscard]] static std::unique_ptr<GeometryBuffer> create_from(const GeometryBuffer& other, std::size_t growthFactor)
  {
    auto newBuffer = GeometryBuffer::create();

    newBuffer->m_currentPointColor = other.m_currentPointColor;
    if (!other.m_points.is_empty())
    {
      newBuffer->m_points = Buffer<float>::create_from(other.m_points, other.m_points.capacity() * growthFactor);
      newBuffer->m_pointColors = Buffer<float>::create_from(other.m_pointColors, other.m_pointColors.capacity() * growthFactor);
      newBuffer->m_pointIndices = Buffer<std::uint32_t>::create_from(other.m_pointIndices, other.m_pointIndices.capacity() * growthFactor);
      newBuffer->m_handleToPointIndexMapping = std::move(other.m_handleToPointIndexMapping);
      newBuffer->m_pointsHaveChanged = true;
    }

    newBuffer->m_currentLineColor = other.m_currentLineColor;

    if (!other.m_lines.is_empty())
    {
      newBuffer->m_lines = Buffer<float>::create_from(other.m_lines, other.m_lines.capacity() * growthFactor);
      newBuffer->m_lineColors = Buffer<float>::create_from(other.m_lineColors, other.m_lineColors.capacity() * growthFactor);
      newBuffer->m_lineIndices = Buffer<std::uint32_t>::create_from(other.m_lineIndices, other.m_lineIndices.capacity() * growthFactor);
      newBuffer->m_handleToLineIndexMapping = std::move(other.m_handleToLineIndexMapping);
      newBuffer->m_linesHaveChanged = true;
    }

    return newBuffer;
  }

  GeometryBuffer(const GeometryBuffer&) = delete;
  GeometryBuffer& operator=(const GeometryBuffer&) = delete;
  GeometryBuffer(GeometryBuffer&&) = default;
  GeometryBuffer& operator=(GeometryBuffer&&) = default;
  ~GeometryBuffer() = default;

  [[nodiscard]] bool has_changed() const { return m_pointsHaveChanged || m_linesHaveChanged; }
  void reset_changed_flag()
  {
    m_pointsHaveChanged = false;
    m_linesHaveChanged = false;
  }

  [[nodiscard]] bool points_have_changed() const { return m_pointsHaveChanged; }
  void reset_points_have_changed() { m_pointsHaveChanged = false; }

  [[nodiscard]] bool lines_have_changed() const { return m_linesHaveChanged; }
  void reset_lines_have_changed() { m_linesHaveChanged = false; }

  [[nodiscard]] bool empty() const
  {
    return m_points.is_empty() && m_lines.is_empty();
  }

  void clear()
  {
    m_points.reset();
    m_pointColors.reset();
    m_pointIndices.reset();
    m_lines.reset();
    m_lineColors.reset();
    m_lineIndices.reset();
    m_handleToPointIndexMapping.clear();
    m_handleToLineIndexMapping.clear();
    m_pointsHaveChanged = true;
  }

  [[nodiscard]] static constexpr std::int32_t get_point_dimension() { return m_pointDimension; }
  [[nodiscard]] static constexpr std::int32_t get_color_dimension() { return m_colorDimension; }

  [[nodiscard]] std::array<float, 3> get_point_color() const { return m_currentPointColor; }
  [[nodiscard]] std::array<float, 3> get_line_color() const { return m_currentLineColor; }

  // clang-format off
  [[nodiscard]] std::span<const float> get_points() const { return m_points.get_as_span(); }
  [[nodiscard]] std::span<const float> get_point_colors() const { return m_pointColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_point_indices() const { return m_pointIndices.get_as_span(); }

  [[nodiscard]] std::span<const float> get_lines() const { return m_lines.get_as_span(); }
  [[nodiscard]] std::span<const float> get_line_colors() const { return m_lineColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_line_indices() const { return m_lineIndices.get_as_span(); }
  // clang-format on

  bool has_space_for_points(std::size_t count) const
  {
    return m_points.free_capacity() >= count * m_pointDimension;
  }

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr)
  {
    add_point(x, y, z, m_currentPointColor[0], m_currentPointColor[1], m_currentPointColor[2], handle);
  }

  void add_point(float x, float y, float z, float r, float g, float b, const core::UUID* handle = nullptr)
  {
    if (has_space_for_points(1) == false)
    {
      throw std::runtime_error("GeometryBuffer: Not enough space for points");
    }

    std::size_t pointIndex = m_points.size() / m_pointDimension;

    m_points.push_back(x);
    m_points.push_back(y);
    m_points.push_back(z);

    m_pointColors.push_back(r);
    m_pointColors.push_back(g);
    m_pointColors.push_back(b);

    m_pointIndices.push_back(static_cast<std::uint32_t>(m_points.size() / m_pointDimension - 1));

    if (handle != nullptr && !handle->is_nil())
    {
      m_handleToPointIndexMapping.emplace(*handle, PointGeoBufferIndex{pointIndex});
    } 

    m_pointsHaveChanged = true;
  }

  void remove_point(core::UUID handle)
  {
    if (handle.is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to remove a point with a nil handle");
    }

    auto it = m_handleToPointIndexMapping.find(handle);
    if (it == m_handleToPointIndexMapping.end())
    {
      return;
    }

    // Iterate the existing indices and decrement those greater than the removed point index.
    // Otherwise they would point to the wrong data after removal of the current point.
    std::size_t pointIndex = it->second.pointIndex;
    for (auto& pairIter : m_handleToPointIndexMapping)
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
  }

  void set_point_color(float r, float g, float b) { m_currentPointColor = {r, g, b}; }

  bool has_space_for_lines(std::size_t count) const
  {
    return m_lines.free_capacity() >= count * 2 * m_pointDimension;
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr)
  {
    add_line(x1, y1, z1, x2, y2, z2, m_currentLineColor[0], m_currentLineColor[1], m_currentLineColor[2], handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, const core::UUID* handle = nullptr)
  {
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
    CORE_ASSERT(!handle.is_nil());

    auto it = m_handleToLineIndexMapping.find(handle);
    if (it == m_handleToLineIndexMapping.end())
    {
      return;
    }

    // Iterate the existing indices and decrement those greater than the removed line index.
    // Otherwise they would point to the wrong data after removal of the current line.
    std::size_t lineIndex = it->second.lineIndex;
    for (auto& pairIter : m_handleToLineIndexMapping)
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

private:
  GeometryBuffer()
      : GeometryBuffer(GeoQikSettings{})
  {
  }

  GeometryBuffer(const GeoQikSettings& settings)
      : m_currentPointColor(settings.defaultPointColor)
      , m_points(settings.initialPointCapacity * m_pointDimension)
      , m_pointColors(settings.initialPointCapacity * m_pointDimension)
      , m_pointIndices(settings.initialPointCapacity)
      , m_currentLineColor(settings.defaultLineColor)
      , m_lines(settings.initialLineCapacity * 2 * m_pointDimension)
      , m_lineColors(settings.initialLineCapacity * 2 * m_pointDimension)
      , m_lineIndices(settings.initialLineCapacity * 2)
  {
    assert(m_points.capacity() % m_pointDimension == 0);
    assert(m_pointColors.capacity() % m_pointDimension == 0);
    assert(m_pointIndices.capacity() == settings.initialPointCapacity);
    assert(m_lines.capacity() % (2 * m_pointDimension) == 0);
    assert(m_lineColors.capacity() % (2 * m_pointDimension) == 0);
    assert(m_lineIndices.capacity() == settings.initialLineCapacity * 2);
  }
};

} // namespace geoqik

#endif // GEOMETRYBUFFER_HPP