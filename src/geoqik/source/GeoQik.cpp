#include "GeoQik/GeoQik.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "Context.hpp"
#include "Core/UUID.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikSettings.hpp"
#include <Renderer/WindowSettings.hpp>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace geoqik;

namespace
{
constexpr std::size_t coordinateCount = 3;
constexpr std::size_t lineCoordinateCount = 6;
constexpr std::size_t uuidByteCount = 16;

constexpr std::size_t defaultMaxMessageQueueSize = 1'000'000;
constexpr std::size_t defaultInitialGeometryCapacity = 100'000;
constexpr std::size_t defaultInitialMeshCapacity = 10'000;
constexpr std::size_t defaultCapacityGrowthFactor = 2;
constexpr float defaultPointSize = 4.0F;
constexpr float defaultLineWidth = 2.0F;
constexpr float defaultColorChannel = 1.0F;
constexpr float defaultBackgroundColorChannel = 0.1F;
constexpr float defaultMeshFillLightDirectionX = -0.45F;
constexpr float defaultMeshFillLightDirectionY = 0.60F;
constexpr float defaultMeshFillLightDirectionZ = 0.35F;
constexpr float defaultMeshFillLightColorRed = 0.70F;
constexpr float defaultMeshFillLightColorGreen = 0.78F;
constexpr float defaultMeshFillLightIntensity = 0.25F;
constexpr float defaultMeshAmbientIntensity = 0.20F;
constexpr float defaultMeshShininess = 32.0F;
constexpr double defaultCameraFarPlaneMultiplier = 3.0;
constexpr double defaultAutoFitZoomOutPadding = 1.15;
constexpr double defaultAutoFitMinViewportOccupancy = 0.20;
constexpr double defaultAutoFitTargetViewportOccupancy = 0.65;
constexpr std::size_t defaultAutoFitSuppressAfterUserCameraInteractionMs = 1000;
constexpr std::size_t defaultFrameProcessingTimeMs = 16;
constexpr std::size_t defaultUpdateSceneFrequency = 5;

constexpr std::uint32_t defaultWindowWidth = 1280;
constexpr std::uint32_t defaultWindowHeight = 720;
constexpr int defaultColorBits = 8;
constexpr int defaultDepthBits = 24;
constexpr int defaultStencilBits = 8;
constexpr int defaultSamples = 8;

std::atomic<bool>& api_is_initialized_storage()
{
  static std::atomic<bool> apiIsInitialized{false}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  return apiIsInitialized;
}

} // namespace

// Forward declarations for internal C++ functions
namespace geoqik_internal
{
static geoqik_settings_t create_default_c_settings()
{
  geoqik_settings_t settings{};

  settings.maxMessageQueueSize = defaultMaxMessageQueueSize;
  settings.initialPointCapacity = defaultInitialGeometryCapacity;
  settings.initialLineCapacity = defaultInitialGeometryCapacity;
  settings.initialMeshCapacity = defaultInitialMeshCapacity;
  settings.defaultMeshColor[0] = defaultColorChannel;
  settings.defaultMeshColor[1] = defaultColorChannel;
  settings.defaultMeshColor[2] = defaultColorChannel;
  settings.defaultMeshColor[3] = defaultColorChannel;
  settings.meshHeadLightColor[0] = defaultColorChannel;
  settings.meshHeadLightColor[1] = defaultColorChannel;
  settings.meshHeadLightColor[2] = defaultColorChannel;
  settings.meshHeadLightIntensity = 1.0F;
  settings.meshFillLightDirection[0] = defaultMeshFillLightDirectionX;
  settings.meshFillLightDirection[1] = defaultMeshFillLightDirectionY;
  settings.meshFillLightDirection[2] = defaultMeshFillLightDirectionZ;
  settings.meshFillLightColor[0] = defaultMeshFillLightColorRed;
  settings.meshFillLightColor[1] = defaultMeshFillLightColorGreen;
  settings.meshFillLightColor[2] = 1.0F;
  settings.meshFillLightIntensity = defaultMeshFillLightIntensity;
  settings.meshAmbientColor[0] = defaultColorChannel;
  settings.meshAmbientColor[1] = defaultColorChannel;
  settings.meshAmbientColor[2] = defaultColorChannel;
  settings.meshAmbientIntensity = defaultMeshAmbientIntensity;
  settings.meshShininess = defaultMeshShininess;
  settings.capacityGrowthFactor = defaultCapacityGrowthFactor;
  settings.defaultPointSize = defaultPointSize;
  settings.defaultLineWidth = defaultLineWidth;
  settings.defaultPointColor[0] = defaultColorChannel;
  settings.defaultPointColor[1] = defaultColorChannel;
  settings.defaultPointColor[2] = defaultColorChannel;
  settings.defaultPointColor[3] = defaultColorChannel;
  settings.defaultLineColor[0] = defaultColorChannel;
  settings.defaultLineColor[1] = defaultColorChannel;
  settings.defaultLineColor[2] = defaultColorChannel;
  settings.defaultLineColor[3] = defaultColorChannel;
  settings.backgroundColor[0] = defaultBackgroundColorChannel;
  settings.backgroundColor[1] = defaultBackgroundColorChannel;
  settings.backgroundColor[2] = defaultBackgroundColorChannel;
  settings.backgroundColor[3] = defaultColorChannel;
  settings.cameraFarPlaneMultiplier = defaultCameraFarPlaneMultiplier;
  settings.autoFitCameraEnabled = 1;
  settings.autoFitZoomInEnabled = 1;
  settings.autoFitZoomOutPadding = defaultAutoFitZoomOutPadding;
  settings.autoFitMinViewportOccupancy = defaultAutoFitMinViewportOccupancy;
  settings.autoFitTargetViewportOccupancy = defaultAutoFitTargetViewportOccupancy;
  settings.autoFitSuppressAfterUserCameraInteractionMs = defaultAutoFitSuppressAfterUserCameraInteractionMs;
  settings.minGeometryProcessingTimeMs = defaultFrameProcessingTimeMs;
  settings.maxFrameProcessingTimeMs = defaultFrameProcessingTimeMs;
  settings.updateSceneFrequency = defaultUpdateSceneFrequency;

  return settings;
}

static geoqik_window_settings_t create_default_c_window_settings()
{
  geoqik_window_settings_t settings{};

  settings.title = "GeoQik Viewer";
  settings.width = defaultWindowWidth;
  settings.height = defaultWindowHeight;
  settings.red_bits = defaultColorBits;
  settings.green_bits = defaultColorBits;
  settings.blue_bits = defaultColorBits;
  settings.alpha_bits = defaultColorBits;
  settings.depth_bits = defaultDepthBits;
  settings.stencil_bits = defaultStencilBits;
  settings.accum_red_bits = 0;
  settings.accum_green_bits = 0;
  settings.accum_blue_bits = 0;
  settings.accum_alpha_bits = 0;
  settings.aux_buffers = 0;
  settings.samples = defaultSamples;
  settings.refresh_rate = -1;
  settings.stereo = 0;
  settings.srgb_capable = 0;
  settings.double_buffer = 1;
  settings.resizable = 1;
  settings.visible = 1;
  settings.decorated = 1;
  settings.focused = 1;
  settings.auto_iconify = 1;
  settings.floating = 0;
  settings.maximized = 0;
  settings.center_cursor = 0;
  settings.transparent_framebuffer = 0;
  settings.focus_on_show = 1;
  settings.scale_to_monitor = 1;

  return settings;
}

static bool api_is_initialized()
{
  return api_is_initialized_storage().load(std::memory_order_acquire);
}

static geoqik::GeoQikSettings convert_to_cpp_settings(const geoqik_settings_t& cSettings)
{
  geoqik::GeoQikSettings cppSettings;
  cppSettings.maxMessageQueueSize = cSettings.maxMessageQueueSize;
  cppSettings.initialPointCapacity = cSettings.initialPointCapacity;
  cppSettings.initialLineCapacity = cSettings.initialLineCapacity;
  cppSettings.initialMeshCapacity = cSettings.initialMeshCapacity;
  cppSettings.defaultMeshColor = Color{cSettings.defaultMeshColor[0], cSettings.defaultMeshColor[1],
                                       cSettings.defaultMeshColor[2], cSettings.defaultMeshColor[3]};
  for (size_t i = 0; i < 3; ++i)
  {
    cppSettings.meshHeadLightColor[i] = cSettings.meshHeadLightColor[i];
    cppSettings.meshFillLightDirection[i] = cSettings.meshFillLightDirection[i];
    cppSettings.meshFillLightColor[i] = cSettings.meshFillLightColor[i];
    cppSettings.meshAmbientColor[i] = cSettings.meshAmbientColor[i];
  }
  cppSettings.meshHeadLightIntensity = cSettings.meshHeadLightIntensity;
  cppSettings.meshFillLightIntensity = cSettings.meshFillLightIntensity;
  cppSettings.meshAmbientIntensity = cSettings.meshAmbientIntensity;
  cppSettings.meshShininess = cSettings.meshShininess;
  cppSettings.capacityGrowthFactor = cSettings.capacityGrowthFactor;
  cppSettings.defaultPointSize = cSettings.defaultPointSize;
  cppSettings.defaultLineWidth = cSettings.defaultLineWidth;

  for (size_t i = 0; i < ColorChannelCount; ++i)
  {
    cppSettings.defaultPointColor[i] = cSettings.defaultPointColor[i];
    cppSettings.defaultLineColor[i] = cSettings.defaultLineColor[i];
    cppSettings.backgroundColor[i] = cSettings.backgroundColor[i];
  }

  cppSettings.cameraFarPlaneMultiplier = cSettings.cameraFarPlaneMultiplier;
  cppSettings.autoFitCameraEnabled = cSettings.autoFitCameraEnabled != 0;
  cppSettings.autoFitZoomInEnabled = cSettings.autoFitZoomInEnabled != 0;
  cppSettings.autoFitZoomOutPadding = cSettings.autoFitZoomOutPadding;
  cppSettings.autoFitMinViewportOccupancy = cSettings.autoFitMinViewportOccupancy;
  cppSettings.autoFitTargetViewportOccupancy = cSettings.autoFitTargetViewportOccupancy;
  cppSettings.autoFitSuppressAfterUserCameraInteraction =
      std::chrono::milliseconds(cSettings.autoFitSuppressAfterUserCameraInteractionMs);
  cppSettings.minGeometryProcessingTime = std::chrono::milliseconds(cSettings.minGeometryProcessingTimeMs);
  cppSettings.maxFrameProcessingTime = std::chrono::milliseconds(cSettings.maxFrameProcessingTimeMs);
  cppSettings.updateSceneFrequency = cSettings.updateSceneFrequency;
  return cppSettings;
}

static geoqik::GeoQikSettings convert_to_cpp_settings(const geoqik_settings_t* cSettings)
{
  if (cSettings != nullptr)
  {
    return convert_to_cpp_settings(*cSettings);
  }
  return convert_to_cpp_settings(create_default_c_settings());
}

static renderer::WindowSettings convert_to_cpp_window_settings(const geoqik_window_settings_t& cSettings)
{
  renderer::WindowSettings cppSettings;
  cppSettings.title = cSettings.title != nullptr ? cSettings.title : create_default_c_window_settings().title;
  cppSettings.width = cSettings.width;
  cppSettings.height = cSettings.height;
  cppSettings.red_bits = cSettings.red_bits;
  cppSettings.green_bits = cSettings.green_bits;
  cppSettings.blue_bits = cSettings.blue_bits;
  cppSettings.alpha_bits = cSettings.alpha_bits;
  cppSettings.depth_bits = cSettings.depth_bits;
  cppSettings.stencil_bits = cSettings.stencil_bits;
  cppSettings.accum_red_bits = cSettings.accum_red_bits;
  cppSettings.accum_green_bits = cSettings.accum_green_bits;
  cppSettings.accum_blue_bits = cSettings.accum_blue_bits;
  cppSettings.accum_alpha_bits = cSettings.accum_alpha_bits;
  cppSettings.aux_buffers = cSettings.aux_buffers;
  cppSettings.samples = cSettings.samples;
  cppSettings.refresh_rate = cSettings.refresh_rate;
  cppSettings.stereo = cSettings.stereo != 0;
  cppSettings.srgb_capable = cSettings.srgb_capable != 0;
  cppSettings.double_buffer = cSettings.double_buffer != 0;
  cppSettings.resizable = cSettings.resizable != 0;
  cppSettings.visible = cSettings.visible != 0;
  cppSettings.decorated = cSettings.decorated != 0;
  cppSettings.focused = cSettings.focused != 0;
  cppSettings.auto_iconify = cSettings.auto_iconify != 0;
  cppSettings.floating = cSettings.floating != 0;
  cppSettings.maximized = cSettings.maximized != 0;
  cppSettings.center_cursor = cSettings.center_cursor != 0;
  cppSettings.transparent_framebuffer = cSettings.transparent_framebuffer != 0;
  cppSettings.focus_on_show = cSettings.focus_on_show != 0;
  cppSettings.scale_to_monitor = cSettings.scale_to_monitor != 0;
  return cppSettings;
}

static renderer::WindowSettings convert_to_cpp_window_settings(const geoqik_window_settings_t* cSettings)
{
  if (cSettings != nullptr)
  {
    return convert_to_cpp_window_settings(*cSettings);
  }
  return convert_to_cpp_window_settings(create_default_c_window_settings());
}

static bool validate_finite_coords(double x, double y, double z)
{
  return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

static bool validate_color(float r, float g, float b, float a)
{
  return r >= 0.0F && r <= 1.0F && g >= 0.0F && g <= 1.0F && b >= 0.0F && b <= 1.0F && a >= 0.0F && a <= 1.0F;
}

static bool validate_color_values(const float* colors, std::size_t colorCount)
{
  if (colorCount == 0)
  {
    return true;
  }
  if (colors == nullptr)
  {
    return false;
  }
  for (std::size_t i = 0; i < colorCount; ++i)
  {
    if (colors[i] < 0.0F || colors[i] > 1.0F)
    {
      return false;
    }
  }
  return true;
}

static bool validate_point_color_count(std::size_t colorCount, std::size_t pointCount)
{
  return colorCount == 0 || colorCount == ColorChannelCount || colorCount == pointCount * ColorChannelCount;
}

static bool validate_line_color_count(std::size_t colorCount, std::size_t lineCount)
{
  return colorCount == 0 || colorCount == ColorChannelCount || colorCount == lineCount * ColorChannelCount;
}

static bool convert_replay_keys(const geoqik_key_t* cKeys,
                                std::size_t cKeyCount,
                                std::initializer_list<geoqik::Key> defaultKeys,
                                std::vector<geoqik::Key>& replayKeys)
{
  replayKeys.clear();
  if (cKeyCount == 0)
  {
    replayKeys.assign(defaultKeys.begin(), defaultKeys.end());
    return true;
  }

  if (cKeys == nullptr)
  {
    return false;
  }

  replayKeys.reserve(cKeyCount);
  for (std::size_t i = 0; i < cKeyCount; ++i)
  {
    if (static_cast<int>(cKeys[i]) <= static_cast<int>(GEOQIK_KEY_UNKNOWN))
    {
      return false;
    }
    replayKeys.push_back(static_cast<geoqik::Key>(static_cast<int>(cKeys[i])));
  }
  return true;
}

static bool read_replay_option_keys(const geoqik_replay_options_t* options, geoqik::ReplayOptions& replayOptions)
{
  const geoqik_key_t* stepKeys = options != nullptr ? options->stepKeys : nullptr;
  const std::size_t stepKeyCount = options != nullptr ? options->stepKeyCount : 0;
  const geoqik_key_t* backwardStepKeys = options != nullptr ? options->backwardStepKeys : nullptr;
  const std::size_t backwardStepKeyCount = options != nullptr ? options->backwardStepKeyCount : 0;
  const geoqik_key_t* resumeKeys = options != nullptr ? options->resumeKeys : nullptr;
  const std::size_t resumeKeyCount = options != nullptr ? options->resumeKeyCount : 0;
  const geoqik_key_t* pauseKeys = options != nullptr ? options->pauseKeys : nullptr;
  const std::size_t pauseKeyCount = options != nullptr ? options->pauseKeyCount : 0;
  const geoqik_key_t* increaseKeys = options != nullptr ? options->increaseEntriesPerStepKeys : nullptr;
  const std::size_t increaseKeyCount = options != nullptr ? options->increaseEntriesPerStepKeyCount : 0;
  const geoqik_key_t* decreaseKeys = options != nullptr ? options->decreaseEntriesPerStepKeys : nullptr;
  const std::size_t decreaseKeyCount = options != nullptr ? options->decreaseEntriesPerStepKeyCount : 0;

  return convert_replay_keys(stepKeys,
                             stepKeyCount,
                             {geoqik::Key::KEY_RIGHT, geoqik::Key::KEY_D},
                             replayOptions.stepKeys) &&
         convert_replay_keys(backwardStepKeys,
                             backwardStepKeyCount,
                             {geoqik::Key::KEY_LEFT, geoqik::Key::KEY_A},
                             replayOptions.backwardStepKeys) &&
         convert_replay_keys(resumeKeys,
                             resumeKeyCount,
                             {geoqik::Key::KEY_SPACE},
                             replayOptions.resumeKeys) &&
         convert_replay_keys(pauseKeys,
                             pauseKeyCount,
                             {geoqik::Key::KEY_SPACE},
                             replayOptions.pauseKeys) &&
         convert_replay_keys(increaseKeys,
                             increaseKeyCount,
                             {geoqik::Key::KEY_UP, geoqik::Key::KEY_W},
                             replayOptions.increaseEntriesPerStepKeys) &&
         convert_replay_keys(decreaseKeys,
                             decreaseKeyCount,
                             {geoqik::Key::KEY_DOWN, geoqik::Key::KEY_S},
                             replayOptions.decreaseEntriesPerStepKeys);
}

static bool convert_replay_options(const geoqik_replay_options_t* options, geoqik::ReplayOptions& replayOptions)
{
  constexpr double defaultEntriesPerSecond = 60.0;
  constexpr double defaultSpeedMultiplier = 1.0;
  constexpr std::size_t defaultMaxEntriesPerFrame = 1024;
  constexpr std::size_t defaultEntriesPerStep = 1;

  double entriesPerSecond = defaultEntriesPerSecond;
  double speedMultiplier = defaultSpeedMultiplier;
  std::size_t maxEntriesPerFrame = defaultMaxEntriesPerFrame;
  bool startPaused = false;
  std::size_t entriesPerStep = defaultEntriesPerStep;

  if (options != nullptr)
  {
    if (options->entriesPerSecond != 0.0)
    {
      if (!std::isfinite(options->entriesPerSecond) || options->entriesPerSecond < 0.0)
      {
        return false;
      }
      entriesPerSecond = options->entriesPerSecond;
    }

    if (options->speedMultiplier != 0.0)
    {
      if (!std::isfinite(options->speedMultiplier) || options->speedMultiplier < 0.0)
      {
        return false;
      }
      speedMultiplier = options->speedMultiplier;
    }

    if (options->maxEntriesPerFrame != 0)
    {
      maxEntriesPerFrame = options->maxEntriesPerFrame;
    }

    startPaused = options->startPaused != 0;

    if (options->entriesPerStep != 0)
    {
      entriesPerStep = options->entriesPerStep;
    }
  }

  if (entriesPerStep == 0 || maxEntriesPerFrame == 0)
  {
    return false;
  }

  const double effectiveEntriesPerSecond = entriesPerSecond * speedMultiplier;
  if (!std::isfinite(effectiveEntriesPerSecond) || effectiveEntriesPerSecond <= 0.0)
  {
    return false;
  }

  replayOptions.entriesPerSecond = effectiveEntriesPerSecond;
  replayOptions.maxEntriesPerFrame = maxEntriesPerFrame;
  replayOptions.startPaused = startPaused;
  replayOptions.entriesPerStep = entriesPerStep;

  return read_replay_option_keys(options, replayOptions);
}

template <typename Func>
static auto execute_with_error_handling(Func&& func) -> std::invoke_result_t<Func>
{
  using ReturnType = std::invoke_result_t<Func>;
  try
  {
    return std::forward<Func>(func)();
  }
  catch (const std::bad_alloc&)
  {
    if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
    {
      return GEOQIK_ERROR_MEMORY_ALLOCATION;
    }
    else
    {
      return geoqik_result_t{GEOQIK_ERROR_MEMORY_ALLOCATION, {}};
    }
  }
  catch (...)
  {
    if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
    {
      return GEOQIK_ERROR_UNKNOWN;
    }
    else
    {
      return geoqik_result_t{GEOQIK_ERROR_UNKNOWN, {}};
    }
  }
}

template <typename Func>
static auto execute_if_initialized(Func&& func) -> std::invoke_result_t<Func>
{
  using ReturnType = std::invoke_result_t<Func>;
  return execute_with_error_handling(
      [&]() -> ReturnType
      {
        if (!api_is_initialized())
        {
          if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
          {
            return GEOQIK_ERROR_NOT_INITIALIZED;
          }
          else
          {
            return geoqik_result_t{GEOQIK_ERROR_NOT_INITIALIZED, {}};
          }
        }
        return std::forward<Func>(func)();
      });
}

template <typename Func>
static auto execute_if_not_initialized(Func&& func) -> std::invoke_result_t<Func>
{
  using ReturnType = std::invoke_result_t<Func>;
  return execute_with_error_handling(
      [&]() -> ReturnType
      {
        if (api_is_initialized())
        {
          if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
          {
            return GEOQIK_ERROR_ALREADY_INITIALIZED;
          }
          else
          {
            return geoqik_result_t{GEOQIK_ERROR_ALREADY_INITIALIZED, {}};
          }
        }
        return std::forward<Func>(func)();
      });
}

static geoqik_error_code_t run_render_thread(const geoqik::GeoQikSettings& geoqikSettings,
                                             const renderer::WindowSettings& settings,
                                             const std::shared_ptr<std::promise<geoqik_error_code_t>>& initResult)
{
  auto setInitResult = [&initResult](geoqik_error_code_t result)
  {
    try
    {
      initResult->set_value(result);
    }
    catch (const std::future_error&)
    {
    }
  };

  try
  {
    auto context = std::make_unique<Context>();
    if (!context->init_window(geoqikSettings, settings))
    {
      setInitResult(GEOQIK_ERROR_UNKNOWN);
      return GEOQIK_ERROR_UNKNOWN;
    }

    setInitResult(GEOQIK_SUCCESS);
    context->run_event_loop();
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    get_message_queue().clear();
    api_is_initialized_storage().store(false, std::memory_order_release);
    setInitResult(GEOQIK_ERROR_MEMORY_ALLOCATION);
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception in GeoQik render thread: " << e.what() << '\n';
    get_message_queue().clear();
    api_is_initialized_storage().store(false, std::memory_order_release);
    setInitResult(GEOQIK_ERROR_UNKNOWN);
    return GEOQIK_ERROR_UNKNOWN;
  }
  catch (...)
  {
    get_message_queue().clear();
    api_is_initialized_storage().store(false, std::memory_order_release);
    setInitResult(GEOQIK_ERROR_UNKNOWN);
    return GEOQIK_ERROR_UNKNOWN;
  }
}

// Idempotent function to start the GeoQik thread
static std::thread& get_render_thread()
{
  static std::thread renderThread;
  return renderThread;
}

static geoqik_error_code_t start_geoqik_thread(const geoqik::GeoQikSettings& geoqikSettings, const renderer::WindowSettings& settings)
{
  try
  {
    auto& renderThread = get_render_thread();
    if (!renderThread.joinable())
    {
      auto initPromise = std::make_shared<std::promise<geoqik_error_code_t>>();
      std::future<geoqik_error_code_t> initFuture = initPromise->get_future();
      renderThread = std::thread(run_render_thread, geoqikSettings, settings, initPromise);

      const geoqik_error_code_t initResult = initFuture.get();
      if (initResult != GEOQIK_SUCCESS)
      {
        if (renderThread.joinable())
        {
          renderThread.join();
        }
        return initResult;
      }
    }
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    throw;
  }
}

} // namespace geoqik_internal

// Helper functions for UUID conversion
namespace
{

core::UUID convert_to_core_uuid(const geoqik_uuid_t& uuid)
{
  std::array<uint8_t, uuidByteCount> bytes{};
  for (size_t i = 0; i < uuidByteCount; ++i)
  {
    bytes[i] = uuid.value[i];
  }
  return core::UUID(bytes);
}

geoqik_uuid_t convert_to_geoqik_uuid(const core::UUID& uuid)
{
  geoqik_uuid_t gUuid{};
  auto bytes = uuid.to_array();
  assert(bytes.size() == uuidByteCount);
  for (size_t i = 0; i < uuidByteCount; ++i)
  {
    gUuid.value[i] = bytes[i];
  }
  return gUuid;
}

geoqik_error_code_t enqueue(GeoQikMessage&& message)
{
  try
  {
    get_message_queue().enqueue(std::move(message));
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

} // namespace

geoqik_error_code_t geoqik_init()
{
  return geoqik_internal::execute_if_not_initialized(
      [&]() -> geoqik_error_code_t
      {
        geoqik::GeoQikSettings defaultGeoQikSettings = geoqik_internal::convert_to_cpp_settings(nullptr);
        renderer::WindowSettings defaultWindowSettings = geoqik_internal::convert_to_cpp_window_settings(nullptr);

        api_is_initialized_storage().store(true, std::memory_order_release);
        try
        {
          init_message_queue(ConcurrentQueue<GeoQikMessage>(defaultGeoQikSettings.maxMessageQueueSize));
          geoqik_error_code_t result = geoqik_internal::start_geoqik_thread(defaultGeoQikSettings, defaultWindowSettings);
          if (result != GEOQIK_SUCCESS)
          {
            api_is_initialized_storage().store(false, std::memory_order_release);
          }
          return result;
        }
        catch (...)
        {
          api_is_initialized_storage().store(false, std::memory_order_release);
          throw;
        }
      });
}

void geoqik_create_default_settings(geoqik_settings_t* settings)
{
  if (settings == nullptr)
  {
    return;
  }

  *settings = geoqik_internal::create_default_c_settings();
}

void geoqik_init_default_window_settings(geoqik_window_settings_t* settings)
{
  if (settings == nullptr)
  {
    return;
  }

  *settings = geoqik_internal::create_default_c_window_settings();
}

geoqik_error_code_t geoqik_init_with_settings(const geoqik_settings_t* geoqikSettings, const geoqik_window_settings_t* windowSettings)
{
  return geoqik_internal::execute_if_not_initialized(
      [&]() -> geoqik_error_code_t
      {
        // Convert C structs to C++ structs
        geoqik::GeoQikSettings cppGeoQikSettings = geoqik_internal::convert_to_cpp_settings(geoqikSettings);
        renderer::WindowSettings cppWindowSettings = geoqik_internal::convert_to_cpp_window_settings(windowSettings);

        api_is_initialized_storage().store(true, std::memory_order_release);
        try
        {
          init_message_queue(ConcurrentQueue<GeoQikMessage>(cppGeoQikSettings.maxMessageQueueSize));
          geoqik_error_code_t result = geoqik_internal::start_geoqik_thread(cppGeoQikSettings, cppWindowSettings);
          if (result != GEOQIK_SUCCESS)
          {
            api_is_initialized_storage().store(false, std::memory_order_release);
          }
          return result;
        }
        catch (...)
        {
          api_is_initialized_storage().store(false, std::memory_order_release);
          throw;
        }
      });
}

geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized)
{
  if (isInitialized == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
    *isInitialized = geoqik_internal::api_is_initialized();
    return GEOQIK_SUCCESS;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

const char* geoqik_get_error_string(geoqik_error_code_t result)
{
  switch (result)
  {
  case GEOQIK_SUCCESS: return "Success";
  case GEOQIK_ERROR_NOT_INITIALIZED: return "GeoQik not initialized";
  case GEOQIK_ERROR_ALREADY_INITIALIZED: return "GeoQik already initialized";
  case GEOQIK_ERROR_INVALID_PARAMETER: return "Invalid parameter";
  case GEOQIK_ERROR_WRONG_COLOR_SIZE: return "Wrong RGBA color size";
  case GEOQIK_ERROR_MEMORY_ALLOCATION: return "Memory allocation error";
  case GEOQIK_ERROR_UNKNOWN: return "Unknown error";
  default: return "Invalid error code";
  }
}

geoqik_error_code_t geoqik_generate_uuid(geoqik_uuid_t* uuid)
{
  if (uuid == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_with_error_handling(
      [&]() -> geoqik_error_code_t
      {
        auto coreUuid = core::UUID::generate().to_array();

        assert(coreUuid.size() == uuidByteCount);
        for (std::size_t i = 0; i < uuidByteCount; ++i)
        {
          uuid->value[i] = coreUuid[i];
        }
        return GEOQIK_SUCCESS;
      });
}

geoqik_result_t geoqik_add_point(double x, double y, double z)
{
  return geoqik_add_point_opts(x, y, z, nullptr);
}

geoqik_result_t geoqik_add_point_with_color(double x, double y, double z, float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  const float color[ColorChannelCount] = {r, g, b, a};
  geoqik_add_points_options_t opts{};
  opts.color = color;
  opts.colorCount = ColorChannelCount;
  return geoqik_add_point_opts(x, y, z, &opts);
}

geoqik_result_t geoqik_add_point_opts(double x, double y, double z, geoqik_add_points_options_t* options)
{
  if (!geoqik_internal::validate_finite_coords(x, y, z))
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  std::vector<float> colorsCopy;

  if (options != nullptr)
  {
    if (!geoqik_internal::validate_point_color_count(options->colorCount, 1))
    {
      return geoqik_result_t{GEOQIK_ERROR_WRONG_COLOR_SIZE, {}};
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    if (options->colorCount > 0)
    {
      colorsCopy.assign(options->color, options->color + options->colorCount);
    }
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options != nullptr)
        {
          if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil())
          {
            commonData.idempotencyId = idem;
          }

          commonData.rgba = std::move(colorsCopy);
        }

        auto enqueueResult =
            enqueue(GeoQikMessage{AddPointWithOpts{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), std::move(commonData)}});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
      });
}

geoqik_result_t geoqik_add_points_opts(const double* points, size_t size, geoqik_add_points_options_t* options)
{
  if (points == nullptr || size == 0 || size % coordinateCount != 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  std::vector<float> pointsCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t pointCount = size / coordinateCount;

  for (size_t i = 0; i < size; i += coordinateCount)
  {
    const double& x = points[i];
    const double& y = points[i + 1];
    const double& z = points[i + 2];
    if (!geoqik_internal::validate_finite_coords(x, y, z))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    pointsCopy[i] = static_cast<float>(x);
    pointsCopy[i + 1] = static_cast<float>(y);
    pointsCopy[i + 2] = static_cast<float>(z);
  }

  if (options != nullptr && options->color != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_point_color_count(options->colorCount, pointCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_WRONG_COLOR_SIZE, {}};
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }
  else if (options != nullptr && options->colorCount > 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options != nullptr)
        {
          if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil())
          {
            commonData.idempotencyId = idem;
          }

          commonData.rgba = std::move(colorsCopy);
        }

        auto enqueueResult = enqueue(GeoQikMessage{AddPointsWithOpts{std::move(pointsCopy), std::move(commonData)}});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
      });
}

geoqik_error_code_t geoqik_update_point(const geoqik_uuid_t* geometryId, double x, double y, double z)
{
  return geoqik_update_point_opts(geometryId, x, y, z, nullptr);
}

geoqik_error_code_t geoqik_update_point_with_color(const geoqik_uuid_t* geometryId, double x, double y, double z, float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  const float color[ColorChannelCount] = {r, g, b, a};
  geoqik_update_points_options_t opts{};
  opts.color = color;
  opts.colorCount = ColorChannelCount;
  return geoqik_update_point_opts(geometryId, x, y, z, &opts);
}

geoqik_error_code_t geoqik_update_point_opts(const geoqik_uuid_t* geometryId,
                                             double x,
                                             double y,
                                             double z,
                                             geoqik_update_points_options_t* options)
{
  if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(x, y, z))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  std::vector<float> colorsCopy;
  if (options != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_point_color_count(options->colorCount, 1))
    {
      return GEOQIK_ERROR_WRONG_COLOR_SIZE;
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{UpdatePointWithOpts{handle,
                                                         static_cast<float>(x),
                                                         static_cast<float>(y),
                                                         static_cast<float>(z),
                                                         std::move(colorsCopy)}});
      });
}

geoqik_error_code_t geoqik_update_points_opts(const geoqik_uuid_t* geometryId,
                                              const double* points,
                                              size_t size,
                                              geoqik_update_points_options_t* options)
{
  if (geometryId == nullptr || points == nullptr || size == 0 || size % coordinateCount != 0)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  std::vector<float> pointsCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t pointCount = size / coordinateCount;

  for (std::size_t i = 0; i < size; i += coordinateCount)
  {
    const double px = points[i + 0];
    const double py = points[i + 1];
    const double pz = points[i + 2];
    if (!geoqik_internal::validate_finite_coords(px, py, pz))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    pointsCopy[i + 0] = static_cast<float>(px);
    pointsCopy[i + 1] = static_cast<float>(py);
    pointsCopy[i + 2] = static_cast<float>(pz);
  }

  if (options != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_point_color_count(options->colorCount, pointCount))
    {
      return GEOQIK_ERROR_WRONG_COLOR_SIZE;
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{UpdatePointsWithOpts{handle, std::move(pointsCopy), std::move(colorsCopy)}});
      });
}

geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId)
{
  if (geometryId == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{RemovePoint{handle}});
      });
}

geoqik_error_code_t geoqik_add_line(double x1, double y1, double z1, double x2, double y2, double z2)
{
  return geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, nullptr).err;
}

geoqik_error_code_t geoqik_add_line_with_color(double x1, double y1, double z1, double x2, double y2, double z2, float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  const float color[ColorChannelCount] = {r, g, b, a};
  geoqik_add_line_opts_t opts{};
  opts.color = color;
  opts.colorCount = ColorChannelCount;
  return geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, &opts).err;
}

geoqik_result_t geoqik_add_line_opts(double x1, double y1, double z1, double x2, double y2, double z2, geoqik_add_line_opts_t* options)
{
  if (!geoqik_internal::validate_finite_coords(x1, y1, z1) || !geoqik_internal::validate_finite_coords(x2, y2, z2))
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  std::vector<float> colorsCopy;

  if (options != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_line_color_count(options->colorCount, 1))
    {
      return geoqik_result_t{GEOQIK_ERROR_WRONG_COLOR_SIZE, {}};
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options != nullptr)
        {
          if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil())
          {
            commonData.idempotencyId = idem;
          }

          commonData.rgba = std::move(colorsCopy);
        }

        auto enqueueResult = enqueue(GeoQikMessage{AddLineWithOpts{static_cast<float>(x1),
                                                                    static_cast<float>(y1),
                                                                    static_cast<float>(z1),
                                                                    static_cast<float>(x2),
                                                                    static_cast<float>(y2),
                                                                    static_cast<float>(z2),
                                                                    std::move(commonData)}});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
      });
}

geoqik_result_t geoqik_add_lines_opts(const double* lines, size_t size, geoqik_add_line_opts_t* options)
{
  if (lines == nullptr || size == 0 || size % lineCoordinateCount != 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  // count is the total number of doubles in the array; each line occupies 6 values (x1,y1,z1,x2,y2,z2)
  std::vector<float> linesCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t lineCount = size / lineCoordinateCount;

  for (size_t i = 0; i < size; i += lineCoordinateCount)
  {
    const size_t base = i;
    const double lx1 = lines[base + 0];
    const double ly1 = lines[base + 1];
    const double lz1 = lines[base + 2];
    const double lx2 = lines[base + 3];
    const double ly2 = lines[base + 4];
    const double lz2 = lines[base + lineCoordinateCount - 1];
    if (!geoqik_internal::validate_finite_coords(lx1, ly1, lz1) || !geoqik_internal::validate_finite_coords(lx2, ly2, lz2))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    linesCopy[base + 0] = static_cast<float>(lx1);
    linesCopy[base + 1] = static_cast<float>(ly1);
    linesCopy[base + 2] = static_cast<float>(lz1);
    linesCopy[base + 3] = static_cast<float>(lx2);
    linesCopy[base + 4] = static_cast<float>(ly2);
    linesCopy[base + lineCoordinateCount - 1] = static_cast<float>(lz2);
  }

  if (options != nullptr && options->color != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_line_color_count(options->colorCount, lineCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_WRONG_COLOR_SIZE, {}};
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }
  else if (options != nullptr && options->colorCount > 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options != nullptr)
        {
          if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil())
          {
            commonData.idempotencyId = idem;
          }

          commonData.rgba = std::move(colorsCopy);
        }

        auto enqueueResult = enqueue(GeoQikMessage{AddLinesWithOpts{std::move(linesCopy), std::move(commonData)}});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
      });
}

geoqik_error_code_t geoqik_update_line(const geoqik_uuid_t* geometryId, double x1, double y1, double z1, double x2, double y2, double z2)
{
  return geoqik_update_line_opts(geometryId, x1, y1, z1, x2, y2, z2, nullptr);
}

geoqik_error_code_t geoqik_update_line_with_color(const geoqik_uuid_t* geometryId,
                                                  double x1,
                                                  double y1,
                                                  double z1,
                                                  double x2,
                                                  double y2,
                                                  double z2,
                                                  float r,
                                                  float g,
                                                  float b,
                                                  float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  const float color[ColorChannelCount] = {r, g, b, a};
  geoqik_update_line_opts_t opts{};
  opts.color = color;
  opts.colorCount = ColorChannelCount;
  return geoqik_update_line_opts(geometryId, x1, y1, z1, x2, y2, z2, &opts);
}

geoqik_error_code_t geoqik_update_line_opts(const geoqik_uuid_t* geometryId,
                                            double x1,
                                            double y1,
                                            double z1,
                                            double x2,
                                            double y2,
                                            double z2,
                                            geoqik_update_line_opts_t* options)
{
  if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(x1, y1, z1) || !geoqik_internal::validate_finite_coords(x2, y2, z2))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  std::vector<float> colorsCopy;
  if (options != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_line_color_count(options->colorCount, 1))
    {
      return GEOQIK_ERROR_WRONG_COLOR_SIZE;
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{UpdateLineWithOpts{handle,
                                                        static_cast<float>(x1),
                                                        static_cast<float>(y1),
                                                        static_cast<float>(z1),
                                                        static_cast<float>(x2),
                                                        static_cast<float>(y2),
                                                        static_cast<float>(z2),
                                                        std::move(colorsCopy)}});
      });
}

geoqik_error_code_t geoqik_update_lines_opts(const geoqik_uuid_t* geometryId,
                                             const double* lines,
                                             size_t size,
                                             geoqik_update_line_opts_t* options)
{
  if (geometryId == nullptr || lines == nullptr || size == 0 || size % lineCoordinateCount != 0)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  std::vector<float> linesCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t lineCount = size / lineCoordinateCount;

  for (std::size_t i = 0; i < size; i += lineCoordinateCount)
  {
    const double lx1 = lines[i + 0];
    const double ly1 = lines[i + 1];
    const double lz1 = lines[i + 2];
    const double lx2 = lines[i + 3];
    const double ly2 = lines[i + 4];
    const double lz2 = lines[i + lineCoordinateCount - 1];
    if (!geoqik_internal::validate_finite_coords(lx1, ly1, lz1) || !geoqik_internal::validate_finite_coords(lx2, ly2, lz2))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    linesCopy[i + 0] = static_cast<float>(lx1);
    linesCopy[i + 1] = static_cast<float>(ly1);
    linesCopy[i + 2] = static_cast<float>(lz1);
    linesCopy[i + 3] = static_cast<float>(lx2);
    linesCopy[i + 4] = static_cast<float>(ly2);
    linesCopy[i + lineCoordinateCount - 1] = static_cast<float>(lz2);
  }

  if (options != nullptr && options->colorCount > 0)
  {
    if (!geoqik_internal::validate_line_color_count(options->colorCount, lineCount))
    {
      return GEOQIK_ERROR_WRONG_COLOR_SIZE;
    }
    if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
    {
      return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    colorsCopy.assign(options->color, options->color + options->colorCount);
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{UpdateLinesWithOpts{handle, std::move(linesCopy), std::move(colorsCopy)}});
      });
}

geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId)
{
  if (geometryId == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{RemoveLine{handle}});
      });
}

geoqik_result_t geoqik_add_mesh_opts(const float* vertices,
                                     size_t vertexCount,
                                     const uint32_t* triangleIndices,
                                     size_t triangleCount,
                                     geoqik_add_mesh_opts_t* options)
{
  if (vertices == nullptr || vertexCount == 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }
  if (triangleIndices == nullptr || triangleCount == 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  std::vector<float> verticesCopy(vertices, vertices + (vertexCount * 3));
  std::vector<std::uint32_t> indicesCopy(triangleIndices, triangleIndices + (triangleCount * 3));
  std::vector<float> normalsCopy;
  std::vector<float> colorsCopy;

  if (options != nullptr)
  {
    if (options->normalsCount > 0 && options->normals != nullptr)
    {
      normalsCopy.assign(options->normals, options->normals + options->normalsCount);
    }
    if (options->colorCount > 0 && options->color != nullptr)
    {
      if (!geoqik_internal::validate_color_values(options->color, options->colorCount))
      {
        return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
      }
      colorsCopy.assign(options->color, options->color + options->colorCount);
    }
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;
        commonData.rgba = std::move(colorsCopy);

        if (options != nullptr)
        {
          if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil())
          {
            commonData.idempotencyId = idem;
          }
        }

        geoqik::AddMeshWithOpts message;
        message.vertices = std::move(verticesCopy);
        message.normals = std::move(normalsCopy);
        message.triangleIndices = std::move(indicesCopy);
        message.commonData = std::move(commonData);

        auto enqueueResult = enqueue(GeoQikMessage{std::move(message)});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
      });
}

geoqik_error_code_t geoqik_remove_mesh(const geoqik_uuid_t* geometryId)
{
  if (geometryId == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{geoqik::RemoveMesh{handle}});
      });
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_all_geometry()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{RemoveAllGeometry{}}); });
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_translate_geometry(const geoqik_uuid_t* geometryId, double dx, double dy, double dz)
{
  if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(dx, dy, dz))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{TranslateGeometry{handle, static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz)}});
      });
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_rotate_geometry(const geoqik_uuid_t* geometryId,
                                                         double centerX,
                                                         double centerY,
                                                         double centerZ,
                                                         double axisX,
                                                         double axisY,
                                                         double axisZ,
                                                         double angle)
{
  if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(centerX, centerY, centerZ) ||
      !geoqik_internal::validate_finite_coords(axisX, axisY, axisZ) || !std::isfinite(angle))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        return enqueue(GeoQikMessage{RotateGeometry{handle,
                                                    static_cast<float>(centerX),
                                                    static_cast<float>(centerY),
                                                    static_cast<float>(centerZ),
                                                    static_cast<float>(axisX),
                                                    static_cast<float>(axisY),
                                                    static_cast<float>(axisZ),
                                                    static_cast<float>(angle)}});
      });
}

geoqik_error_code_t geoqik_draw()
{
  return geoqik_internal::execute_if_initialized([&]() -> geoqik_error_code_t
                                                 { return enqueue(GeoQikMessage{Draw{}}); });
}

geoqik_error_code_t geoqik_stop_drawing()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StopDraw{}}); });
}

geoqik_error_code_t geoqik_save_log(const char* path, geoqik_log_format_t format)
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
        std::future<geoqik_error_code_t> future = promise->get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            SaveLog{[promise, path = std::move(pathCopy), format](Context& context) { promise->set_value(context.save_log(path.c_str(), format)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_load_log(const char* path, geoqik_log_format_t format)
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
        std::future<geoqik_error_code_t> future = promise->get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            LoadLog{[promise, path = std::move(pathCopy), format](Context& context) { promise->set_value(context.load_log(path.c_str(), format)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_replay_log(const char* path, geoqik_log_format_t format, const geoqik_replay_options_t* options)
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  geoqik::ReplayOptions replayOptions;
  if (!geoqik_internal::convert_replay_options(options, replayOptions))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
        std::future<geoqik_error_code_t> future = promise->get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            ReplayLog{[promise, path = std::move(pathCopy), format, replayOptions](Context& context)
                      { promise->set_value(context.replay_log(path.c_str(), format, replayOptions)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_replay_current_log(const geoqik_replay_options_t* options)
{
  geoqik::ReplayOptions replayOptions;
  if (!geoqik_internal::convert_replay_options(options, replayOptions))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
        std::future<geoqik_error_code_t> future = promise->get_future();

        auto enqueueResult = enqueue(GeoQikMessage{
            ReplayCurrentLog{[promise, replayOptions](Context& context) { promise->set_value(context.replay_current_log(replayOptions)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_cancel_replay()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        request_replay_cancel();
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_pause_replay()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{PauseReplay{}}); });
}

geoqik_error_code_t geoqik_resume_replay()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{ResumeReplay{}}); });
}

geoqik_error_code_t geoqik_step_replay()
{
  return geoqik_step_replay_n(1);
}

geoqik_error_code_t geoqik_step_replay_n(size_t count)
{
  if (count == 0)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StepReplay{count}}); });
}

geoqik_error_code_t geoqik_step_replay_backward()
{
  return geoqik_step_replay_backward_n(1);
}

geoqik_error_code_t geoqik_step_replay_backward_n(size_t count)
{
  if (count == 0)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StepReplayBackward{count}}); });
}

geoqik_error_code_t geoqik_get_replay_state(geoqik_replay_state_t* state)
{
  if (state == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<geoqik_replay_state_t>>();
        std::future<geoqik_replay_state_t> future = promise->get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetReplayState{[promise](Context& context) { promise->set_value(context.get_replay_state()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        *state = future.get();
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_get_replay_progress(size_t* currentEntry, size_t* totalEntries)
{
  if (currentEntry == nullptr || totalEntries == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<std::pair<std::size_t, std::size_t>>>();
        std::future<std::pair<std::size_t, std::size_t>> future = promise->get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetReplayProgress{[promise](Context& context) { promise->set_value(context.get_replay_progress()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        const auto [current, total] = future.get();
        *currentEntry = current;
        *totalEntries = total;
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_set_point_size(float pointSize)
{
  if (pointSize <= 0.0F)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetPointSize{pointSize}}); });
}

geoqik_error_code_t geoqik_get_point_size(float* result)
{
  if (result == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<float>>();
        std::future<float> future = promise->get_future();

        auto enqueueResult = enqueue(GeoQikMessage{GetPointSize{[promise](Context& context) { promise->set_value(context.get_point_size()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        *result = future.get();
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_set_point_color(float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        return enqueue(GeoQikMessage{SetPointColor{Color{r, g, b, a}}});
      });
}

geoqik_error_code_t geoqik_get_point_color(float* r, float* g, float* b, float* a)
{
  if (r == nullptr || g == nullptr || b == nullptr || a == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<Color>>();
        std::future<Color> future = promise->get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetPointColor{[promise](Context& context) { promise->set_value(context.get_point_color()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        auto color = future.get();
        *r = color[0];
        *g = color[1];
        *b = color[2];
        *a = color[3];
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_set_line_width(float lineWidth)
{
  if (lineWidth <= 0.0F)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetLineWidth{lineWidth}}); });
}

geoqik_error_code_t geoqik_get_line_width(float* result)
{
  if (result == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<float>>();
        std::future<float> future = promise->get_future();

        auto enqueueResult = enqueue(GeoQikMessage{GetLineWidth{[promise](Context& context) { promise->set_value(context.get_line_width()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        *result = future.get();
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_set_line_color(float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        Color colorData{r, g, b, a};
        return enqueue(GeoQikMessage{SetLineColor{colorData}});
      });
}

geoqik_error_code_t geoqik_get_line_color(float* r, float* g, float* b, float* a)
{
  if (r == nullptr || g == nullptr || b == nullptr || a == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<Color>>();
        std::future<Color> future = promise->get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetLineColor{[promise](Context& context) { promise->set_value(context.get_line_color()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        auto color = future.get();
        *r = color[0];
        *g = color[1];
        *b = color[2];
        *a = color[3];
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_set_mesh_color(float r, float g, float b, float a)
{
  if (!geoqik_internal::validate_color(r, g, b, a))
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t {
        Color colorData{r, g, b, a};
        return enqueue(GeoQikMessage{SetMeshColor{colorData}});
      });
}

geoqik_error_code_t geoqik_get_mesh_color(float* r, float* g, float* b, float* a)
{
  if (r == nullptr || g == nullptr || b == nullptr || a == nullptr)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto promise = std::make_shared<std::promise<Color>>();
        std::future<Color> future = promise->get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetMeshColor{
                [promise](Context& context) { promise->set_value(context.get_mesh_color()); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        auto color = future.get();
        *r = color[0];
        *g = color[1];
        *b = color[2];
        *a = color[3];
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_wait_for_exit_and_cleanup()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        auto& renderThread = geoqik_internal::get_render_thread();
        if (renderThread.joinable())
        {
          renderThread.join();
        }

        api_is_initialized_storage().store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_cleanup()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        get_message_queue().try_enqueue(GeoQikMessage{Cleanup{}});

        auto& renderThread = geoqik_internal::get_render_thread();
        if (renderThread.joinable())
        {
          renderThread.join(); // Wait for the render thread to finish
        }

        api_is_initialized_storage().store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
      });
}
