#include "GeometryBuffers/MeshBuffer.hpp"
#include "Core/UUID.hpp"
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <span>
#include <vector>

template <class T>
std::vector<T> to_vector(std::span<const T> values)
{
  return {values.begin(), values.end()};
}

class MeshBufferTest : public ::testing::Test
{
  void SetUp() override
  {
    m_settings.initialMeshCapacity = 100;
  }

protected:
  geoqik::GeoQikSettings m_settings;
};

// =============================================================================
// Test 1: add_mesh with explicit normals → get_normals() returns exact input values
// =============================================================================

TEST_F(MeshBufferTest, AddMesh_ExplicitNormals)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals  = {0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f};
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);

  auto gotNormals = to_vector(buffer->get_normals());
  ASSERT_EQ(gotNormals.size(), normals.size());
  for (std::size_t i = 0; i < normals.size(); ++i)
  {
    EXPECT_FLOAT_EQ(gotNormals[i], normals[i]);
  }
}

// =============================================================================
// Test 2: add_mesh with no normals → computed flat normals, each has length ≈ 1
// =============================================================================

TEST_F(MeshBufferTest, AddMesh_ComputedFlatNormals)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  // Triangle in XY plane → normal should be (0,0,1)
  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals; // empty → compute
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);

  auto gotNormals = to_vector(buffer->get_normals());
  ASSERT_EQ(gotNormals.size(), vertices.size());

  for (std::size_t i = 0; i < gotNormals.size() / 3; ++i)
  {
    float nx = gotNormals[i*3 + 0];
    float ny = gotNormals[i*3 + 1];
    float nz = gotNormals[i*3 + 2];
    float len = std::sqrt(nx*nx + ny*ny + nz*nz);
    EXPECT_NEAR(len, 1.0f, 1e-5f) << "Normal at vertex " << i << " is not unit length";
  }
}

// =============================================================================
// Test 3: add_mesh with single color (4 floats) → all vertices have that color
// =============================================================================

TEST_F(MeshBufferTest, AddMesh_SingleColor_BroadcastedToAllVertices)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors = {0.5f, 0.6f, 0.7f, 0.8f}; // single color
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);

  auto gotColors = to_vector(buffer->get_colors());
  ASSERT_EQ(gotColors.size(), 3 * 4u); // 3 vertices * 4 channels
  for (std::size_t v = 0; v < 3; ++v)
  {
    EXPECT_FLOAT_EQ(gotColors[v*4 + 0], 0.5f);
    EXPECT_FLOAT_EQ(gotColors[v*4 + 1], 0.6f);
    EXPECT_FLOAT_EQ(gotColors[v*4 + 2], 0.7f);
    EXPECT_FLOAT_EQ(gotColors[v*4 + 3], 0.8f);
  }
}

// =============================================================================
// Test 4: add_mesh with per-vertex colors → each vertex has its own color
// =============================================================================

TEST_F(MeshBufferTest, AddMesh_PerVertexColors)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors = {
    1.0f, 0.0f, 0.0f, 1.0f,  // v0: red
    0.0f, 1.0f, 0.0f, 1.0f,  // v1: green
    0.0f, 0.0f, 1.0f, 1.0f,  // v2: blue
  };
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);

  auto gotColors = to_vector(buffer->get_colors());
  ASSERT_EQ(gotColors.size(), colors.size());
  for (std::size_t i = 0; i < colors.size(); ++i)
  {
    EXPECT_FLOAT_EQ(gotColors[i], colors[i]);
  }
}

// =============================================================================
// Test 5: remove_mesh by handle removes only the right mesh when multiple exist
// =============================================================================

TEST_F(MeshBufferTest, RemoveMesh_RemovesOnlyCorrectMesh)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> verts1 = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> verts2 = {2.0f, 0.0f, 0.0f,  3.0f, 0.0f, 0.0f,  2.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  core::UUID handle1 = core::UUID::generate();
  core::UUID handle2 = core::UUID::generate();

  buffer->add_mesh(verts1, normals, colors, indices, &handle1);
  buffer->add_mesh(verts2, normals, colors, indices, &handle2);

  EXPECT_EQ(buffer->get_vertices().size(), 18u); // 6 vertices * 3

  buffer->remove_mesh(handle1);
  EXPECT_TRUE(buffer->has_changed());

  auto remainingVerts = to_vector(buffer->get_vertices());
  ASSERT_EQ(remainingVerts.size(), 9u);
  // verts2 should remain
  EXPECT_FLOAT_EQ(remainingVerts[0], 2.0f);
  EXPECT_FLOAT_EQ(remainingVerts[1], 0.0f);
  EXPECT_FLOAT_EQ(remainingVerts[2], 0.0f);
}

// =============================================================================
// Test 6: translate_geometry → vertex positions shifted correctly
// =============================================================================

TEST_F(MeshBufferTest, TranslateGeometry_ShiftsVertices)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  core::UUID handle = core::UUID::generate();
  buffer->add_mesh(vertices, normals, colors, indices, &handle);
  buffer->reset_changed_flag();

  buffer->translate_geometry(handle, 1.0f, 2.0f, 3.0f);
  EXPECT_TRUE(buffer->has_changed());

  auto verts = to_vector(buffer->get_vertices());
  EXPECT_FLOAT_EQ(verts[0], 1.0f);
  EXPECT_FLOAT_EQ(verts[1], 2.0f);
  EXPECT_FLOAT_EQ(verts[2], 3.0f);
  EXPECT_FLOAT_EQ(verts[3], 2.0f);
  EXPECT_FLOAT_EQ(verts[4], 2.0f);
  EXPECT_FLOAT_EQ(verts[5], 3.0f);
  EXPECT_FLOAT_EQ(verts[6], 1.0f);
  EXPECT_FLOAT_EQ(verts[7], 3.0f);
  EXPECT_FLOAT_EQ(verts[8], 3.0f);
}

// =============================================================================
// Test 7: has_changed() / reset_changed_flag() lifecycle
// =============================================================================

TEST_F(MeshBufferTest, HasChangedLifecycle)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  EXPECT_FALSE(buffer->has_changed());

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);
  EXPECT_TRUE(buffer->has_changed());

  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->has_changed());
}

// =============================================================================
// Test 8: Wrong color size throws std::runtime_error
// =============================================================================

TEST_F(MeshBufferTest, WrongColorSizeThrows)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> badColors = {0.5f, 0.5f, 0.5f}; // 3 floats - invalid
  std::vector<uint32_t> indices = {0, 1, 2};

  EXPECT_THROW(buffer->add_mesh(vertices, normals, badColors, indices), std::runtime_error);
  EXPECT_TRUE(buffer->empty());
}

// =============================================================================
// Test 9: clear() → buffer empty, has_changed() true
// =============================================================================

TEST_F(MeshBufferTest, SetDefaultColor_AppliesToNewlyAddedMesh)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  buffer->set_default_color(1.0f, 0.0f, 0.0f, 1.0f); // red

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2};
  buffer->add_mesh(vertices, {}, {}, indices); // no explicit color

  auto colors = to_vector(buffer->get_colors());
  // 3 vertices x 4 channels = 12 floats, all r=1 g=0 b=0 a=1
  ASSERT_EQ(colors.size(), 12u);
  for (std::size_t i = 0; i < 3; ++i)
  {
    EXPECT_FLOAT_EQ(colors[i * 4 + 0], 1.0f);
    EXPECT_FLOAT_EQ(colors[i * 4 + 1], 0.0f);
    EXPECT_FLOAT_EQ(colors[i * 4 + 2], 0.0f);
    EXPECT_FLOAT_EQ(colors[i * 4 + 3], 1.0f);
  }
}

TEST_F(MeshBufferTest, Clear_EmptiesBufferAndSetsChanged)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  ASSERT_TRUE(buffer != nullptr);

  std::vector<float> vertices = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> normals;
  std::vector<float> colors;
  std::vector<uint32_t> indices = {0, 1, 2};

  buffer->add_mesh(vertices, normals, colors, indices);
  EXPECT_FALSE(buffer->empty());

  buffer->reset_changed_flag();
  EXPECT_FALSE(buffer->has_changed());

  buffer->clear();
  EXPECT_TRUE(buffer->empty());
  EXPECT_TRUE(buffer->has_changed());
  EXPECT_EQ(buffer->get_vertices().size(), 0u);
  EXPECT_EQ(buffer->get_normals().size(), 0u);
  EXPECT_EQ(buffer->get_colors().size(), 0u);
  EXPECT_EQ(buffer->get_triangle_indices().size(), 0u);
}
