#include "sleep_helper.hpp"

#include <GeoQik/GeoQik.hpp>
#include <cstdint>
#include <vector>
#include <cassert>

// Demonstrates the full mesh-overlay styling surface:
//   * a chosen subset of edges drawn as a styled wireframe (dashed, round cap/join, thick),
//   * a non-zero depthLayer so the edges sit ON TOP of the surface and each other without
//     Z-fighting, and
//   * mesh vertices drawn as world-space sphere points with per-vertex radii.
int main() {
    geoqik_init();
    geoqik_draw();

    // Cube with 24 vertices (4 per face, no sharing across faces). Flat normals are computed
    // per-vertex, so sharing a corner across faces would overwrite its normal and shade the
    // cube unevenly. Duplicating corners per face gives each face one clean, constant normal.
    // Each 4-vertex block below is one face, wound CCW as seen from outside.
    std::vector<float> vertices = {
        // back face (-Z): corners 0,3,2,1
        -1.0f, -1.0f, -1.0f, // 0  (corner 0)
        -1.0f, 1.0f,  -1.0f, // 1  (corner 3)
        1.0f,  1.0f,  -1.0f, // 2  (corner 2)
        1.0f,  -1.0f, -1.0f, // 3  (corner 1)
        // front face (+Z): corners 4,5,6,7
        -1.0f, -1.0f, 1.0f, // 4  (corner 4)
        1.0f,  -1.0f, 1.0f, // 5  (corner 5)
        1.0f,  1.0f,  1.0f, // 6  (corner 6)
        -1.0f, 1.0f,  1.0f, // 7  (corner 7)
        // bottom face (-Y): corners 0,1,5,4
        -1.0f, -1.0f, -1.0f, // 8  (corner 0)
        1.0f,  -1.0f, -1.0f, // 9  (corner 1)
        1.0f,  -1.0f, 1.0f,  // 10 (corner 5)
        -1.0f, -1.0f, 1.0f,  // 11 (corner 4)
        // top face (+Y): corners 3,7,6,2
        -1.0f, 1.0f, -1.0f, // 12 (corner 3)
        -1.0f, 1.0f, 1.0f,  // 13 (corner 7)
        1.0f,  1.0f, 1.0f,  // 14 (corner 6)
        1.0f,  1.0f, -1.0f, // 15 (corner 2)
        // left face (-X): corners 0,4,7,3
        -1.0f, -1.0f, -1.0f, // 16 (corner 0)
        -1.0f, -1.0f, 1.0f,  // 17 (corner 4)
        -1.0f, 1.0f,  1.0f,  // 18 (corner 7)
        -1.0f, 1.0f,  -1.0f, // 19 (corner 3)
        // right face (+X): corners 1,2,6,5
        1.0f, -1.0f, -1.0f, // 20 (corner 1)
        1.0f, 1.0f,  -1.0f, // 21 (corner 2)
        1.0f, 1.0f,  1.0f,  // 22 (corner 6)
        1.0f, -1.0f, 1.0f,  // 23 (corner 5)
    };

    // Two triangles per face, referencing only that face's own 4-vertex block (CCW-outward).
    std::vector<uint32_t> indices = {
        0,  1,  2,  2,  3,  0,  // back
        4,  5,  6,  6,  7,  4,  // front
        8,  9,  10, 10, 11, 8,  // bottom
        12, 13, 14, 14, 15, 12, // top
        16, 17, 18, 18, 19, 16, // left
        20, 21, 22, 22, 23, 20, // right
    };

    // Draw only the 12 cube edges as segments (not the triangle diagonals) — this is the
    // "which lines are drawn" control: explicit segment index pairs. With the 24-vertex layout
    // each edge is expressed via a face block that contains both of its endpoints.
    std::vector<uint32_t> segmentIndices = {
        0, 1, 1, 2, 2, 3, 3, 0,   // back face loop (block 0)
        4, 5, 5, 6, 6, 7, 7, 4,   // front face loop (block 4)
        8, 11, 9, 10, 21, 22, 12, 13, // connecting edges (0-4,1-5 on bottom; 2-6 right; 3-7 top)
    };

    // Per-vertex sphere radii (world units). Only one representative vertex per geometric corner
    // gets a non-zero radius (the duplicates are 0.0f), so 8 spheres are drawn — corners alternate
    // large/small so the world-space sizing is obvious as you zoom. Blocks 0..7 are the eight
    // representatives, one per corner.
    std::vector<float> vertexRadii = {
        0.12f, 0.06f, 0.12f, 0.06f, 0.12f, 0.06f, 0.12f, 0.06f, // representatives (corners 0,3,2,1,4,5,6,7)
        0.0f,  0.0f,  0.0f,  0.0f,                              // bottom-face duplicates
        0.0f,  0.0f,  0.0f,  0.0f,                              // top-face duplicates
        0.0f,  0.0f,  0.0f,  0.0f,                              // left-face duplicates
        0.0f,  0.0f,  0.0f,  0.0f,                              // right-face duplicates
    };

    float meshColor[4] = {0.8f, 0.6f, 0.4f, 1.0f};
    float segmentColor[4] = {0.05f, 0.05f, 0.05f, 1.0f};
    float vertexColor[4] = {0.9f, 0.1f, 0.1f, 1.0f};

    // Dashed, thick, round-capped/round-joined edges lifted onto depthLayer 2 so they render
    // crisply on top of the shaded surface instead of Z-fighting it.
    float dashPattern[2] = {0.25f, 0.15f};

    geoqik_add_mesh_opts_t opts{};
    opts.color = meshColor;
    opts.colorCount = 4;

    opts.segmentIndices = segmentIndices.data();
    opts.segmentIndexCount = segmentIndices.size();
    opts.segmentColor = segmentColor;
    opts.showSegments = 1;
    opts.segmentStyleSet = 1;
    opts.segmentStyle.lineWidth = 4.0f;
    opts.segmentStyle.cap = GEOQIK_LINE_CAP_ROUND;
    opts.segmentStyle.join = GEOQIK_LINE_JOIN_ROUND;
    opts.segmentStyle.dashPattern = dashPattern;
    opts.segmentStyle.dashPatternCount = 2;
    opts.segmentStyle.dashSpace = GEOQIK_DASH_SPACE_WORLD;
    opts.segmentStyle.depthLayer = 2; // on top of the surface, no Z-fighting
    opts.segmentLineType = GEOQIK_LINE_TYPE_LINES;

    opts.vertexColor = vertexColor;
    opts.showVertices = 1;
    opts.vertexRadii = vertexRadii.data();
    opts.vertexRadiusCount = vertexRadii.size();
    opts.vertexSizeSpace = GEOQIK_SPHERE_SIZE_SPACE_WORLD;

    geoqik_result_t result =
        geoqik_add_mesh_opts(vertices.data(), vertices.size() / 3, indices.data(), indices.size() / 3, &opts);

    // After a moment, restyle the overlay at runtime WITHOUT resubmitting the mesh: switch the
    // edges to solid screen-space dashes on a higher depth layer and shrink the vertices.
    geoqik::examples::sleep_for_seconds(2.0);

    // Same representative-vertex scheme as vertexRadii: 8 visible spheres (blocks 0..7), zero
    // radius on the 16 duplicates.
    std::vector<float> smallerRadii(24, 0.0f);
    for (std::size_t i = 0; i < 8; ++i)
        smallerRadii[i] = 0.05f;
    geoqik_mesh_overlay_opts_t overlay{};
    overlay.showSegments = 1;
    overlay.showVertices = 1;
    overlay.segmentStyleSet = 1;
    overlay.segmentStyle.lineWidth = 2.5f;
    overlay.segmentStyle.cap = GEOQIK_LINE_CAP_BUTT;
    overlay.segmentStyle.join = GEOQIK_LINE_JOIN_MITER;
    overlay.segmentStyle.depthLayer = 3;
    overlay.segmentLineType = GEOQIK_LINE_TYPE_LINES;
    overlay.vertexStyleSet = 1;
    overlay.vertexRadii = smallerRadii.data();
    overlay.vertexRadiusCount = smallerRadii.size();
    overlay.vertexSizeSpace = GEOQIK_SPHERE_SIZE_SPACE_WORLD;
    geoqik_set_mesh_overlay_opts(&result.geometryId, &overlay);

    geoqik_mesh_rendering_opts_t renderOpts{};
    renderOpts.cullMode = GEOQIK_MESH_CULL_NONE;
    renderOpts.surfaceVisible = 1;
    assert(geoqik_set_mesh_rendering_opts(&result.geometryId, &renderOpts) == GEOQIK_SUCCESS);

    geoqik_wait_for_exit_and_cleanup();
    return 0;
}
