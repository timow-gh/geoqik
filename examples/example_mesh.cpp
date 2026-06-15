#include <GeoQik/GeoQik.hpp>
#include <cstdint>
#include <vector>

int main()
{
  geoqik_init();
  geoqik_draw();

  std::vector<float> vertices = {
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
  };

  std::vector<uint32_t> indices = {
    0, 1, 2,  2, 3, 0,
    5, 4, 7,  7, 6, 5,
    4, 0, 3,  3, 7, 4,
    1, 5, 6,  6, 2, 1,
    3, 2, 6,  6, 7, 3,
    4, 5, 1,  1, 0, 4,
  };

  float color[4] = {0.8f, 0.6f, 0.4f, 0.85f};
  geoqik_add_mesh_opts_t opts{};
  opts.color      = color;
  opts.colorCount = 4;
  geoqik_add_mesh_opts(vertices.data(),
                        static_cast<size_t>(vertices.size() / 3),
                        indices.data(),
                        static_cast<size_t>(indices.size() / 3),
                        &opts);

  geoqik_wait_for_exit_and_cleanup();
  return 0;
}
