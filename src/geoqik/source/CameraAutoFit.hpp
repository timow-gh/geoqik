#ifndef CAMERA_AUTOFIT_HPP
#define CAMERA_AUTOFIT_HPP

#include "CameraProjectionType.hpp"
#include "Scene.hpp"
#include <chrono>

namespace geoqik
{

struct CameraAutoFitSettings
{
  bool enabled{true};
  bool zoomInEnabled{true};
  double zoomOutPadding{1.15};
  double minViewportOccupancy{0.20};
  double targetViewportOccupancy{0.65};
  std::chrono::milliseconds suppressAfterUserCameraInteraction{1000};
};

struct CameraAutoFitInput
{
  linal::double3 position{0.0};
  linal::double3 target{0.0};
  linal::double3 vertical{0.0, 0.0, 1.0};
  CameraProjectionType projectionType{CameraProjectionType::PERSPECTIVE};
  double verticalFovDegrees{30.0};
  double orthographicWidth{10.0};
  double orthographicHeight{10.0};
  double aspectRatio{1.0};
  double nearPlane{0.01};
  double farPlaneMultiplier{3.0};
  bool suppressZoomIn{false};
  CameraAutoFitSettings settings;
};

struct CameraAutoFitResult
{
  bool hasGeometry{false};
  bool changed{false};
  bool zoomedOut{false};
  bool zoomedIn{false};
  bool panned{false};
  double viewportOccupancy{0.0};
  double farPlane{0.0};
  double orthographicWidth{10.0};
  double orthographicHeight{10.0};
  linal::double3 position{0.0};
  linal::double3 target{0.0};
  linal::double3 vertical{0.0, 0.0, 1.0};
};

[[nodiscard]] CameraAutoFitResult calculate_camera_auto_fit(const Scene& scene, const CameraAutoFitInput& input);

} // namespace geoqik

#endif // CAMERA_AUTOFIT_HPP
