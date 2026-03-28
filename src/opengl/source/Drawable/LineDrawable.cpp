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
  auto lineIndicesBuffer = IndexBuffer::create(lineIndices.data(), static_cast<GLsizei>(lineIndices.size()), accessPattern);
  if (!lineIndicesBuffer.has_value())
  {
    return std::nullopt;
  }
  return LineDrawable{program,
                      std::move(vertexArray),
                      std::move(vertexBuffer.value()),
                      std::move(colorBuffer.value()),
                      std::move(lineIndicesBuffer.value()),
                      lineThickness,
                      pointThickness,
                      lineType};
}

} // namespace opengl