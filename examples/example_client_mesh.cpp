#include <GeoQikClient/GeoQikClient.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numbers>

namespace {

constexpr float cubeVertices[] = {
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f
};
constexpr std::uint32_t cubeTriangles[] = {
    0,1,2, 0,2,3,
    4,6,5, 4,7,6,
    0,4,5, 0,5,1,
    2,6,7, 2,7,3,
    0,3,7, 0,7,4,
    1,5,6, 1,6,2
};
constexpr std::size_t cubeVertexCount   = 8;
constexpr std::size_t cubeTriangleCount = 12;

constexpr float red[]  = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr float blue[] = {0.0f, 0.0f, 1.0f, 1.0f};

} // namespace

int main() {
    auto err = geoqik_init();
    assert(err == GEOQIK_SUCCESS && "geoqik_init failed; is GEOQIK_EXE_PATH set?");
    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    auto r1 = geoqik_add_mesh_opts(
        cubeVertices, cubeVertexCount,
        cubeTriangles, cubeTriangleCount,
        nullptr);
    assert(r1.err == GEOQIK_SUCCESS);

    geoqik_add_mesh_opts_t meshOpts{};
    meshOpts.color      = red;
    meshOpts.colorCount = 4;
    meshOpts.showSegments     = 1;
    meshOpts.segmentLineWidth = 2.0f;

    auto r2 = geoqik_add_mesh_opts(
        cubeVertices, cubeVertexCount,
        cubeTriangles, cubeTriangleCount,
        &meshOpts);
    assert(r2.err == GEOQIK_SUCCESS);

    std::array<float, cubeVertexCount * 3> updVerts;
    for (std::size_t i = 0; i < cubeVertexCount * 3; ++i) {
        updVerts[i] = cubeVertices[i] + (i % 3 == 0 ? 2.0f : 0.0f);
    }
    geoqik_update_mesh_opts_t upOpts{};
    upOpts.color      = blue;
    upOpts.colorCount = 4;
    err = geoqik_update_mesh_opts(&r1.geometryId, updVerts.data(), cubeVertexCount, &upOpts);
    assert(err == GEOQIK_SUCCESS);

    geoqik_mesh_overlay_opts_t overlayOpts{};
    overlayOpts.showSegments = 1;
    overlayOpts.showVertices = 1;
    err = geoqik_set_mesh_overlay_opts(&r1.geometryId, &overlayOpts);
    assert(err == GEOQIK_SUCCESS);

    geoqik_mesh_rendering_opts_t renderOpts{};
    renderOpts.cullMode      = GEOQIK_MESH_CULL_NONE;
    renderOpts.surfaceVisible = 1;
    err = geoqik_set_mesh_rendering_opts(&r1.geometryId, &renderOpts);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_remove_mesh(&r2.geometryId);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_wait_for_exit_and_cleanup();
    assert(err == GEOQIK_SUCCESS);
    return 0;
}
