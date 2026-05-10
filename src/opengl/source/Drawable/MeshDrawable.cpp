#include "OpenGL/Drawable/MeshDrawable.hpp"

namespace opengl
{

std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                            std::span<const float> vertices,
                            std::int32_t vertexDimension,
                            std::span<const float> normals,
                            std::span<const float> colors,
                            std::int32_t colorDimension,
                            std::span<const std::uint32_t> triangleIndices,
                            BufferAccessPattern accessPattern)
{
  // Order of creation matters! Create the vertex array first, then the buffers.
  auto vertexArray = VertexArray::create();
  auto vertexBuffer = VertexBuffer::create(vertices, vertexDimension, program.get_pos_location(), accessPattern);
  if (!vertexBuffer.has_value())
  {
    CORE_ASSERT(false);
    return std::nullopt;
  }
  auto vertexNormalsBuffer = VertexBuffer::create(normals, vertexDimension, program.get_normal_location(), accessPattern);
  if (!vertexNormalsBuffer.has_value())
  {
    CORE_ASSERT(false);
    return std::nullopt;
  }
  auto colorBuffer = VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
  if (!colorBuffer.has_value())
  {
    CORE_ASSERT(false);
    return std::nullopt;
  }
  auto triangleIndicesBuffer = IndexBuffer::create(triangleIndices.data(), static_cast<GLsizei>(triangleIndices.size()), accessPattern);
  if (!triangleIndicesBuffer.has_value())
  {
    CORE_ASSERT(false);
    return std::nullopt;
  }
  return MeshDrawable{program,
                      std::move(vertexArray),
                      std::move(vertexBuffer.value()),
                      std::move(vertexNormalsBuffer.value()),
                      std::move(colorBuffer.value()),
                      std::move(triangleIndicesBuffer.value()),
                      make_drawable_transparency_info(vertices, vertexDimension, colors, colorDimension)};
}

} // namespace opengl
