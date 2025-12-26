#include "OpenGL/Drawable/MeshDrawable.hpp"

namespace opengl
{

MeshDrawable make_mesh_soup(MeshProgram& program,
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
  auto vertexNormalsBuffer = VertexBuffer::create(normals, vertexDimension, program.get_normal_location(), accessPattern);
  auto colorBuffer = VertexBuffer::create(colors, colorDimension, program.get_color_location(), accessPattern);
  auto triangleIndicesBuffer = IndexBuffer::create(triangleIndices.data(), static_cast<GLsizei>(triangleIndices.size()), accessPattern);
  return MeshDrawable{program, vertexArray, vertexBuffer, vertexNormalsBuffer, colorBuffer, triangleIndicesBuffer};
}

} // namespace opengl