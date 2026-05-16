#include "GeoQik/GeoQik.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "Context.hpp"
#include "Core/UUID.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikSettings.hpp"
#include "WindowSettings.hpp"
#include <atomic>
#include <barrier>
#include <cmath>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using namespace geoqik;

namespace
{
std::atomic<bool> g_apiIsInitialized{false};
}

// Forward declarations for internal C++ functions
namespace geoqik_internal
{
static bool api_is_initialized()
{
  return g_apiIsInitialized.load(std::memory_order_acquire);
}

static geoqik::GeoQikSettings convert_to_cpp_settings(const geoqik_settings_t* c_settings)
{
  geoqik::GeoQikSettings cpp_settings;
  if (c_settings)
  {
    cpp_settings.maxMessageQueueSize = c_settings->maxMessageQueueSize;
    cpp_settings.initialPointCapacity = c_settings->initialPointCapacity;
    cpp_settings.initialLineCapacity = c_settings->initialLineCapacity;
    cpp_settings.capacityGrowthFactor = c_settings->capacityGrowthFactor;
    cpp_settings.defaultPointSize = c_settings->defaultPointSize;
    cpp_settings.defaultLineWidth = c_settings->defaultLineWidth;

    for (size_t i = 0; i < ColorChannelCount; ++i)
    {
      cpp_settings.defaultPointColor[i] = c_settings->defaultPointColor[i];
      cpp_settings.defaultLineColor[i] = c_settings->defaultLineColor[i];
      cpp_settings.backgroundColor[i] = c_settings->backgroundColor[i];
    }

    cpp_settings.cameraFarPlaneMultiplier = c_settings->cameraFarPlaneMultiplier;
    cpp_settings.minGeometryProcessingTime = std::chrono::milliseconds(c_settings->minGeometryProcessingTimeMs);
    cpp_settings.maxFrameProcessingTime = std::chrono::milliseconds(c_settings->maxFrameProcessingTimeMs);
    cpp_settings.updateSceneFrequency = c_settings->updateSceneFrequency;
  }
  return cpp_settings;
}

static geoqik::WindowSettings convert_to_cpp_window_settings(const geoqik_window_settings_t* c_settings)
{
  geoqik::WindowSettings cpp_settings;
  if (c_settings)
  {
    cpp_settings.title = c_settings->title ? c_settings->title : "GeoQik Viewer";
    cpp_settings.width = c_settings->width;
    cpp_settings.height = c_settings->height;
    cpp_settings.red_bits = c_settings->red_bits;
    cpp_settings.green_bits = c_settings->green_bits;
    cpp_settings.blue_bits = c_settings->blue_bits;
    cpp_settings.alpha_bits = c_settings->alpha_bits;
    cpp_settings.depth_bits = c_settings->depth_bits;
    cpp_settings.stencil_bits = c_settings->stencil_bits;
    cpp_settings.accum_red_bits = c_settings->accum_red_bits;
    cpp_settings.accum_green_bits = c_settings->accum_green_bits;
    cpp_settings.accum_blue_bits = c_settings->accum_blue_bits;
    cpp_settings.accum_alpha_bits = c_settings->accum_alpha_bits;
    cpp_settings.aux_buffers = c_settings->aux_buffers;
    cpp_settings.samples = c_settings->samples;
    cpp_settings.refresh_rate = c_settings->refresh_rate;
    cpp_settings.stereo = c_settings->stereo != 0;
    cpp_settings.srgb_capable = c_settings->srgb_capable != 0;
    cpp_settings.double_buffer = c_settings->double_buffer != 0;
    cpp_settings.resizable = c_settings->resizable != 0;
    cpp_settings.visible = c_settings->visible != 0;
    cpp_settings.decorated = c_settings->decorated != 0;
    cpp_settings.focused = c_settings->focused != 0;
    cpp_settings.auto_iconify = c_settings->auto_iconify != 0;
    cpp_settings.floating = c_settings->floating != 0;
    cpp_settings.maximized = c_settings->maximized != 0;
    cpp_settings.center_cursor = c_settings->center_cursor != 0;
    cpp_settings.transparent_framebuffer = c_settings->transparent_framebuffer != 0;
    cpp_settings.focus_on_show = c_settings->focus_on_show != 0;
    cpp_settings.scale_to_monitor = c_settings->scale_to_monitor != 0;
  }
  return cpp_settings;
}

static bool validate_finite_coords(double x, double y, double z)
{
  return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

static bool validate_color(float r, float g, float b, float a)
{
  return r >= 0.0f && r <= 1.0f && g >= 0.0f && g <= 1.0f && b >= 0.0f && b <= 1.0f && a >= 0.0f && a <= 1.0f;
}

static bool validate_color_values(const float* colors, std::size_t colorCount)
{
  if (colorCount == 0)
  {
    return true;
  }
  if (!colors)
  {
    return false;
  }
  for (std::size_t i = 0; i < colorCount; ++i)
  {
    if (colors[i] < 0.0f || colors[i] > 1.0f)
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

static bool convert_replay_options(const geoqik_replay_options_t* options, geoqik::ReplayOptions& replayOptions)
{
  constexpr double defaultEntriesPerSecond = 60.0;
  constexpr double defaultSpeedMultiplier = 1.0;
  constexpr std::size_t defaultMaxEntriesPerFrame = 1024;

  double entriesPerSecond = defaultEntriesPerSecond;
  double speedMultiplier = defaultSpeedMultiplier;
  std::size_t maxEntriesPerFrame = defaultMaxEntriesPerFrame;

  if (options)
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
  }

  const double effectiveEntriesPerSecond = entriesPerSecond * speedMultiplier;
  if (!std::isfinite(effectiveEntriesPerSecond) || effectiveEntriesPerSecond <= 0.0 || maxEntriesPerFrame == 0)
  {
    return false;
  }

  replayOptions.entriesPerSecond = effectiveEntriesPerSecond;
  replayOptions.maxEntriesPerFrame = maxEntriesPerFrame;
  return true;
}

template <typename Func>
static auto execute_with_error_handling(Func&& func) -> std::invoke_result_t<Func>
{
  using ReturnType = std::invoke_result_t<Func>;
  try
  {
    return func();
  }
  catch (const std::bad_alloc&)
  {
    if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
      return GEOQIK_ERROR_MEMORY_ALLOCATION;
    else
      return geoqik_result_t{GEOQIK_ERROR_MEMORY_ALLOCATION, {}};
  }
  catch (...)
  {
    if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>)
      return GEOQIK_ERROR_UNKNOWN;
    else
      return geoqik_result_t{GEOQIK_ERROR_UNKNOWN, {}};
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
            return GEOQIK_ERROR_NOT_INITIALIZED;
          else
            return geoqik_result_t{GEOQIK_ERROR_NOT_INITIALIZED, {}};
        }
        return func();
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
            return GEOQIK_ERROR_ALREADY_INITIALIZED;
          else
            return geoqik_result_t{GEOQIK_ERROR_ALREADY_INITIALIZED, {}};
        }
        return func();
      });
}

static geoqik_error_code_t run_render_thread(const geoqik::GeoQikSettings& geoqikSettings,
                                             const geoqik::WindowSettings& settings,
                                             std::shared_ptr<std::barrier<>> initBarrier)
{
  try
  {
    auto context = std::make_unique<Context>();
    context->init_window(geoqikSettings, settings);

    initBarrier->arrive_and_wait(); // Synchronize with the calling thread, calls to the GeoQik API aren't allowed before this point.

    context->run_event_loop();
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch(const std::exception& e)
  {
    std::cerr << "Exception in GeoQik render thread: " << e.what() << std::endl;
    return GEOQIK_ERROR_UNKNOWN;
  }
  catch (...)
  {
    get_message_queue().clear();
    g_apiIsInitialized.store(false, std::memory_order_release);
    return GEOQIK_ERROR_UNKNOWN;
  }
}

// Idempotent function to start the GeoQik thread
static std::thread& get_render_thread()
{
  static std::thread renderThread;
  return renderThread;
}

static geoqik_error_code_t start_geoqik_thread(const geoqik::GeoQikSettings& geoqikSettings, const geoqik::WindowSettings& settings)
{
  try
  {
    auto& renderThread = get_render_thread();
    if (!renderThread.joinable())
    {
      std::shared_ptr<std::barrier<>> initBarrier = std::make_shared<std::barrier<>>(2);
      renderThread = std::thread(run_render_thread, geoqikSettings, settings, initBarrier);
      initBarrier->arrive_and_wait(); // Ensure the thread is initialized before proceeding
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

static core::UUID convert_to_core_uuid(const geoqik_uuid_t& uuid)
{
  std::array<uint8_t, 16> bytes;
  for (size_t i = 0; i < 16; ++i)
  {
    bytes[i] = uuid.value[i];
  }
  return core::UUID(bytes);
}

static geoqik_uuid_t convert_to_geoqik_uuid(const core::UUID& uuid)
{
  geoqik_uuid_t gUuid;
  auto bytes = uuid.to_array();
  assert(bytes.size() == 16);
  for (size_t i = 0; i < 16; ++i)
  {
    gUuid.value[i] = bytes[i];
  }
  return gUuid;
}

static geoqik_error_code_t enqueue(GeoQikMessage&& message)
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

} // anonymous namespace

geoqik_error_code_t geoqik_init()
{
  return geoqik_internal::execute_if_not_initialized(
      [&]() -> geoqik_error_code_t
      {
        // Use default settings
        geoqik::GeoQikSettings defaultGeoQikSettings;
        geoqik::WindowSettings defaultWindowSettings;

        g_apiIsInitialized.store(true, std::memory_order_release);
        try
        {
          init_message_queue(ConcurrentQueue<GeoQikMessage>(defaultGeoQikSettings.maxMessageQueueSize));
          geoqik_internal::start_geoqik_thread(defaultGeoQikSettings, defaultWindowSettings);
          return GEOQIK_SUCCESS;
        }
        catch (...)
        {
          g_apiIsInitialized.store(false, std::memory_order_release);
          throw;
        }
      });
}

void geoqik_create_default_settings(geoqik_settings_t* settings)
{
  if (!settings)
    return;

  // Initialize with default values matching GeoQikSettings defaults
  settings->maxMessageQueueSize = 1000000;
  settings->initialPointCapacity = 100000;
  settings->initialLineCapacity = 100000;
  settings->capacityGrowthFactor = 2;
  settings->defaultPointSize = 4.0f;
  settings->defaultLineWidth = 2.0f;
  settings->defaultPointColor[0] = 1.0f;
  settings->defaultPointColor[1] = 1.0f;
  settings->defaultPointColor[2] = 1.0f;
  settings->defaultPointColor[3] = 1.0f;
  settings->defaultLineColor[0] = 1.0f;
  settings->defaultLineColor[1] = 1.0f;
  settings->defaultLineColor[2] = 1.0f;
  settings->defaultLineColor[3] = 1.0f;
  settings->backgroundColor[0] = 0.1f;
  settings->backgroundColor[1] = 0.1f;
  settings->backgroundColor[2] = 0.1f;
  settings->backgroundColor[3] = 1.0f;
  settings->cameraFarPlaneMultiplier = 3.0;
  settings->minGeometryProcessingTimeMs = 16;
  settings->maxFrameProcessingTimeMs = 16;
  settings->updateSceneFrequency = 5;
}

void geoqik_init_default_window_settings(geoqik_window_settings_t* settings)
{
  if (!settings)
    return;

  // Initialize with default values matching WindowSettings defaults
  settings->title = "GeoQik Viewer";
  settings->width = 1920;
  settings->height = 1080;
  settings->red_bits = 8;
  settings->green_bits = 8;
  settings->blue_bits = 8;
  settings->alpha_bits = 8;
  settings->depth_bits = 24;
  settings->stencil_bits = 8;
  settings->accum_red_bits = 0;
  settings->accum_green_bits = 0;
  settings->accum_blue_bits = 0;
  settings->accum_alpha_bits = 0;
  settings->aux_buffers = 0;
  settings->samples = 8;
  settings->refresh_rate = -1;
  settings->stereo = 0;
  settings->srgb_capable = 0;
  settings->double_buffer = 1;
  settings->resizable = 1;
  settings->visible = 1;
  settings->decorated = 1;
  settings->focused = 1;
  settings->auto_iconify = 1;
  settings->floating = 0;
  settings->maximized = 0;
  settings->center_cursor = 0;
  settings->transparent_framebuffer = 0;
  settings->focus_on_show = 1;
  settings->scale_to_monitor = 1;
}

geoqik_error_code_t geoqik_init_with_settings(const geoqik_settings_t* geoqikSettings, const geoqik_window_settings_t* windowSettings)
{
  return geoqik_internal::execute_if_not_initialized(
      [&]() -> geoqik_error_code_t
      {
        // Convert C structs to C++ structs
        geoqik::GeoQikSettings cppGeoQikSettings = geoqik_internal::convert_to_cpp_settings(geoqikSettings);
        geoqik::WindowSettings cppWindowSettings = geoqik_internal::convert_to_cpp_window_settings(windowSettings);

        g_apiIsInitialized.store(true, std::memory_order_release);
        try
        {
          init_message_queue(ConcurrentQueue<GeoQikMessage>(cppGeoQikSettings.maxMessageQueueSize));
          geoqik_internal::start_geoqik_thread(cppGeoQikSettings, cppWindowSettings);
          return GEOQIK_SUCCESS;
        }
        catch (...)
        {
          g_apiIsInitialized.store(false, std::memory_order_release);
          throw;
        }
      });
}

geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized)
{
  if (!isInitialized)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
    *isInitialized = geoqik_internal::api_is_initialized() ? true : false;
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
  if (!uuid)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_with_error_handling(
      [&]() -> geoqik_error_code_t
      {
        auto coreUuid = core::UUID::generate().to_array();

        assert(coreUuid.size() == 16);
        for (std::size_t i = 0; i < 16; ++i)
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

  if (options)
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

        if (options)
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
  if (!points || size == 0 || size % 3 != 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  std::vector<float> pointsCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t pointCount = size / 3;

  for (size_t i = 0; i < size; i += 3)
  {
    auto& x = points[i];
    auto& y = points[i + 1];
    auto& z = points[i + 2];
    if (!geoqik_internal::validate_finite_coords(x, y, z))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    pointsCopy[i] = static_cast<float>(x);
    pointsCopy[i + 1] = static_cast<float>(y);
    pointsCopy[i + 2] = static_cast<float>(z);
  }

  if (options && options->color && options->colorCount > 0)
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
  else if (options && options->colorCount > 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options)
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

geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId)
{
  if (!geometryId)
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

  if (options && options->colorCount > 0)
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

        if (options)
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
  if (!lines || size == 0 || size % 6 != 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  // count is the total number of doubles in the array; each line occupies 6 values (x1,y1,z1,x2,y2,z2)
  std::vector<float> linesCopy(size);
  std::vector<float> colorsCopy;
  const std::size_t lineCount = size / 6;

  for (size_t i = 0; i < size; i += 6)
  {
    const size_t base = i;
    const double lx1 = lines[base + 0], ly1 = lines[base + 1], lz1 = lines[base + 2];
    const double lx2 = lines[base + 3], ly2 = lines[base + 4], lz2 = lines[base + 5];
    if (!geoqik_internal::validate_finite_coords(lx1, ly1, lz1) || !geoqik_internal::validate_finite_coords(lx2, ly2, lz2))
    {
      return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
    }
    linesCopy[base + 0] = static_cast<float>(lx1);
    linesCopy[base + 1] = static_cast<float>(ly1);
    linesCopy[base + 2] = static_cast<float>(lz1);
    linesCopy[base + 3] = static_cast<float>(lx2);
    linesCopy[base + 4] = static_cast<float>(ly2);
    linesCopy[base + 5] = static_cast<float>(lz2);
  }

  if (options && options->color && options->colorCount > 0)
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
  else if (options && options->colorCount > 0)
  {
    return geoqik_result_t{GEOQIK_ERROR_INVALID_PARAMETER, {}};
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_result_t
      {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options)
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

geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId)
{
  if (!geometryId)
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

GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_all_geometry()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{RemoveAllGeometry{}}); });
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_translate_geometry(const geoqik_uuid_t* geometryId, double dx, double dy, double dz)
{
  if (!geometryId || !geoqik_internal::validate_finite_coords(dx, dy, dz))
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
  if (!geometryId || !geoqik_internal::validate_finite_coords(centerX, centerY, centerZ) ||
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
  if (!path || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<geoqik_error_code_t> promise;
        std::future<geoqik_error_code_t> future = promise.get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            SaveLog{[&promise, path = std::move(pathCopy), format](Context& context) { promise.set_value(context.save_log(path.c_str(), format)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_load_log(const char* path, geoqik_log_format_t format)
{
  if (!path || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<geoqik_error_code_t> promise;
        std::future<geoqik_error_code_t> future = promise.get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            LoadLog{[&promise, path = std::move(pathCopy), format](Context& context) { promise.set_value(context.load_log(path.c_str(), format)); }}});
        if (enqueueResult != GEOQIK_SUCCESS)
        {
          return enqueueResult;
        }

        return future.get();
      });
}

geoqik_error_code_t geoqik_replay_log(const char* path, geoqik_log_format_t format, const geoqik_replay_options_t* options)
{
  if (!path || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
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
        std::promise<geoqik_error_code_t> promise;
        std::future<geoqik_error_code_t> future = promise.get_future();
        std::string pathCopy(path);

        auto enqueueResult = enqueue(GeoQikMessage{
            ReplayLog{[&promise, path = std::move(pathCopy), format, replayOptions](Context& context)
                      { promise.set_value(context.replay_log(path.c_str(), format, replayOptions)); }}});
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
        std::promise<geoqik_error_code_t> promise;
        std::future<geoqik_error_code_t> future = promise.get_future();

        auto enqueueResult = enqueue(GeoQikMessage{
            ReplayCurrentLog{[&promise, replayOptions](Context& context) { promise.set_value(context.replay_current_log(replayOptions)); }}});
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

geoqik_error_code_t geoqik_set_point_size(float pointSize)
{
  if (pointSize <= 0.0f)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetPointSize{pointSize}}); });
}

geoqik_error_code_t geoqik_get_point_size(float* result)
{
  if (!result)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<float> promise;
        std::future<float> future = promise.get_future();

        auto enqueueResult = enqueue(GeoQikMessage{GetPointSize{[&promise](Context& context) { promise.set_value(context.get_point_size()); }}});
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
  if (!r || !g || !b || !a)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<Color> promise;
        std::future<Color> future = promise.get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetPointColor{[&promise](Context& context) { promise.set_value(context.get_point_color()); }}});
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
  if (lineWidth <= 0.0f)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetLineWidth{lineWidth}}); });
}

geoqik_error_code_t geoqik_get_line_width(float* result)
{
  if (!result)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<float> promise;
        std::future<float> future = promise.get_future();

        auto enqueueResult = enqueue(GeoQikMessage{GetLineWidth{[&promise](Context& context) { promise.set_value(context.get_line_width()); }}});
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
  if (!r || !g || !b || !a)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        std::promise<Color> promise;
        std::future<Color> future = promise.get_future();

        auto enqueueResult =
            enqueue(GeoQikMessage{GetLineColor{[&promise](Context& context) { promise.set_value(context.get_line_color()); }}});
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

        g_apiIsInitialized.store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
      });
}

geoqik_error_code_t geoqik_cleanup()
{
  return geoqik_internal::execute_if_initialized(
      [&]() -> geoqik_error_code_t
      {
        enqueue(GeoQikMessage{Cleanup{}});

        auto& renderThread = geoqik_internal::get_render_thread();
        if (renderThread.joinable())
        {
          renderThread.join(); // Wait for the render thread to finish
        }

        g_apiIsInitialized.store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
      });
}
