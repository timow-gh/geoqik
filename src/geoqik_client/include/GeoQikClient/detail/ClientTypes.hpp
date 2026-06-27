#ifndef GEOQIK_CLIENT_DETAIL_CLIENT_TYPES_HPP
#define GEOQIK_CLIENT_DETAIL_CLIENT_TYPES_HPP

#include <cstddef>
#include <cstdint>

// Mirror of geoqik_settings_t from GeoQik.hpp — must be kept in sync.
// Defined here so that ProcessManager.hpp can use these types without
// depending on GeoQikClient.hpp
struct geoqik_settings_t { // NOLINT(readability-identifier-naming)
    std::int64_t maxMessageQueueSize{};
    std::size_t initialPointCapacity{};
    std::size_t initialLineCapacity{};
    std::size_t initialMeshCapacity{};
    float defaultMeshColor[4]{};
    float meshHeadLightColor[3]{};
    float meshHeadLightIntensity{};
    float meshFillLightDirection[3]{};
    float meshFillLightColor[3]{};
    float meshFillLightIntensity{};
    float meshAmbientColor[3]{};
    float meshAmbientIntensity{};
    float meshShininess{};
    std::size_t capacityGrowthFactor{};
    float defaultPointSize{};
    float defaultLineWidth{};
    float defaultPointColor[4]{};
    float defaultLineColor[4]{};
    float backgroundColor[4]{};
    double cameraFarPlaneMultiplier{};
    int autoFitCameraEnabled{};
    int autoFitZoomInEnabled{};
    double autoFitZoomOutPadding{};
    double autoFitMinViewportOccupancy{};
    double autoFitTargetViewportOccupancy{};
    std::int64_t autoFitSuppressAfterUserCameraInteractionMs{};
    std::int64_t minGeometryProcessingTimeMs{};
    std::int64_t maxFrameProcessingTimeMs{};
    std::size_t updateSceneFrequency{};
};

struct geoqik_window_settings_t { // NOLINT(readability-identifier-naming)
    const char* title{};
    std::uint32_t width{};
    std::uint32_t height{};
    int red_bits{};
    int green_bits{};
    int blue_bits{};
    int alpha_bits{};
    int depth_bits{};
    int stencil_bits{};
    int accum_red_bits{};
    int accum_green_bits{};
    int accum_blue_bits{};
    int accum_alpha_bits{};
    int aux_buffers{};
    int samples{};
    int refresh_rate{};
    int stereo{};
    int srgb_capable{};
    int double_buffer{};
    int resizable{};
    int visible{};
    int decorated{};
    int focused{};
    int auto_iconify{};
    int floating{};
    int maximized{};
    int center_cursor{};
    int transparent_framebuffer{};
    int focus_on_show{};
    int scale_to_monitor{};
};

#endif // GEOQIK_CLIENT_DETAIL_CLIENT_TYPES_HPP
