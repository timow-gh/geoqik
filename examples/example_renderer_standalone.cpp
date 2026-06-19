#include <Renderer/Renderer.hpp>
#include <Renderer/WindowSettings.hpp>
#include <OpenGL/LineType.hpp>
#include <array>
#include <cstdint>

namespace
{
constexpr std::uint32_t defaultWindowWidth  = 1024;
constexpr std::uint32_t defaultWindowHeight = 768;
constexpr float standalonePointSize = 12.0F;
constexpr float standaloneLineWidth = 2.0F;
} // namespace

int main()
{
  renderer::WindowSettings settings;
  settings.title  = "renderer standalone example";
  settings.width  = defaultWindowWidth;
  settings.height = defaultWindowHeight;

  auto rendererOpt = renderer::Renderer::create(settings);
  if (!rendererOpt)
  {
    return 1;
  }
  renderer::Renderer& r = *rendererOpt;

  // One point at the origin.
  const std::array<float, 3>        pointVertices{0.0F, 0.0F, 0.0F};
  const std::array<float, 4>        pointColors{1.0F, 1.0F, 0.0F, 1.0F}; // yellow
  const std::array<std::uint32_t,1> pointIndices{0};
  r.add_point_drawable(pointVertices, pointColors, pointIndices, standalonePointSize);

  // A cross made of two line segments through the origin.
  const std::array<float, 12> lineVertices{
      -1.0F, 0.0F, 0.0F,
       1.0F, 0.0F, 0.0F,
       0.0F,-1.0F, 0.0F,
       0.0F, 1.0F, 0.0F};
  const std::array<float, 16> lineColors{
      1.0F, 0.0F, 0.0F, 1.0F,
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F};
  const std::array<std::uint32_t, 4> lineIndices{0, 1, 2, 3};
  r.add_line_drawable(lineVertices, lineIndices, lineColors,
                      opengl::LineType::lines(), standaloneLineWidth);

  while (!r.should_close())
  {
    renderer::Renderer::poll_events();
    if (r.is_escape_pressed())
    {
      break;
    }
    r.begin_frame();
    r.draw();
    r.end_frame();
  }

  return 0;
}
