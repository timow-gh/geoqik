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
// Test: update_mesh replaces vertices and recomputes flat normals
// =============================================================================

TEST_F(MeshBufferTest, UpdateMesh_ReplacesVerticesAndRecomputesNormals)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);

  // Add a flat XY-plane triangle.
  std::vector<float> v1 = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<uint32_t> idx = {0, 1, 2};
  core::UUID handle = core::UUID::generate();
  buffer->add_mesh(v1, {}, {}, idx, &handle);

  // Update to a different triangle (rotated 90 degrees around X).
  std::vector<float> v2 = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f};
  bool ok = buffer->update_mesh(handle, v2, {}, {});
  EXPECT_TRUE(ok);

  auto verts = to_vector(buffer->get_vertices());
  EXPECT_FLOAT_EQ(verts[6], 0.0f); // updated z of vertex 2
  EXPECT_FLOAT_EQ(verts[8], 1.0f);
}

TEST_F(MeshBufferTest, UpdateMesh_RefreshesOverlayPositions)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);

  std::vector<float> v1 = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<std::uint32_t> idx = {0, 1, 2};
  core::UUID handle = core::UUID::generate();
  buffer->add_mesh(v1, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showSegments = true;
  overlayData.showVertices = true;
  overlayData.segmentPositions = v1;
  overlayData.segmentIndices = geoqik::derive_segment_indices_from_triangles(buffer->get_local_triangle_indices(handle));
  buffer->set_mesh_overlay_data(handle, overlayData);

  std::vector<float> v2 = {0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,  0.0f, 2.0f, 1.0f};
  ASSERT_TRUE(buffer->update_mesh(handle, v2, {}, {}));

  const auto& updatedOverlay = buffer->get_mesh_overlay_data(handle);
  EXPECT_EQ(updatedOverlay.segmentPositions, v2);
  EXPECT_EQ(updatedOverlay.segmentIndices, (std::vector<std::uint32_t>{0, 1, 1, 2, 2, 0}));
}

TEST_F(MeshBufferTest, UpdateMesh_BeforeFirstSyncKeepsMeshOnlyAdded)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);

  std::vector<float> v1 = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<float> v2 = {0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,  0.0f, 2.0f, 0.0f};
  std::vector<std::uint32_t> idx = {0, 1, 2};
  core::UUID handle = core::UUID::generate();

  buffer->add_mesh(v1, {}, {}, idx, &handle);
  ASSERT_TRUE(buffer->update_mesh(handle, v2, {}, {}));

  EXPECT_EQ(buffer->get_added_meshes().count(handle), 1u);
  EXPECT_EQ(buffer->get_updated_meshes().count(handle), 0u);
}

// =============================================================================
// Test: update_mesh rejects wrong vertex count
// =============================================================================

TEST_F(MeshBufferTest, UpdateMesh_WrongVertexCountReturnsFalse)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);

  std::vector<float> v = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<uint32_t> idx = {0, 1, 2};
  core::UUID handle = core::UUID::generate();
  buffer->add_mesh(v, {}, {}, idx, &handle);

  std::vector<float> wrong = {0.0f, 0.0f, 0.0f}; // only 1 vertex, need 3
  bool ok = buffer->update_mesh(handle, wrong, {}, {});
  EXPECT_FALSE(ok);
}

// =============================================================================
// Test: update_mesh rejects unknown handle
// =============================================================================

TEST_F(MeshBufferTest, UpdateMesh_UnknownHandleReturnsFalse)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID unknown = core::UUID::generate();
  std::vector<float> v = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  bool ok = buffer->update_mesh(unknown, v, {}, {});
  EXPECT_FALSE(ok);
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

// =============================================================================
// Test: create_snapshot / restore_snapshot preserves all data
// =============================================================================

TEST_F(MeshBufferTest, SnapshotRestore_PreservesAllData)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  buffer->set_default_color(0.5f, 0.5f, 0.5f, 1.0f);

  std::vector<float> v  = {0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f};
  std::vector<uint32_t> idx = {0, 1, 2};
  core::UUID handle = core::UUID::generate();
  buffer->add_mesh(v, {}, {}, idx, &handle);

  auto snap = buffer->create_snapshot();

  // Mutate
  buffer->clear();
  EXPECT_TRUE(buffer->empty());

  // Restore
  buffer->restore_snapshot(snap);
  EXPECT_FALSE(buffer->empty());
  EXPECT_EQ(to_vector(buffer->get_vertices()), v);
}

TEST_F(MeshBufferTest, DirtyTracking_AddedSetPopulated)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();

  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  EXPECT_EQ(buffer->get_added_meshes().count(handle), 1u);
  EXPECT_EQ(buffer->get_updated_meshes().size(), 0u);
  EXPECT_EQ(buffer->get_removed_meshes().size(), 0u);
  EXPECT_FALSE(buffer->is_full_rebuild_needed());

  buffer->clear_change_tracking();
  EXPECT_EQ(buffer->get_added_meshes().size(), 0u);
}

TEST_F(MeshBufferTest, DirtyTracking_RemovedSetPopulated)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);
  buffer->clear_change_tracking();

  buffer->remove_mesh(handle);
  EXPECT_EQ(buffer->get_removed_meshes().count(handle), 1u);
  EXPECT_EQ(buffer->get_added_meshes().size(), 0u);
}

TEST_F(MeshBufferTest, DirtyTracking_FullRebuildOnSnapshotRestore)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  auto snap = buffer->create_snapshot();
  buffer->clear_change_tracking();

  buffer->restore_snapshot(snap);
  EXPECT_TRUE(buffer->is_full_rebuild_needed());
}

TEST_F(MeshBufferTest, PerMeshAccessors_ReturnCorrectSubspan)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID h1 = core::UUID::generate();
  core::UUID h2 = core::UUID::generate();

  std::vector<float> v1 = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<float> v2 = {2.f,0.f,0.f, 3.f,0.f,0.f, 2.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};

  buffer->add_mesh(v1, {}, {}, idx, &h1);
  buffer->add_mesh(v2, {}, {}, idx, &h2);

  auto verts1 = buffer->get_mesh_vertices(h1);
  EXPECT_EQ(verts1.size(), 9u);
  EXPECT_FLOAT_EQ(verts1[0], 0.f);

  auto verts2 = buffer->get_mesh_vertices(h2);
  EXPECT_EQ(verts2.size(), 9u);
  EXPECT_FLOAT_EQ(verts2[0], 2.f);

  auto local1 = buffer->get_local_triangle_indices(h1);
  EXPECT_EQ(local1, (std::vector<std::uint32_t>{0,1,2}));

  auto local2 = buffer->get_local_triangle_indices(h2);
  EXPECT_EQ(local2, (std::vector<std::uint32_t>{0,1,2}));
}

// =============================================================================
// Plan 005 — Segment overlay tests
// =============================================================================

TEST_F(MeshBufferTest, SegmentOverlay_AutoDerived_CorrectEdgeCount)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  auto localTris = buffer->get_local_triangle_indices(handle);
  auto segs = geoqik::derive_segment_indices_from_triangles(localTris);

  // 1 triangle = 3 edges = 6 indices
  EXPECT_EQ(segs.size(), 6u);
  EXPECT_EQ(segs[0], 0u); EXPECT_EQ(segs[1], 1u);
  EXPECT_EQ(segs[2], 1u); EXPECT_EQ(segs[3], 2u);
  EXPECT_EQ(segs[4], 2u); EXPECT_EQ(segs[5], 0u);
}

TEST_F(MeshBufferTest, SegmentOverlay_OverlayDataStoredAndRetrieved)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showSegments = true;
  overlayData.segmentLineWidth = 2.0f;
  overlayData.segmentPositions = std::vector<float>(v.begin(), v.end());
  overlayData.segmentIndices = {0,1,1,2,2,0};
  buffer->set_mesh_overlay_data(handle, overlayData);

  EXPECT_TRUE(buffer->has_mesh_overlay_data(handle));
  EXPECT_TRUE(buffer->get_mesh_overlay_data(handle).showSegments);
  EXPECT_FLOAT_EQ(buffer->get_mesh_overlay_data(handle).segmentLineWidth, 2.0f);
}

TEST_F(MeshBufferTest, SegmentOverlay_SetBeforeFirstSyncKeepsMeshOnlyAdded)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showSegments = true;
  overlayData.segmentPositions = v;
  buffer->set_mesh_overlay_data(handle, overlayData);

  EXPECT_EQ(buffer->get_added_meshes().count(handle), 1u);
  EXPECT_EQ(buffer->get_updated_meshes().count(handle), 0u);
}

TEST_F(MeshBufferTest, SegmentOverlay_VisibilityToggle_NoBuildNeeded)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showSegments = true;
  buffer->set_mesh_overlay_data(handle, overlayData);
  buffer->clear_change_tracking();

  // Visibility toggle must NOT add to updatedMeshes (no drawable rebuild needed).
  buffer->set_mesh_segments_visible(handle, false);
  EXPECT_EQ(buffer->get_updated_meshes().size(), 0u);
  EXPECT_FALSE(buffer->get_mesh_overlay_data(handle).showSegments);
}

TEST_F(MeshBufferTest, SegmentOverlay_RemovedWithMesh)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);
  buffer->set_mesh_overlay_data(handle, geoqik::PerMeshOverlayData{});

  buffer->remove_mesh(handle);
  EXPECT_FALSE(buffer->has_mesh_overlay_data(handle));
}

// =============================================================================
// Plan 006 — Vertex overlay tests
// =============================================================================

TEST_F(MeshBufferTest, VertexOverlay_StoredAndRetrieved)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showVertices     = true;
  overlayData.vertexPointSize  = 5.0f;
  overlayData.vertexColor      = renderer::Color{1.0f, 0.0f, 0.0f, 1.0f};
  overlayData.segmentPositions = v;
  buffer->set_mesh_overlay_data(handle, overlayData);

  ASSERT_TRUE(buffer->has_mesh_overlay_data(handle));
  EXPECT_TRUE(buffer->get_mesh_overlay_data(handle).showVertices);
  EXPECT_FLOAT_EQ(buffer->get_mesh_overlay_data(handle).vertexPointSize, 5.0f);
}

TEST_F(MeshBufferTest, VertexOverlay_VisibilityToggle_NoBuildNeeded)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showVertices = true;
  buffer->set_mesh_overlay_data(handle, overlayData);
  buffer->clear_change_tracking();

  buffer->set_mesh_vertices_visible(handle, false);
  EXPECT_EQ(buffer->get_updated_meshes().size(), 0u);
  EXPECT_FALSE(buffer->get_mesh_overlay_data(handle).showVertices);
}

TEST_F(MeshBufferTest, VertexOverlay_SnapshotPreservesVertexData)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshOverlayData overlayData;
  overlayData.showVertices    = true;
  overlayData.vertexPointSize = 7.0f;
  buffer->set_mesh_overlay_data(handle, overlayData);

  auto snap = buffer->create_snapshot();

  buffer->clear();
  EXPECT_FALSE(buffer->has_mesh_overlay_data(handle));

  buffer->restore_snapshot(snap);
  ASSERT_TRUE(buffer->has_mesh_overlay_data(handle));
  EXPECT_FLOAT_EQ(buffer->get_mesh_overlay_data(handle).vertexPointSize, 7.0f);
}

// =============================================================================
// Plan 007 — Rendering opts tests
// =============================================================================

TEST_F(MeshBufferTest, RenderingOpts_DefaultIsBackCullAndVisible)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  EXPECT_FALSE(buffer->has_mesh_rendering_opts(handle));

  // Setting opts triggers updated tracking.
  geoqik::PerMeshRenderingOpts opts;
  opts.cullMode       = geoqik::MeshCullMode::none;
  opts.surfaceVisible = true;
  buffer->set_mesh_rendering_opts(handle, opts);

  ASSERT_TRUE(buffer->has_mesh_rendering_opts(handle));
  EXPECT_EQ(buffer->get_mesh_rendering_opts(handle).cullMode, geoqik::MeshCullMode::none);
  EXPECT_EQ(buffer->get_updated_meshes().count(handle), 1u);
}

TEST_F(MeshBufferTest, RenderingOpts_SnapshotRoundtrip)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  geoqik::PerMeshRenderingOpts opts{geoqik::MeshCullMode::front, false};
  buffer->set_mesh_rendering_opts(handle, opts);

  auto snap = buffer->create_snapshot();
  buffer->clear();
  EXPECT_FALSE(buffer->has_mesh_rendering_opts(handle));

  buffer->restore_snapshot(snap);
  ASSERT_TRUE(buffer->has_mesh_rendering_opts(handle));
  EXPECT_EQ(buffer->get_mesh_rendering_opts(handle).cullMode, geoqik::MeshCullMode::front);
  EXPECT_EQ(buffer->get_mesh_rendering_opts(handle).surfaceVisible, false);
}

TEST_F(MeshBufferTest, RenderingOpts_RemovedWithMesh)
{
  auto buffer = geoqik::MeshBuffer::create(m_settings);
  core::UUID handle = core::UUID::generate();
  std::vector<float> v = {0.f,0.f,0.f, 1.f,0.f,0.f, 0.f,1.f,0.f};
  std::vector<std::uint32_t> idx = {0,1,2};
  buffer->add_mesh(v, {}, {}, idx, &handle);

  buffer->set_mesh_rendering_opts(handle, geoqik::PerMeshRenderingOpts{});
  ASSERT_TRUE(buffer->has_mesh_rendering_opts(handle));

  buffer->remove_mesh(handle);
  EXPECT_FALSE(buffer->has_mesh_rendering_opts(handle));
}
