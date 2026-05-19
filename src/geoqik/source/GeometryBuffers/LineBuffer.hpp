#ifndef LINEBUFFER_HPP
#define LINEBUFFER_HPP

#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "GeoQikSettings.hpp"
#include "GeometryBuffers/GeometryBufferConcept.hpp"
#include "linal/linal.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

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

struct LineBufferSnapshot
{
  Color currentLineColor;
  std::vector<float> lines;
  std::vector<float> lineColors;
  std::vector<std::uint32_t> lineIndices;
  std::size_t lineCapacity{0};
  std::size_t lineColorCapacity{0};
  std::size_t lineIndexCapacity{0};
  std::unordered_map<core::UUID, LineGeoBufferIndex> handleToLineIndexMapping;
  std::unordered_map<core::UUID, LinesGeoBufferIndex> handleToLinesIndexMapping;
};

struct LineBufferGeometry
{
  std::vector<float> lines;
  std::vector<float> colors;
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

  [[nodiscard]] std::optional<LineBufferGeometry> get_geometry(core::UUID handle) const
  {
    if (auto it = m_handleToLineIndexMapping.find(handle); it != m_handleToLineIndexMapping.end())
    {
      const std::size_t lineIndex = it->second.lineIndex;
      return create_geometry(lineIndex, lineIndex);
    }

    if (auto it = m_handleToLinesIndexMapping.find(handle); it != m_handleToLinesIndexMapping.end())
    {
      return create_geometry(it->second.lineStartIndex, it->second.lineEndIndex);
    }

    return std::nullopt;
  }

  [[nodiscard]] LineBufferSnapshot create_snapshot() const
  {
    LineBufferSnapshot snapshot;
    snapshot.currentLineColor = m_currentLineColor;
    snapshot.lines.assign(m_lines.begin(), m_lines.end());
    snapshot.lineColors.assign(m_lineColors.begin(), m_lineColors.end());
    snapshot.lineIndices.assign(m_lineIndices.begin(), m_lineIndices.end());
    snapshot.lineCapacity = m_lines.capacity();
    snapshot.lineColorCapacity = m_lineColors.capacity();
    snapshot.lineIndexCapacity = m_lineIndices.capacity();
    snapshot.handleToLineIndexMapping = m_handleToLineIndexMapping;
    snapshot.handleToLinesIndexMapping = m_handleToLinesIndexMapping;
    return snapshot;
  }

  void restore_snapshot(const LineBufferSnapshot& snapshot)
  {
    m_currentLineColor = snapshot.currentLineColor;
    m_lines = Buffer<float>(std::max(snapshot.lineCapacity, snapshot.lines.size()));
    m_lineColors = Buffer<float>(std::max(snapshot.lineColorCapacity, snapshot.lineColors.size()));
    m_lineIndices = Buffer<std::uint32_t>(std::max(snapshot.lineIndexCapacity, snapshot.lineIndices.size()));

    for (float line: snapshot.lines)
    {
      m_lines.push_back(line);
    }
    for (float color: snapshot.lineColors)
    {
      m_lineColors.push_back(color);
    }
    for (std::uint32_t index: snapshot.lineIndices)
    {
      m_lineIndices.push_back(index);
    }

    m_handleToLineIndexMapping = snapshot.handleToLineIndexMapping;
    m_handleToLinesIndexMapping = snapshot.handleToLinesIndexMapping;
    m_linesHaveChanged = true;
  }

  bool has_space_for_lines(std::size_t count) const { return get_free_line_capacity() >= count; }

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
    if (it != m_handleToLineIndexMapping.end())
    {
      remove_single_line(it);
      return;
    }

    if (remove_lines(handle))
    {
      return;
    }
  }

  [[nodiscard]] bool update_line(core::UUID handle,
                                 float x1,
                                 float y1,
                                 float z1,
                                 float x2,
                                 float y2,
                                 float z2,
                                 std::span<const float> colors = {})
  {
    if (handle.is_nil())
    {
      return false;
    }

    auto it = m_handleToLineIndexMapping.find(handle);
    if (it == m_handleToLineIndexMapping.end())
    {
      return false;
    }
    if (!colors.empty() && colors.size() != ColorChannelCount)
    {
      return false;
    }

    const std::size_t lineIndex = it->second.lineIndex;
    const std::size_t lineStart = lineIndex * 2 * m_pointDimension;
    m_lines[lineStart] = x1;
    m_lines[lineStart + 1] = y1;
    m_lines[lineStart + 2] = z1;
    m_lines[lineStart + 3] = x2;
    m_lines[lineStart + 4] = y2;
    m_lines[lineStart + 5] = z2;

    if (!colors.empty())
    {
      const std::size_t colorStart = lineIndex * 2 * m_colorDimension;
      for (std::size_t vertex = 0; vertex < 2; ++vertex)
      {
        for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex)
        {
          m_lineColors[colorStart + vertex * m_colorDimension + colorIndex] = colors[colorIndex];
        }
      }
    }

    m_linesHaveChanged = true;
    return true;
  }

  [[nodiscard]] bool update_lines(core::UUID handle, std::span<const float> lines, std::span<const float> colors = {})
  {
    if (handle.is_nil())
    {
      return false;
    }
    if (lines.size() % (2 * m_pointDimension) != 0)
    {
      throw std::runtime_error("GeometryBuffer: The size of the lines span must be a multiple of 6 (2 * pointDimension).");
    }

    if (auto lineIt = m_handleToLineIndexMapping.find(handle); lineIt != m_handleToLineIndexMapping.end())
    {
      if (lines.size() != 2 * m_pointDimension)
      {
        return false;
      }
      return update_line(handle, lines[0], lines[1], lines[2], lines[3], lines[4], lines[5], colors);
    }

    auto linesIt = m_handleToLinesIndexMapping.find(handle);
    if (linesIt == m_handleToLinesIndexMapping.end())
    {
      return false;
    }

    const std::size_t lineStartIndex = linesIt->second.lineStartIndex;
    const std::size_t lineEndIndex = linesIt->second.lineEndIndex;
    const std::size_t lineCount = lineEndIndex - lineStartIndex + 1;
    if (lines.size() != lineCount * 2 * m_pointDimension)
    {
      return false;
    }
    if (!colors.empty() && colors.size() != ColorChannelCount && colors.size() != lineCount * ColorChannelCount)
    {
      return false;
    }

    const std::size_t lineStart = lineStartIndex * 2 * m_pointDimension;
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
      m_lines[lineStart + i] = lines[i];
    }

    if (colors.size() == ColorChannelCount)
    {
      for (std::size_t lineIndex = lineStartIndex; lineIndex <= lineEndIndex; ++lineIndex)
      {
        const std::size_t colorStart = lineIndex * 2 * m_colorDimension;
        for (std::size_t vertex = 0; vertex < 2; ++vertex)
        {
          for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex)
          {
            m_lineColors[colorStart + vertex * m_colorDimension + colorIndex] = colors[colorIndex];
          }
        }
      }
    }
    else if (colors.size() == lineCount * ColorChannelCount)
    {
      for (std::size_t lineIndex = 0; lineIndex < lineCount; ++lineIndex)
      {
        const std::size_t targetColorStart = (lineStartIndex + lineIndex) * 2 * m_colorDimension;
        const std::size_t sourceColorStart = lineIndex * ColorChannelCount;
        for (std::size_t vertex = 0; vertex < 2; ++vertex)
        {
          for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex)
          {
            m_lineColors[targetColorStart + vertex * m_colorDimension + colorIndex] = colors[sourceColorStart + colorIndex];
          }
        }
      }
    }

    m_linesHaveChanged = true;
    return true;
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

    auto linesIt = m_handleToLinesIndexMapping.find(handle);
    if (linesIt != m_handleToLinesIndexMapping.end())
    {
      translate_line_range(linesIt->second.lineStartIndex, linesIt->second.lineEndIndex, dx, dy, dz);
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
      return;
    }

    auto linesIt = m_handleToLinesIndexMapping.find(handle);
    if (linesIt != m_handleToLinesIndexMapping.end())
    {
      rotate_line_range(linesIt->second.lineStartIndex, linesIt->second.lineEndIndex, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
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

  [[nodiscard]] LineBufferGeometry create_geometry(std::size_t lineStartIndex, std::size_t lineEndIndex) const
  {
    LineBufferGeometry geometry;
    const std::size_t lineCount = lineEndIndex - lineStartIndex + 1;
    const std::size_t lineStart = lineStartIndex * 2 * m_pointDimension;
    geometry.lines.assign(m_lines.begin() + lineStart, m_lines.begin() + lineStart + lineCount * 2 * m_pointDimension);
    geometry.colors.reserve(lineCount * m_colorDimension);
    for (std::size_t lineIndex = lineStartIndex; lineIndex <= lineEndIndex; ++lineIndex)
    {
      const std::size_t colorStart = lineIndex * 2 * m_colorDimension;
      geometry.colors.insert(geometry.colors.end(), m_lineColors.begin() + colorStart, m_lineColors.begin() + colorStart + m_colorDimension);
    }
    return geometry;
  }

  void remove_single_line(std::unordered_map<core::UUID, LineGeoBufferIndex>::iterator it)
  {
    const core::UUID handle = it->first;
    const std::size_t lineIndex = it->second.lineIndex;
    remove_line_range(lineIndex, lineIndex);
    m_handleToLineIndexMapping.erase(handle);
  }

  bool remove_lines(core::UUID handle)
  {
    if (handle.is_nil())
    {
      throw std::runtime_error("GeometryBuffer: Attempting to remove lines with a nil handle");
    }

    auto it = m_handleToLinesIndexMapping.find(handle);
    if (it == m_handleToLinesIndexMapping.end())
    {
      return false;
    }

    const std::size_t lineStartIndex = it->second.lineStartIndex;
    const std::size_t lineEndIndex = it->second.lineEndIndex;
    remove_line_range(lineStartIndex, lineEndIndex);
    m_handleToLinesIndexMapping.erase(handle);
    return true;
  }

  void remove_line_range(std::size_t lineStartIndex, std::size_t lineEndIndex)
  {
    const std::size_t numberOfLinesToRemove = lineEndIndex - lineStartIndex + 1;
    const std::size_t lineStart = lineStartIndex * 2 * m_pointDimension;
    const std::size_t colorStart = lineStartIndex * 2 * m_colorDimension;
    m_lines.remove(lineStart, numberOfLinesToRemove * 2 * m_pointDimension);
    m_lineColors.remove(colorStart, numberOfLinesToRemove * 2 * m_colorDimension);

    const std::size_t indicesToRemove = lineStartIndex * 2;
    const std::uint32_t removedVertexCount = static_cast<std::uint32_t>(numberOfLinesToRemove * 2);
    const std::uint32_t lastRemovedVertexIndex = static_cast<std::uint32_t>(lineEndIndex * 2 + 1);
    for (std::size_t i = 0; i < m_lineIndices.size(); ++i)
    {
      if (m_lineIndices[i] > lastRemovedVertexIndex)
      {
        m_lineIndices[i] -= removedVertexCount;
      }
    }
    m_lineIndices.remove(indicesToRemove, numberOfLinesToRemove * 2);

    for (auto& pairIter: m_handleToLineIndexMapping)
    {
      if (lineEndIndex < pairIter.second.lineIndex)
      {
        pairIter.second.lineIndex -= numberOfLinesToRemove;
      }
    }
    for (auto& pairIter: m_handleToLinesIndexMapping)
    {
      if (lineEndIndex < pairIter.second.lineStartIndex)
      {
        pairIter.second.lineStartIndex -= numberOfLinesToRemove;
        pairIter.second.lineEndIndex -= numberOfLinesToRemove;
      }
    }

    m_linesHaveChanged = true;
  }

  void translate_line_range(std::size_t lineStartIndex, std::size_t lineEndIndex, float dx, float dy, float dz)
  {
    for (std::size_t lineIndex = lineStartIndex; lineIndex <= lineEndIndex; ++lineIndex)
    {
      const std::size_t lineStart = lineIndex * 2 * m_pointDimension;
      for (std::size_t i = 0; i < 2; ++i)
      {
        m_lines[lineStart + i * m_pointDimension] += dx;
        m_lines[lineStart + i * m_pointDimension + 1] += dy;
        m_lines[lineStart + i * m_pointDimension + 2] += dz;
      }
    }
    m_linesHaveChanged = true;
  }

  void rotate_line_range(std::size_t lineStartIndex,
                         std::size_t lineEndIndex,
                         float centerX,
                         float centerY,
                         float centerZ,
                         float axisX,
                         float axisY,
                         float axisZ,
                         float angle)
  {
    linal::float3 center{centerX, centerY, centerZ};
    linal::float3 axis{axisX, axisY, axisZ};
    axis = axis.normalize();

    linal::float33 rotationMatrix;
    linal::rot_axis(rotationMatrix, axis, angle);

    for (std::size_t lineIndex = lineStartIndex; lineIndex <= lineEndIndex; ++lineIndex)
    {
      const std::size_t lineStart = lineIndex * 2 * m_pointDimension;
      for (std::size_t i = 0; i < 2; ++i)
      {
        linal::float3 point{m_lines[lineStart + i * m_pointDimension],
                            m_lines[lineStart + i * m_pointDimension + 1],
                            m_lines[lineStart + i * m_pointDimension + 2]};
        linal::float3 rotatedPoint = rotationMatrix * (point - center) + center;
        m_lines[lineStart + i * m_pointDimension] = rotatedPoint[0];
        m_lines[lineStart + i * m_pointDimension + 1] = rotatedPoint[1];
        m_lines[lineStart + i * m_pointDimension + 2] = rotatedPoint[2];
      }
    }
    m_linesHaveChanged = true;
  }
};

static_assert(GeometryBuffer<LineBuffer>);

}

#endif // LINEBUFFER_HPP
