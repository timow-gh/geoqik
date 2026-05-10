#include "OpenGL/Drawable/PointDrawable.hpp"

namespace opengl
{

std::optional<PointDrawable> make_point_drawable(PointProgram& program,
                                                 std::span<const float> vertices,
                                                 std::int32_t vertexDimension,
                                                 std::span<const float> colors,
                                                 std::int32_t colorDimension,
                                                 std::span<const std::uint32_t> indices,
                                                 float pointSize,
                                                 BufferAccessPattern accessPattern)
{
  // clang-format off
  auto vertexArray = opengl::VertexArray::create();
  auto vertexBuffer = opengl::VertexBuffer::create(vertices, vertexDimension, program.get_pos_location(), accessPattern);
  if (!vertexBuffer.has_value())
  {
    CORE_ASSERT(false);
    return {};
  }
  auto colorBuffer = opengl::VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
  if (!colorBuffer.has_value())
  {
    CORE_ASSERT(false);
    return {};
  }
  auto vertexPositions = make_vertex_sort_positions(vertices, vertexDimension);
  auto vertexTranslucency = make_vertex_translucency_flags(colors, colorDimension);
  auto pointIndices = std::vector<std::uint32_t>(indices.begin(), indices.end());
  auto split = split_point_indices_by_transparency(pointIndices, vertexPositions, vertexTranslucency);
  std::vector<std::uint32_t> translucentIndices;
  translucentIndices.reserve(split.translucentIndices.size());
  for (const SortablePointIndex& pointIndex: split.translucentIndices)
  {
    translucentIndices.push_back(pointIndex.index);
  }
  auto opaqueIndexBuffer = opengl::IndexBuffer::create(split.opaqueIndices, accessPattern);
  if (!opaqueIndexBuffer.has_value())
  {
    CORE_ASSERT(false);
    return {};
  }
  auto translucentIndexBuffer = opengl::IndexBuffer::create(translucentIndices, accessPattern);
  if (!translucentIndexBuffer.has_value())
  {
    CORE_ASSERT(false);
    return {};
  }
  // clang-format on
  return PointDrawable{program,
                       std::move(vertexArray),
                       std::move(vertexBuffer.value()),
                       std::move(colorBuffer.value()),
                       std::move(opaqueIndexBuffer.value()),
                       std::move(translucentIndexBuffer.value()),
                       pointSize,
                       make_drawable_transparency_info(vertices, vertexDimension, colors, colorDimension),
                       vertexDimension,
                       colorDimension,
                       std::move(vertexPositions),
                       std::move(vertexTranslucency),
                       std::move(pointIndices),
                       std::move(split.translucentIndices)};
}

} // namespace opengl
