#ifndef OPENGL_POINTSOUP_HPP
#define OPENGL_POINTSOUP_HPP

#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <utility>
#include <vector>

DISABLE_ALL_WARNINGS

namespace opengl
{

class OPENGL_EXPORT PointDrawable
{
  PointProgram* m_program{nullptr};
  opengl::VertexArray m_vertexArray;
  opengl::VertexBuffer m_vertexBuffer;
  opengl::VertexBuffer m_colorBuffer;
  opengl::IndexBuffer m_opaquePointIndicesBuffer;
  opengl::IndexBuffer m_translucentPointIndicesBuffer;
  float m_pointSize{1.0F};
  std::int32_t m_vertexDimension{0};
  std::int32_t m_colorDimension{0};
  std::vector<linal::float3> m_vertexPositions;
  std::vector<std::uint8_t> m_vertexTranslucency;
  std::vector<std::uint32_t> m_pointIndices;
  std::vector<SortablePointIndex> m_translucentPointIndices;
  DrawableTransparencyInfo m_transparencyInfo;

public:
  PointDrawable(PointProgram& program,
                VertexArray vertexArray,
                VertexBuffer vertexBuffer,
                VertexBuffer colorBuffer,
                IndexBuffer opaquePointIndicesBuffer,
                IndexBuffer translucentPointIndicesBuffer,
                float pointSize = 1.0F,
                DrawableTransparencyInfo transparencyInfo = {},
                std::int32_t vertexDimension = 3,
                std::int32_t colorDimension = 4,
                std::vector<linal::float3> vertexPositions = {},
                std::vector<std::uint8_t> vertexTranslucency = {},
                std::vector<std::uint32_t> pointIndices = {},
                std::vector<SortablePointIndex> translucentPointIndices = {}) noexcept
      : m_program(&program)
      , m_vertexArray(std::move(vertexArray))
      , m_vertexBuffer(std::move(vertexBuffer))
      , m_colorBuffer(std::move(colorBuffer))
      , m_opaquePointIndicesBuffer(std::move(opaquePointIndicesBuffer))
      , m_translucentPointIndicesBuffer(std::move(translucentPointIndicesBuffer))
      , m_pointSize(pointSize)
      , m_vertexDimension(vertexDimension)
      , m_colorDimension(colorDimension)
      , m_vertexPositions(std::move(vertexPositions))
      , m_vertexTranslucency(std::move(vertexTranslucency))
      , m_pointIndices(std::move(pointIndices))
      , m_translucentPointIndices(std::move(translucentPointIndices))
      , m_transparencyInfo(transparencyInfo)
  {
  }

  constexpr PointDrawable(const PointDrawable& other) = delete;
  constexpr PointDrawable& operator=(const PointDrawable& other) = delete;
  PointDrawable(PointDrawable&& other) noexcept
      : m_program(std::exchange(other.m_program, nullptr))
      , m_vertexArray(std::move(other.m_vertexArray))
      , m_vertexBuffer(std::move(other.m_vertexBuffer))
      , m_colorBuffer(std::move(other.m_colorBuffer))
      , m_opaquePointIndicesBuffer(std::move(other.m_opaquePointIndicesBuffer))
      , m_translucentPointIndicesBuffer(std::move(other.m_translucentPointIndicesBuffer))
      , m_pointSize(other.m_pointSize)
      , m_vertexDimension(other.m_vertexDimension)
      , m_colorDimension(other.m_colorDimension)
      , m_vertexPositions(std::move(other.m_vertexPositions))
      , m_vertexTranslucency(std::move(other.m_vertexTranslucency))
      , m_pointIndices(std::move(other.m_pointIndices))
      , m_translucentPointIndices(std::move(other.m_translucentPointIndices))
      , m_transparencyInfo(other.m_transparencyInfo)
  {
  }
  PointDrawable& operator=(PointDrawable&& other) noexcept
  {
    if (this != &other)
    {
      m_program = std::exchange(other.m_program, nullptr);
      m_vertexArray = std::move(other.m_vertexArray);
      m_vertexBuffer = std::move(other.m_vertexBuffer);
      m_colorBuffer = std::move(other.m_colorBuffer);
      m_opaquePointIndicesBuffer = std::move(other.m_opaquePointIndicesBuffer);
      m_translucentPointIndicesBuffer = std::move(other.m_translucentPointIndicesBuffer);
      m_pointSize = other.m_pointSize;
      m_vertexDimension = other.m_vertexDimension;
      m_colorDimension = other.m_colorDimension;
      m_vertexPositions = std::move(other.m_vertexPositions);
      m_vertexTranslucency = std::move(other.m_vertexTranslucency);
      m_pointIndices = std::move(other.m_pointIndices);
      m_translucentPointIndices = std::move(other.m_translucentPointIndices);
      m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
  }
  ~PointDrawable() = default;

  constexpr void set_point_size(float pointSize) noexcept { m_pointSize = pointSize; }

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
    m_pointIndices.assign(indices.begin(), indices.end());
    rebuild_index_buffers(accessPattern);
  }

  void update_point_drawable(std::span<const float> vertices,
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
    draw_index_buffer(mvp, m_translucentPointIndicesBuffer);
  }

  void draw_opaque(const linal::hmatf& mvp) const
  {
    draw_index_buffer(mvp, m_opaquePointIndicesBuffer);
  }

  void draw_translucent(const linal::hmatf& mvp, const linal::double3& viewPosition)
  {
    const std::vector<std::uint32_t> sortedIndices = sort_translucent_point_indices_back_to_front(m_translucentPointIndices, viewPosition);
    m_translucentPointIndicesBuffer.update_indices_buffer(sortedIndices, BufferAccessPattern::STREAM_DRAW);
    draw_index_buffer(mvp, m_translucentPointIndicesBuffer);
  }

  [[nodiscard]] bool has_opaque_primitives() const noexcept { return m_opaquePointIndicesBuffer.get_index_count() > 0; }

  [[nodiscard]] bool has_translucent_primitives() const noexcept { return m_translucentPointIndicesBuffer.get_index_count() > 0; }

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
    glPointSize(m_pointSize);
    m_vertexArray.bind();
    indexBuffer.bind();

    glDrawElements(GL_POINTS, indexBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
  }

  void rebuild_index_buffers(BufferAccessPattern accessPattern)
  {
    PointTransparencyIndexSplit split = split_point_indices_by_transparency(m_pointIndices, m_vertexPositions, m_vertexTranslucency);
    m_translucentPointIndices = std::move(split.translucentIndices);
    m_opaquePointIndicesBuffer.update_indices_buffer(split.opaqueIndices, accessPattern);
    m_translucentPointIndicesBuffer.update_indices_buffer(flatten_translucent_indices(m_translucentPointIndices), accessPattern);
    m_transparencyInfo.isTranslucent = !m_translucentPointIndices.empty();
  }

  [[nodiscard]] static std::vector<std::uint32_t> flatten_translucent_indices(std::span<const SortablePointIndex> indices)
  {
    std::vector<std::uint32_t> flattened;
    flattened.reserve(indices.size());
    for (const SortablePointIndex& index: indices)
    {
      flattened.push_back(index.index);
    }
    return flattened;
  }
};

OPENGL_EXPORT std::optional<PointDrawable> make_point_drawable(PointProgram& program,
                                                               std::span<const float> vertices,
                                                               std::int32_t vertexDimension,
                                                               std::span<const float> colors,
                                                               std::int32_t colorDimension,
                                                               std::span<const std::uint32_t> indices,
                                                               float pointSize,
                                                               BufferAccessPattern accessPattern);

} // namespace opengl

ENABLE_ALL_WARNINGS

#endif // OPENGL_POINTSOUP_HPP
