#include "Rendering/GeoQikSceneRenderer.hpp"

#include "Rendering/StyleTranslation.hpp"

#include <plinth/WindowSettings.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <tuple>
#include <vector>

namespace {

using OverlayParams = std::tuple<bool, bool, std::array<float, 4>>;

class GeoQikSceneRendererTest : public ::testing::TestWithParam<OverlayParams> {
  protected:
    static std::vector<std::uint8_t> render_pixels(renderer::Renderer& renderer) {
        renderer.begin_frame();
        renderer.draw();
        renderer.end_frame();
        std::vector<std::uint8_t> pixels;
        int width = 0;
        int height = 0;
        EXPECT_TRUE(renderer.read_scene_pixels(pixels, width, height));
        EXPECT_GT(width, 0);
        EXPECT_GT(height, 0);
        return pixels;
    }
};

TEST_P(GeoQikSceneRendererTest, SegmentOverlayMatchesUniformColorLinesOnCreationAndVisibilityToggle) {
    const auto& [styled, derivedIndices, color] = GetParam();
    renderer::WindowSettings windowSettings;
    windowSettings.visible = false;
    windowSettings.overlay = renderer::OverlayKind::None;
    windowSettings.width = 128;
    windowSettings.height = 128;
    windowSettings.samples = 1;
    auto renderer = renderer::Renderer::create(windowSettings);
    ASSERT_NE(renderer, nullptr);

    auto scene = geoqik::Scene::create(geoqik::GeoQikSettings{});
    geoqik::GeoQikSceneRenderer sceneRenderer(*renderer);
    const auto mesh = core::UUID::generate();
    const std::vector<float> vertices{-1.0F, -1.0F, 0.0F, 1.0F, -1.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    const std::vector<std::uint32_t> triangles{0, 1, 2};
    const std::array<float, 4> surfaceColor{0.2F, 0.6F, 0.3F, 1.0F};
    scene.add_mesh(vertices, {}, surfaceColor, triangles, &mesh);
    scene.set_mesh_rendering_opts(mesh, {geoqik::MeshCullMode::none, true});

    geoqik::PerMeshOverlayData overlay;
    overlay.segmentPositions = vertices;
    overlay.segmentIndices = derivedIndices ? geoqik::derive_segment_indices_from_triangles(triangles)
                                            : std::vector<std::uint32_t>{1, 2, 2, 0, 0, 1};
    if (color != std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}) {
        overlay.segmentColor = geoqik::Color{color[0], color[1], color[2], color[3]};
    }
    overlay.segmentLineWidth = 6.0F;
    overlay.segmentStyleSet = styled;
    overlay.segmentStyle.depthLayer = 1;
    overlay.showSegments = true;
    scene.get_mesh_buffer().set_mesh_overlay_data(mesh, overlay);
    ASSERT_TRUE(sceneRenderer.sync_scene(scene));
    const auto initialPixels = render_pixels(*renderer);

    scene.set_mesh_overlay_opts(mesh, false, false);
    ASSERT_TRUE(sceneRenderer.sync_scene(scene));
    const auto surfacePixels = render_pixels(*renderer);

    // Use plinth's uniform-color overload as an independent reference, with the
    // same colored mesh surface. Include an edge whose endpoints both exceed 0.
    std::vector<float> lineVertices;
    for (const auto index: overlay.segmentIndices) {
        const auto begin = vertices.begin() + static_cast<std::ptrdiff_t>(index * 3);
        lineVertices.insert(lineVertices.end(), begin, begin + 3);
    }
    const auto style = styled ? geoqik::build_stroke_style(overlay.segmentStyle, overlay.segmentLineWidth)
                              : renderer::StrokeStyle{overlay.segmentLineWidth};
    const auto reference = renderer->add_line_drawable(lineVertices, color, renderer::LineType::lines(), style);
    ASSERT_TRUE(reference.is_valid());
    const auto referencePixels = render_pixels(*renderer);
    ASSERT_FALSE(referencePixels.empty());
    ASSERT_TRUE(referencePixels != surfacePixels) << "Reference lines must be visible";
    EXPECT_TRUE(initialPixels == referencePixels) << "Initial overlay must match uniform-color lines";

    ASSERT_TRUE(renderer->remove_drawable(reference));
    scene.set_mesh_overlay_opts(mesh, true, false);
    ASSERT_TRUE(sceneRenderer.sync_scene(scene));
    EXPECT_TRUE(render_pixels(*renderer) == referencePixels) << "Runtime overlay must match uniform-color lines";
}

INSTANTIATE_TEST_SUITE_P(SegmentColors,
                         GeoQikSceneRendererTest,
                         ::testing::Combine(::testing::Bool(),
                                            ::testing::Bool(),
                                            ::testing::Values(std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F},
                                                              std::array<float, 4>{1.0F, 0.0F, 0.0F, 1.0F},
                                                              std::array<float, 4>{0.2F, 0.4F, 0.8F, 0.5F})));

} // namespace
