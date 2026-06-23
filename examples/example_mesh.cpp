#include <GeoQik/GeoQik.hpp>
#include "sleep_helper.hpp"
#include <cstdint>
#include <vector>

int main()
{
  geoqik_init();
  geoqik_draw();

  // Cube vertices
  std::vector<float> vertices = {
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,
  };

  std::vector<uint32_t> indices = {
     0,  1,  2,   2,  3,  0,
     4,  5,  6,   6,  7,  4,
     8,  9, 10,  10, 11,  8,
    12, 13, 14,  14, 15, 12,
    16, 17, 18,  18, 19, 16,
    20, 21, 22,  22, 23, 20,
  };

  float color[4] = {0.8f, 0.6f, 0.4f, 1.0f};
  geoqik_add_mesh_opts_t opts{};
  opts.normals = NULL;
  opts.normalsCount = 0;
  opts.color = color;
  opts.colorCount = 4;
  opts.segmentIndices = NULL;
  opts.segmentIndexCount = 0;
  opts.segmentColor = NULL;
  opts.showSegments = 1;
  opts.segmentLineWidth = 1.0f;
  opts.vertexColor = NULL;
  opts.showVertices = 1;
  opts.vertexPointSize = 3.0f;
  geoqik_result_t result = geoqik_add_mesh_opts(vertices.data(),
                                                vertices.size() / 3,
                                                indices.data(),
                                                indices.size() / 3,
                                                &opts);

  geoqik::examples::sleep_for_seconds(1.0);

  // Double the height in the z axis
  std::vector<float> updatedVertices = vertices;
  for (std::size_t i = 2; i < updatedVertices.size(); i += 3)
    updatedVertices[i] *= 2.0f;

  // Orange color
  float orangeColor[4] = {1.0f, 0.647f, 0.0f, 1.0f};
  geoqik_update_mesh_opts_t updateOpts{};
  updateOpts.color = orangeColor;
  updateOpts.colorCount = 4;
  geoqik_update_mesh_opts(&result.geometryId,
                          updatedVertices.data(),
                          updatedVertices.size() / 3,
                          &updateOpts);

  geoqik_wait_for_exit_and_cleanup();
  return 0;
}
