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
  auto indexBuffer =  opengl::IndexBuffer::create(indices.data(), static_cast<GLsizei>(indices.size()), accessPattern);
  if (!indexBuffer.has_value())
  {
    CORE_ASSERT(false);
    return {};
  }
  // clang-format on
  return PointDrawable{program,
                       std::move(vertexArray),
                       std::move(vertexBuffer.value()),
                       std::move(colorBuffer.value()),
                       std::move(indexBuffer.value()),
                       pointSize,
                       make_drawable_transparency_info(vertices, vertexDimension, colors, colorDimension)};
}

} // namespace opengl
