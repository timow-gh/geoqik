#include "OpenGL/Drawable/LineDrawable.hpp"

namespace opengl
{

std::optional<LineDrawable> make_line_drawable(LineProgram& program,
                                               std::span<const float> lineVertices,
                                               std::int32_t lineVertexDimension,
                                               std::span<const std::uint32_t> lineIndices,
                                               std::span<const float> lineColors,
                                               std::int32_t lineColorDimension,
                                               LineType lineType,
                                               float lineThickness,
                                               float pointThickness,
                                               BufferAccessPattern accessPattern)
{
  VertexArray vertexArray = VertexArray::create();
  auto vertexBuffer = VertexBuffer::create(lineVertices, lineVertexDimension, program.get_vertex_location(), accessPattern);
  if (!vertexBuffer.has_value())
  {
    return std::nullopt;
  }
  auto colorBuffer = VertexBuffer::create(lineColors, lineColorDimension, program.get_color_location(), accessPattern);
  if (!colorBuffer.has_value())
  {
    return std::nullopt;
  }
  auto vertexPositions = make_vertex_sort_positions(lineVertices, lineVertexDimension);
  auto vertexTranslucency = make_vertex_translucency_flags(lineColors, lineColorDimension);
  auto lineIndexList = std::vector<std::uint32_t>(lineIndices.begin(), lineIndices.end());
  auto split = split_line_indices_by_transparency(lineIndexList, vertexPositions, vertexTranslucency);
  std::vector<std::uint32_t> translucentIndices;
  translucentIndices.reserve(split.translucentSegments.size() * 2U);
  for (const SortableLineSegment& segment: split.translucentSegments)
  {
    translucentIndices.push_back(segment.firstIndex);
    translucentIndices.push_back(segment.secondIndex);
  }
  auto opaqueLineIndicesBuffer = IndexBuffer::create(split.opaqueIndices, accessPattern);
  if (!opaqueLineIndicesBuffer.has_value())
  {
    return std::nullopt;
  }
  auto translucentLineIndicesBuffer = IndexBuffer::create(translucentIndices, accessPattern);
  if (!translucentLineIndicesBuffer.has_value())
  {
    return std::nullopt;
  }
  return LineDrawable{program,
                      std::move(vertexArray),
                      std::move(vertexBuffer.value()),
                      std::move(colorBuffer.value()),
                      std::move(opaqueLineIndicesBuffer.value()),
                      std::move(translucentLineIndicesBuffer.value()),
                      lineThickness,
                      pointThickness,
                      lineType,
                      make_drawable_transparency_info(lineVertices, lineVertexDimension, lineColors, lineColorDimension),
                      lineVertexDimension,
                      lineColorDimension,
                      std::move(vertexPositions),
                      std::move(vertexTranslucency),
                      std::move(lineIndexList),
                      std::move(split.translucentSegments)};
}

} // namespace opengl
