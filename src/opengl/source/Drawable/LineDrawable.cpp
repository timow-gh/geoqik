#include "OpenGL/Drawable/LineDrawable.hpp"

namespace opengl
{

LineDrawable make_line_drawable(LineProgram& program,
                        std::span<const float> lineVertices,
                        std::int32_t lineVertexDimension,
                        std::span<const std::uint32_t> lineIndices,
                        std::span<const float> lineColors,
                        std::int32_t lineColorDimension,
                        LineType lineType,
                        float lineThickness,
                        float pointThickness,
                        BufferAccessPattern accessPattern) {
  VertexArray vertexArray = VertexArray::create();
  VertexBuffer vertexBuffer = VertexBuffer::create(lineVertices,
                                                   lineVertexDimension,
                                                   program.get_vertex_location(),
                                                   accessPattern);
  VertexBuffer colorBuffer = VertexBuffer::create(lineColors,
                                                  lineColorDimension,
                                                  program.get_color_location(),
                                                  accessPattern);
  IndexBuffer lineIndicesBuffer = IndexBuffer::create(lineIndices.data(),
                                                      static_cast<GLsizei>(lineIndices.size()),
                                                      accessPattern);
  return {program,
          vertexArray,
          vertexBuffer,
          colorBuffer,
          lineIndicesBuffer,
          lineThickness,
          pointThickness,
          lineType};
}

} // namespace opengl