#ifndef LINEBUFFER_HPP
#define LINEBUFFER_HPP

#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "GeoQikSettings.hpp"
#include "GeometryBuffers/GeometryBufferConcept.hpp"
#include "linal/linal.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <unordered_map>

namespace geoqik
{

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


class LineBuffer
{
  static constexpr std::int32_t m_pointDimension = 3; // x, y, z
  static constexpr std::int32_t m_colorDimension = static_cast<std::int32_t>(ColorChannelCount); // r, g, b, a

  Color m_currentLineColor{1.0f, 1.0f, 1.0f, 1.0f};
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

  // clang-format off
  [[nodiscard]] std::span<const float> get_lines() const { return m_lines.get_as_span(); }
  [[nodiscard]] std::span<const float> get_line_colors() const { return m_lineColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_line_indices() const { return m_lineIndices.get_as_span(); }
  // clang-format on

  [[nodiscard]] std::size_t get_line_capacity() const { return m_lines.capacity() / (2 * m_pointDimension); }
  [[nodiscard]] std::size_t get_free_line_capacity() const { return m_lines.free_capacity() / (2 * m_pointDimension); }

  [[nodiscard]] Color get_default_color() const { return m_currentLineColor; }
  void set_default_color(float r, float g, float b, float a) { m_currentLineColor = {r, g, b, a}; }

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
        for (std::size_t vertex = 0; vertex < 2; ++vertex)
        {
          for (float channel: m_currentLineColor)
          {
            m_lineColors.push_back(channel);
          }
        }
      }
    }
    else if (colors.size() == ColorChannelCount)
    {
      for (std::size_t i = 0; i < lines.size(); ++i)
        m_lines.push_back(lines[i]);
      for (std::size_t i = 0; i < lineCount; ++i)
      {
        for (std::size_t vertex = 0; vertex < 2; ++vertex)
        {
          for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex)
          {
            m_lineColors.push_back(colors[colorIndex]);
          }
        }
      }
    }
    else if (colors.size() == lineCount * ColorChannelCount) // one color (4 floats) per line
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
        for (std::size_t vertex = 0; vertex < 2; ++vertex)
        {
          for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex)
          {
            m_lineColors.push_back(colors[colorBase + colorIndex]);
          }
        }
      }
    }
    else
    {
      throw std::runtime_error(
          "GeometryBuffer: Invalid colors span for add_lines. It must be empty, have a size of 4, or match lineCount * 4.");
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
    add_line(x1, y1, z1, x2, y2, z2, m_currentLineColor[0], m_currentLineColor[1], m_currentLineColor[2], m_currentLineColor[3], handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr)
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
    m_lineColors.push_back(a);
    m_lineColors.push_back(r);
    m_lineColors.push_back(g);
    m_lineColors.push_back(b);
    m_lineColors.push_back(a);

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
    std::size_t colorStart = lineIndex * 2 * m_colorDimension;
    m_lines.remove(lineStart, 2 * m_pointDimension);
    m_lineColors.remove(colorStart, 2 * m_colorDimension);

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
      , m_lineColors(settings.initialLineCapacity * 2 * m_colorDimension)
      , m_lineIndices(settings.initialLineCapacity * 2)
  {
    assert(m_lines.capacity() % (2 * m_pointDimension) == 0);
    assert(m_lineColors.capacity() % (2 * m_colorDimension) == 0);
    assert(m_lineIndices.capacity() == settings.initialLineCapacity * 2);
  }
};

static_assert(GeometryBuffer<LineBuffer>);

}

#endif // LINEBUFFER_HPP
