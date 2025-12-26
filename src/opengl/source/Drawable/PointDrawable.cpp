#include "OpenGL/Drawable/PointDrawable.hpp"

namespace opengl
{

PointDrawable make_point_drawable(PointProgram& program,
                          std::span<const float> vertices,
                          std::int32_t vertexDimension,
                          std::span<const float> colors,
                          std::int32_t colorDimension,
                          std::span<const std::uint32_t> indices,
                          float pointSize,
                          BufferAccessPattern accessPattern) {
  // clang-format off
  auto vertexArray = opengl::VertexArray::create();
  auto vertexBuffer = opengl::VertexBuffer::create(vertices, vertexDimension, program.get_pos_location(), accessPattern);
  auto colorBuffer = opengl::VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
  auto indexBuffer =  opengl::IndexBuffer::create(indices.data(), static_cast<GLsizei>(indices.size()), accessPattern);
  // clang-format on
  return PointDrawable{program, vertexArray, vertexBuffer, colorBuffer, indexBuffer, pointSize};
}

} // namespace opengl