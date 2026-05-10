#ifndef OPENGL_LINESOUP_HPP
#define OPENGL_LINESOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/LineType.hpp"
#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <vector>
DISABLE_ALL_WARNINGS
namespace opengl
{

class OPENGL_EXPORT LineDrawable
{
  LineProgram* m_program{nullptr};
  VertexArray m_vertexArray;
  VertexBuffer m_vertexBuffer;
  VertexBuffer m_colorBuffer;
  IndexBuffer m_opaqueLineIndicesBuffer;
  IndexBuffer m_translucentLineIndicesBuffer;
  float m_lineThickness{1.0F};
  LineType m_lineType{};
  float m_pointSize{1.0F};
  std::int32_t m_vertexDimension{0};
  std::int32_t m_colorDimension{0};
  std::vector<linal::float3> m_vertexPositions;
  std::vector<std::uint8_t> m_vertexTranslucency;
  std::vector<std::uint32_t> m_lineIndices;
  std::vector<SortableLineSegment> m_translucentLineSegments;
  DrawableTransparencyInfo m_transparencyInfo;

public:
  LineDrawable(LineProgram& program,
               VertexArray vertexArray,
               VertexBuffer vertexBuffer,
               VertexBuffer colorBuffer,
               IndexBuffer opaqueLineIndicesBuffer,
               IndexBuffer translucentLineIndicesBuffer,
               float lineThickness,
               float pointSize = 1.0F,
               LineType lineType = LineType::lines(),
               DrawableTransparencyInfo transparencyInfo = {},
               std::int32_t vertexDimension = 3,
               std::int32_t colorDimension = 4,
               std::vector<linal::float3> vertexPositions = {},
               std::vector<std::uint8_t> vertexTranslucency = {},
               std::vector<std::uint32_t> lineIndices = {},
               std::vector<SortableLineSegment> translucentLineSegments = {})
      : m_program(&program)
      , m_vertexArray(std::move(vertexArray))
      , m_vertexBuffer(std::move(vertexBuffer))
      , m_colorBuffer(std::move(colorBuffer))
      , m_opaqueLineIndicesBuffer(std::move(opaqueLineIndicesBuffer))
      , m_translucentLineIndicesBuffer(std::move(translucentLineIndicesBuffer))
      , m_lineThickness(lineThickness)
      , m_lineType(lineType)
      , m_pointSize(pointSize)
      , m_vertexDimension(vertexDimension)
      , m_colorDimension(colorDimension)
      , m_vertexPositions(std::move(vertexPositions))
      , m_vertexTranslucency(std::move(vertexTranslucency))
      , m_lineIndices(std::move(lineIndices))
      , m_translucentLineSegments(std::move(translucentLineSegments))
      , m_transparencyInfo(transparencyInfo)
  {
  }

  LineDrawable(const LineDrawable&) = delete;
  LineDrawable& operator=(const LineDrawable&) = delete;
  LineDrawable(LineDrawable&& other) noexcept
      : m_program(std::exchange(other.m_program, nullptr))
      , m_vertexArray(std::move(other.m_vertexArray))
      , m_vertexBuffer(std::move(other.m_vertexBuffer))
      , m_colorBuffer(std::move(other.m_colorBuffer))
      , m_opaqueLineIndicesBuffer(std::move(other.m_opaqueLineIndicesBuffer))
      , m_translucentLineIndicesBuffer(std::move(other.m_translucentLineIndicesBuffer))
      , m_lineThickness(other.m_lineThickness)
      , m_lineType(other.m_lineType)
      , m_pointSize(other.m_pointSize)
      , m_vertexDimension(other.m_vertexDimension)
      , m_colorDimension(other.m_colorDimension)
      , m_vertexPositions(std::move(other.m_vertexPositions))
      , m_vertexTranslucency(std::move(other.m_vertexTranslucency))
      , m_lineIndices(std::move(other.m_lineIndices))
      , m_translucentLineSegments(std::move(other.m_translucentLineSegments))
      , m_transparencyInfo(other.m_transparencyInfo)
  {
  }
  LineDrawable& operator=(LineDrawable&& other) noexcept
  {
    if (this != &other)
    {
      m_program = std::exchange(other.m_program, nullptr);
      m_vertexArray = std::move(other.m_vertexArray);
      m_vertexBuffer = std::move(other.m_vertexBuffer);
      m_colorBuffer = std::move(other.m_colorBuffer);
      m_opaqueLineIndicesBuffer = std::move(other.m_opaqueLineIndicesBuffer);
      m_translucentLineIndicesBuffer = std::move(other.m_translucentLineIndicesBuffer);
      m_lineThickness = other.m_lineThickness;
      m_lineType = other.m_lineType;
      m_pointSize = other.m_pointSize;
      m_vertexDimension = other.m_vertexDimension;
      m_colorDimension = other.m_colorDimension;
      m_vertexPositions = std::move(other.m_vertexPositions);
      m_vertexTranslucency = std::move(other.m_vertexTranslucency);
      m_lineIndices = std::move(other.m_lineIndices);
      m_translucentLineSegments = std::move(other.m_translucentLineSegments);
      m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
  }
  ~LineDrawable() = default;

  [[nodiscard]] float get_line_thickness() const { return m_lineThickness; }
  void set_line_thickness(float lineThickness) { m_lineThickness = lineThickness; }

  [[nodiscard]] float get_point_size() const { return m_pointSize; }
  void set_point_size(float pointSize) { m_pointSize = pointSize; }

  void update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern)
  {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
    m_vertexPositions = make_vertex_sort_positions(vertices, m_vertexDimension);
    m_transparencyInfo.sortCenter = calc_sort_center(vertices, m_vertexDimension);
    rebuild_index_buffers(accessPattern);
  }

  void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern)
  {
    m_colorBuffer.update_buffer(colors, accessPattern);
    m_vertexTranslucency = make_vertex_translucency_flags(colors, m_colorDimension);
    rebuild_index_buffers(accessPattern);
  }

  void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern)
  {
    m_lineIndices.assign(indices.begin(), indices.end());
    rebuild_index_buffers(accessPattern);
  }

  void update_line_drawable(std::span<const float> vertices,
                            std::span<const float> colors,
                            std::span<const std::uint32_t> indices,
                            BufferAccessPattern accessPattern)
  {
    update_vertex_buffer(vertices, accessPattern);
    update_color_buffer(colors, accessPattern);
    update_indices_buffer(indices, accessPattern);
  }

  void draw(const linal::hmatf& mvp) const
  {
    draw_opaque(mvp);
    draw_index_buffer(mvp, m_translucentLineIndicesBuffer);
  }

  void draw_opaque(const linal::hmatf& mvp) const
  {
    draw_index_buffer(mvp, m_opaqueLineIndicesBuffer);
  }

  void draw_translucent(const linal::hmatf& mvp, const linal::double3& viewPosition)
  {
    const std::vector<std::uint32_t> sortedIndices = sort_translucent_line_indices_back_to_front(m_translucentLineSegments, viewPosition);
    m_translucentLineIndicesBuffer.update_indices_buffer(sortedIndices, BufferAccessPattern::STREAM_DRAW);
    draw_index_buffer(mvp, m_translucentLineIndicesBuffer);
  }

  [[nodiscard]] bool has_opaque_primitives() const noexcept { return m_opaqueLineIndicesBuffer.get_index_count() > 0; }

  [[nodiscard]] bool has_translucent_primitives() const noexcept { return m_translucentLineIndicesBuffer.get_index_count() > 0; }

  [[nodiscard]] bool is_translucent() const noexcept { return has_translucent_primitives(); }

  [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition) const noexcept
  {
    return m_transparencyInfo.distance_squared_to(viewPosition);
  }

private:
  void draw_index_buffer(const linal::hmatf& mvp, const IndexBuffer& indexBuffer) const
  {
    if (indexBuffer.get_index_count() == 0)
    {
      return;
    }

    CORE_ASSERT(m_program != nullptr);
    auto& prog = *m_program;
    prog.use();
    glUniformMatrix4fv(prog.get_mvp_location().get_value(), 1, GL_FALSE, (const GLfloat*)mvp.data());
    glLineWidth(m_lineThickness);
    m_vertexArray.bind();
    indexBuffer.bind();
    glDrawElements(m_lineType.get_gl_type(), indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);

    if (m_pointSize != 0.0F)
    {
      glPointSize(m_pointSize);
      glDrawElements(GL_POINTS, indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
    }
  }

  void rebuild_index_buffers(BufferAccessPattern accessPattern)
  {
    LineTransparencyIndexSplit split = split_line_indices_by_transparency(m_lineIndices, m_vertexPositions, m_vertexTranslucency);
    m_translucentLineSegments = std::move(split.translucentSegments);
    m_opaqueLineIndicesBuffer.update_indices_buffer(split.opaqueIndices, accessPattern);
    m_translucentLineIndicesBuffer.update_indices_buffer(flatten_translucent_indices(m_translucentLineSegments), accessPattern);
    m_transparencyInfo.isTranslucent = !m_translucentLineSegments.empty();
  }

  [[nodiscard]] static std::vector<std::uint32_t> flatten_translucent_indices(std::span<const SortableLineSegment> segments)
  {
    std::vector<std::uint32_t> flattened;
    flattened.reserve(segments.size() * 2U);
    for (const SortableLineSegment& segment: segments)
    {
      flattened.push_back(segment.firstIndex);
      flattened.push_back(segment.secondIndex);
    }
    return flattened;
  }
};

OPENGL_EXPORT std::optional<LineDrawable> make_line_drawable(LineProgram& program,
                                                             std::span<const float> lineVertices,
                                                             std::int32_t lineVertexDimension,
                                                             std::span<const std::uint32_t> lineIndices,
                                                             std::span<const float> lineColors,
                                                             std::int32_t lineColorDimension,
                                                             LineType lineType,
                                                             float lineThickness,
                                                             float pointThickness,
                                                             opengl::BufferAccessPattern accessPattern);

} // namespace opengl

ENABLE_ALL_WARNINGS

#endif // OPENGL_LINESOUP_HPP
