#ifndef GEOQIKSETTINGS_HPP
#define GEOQIKSETTINGS_HPP

#include <GeoQik/geoqik_export.h>
#include <Renderer/CameraProjectionType.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/InputState.hpp>
#include <Renderer/WindowSettings.hpp>
#include <chrono>
#include <cstdint>

namespace geoqik {

using renderer::CameraProjectionType;
using renderer::Color;
using renderer::ColorChannelCount;
using renderer::CursorPosState;
using renderer::InputState;
using renderer::WindowSettings;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251) // class 'std::xxx' needs to have dll-interface
#pragma warning(disable : 4275) // non dll-interface class 'std::xxx' used as base
#endif

struct GEOQIK_EXPORT GeoQikSettings {
    std::int64_t maxMessageQueueSize{
        1000000}; // Maximum size of the message queue. Each call to the GeoQik API will enqueue a message, so
                  // this should be large enough to handle the expected number of messages.

    std::size_t initialPointCapacity{
        100000}; // Initial number of points that can be stored in the point buffer. Exceeding this number will require
                 // to create a larger buffer and copy the existing data to it.
    std::size_t initialLineCapacity{
        100000}; // Initial number of lines that can be stored in the line buffer. Exceeding this number will require to
                 // create a larger buffer and copy the existing data to it.
    std::size_t initialMeshCapacity{10000}; // Initial number of mesh vertices that can be stored in the mesh buffer.
    Color defaultMeshColor{
        1.0f,
        1.0f,
        1.0f,
        1.0f}; // Default color for meshes. If no other color is set using the GeoQik API, this color will be used.
    std::array<float, 3> meshHeadLightColor{1.0f, 1.0f, 1.0f};
    float meshHeadLightIntensity{1.0f};
    std::array<float, 3> meshFillLightDirection{-0.45f, 0.60f, 0.35f};
    std::array<float, 3> meshFillLightColor{0.70f, 0.78f, 1.0f};
    float meshFillLightIntensity{0.25f};
    std::array<float, 3> meshAmbientColor{1.0f, 1.0f, 1.0f};
    float meshAmbientIntensity{0.20f};
    float meshShininess{32.0f};
    std::size_t capacityGrowthFactor{2}; // Growth factor for the geometry buffers. Then new buffer will have a capacity
                                         // of current_capacity * growth_factor.

    float defaultPointSize{
        4.0f}; // Default size for points. If not other size is set using the GeoQik API, this size will be used.
    float defaultLineWidth{
        2.0f}; // Default width for lines. If not other width is set using the GeoQik API, this width will be used.
    Color defaultPointColor{
        1.0f,
        1.0f,
        1.0f,
        1.0f}; // Default color for points. If no other color is set using the GeoQik API, this color will be used.
    Color defaultLineColor{
        1.0f,
        1.0f,
        1.0f,
        1.0f}; // Default color for lines. If no other color is set using the GeoQik API, this color will be used.
    Color backgroundColor{0.1f, 0.1f, 0.1f, 1.0f}; // Default background color for the window.

    double cameraFarPlaneMultiplier{
        3.0}; // The far plane of the camera is set to the radius of the bounding sphere multiplied by this factor.
    bool autoFitCameraEnabled{true};
    bool autoFitZoomInEnabled{true};
    double autoFitZoomOutPadding{1.15};
    double autoFitMinViewportOccupancy{0.20};
    double autoFitTargetViewportOccupancy{0.65};
    std::chrono::milliseconds autoFitSuppressAfterUserCameraInteraction{1000};

    // Minimum time to be spent processing geometries per frame. This is used to ensure that the API does process
    // geometries. This reduces the frame rate at the expense of faster geometry processing. If the processing time is
    // set to 0, only the remaining time after drawing the geometries will be used for processing messages.
    std::chrono::milliseconds minGeometryProcessingTime{16}; // in milliseconds
    // Target time to be spent processing messages per frame. If the minGeometryProcessingTime
    // is set to 0, this will be the maximum time spent processing messages per frame.
    std::chrono::milliseconds maxFrameProcessingTime{16}; // in milliseconds
    // Frequency of updating the scene.
    std::size_t updateSceneFrequency{5};
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace geoqik

#endif // GEOQIKSETTINGS_HPP
