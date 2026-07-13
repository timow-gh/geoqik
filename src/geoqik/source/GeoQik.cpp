#include "GeoQik/GeoQik.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "Context.hpp"
#include "Core/UUID.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikSettings.hpp"
#include <plinth/WindowSettings.hpp>
#include <array>
#include <atomic>
#include <cmath>
#include <future>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace geoqik;

namespace {
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

std::atomic<bool>& api_is_initialized_storage() {
    static std::atomic<bool> apiIsInitialized{false}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return apiIsInitialized;
}

enum class ErrorDomain : std::uint8_t {
    Api = 1,
    Renderer = 2,
    Io = 3
};

enum class ApiDiagnosticId : std::uint8_t {
    Success,
    NotInitialized,
    AlreadyInitialized,
    InvalidParameter,
    NullOutputPointer,
    NullInputPointer,
    NonFiniteCoordinate,
    InvalidColorRange,
    WrongColorSize,
    InvalidReplayOptions,
    MemoryAllocation,
    UnknownException,
    RendererInitFailed,
    IoFailure,
    UnsupportedFormat,
    InvalidState
};

class GeoQikErrorCategory : public std::error_category {
  public:
    [[nodiscard]]
    const char* name() const noexcept override {
        return "geoqik";
    }

    [[nodiscard]]
    std::string message(int ev) const override {
        switch (static_cast<ErrorDomain>(static_cast<std::uint8_t>(ev))) {
        case ErrorDomain::Api:      return "api error";
        case ErrorDomain::Renderer: return "renderer error";
        case ErrorDomain::Io:       return "io error";
        default:                    return "unknown geoqik error";
        }
    }
};

const std::error_category& geoqik_error_category() noexcept {
    static const GeoQikErrorCategory instance;
    return instance;
}

struct Diagnostic {
    std::error_code error;
    geoqik_error_code_t code = GEOQIK_SUCCESS;
    std::string operation;
    std::string what;
    std::string why;
    std::string action;
    std::string details;
};

Diagnostic& last_error_storage() {
    thread_local Diagnostic diagnostic; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return diagnostic;
}

[[nodiscard]]
std::error_code make_internal_error(ErrorDomain domain) {
    return {static_cast<int>(domain), geoqik_error_category()};
}

struct ApiDiagnosticEntry {
    ApiDiagnosticId id;
    geoqik_error_code_t code;
    ErrorDomain domain;
    const char* shortMessage;
    const char* what;
    const char* why;
    const char* action;
};

constexpr std::array<ApiDiagnosticEntry, 16> apiDiagnosticCatalog = {
    {{ApiDiagnosticId::Success, GEOQIK_SUCCESS, ErrorDomain::Api, "Success", "", "", ""},
     {ApiDiagnosticId::NotInitialized,
      GEOQIK_ERROR_NOT_INITIALIZED,
      ErrorDomain::Api,
      "GeoQik not initialized",
      "GeoQik is not initialized.",
      "This function requires a successful geoqik_init or geoqik_init_with_settings call first.",
      "Call geoqik_init before using this API."},
     {ApiDiagnosticId::AlreadyInitialized,
      GEOQIK_ERROR_ALREADY_INITIALIZED,
      ErrorDomain::Api,
      "GeoQik already initialized",
      "GeoQik is already initialized.",
      "The initialization API may only be called once before cleanup.",
      "Call geoqik_cleanup before initializing GeoQik again."},
     {ApiDiagnosticId::InvalidParameter,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "One or more parameters are invalid.",
      "The API contract for this function was not satisfied.",
      "Check the details field and pass values that match the documented parameter requirements."},
     {ApiDiagnosticId::NullOutputPointer,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "An output pointer is null.",
      "GeoQik needs a valid pointer to write the requested value.",
      "Pass the address of a variable with the expected type."},
     {ApiDiagnosticId::NullInputPointer,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "An input pointer is null.",
      "GeoQik needs a valid pointer to read the requested value.",
      "Pass a valid pointer or use the documented null/default behavior where supported."},
     {ApiDiagnosticId::NonFiniteCoordinate,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "A coordinate value is not finite.",
      "Geometry coordinates must not be NaN or infinity.",
      "Replace non-finite coordinate values before calling this function."},
     {ApiDiagnosticId::InvalidColorRange,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "A color channel is outside the valid RGBA range.",
      "Each color channel must be between 0.0 and 1.0.",
      "Clamp or normalize RGBA color values before calling this function."},
     {ApiDiagnosticId::WrongColorSize,
      GEOQIK_ERROR_WRONG_COLOR_SIZE,
      ErrorDomain::Api,
      "Wrong RGBA color size",
      "The color array has the wrong size.",
      "Color arrays must contain 0 floats, one RGBA color, or one RGBA color per submitted geometry item.",
      "Set colorCount to 0, 4, or item_count * 4 as appropriate."},
     {ApiDiagnosticId::InvalidReplayOptions,
      GEOQIK_ERROR_INVALID_PARAMETER,
      ErrorDomain::Api,
      "Invalid parameter",
      "The replay options are invalid.",
      "Replay rates, multipliers, counts, and key arrays must be finite and internally consistent.",
      "Use positive finite rates, non-zero counts, and non-null key arrays when key counts are non-zero."},
     {ApiDiagnosticId::MemoryAllocation,
      GEOQIK_ERROR_MEMORY_ALLOCATION,
      ErrorDomain::Api,
      "Memory allocation error",
      "GeoQik could not allocate memory.",
      "The process memory allocator reported an allocation failure.",
      "Reduce the amount of submitted geometry or free memory before retrying."},
     {ApiDiagnosticId::UnknownException,
      GEOQIK_ERROR_UNKNOWN,
      ErrorDomain::Api,
      "Unknown error",
      "GeoQik failed unexpectedly.",
      "An internal operation failed without a more specific mapped error code.",
      "Check the details field and report the call sequence if this is reproducible."},
     {ApiDiagnosticId::RendererInitFailed,
      GEOQIK_ERROR_RENDERER_INIT_FAILED,
      ErrorDomain::Renderer,
      "Renderer initialization failed",
      "GeoQik could not initialize the renderer window.",
      "The renderer or windowing backend failed while creating the viewer context.",
      "Check that a graphics context is available and that the requested window settings are supported."},
     {ApiDiagnosticId::IoFailure,
      GEOQIK_ERROR_IO,
      ErrorDomain::Io,
      "I/O error",
      "GeoQik failed to read or write a file.",
      "The file operation failed in the render thread.",
      "Check that the path exists, permissions are correct, and the file is a valid GeoQik binary log when reading."},
     {ApiDiagnosticId::UnsupportedFormat,
      GEOQIK_ERROR_UNSUPPORTED_FORMAT,
      ErrorDomain::Io,
      "Unsupported format",
      "The requested format is not supported.",
      "GeoQik supports GEOQIK_LOG_FORMAT_BINARY and GEOQIK_LOG_FORMAT_JSON.",
      "Pass GEOQIK_LOG_FORMAT_BINARY or GEOQIK_LOG_FORMAT_JSON."},
     {ApiDiagnosticId::InvalidState,
      GEOQIK_ERROR_INVALID_STATE,
      ErrorDomain::Api,
      "Invalid state",
      "GeoQik is not in the required state for this operation.",
      "The requested operation conflicts with the current API state.",
      "Complete or cancel the current operation before retrying."}}};

const ApiDiagnosticEntry& api_entry(ApiDiagnosticId id) {
    for (const ApiDiagnosticEntry& entry: apiDiagnosticCatalog) {
        if (entry.id == id) {
            return entry;
        }
    }
    return apiDiagnosticCatalog[0];
}

const ApiDiagnosticEntry* api_entry_for_code(geoqik_error_code_t code) {
    for (const ApiDiagnosticEntry& entry: apiDiagnosticCatalog) {
        if (entry.code == code) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

// Forward declarations for internal C++ functions
namespace geoqik_internal {
static void clear_last_error() noexcept {
    try {
        last_error_storage() = Diagnostic{};
    } catch (...) {}
}

static void set_last_error(ApiDiagnosticId id, const char* operation, const char* details = "") {
    const ApiDiagnosticEntry& entry = api_entry(id);
    try {
        Diagnostic diagnostic;
        diagnostic.error = make_internal_error(entry.domain);
        diagnostic.code = entry.code;
        diagnostic.operation = operation != nullptr ? operation : "";
        diagnostic.what = entry.what;
        diagnostic.why = entry.why;
        diagnostic.action = entry.action;
        diagnostic.details = details != nullptr ? details : "";
        last_error_storage() = std::move(diagnostic);
    } catch (...) {
        Diagnostic& diagnostic = last_error_storage();
        diagnostic.error = make_internal_error(ErrorDomain::Api);
        diagnostic.code = entry.code;
        diagnostic.operation = "geoqik error reporting";
        diagnostic.what = "Diagnostic unavailable";
        diagnostic.why = "GeoQik could not allocate memory for the detailed diagnostic.";
        diagnostic.action = "Use the numeric error code and short error string.";
        diagnostic.details.clear();
    }
}

static ApiDiagnosticId diagnostic_id_for_code(geoqik_error_code_t code) {
    if (const ApiDiagnosticEntry* entry = api_entry_for_code(code); entry != nullptr) {
        return entry->id;
    }
    return ApiDiagnosticId::UnknownException;
}

static geoqik_error_code_t fail(ApiDiagnosticId id, const char* operation, const char* details = "") {
    const ApiDiagnosticEntry& entry = api_entry(id);
    set_last_error(id, operation, details);
    return entry.code;
}

static geoqik_result_t fail_result(ApiDiagnosticId id, const char* operation, const char* details = "") {
    return geoqik_result_t{fail(id, operation, details), {}};
}

static geoqik_error_code_t complete(geoqik_error_code_t code, const char* operation, const char* details = "") {
    if (code == GEOQIK_SUCCESS) {
        clear_last_error();
        return code;
    }

    set_last_error(diagnostic_id_for_code(code), operation, details);
    return code;
}

static geoqik_result_t complete_result(geoqik_result_t result, const char* operation, const char* details = "") {
    result.err = complete(result.err, operation, details);
    return result;
}

static geoqik_error_code_t invalid_parameter(const char* operation, const char* details) {
    return fail(ApiDiagnosticId::InvalidParameter, operation, details);
}

static geoqik_result_t invalid_parameter_result(const char* operation, const char* details) {
    return fail_result(ApiDiagnosticId::InvalidParameter, operation, details);
}

static geoqik_error_code_t wrong_color_size(const char* operation, const char* details) {
    return fail(ApiDiagnosticId::WrongColorSize, operation, details);
}

static geoqik_result_t wrong_color_size_result(const char* operation, const char* details) {
    return fail_result(ApiDiagnosticId::WrongColorSize, operation, details);
}

static geoqik_settings_t create_default_c_settings() {
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

static geoqik_window_settings_t create_default_c_window_settings() {
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

static bool api_is_initialized() {
    return api_is_initialized_storage().load(std::memory_order_acquire);
}

static geoqik::GeoQikSettings convert_to_cpp_settings(const geoqik_settings_t& cSettings) {
    geoqik::GeoQikSettings cppSettings;
    cppSettings.maxMessageQueueSize = cSettings.maxMessageQueueSize;
    cppSettings.initialPointCapacity = cSettings.initialPointCapacity;
    cppSettings.initialLineCapacity = cSettings.initialLineCapacity;
    cppSettings.initialMeshCapacity = cSettings.initialMeshCapacity;
    cppSettings.defaultMeshColor = Color{cSettings.defaultMeshColor[0],
                                         cSettings.defaultMeshColor[1],
                                         cSettings.defaultMeshColor[2],
                                         cSettings.defaultMeshColor[3]};
    for (size_t i = 0; i < 3; ++i) {
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

    for (size_t i = 0; i < ColorChannelCount; ++i) {
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

static geoqik::GeoQikSettings convert_to_cpp_settings(const geoqik_settings_t* cSettings) {
    if (cSettings != nullptr) {
        return convert_to_cpp_settings(*cSettings);
    }
    return convert_to_cpp_settings(create_default_c_settings());
}

static renderer::WindowSettings convert_to_cpp_window_settings(const geoqik_window_settings_t& cSettings) {
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

static renderer::WindowSettings convert_to_cpp_window_settings(const geoqik_window_settings_t* cSettings) {
    if (cSettings != nullptr) {
        return convert_to_cpp_window_settings(*cSettings);
    }
    return convert_to_cpp_window_settings(create_default_c_window_settings());
}

static bool validate_finite_coords(double x, double y, double z) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

static bool validate_color(float r, float g, float b, float a) {
    return r >= 0.0F && r <= 1.0F && g >= 0.0F && g <= 1.0F && b >= 0.0F && b <= 1.0F && a >= 0.0F && a <= 1.0F;
}

static bool validate_color_values(const float* colors, std::size_t colorCount) {
    if (colorCount == 0) {
        return true;
    }
    if (colors == nullptr) {
        return false;
    }
    for (std::size_t i = 0; i < colorCount; ++i) {
        if (colors[i] < 0.0F || colors[i] > 1.0F) {
            return false;
        }
    }
    return true;
}

static bool validate_point_color_count(std::size_t colorCount, std::size_t pointCount) {
    return colorCount == 0 || colorCount == ColorChannelCount || colorCount == pointCount * ColorChannelCount;
}

static bool validate_line_color_count(std::size_t colorCount, std::size_t lineCount) {
    return colorCount == 0 || colorCount == ColorChannelCount || colorCount == lineCount * ColorChannelCount;
}

static bool convert_replay_keys(const geoqik_key_t* cKeys,
                                std::size_t cKeyCount,
                                std::initializer_list<geoqik::Key> defaultKeys,
                                std::vector<geoqik::Key>& replayKeys) {
    replayKeys.clear();
    if (cKeyCount == 0) {
        replayKeys.assign(defaultKeys.begin(), defaultKeys.end());
        return true;
    }

    if (cKeys == nullptr) {
        return false;
    }

    replayKeys.reserve(cKeyCount);
    for (std::size_t i = 0; i < cKeyCount; ++i) {
        if (static_cast<int>(cKeys[i]) <= static_cast<int>(GEOQIK_KEY_UNKNOWN)) {
            return false;
        }
        replayKeys.push_back(static_cast<geoqik::Key>(static_cast<int>(cKeys[i])));
    }
    return true;
}

static bool read_replay_option_keys(const geoqik_replay_options_t* options, geoqik::ReplayOptions& replayOptions) {
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
           convert_replay_keys(resumeKeys, resumeKeyCount, {geoqik::Key::KEY_SPACE}, replayOptions.resumeKeys) &&
           convert_replay_keys(pauseKeys, pauseKeyCount, {geoqik::Key::KEY_SPACE}, replayOptions.pauseKeys) &&
           convert_replay_keys(increaseKeys,
                               increaseKeyCount,
                               {geoqik::Key::KEY_UP, geoqik::Key::KEY_W},
                               replayOptions.increaseEntriesPerStepKeys) &&
           convert_replay_keys(decreaseKeys,
                               decreaseKeyCount,
                               {geoqik::Key::KEY_DOWN, geoqik::Key::KEY_S},
                               replayOptions.decreaseEntriesPerStepKeys);
}

static bool convert_replay_options(const geoqik_replay_options_t* options, geoqik::ReplayOptions& replayOptions) {
    constexpr double defaultEntriesPerSecond = 60.0;
    constexpr double defaultSpeedMultiplier = 1.0;
    constexpr std::size_t defaultMaxEntriesPerFrame = 1024;
    constexpr std::size_t defaultEntriesPerStep = 1;

    double entriesPerSecond = defaultEntriesPerSecond;
    double speedMultiplier = defaultSpeedMultiplier;
    std::size_t maxEntriesPerFrame = defaultMaxEntriesPerFrame;
    bool startPaused = false;
    std::size_t entriesPerStep = defaultEntriesPerStep;

    if (options != nullptr) {
        if (options->entriesPerSecond != 0.0) {
            if (!std::isfinite(options->entriesPerSecond) || options->entriesPerSecond < 0.0) {
                return false;
            }
            entriesPerSecond = options->entriesPerSecond;
        }

        if (options->speedMultiplier != 0.0) {
            if (!std::isfinite(options->speedMultiplier) || options->speedMultiplier < 0.0) {
                return false;
            }
            speedMultiplier = options->speedMultiplier;
        }

        if (options->maxEntriesPerFrame != 0) {
            maxEntriesPerFrame = options->maxEntriesPerFrame;
        }

        startPaused = options->startPaused != 0;

        if (options->entriesPerStep != 0) {
            entriesPerStep = options->entriesPerStep;
        }
    }

    if (entriesPerStep == 0 || maxEntriesPerFrame == 0) {
        return false;
    }

    const double effectiveEntriesPerSecond = entriesPerSecond * speedMultiplier;
    if (!std::isfinite(effectiveEntriesPerSecond) || effectiveEntriesPerSecond <= 0.0) {
        return false;
    }

    replayOptions.entriesPerSecond = effectiveEntriesPerSecond;
    replayOptions.maxEntriesPerFrame = maxEntriesPerFrame;
    replayOptions.startPaused = startPaused;
    replayOptions.entriesPerStep = entriesPerStep;

    return read_replay_option_keys(options, replayOptions);
}

template <typename Func>
static auto execute_with_error_handling(Func&& func, const char* operation = "GeoQik API call")
    -> std::invoke_result_t<Func> {
    using ReturnType = std::invoke_result_t<Func>;
    try {
        auto result = std::forward<Func>(func)();
        if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
            if (result != GEOQIK_SUCCESS && last_error_storage().code == result) {
                return result;
            }
            return complete(result, operation);
        } else {
            if (result.err != GEOQIK_SUCCESS && last_error_storage().code == result.err) {
                return result;
            }
            return complete_result(result, operation);
        }
    } catch (const std::bad_alloc&) {
        if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
            return fail(ApiDiagnosticId::MemoryAllocation, operation);
        } else {
            return fail_result(ApiDiagnosticId::MemoryAllocation, operation);
        }
    } catch (const std::exception& e) {
        if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
            return fail(ApiDiagnosticId::UnknownException, operation, e.what());
        } else {
            return fail_result(ApiDiagnosticId::UnknownException, operation, e.what());
        }
    } catch (...) {
        if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
            return fail(ApiDiagnosticId::UnknownException, operation);
        } else {
            return fail_result(ApiDiagnosticId::UnknownException, operation);
        }
    }
}

template <typename Func>
static auto execute_if_initialized(Func&& func, const char* operation = "GeoQik API call")
    -> std::invoke_result_t<Func> {
    using ReturnType = std::invoke_result_t<Func>;
    return execute_with_error_handling(
        [&]() -> ReturnType {
            if (!api_is_initialized()) {
                if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
                    return fail(ApiDiagnosticId::NotInitialized, operation);
                } else {
                    return fail_result(ApiDiagnosticId::NotInitialized, operation);
                }
            }
            return std::forward<Func>(func)();
        },
        operation);
}

template <typename Func>
static auto execute_if_not_initialized(Func&& func, const char* operation = "GeoQik initialization")
    -> std::invoke_result_t<Func> {
    using ReturnType = std::invoke_result_t<Func>;
    return execute_with_error_handling(
        [&]() -> ReturnType {
            if (api_is_initialized()) {
                if constexpr (std::is_same_v<ReturnType, geoqik_error_code_t>) {
                    return fail(ApiDiagnosticId::AlreadyInitialized, operation);
                } else {
                    return fail_result(ApiDiagnosticId::AlreadyInitialized, operation);
                }
            }
            return std::forward<Func>(func)();
        },
        operation);
}

static geoqik_error_code_t run_render_thread(const geoqik::GeoQikSettings& geoqikSettings,
                                             const renderer::WindowSettings& settings,
                                             const std::shared_ptr<std::promise<geoqik_error_code_t>>& initResult) {
    auto setInitResult = [&initResult](geoqik_error_code_t result) {
        try {
            initResult->set_value(result);
        } catch (const std::future_error&) {}
    };

    try {
        auto context = std::make_unique<Context>();
        if (!context->init_window(geoqikSettings, settings)) {
            setInitResult(GEOQIK_ERROR_RENDERER_INIT_FAILED);
            return fail(ApiDiagnosticId::RendererInitFailed, "geoqik_init", "context->init_window returned false");
        }

        setInitResult(GEOQIK_SUCCESS);
        context->run_event_loop();
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        get_message_queue().clear();
        api_is_initialized_storage().store(false, std::memory_order_release);
        setInitResult(GEOQIK_ERROR_MEMORY_ALLOCATION);
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (const std::exception& e) {
        std::cerr << "Exception in GeoQik render thread: " << e.what() << '\n';
        get_message_queue().clear();
        api_is_initialized_storage().store(false, std::memory_order_release);
        setInitResult(GEOQIK_ERROR_RENDERER_INIT_FAILED);
        return fail(ApiDiagnosticId::RendererInitFailed, "geoqik_init", e.what());
    } catch (...) {
        get_message_queue().clear();
        api_is_initialized_storage().store(false, std::memory_order_release);
        setInitResult(GEOQIK_ERROR_RENDERER_INIT_FAILED);
        return fail(ApiDiagnosticId::RendererInitFailed, "geoqik_init", "non-standard exception in render thread");
    }
}

// Idempotent function to start the GeoQik thread
static std::thread& get_render_thread() {
    static std::thread renderThread;
    return renderThread;
}

static geoqik_error_code_t start_geoqik_thread(const geoqik::GeoQikSettings& geoqikSettings,
                                               const renderer::WindowSettings& settings) {
    try {
        auto& renderThread = get_render_thread();
        if (!renderThread.joinable()) {
            auto initPromise = std::make_shared<std::promise<geoqik_error_code_t>>();
            std::future<geoqik_error_code_t> initFuture = initPromise->get_future();
            renderThread = std::thread(run_render_thread, geoqikSettings, settings, initPromise);

            const geoqik_error_code_t initResult = initFuture.get();
            if (initResult != GEOQIK_SUCCESS) {
                if (renderThread.joinable()) {
                    renderThread.join();
                }
                if (initResult == GEOQIK_ERROR_RENDERER_INIT_FAILED) {
                    return fail(ApiDiagnosticId::RendererInitFailed,
                                "geoqik_init",
                                "renderer thread reported startup failure");
                }
                return initResult;
            }
        }
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        throw;
    }
}

} // namespace geoqik_internal

// Helper functions for UUID conversion
namespace {

core::UUID convert_to_core_uuid(const geoqik_uuid_t& uuid) {
    std::array<uint8_t, uuidByteCount> bytes{};
    for (size_t i = 0; i < uuidByteCount; ++i) {
        bytes[i] = uuid.value[i];
    }
    return core::UUID(bytes);
}

geoqik_uuid_t convert_to_geoqik_uuid(const core::UUID& uuid) {
    geoqik_uuid_t gUuid{};
    auto bytes = uuid.to_array();
    assert(bytes.size() == uuidByteCount);
    for (size_t i = 0; i < uuidByteCount; ++i) {
        gUuid.value[i] = bytes[i];
    }
    return gUuid;
}

geoqik_error_code_t enqueue(GeoQikMessage&& message) {
    try {
        get_message_queue().enqueue(std::move(message));
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

} // namespace

geoqik_error_code_t geoqik_init() {
    return geoqik_internal::execute_if_not_initialized(
        [&]() -> geoqik_error_code_t {
            geoqik::GeoQikSettings defaultGeoQikSettings = geoqik_internal::convert_to_cpp_settings(nullptr);
            renderer::WindowSettings defaultWindowSettings = geoqik_internal::convert_to_cpp_window_settings(nullptr);

            api_is_initialized_storage().store(true, std::memory_order_release);
            try {
                init_message_queue(ConcurrentQueue<GeoQikMessage>(defaultGeoQikSettings.maxMessageQueueSize));
                geoqik_error_code_t result =
                    geoqik_internal::start_geoqik_thread(defaultGeoQikSettings, defaultWindowSettings);
                if (result != GEOQIK_SUCCESS) {
                    api_is_initialized_storage().store(false, std::memory_order_release);
                }
                return result;
            } catch (...) {
                api_is_initialized_storage().store(false, std::memory_order_release);
                throw;
            }
        },
        "geoqik_init");
}

void geoqik_create_default_settings(geoqik_settings_t* settings) {
    if (settings == nullptr) {
        return;
    }

    *settings = geoqik_internal::create_default_c_settings();
}

void geoqik_init_default_window_settings(geoqik_window_settings_t* settings) {
    if (settings == nullptr) {
        return;
    }

    *settings = geoqik_internal::create_default_c_window_settings();
}

geoqik_error_code_t geoqik_init_with_settings(const geoqik_settings_t* geoqikSettings,
                                              const geoqik_window_settings_t* windowSettings) {
    return geoqik_internal::execute_if_not_initialized(
        [&]() -> geoqik_error_code_t {
            // Convert C structs to C++ structs
            geoqik::GeoQikSettings cppGeoQikSettings = geoqik_internal::convert_to_cpp_settings(geoqikSettings);
            renderer::WindowSettings cppWindowSettings =
                geoqik_internal::convert_to_cpp_window_settings(windowSettings);

            api_is_initialized_storage().store(true, std::memory_order_release);
            try {
                init_message_queue(ConcurrentQueue<GeoQikMessage>(cppGeoQikSettings.maxMessageQueueSize));
                geoqik_error_code_t result = geoqik_internal::start_geoqik_thread(cppGeoQikSettings, cppWindowSettings);
                if (result != GEOQIK_SUCCESS) {
                    api_is_initialized_storage().store(false, std::memory_order_release);
                }
                return result;
            } catch (...) {
                api_is_initialized_storage().store(false, std::memory_order_release);
                throw;
            }
        },
        "geoqik_init_with_settings");
}

geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized) {
    if (isInitialized == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer,
                                     "geoqik_is_api_initialized",
                                     "parameter: isInitialized");
    }

    return geoqik_internal::execute_with_error_handling(
        [&]() -> geoqik_error_code_t {
            *isInitialized = geoqik_internal::api_is_initialized();
            return GEOQIK_SUCCESS;
        },
        "geoqik_is_api_initialized");
}

const char* geoqik_get_error_string(geoqik_error_code_t result) {
    if (const ApiDiagnosticEntry* entry = api_entry_for_code(result); entry != nullptr) {
        return entry->shortMessage;
    }
    return "Invalid error code";
}

geoqik_error_code_t geoqik_get_last_error_info(geoqik_error_info_t* info) {
    if (info == nullptr || info->struct_size < sizeof(geoqik_error_info_t)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    const Diagnostic& diagnostic = last_error_storage();
    info->code = diagnostic.code;
    info->operation = diagnostic.operation.c_str();
    info->what = diagnostic.what.c_str();
    info->why = diagnostic.why.c_str();
    info->action = diagnostic.action.c_str();
    info->details = diagnostic.details.c_str();
    return GEOQIK_SUCCESS;
}

void geoqik_clear_last_error() {
    geoqik_internal::clear_last_error();
}

geoqik_error_code_t geoqik_generate_uuid(geoqik_uuid_t* uuid) {
    if (uuid == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer, "geoqik_generate_uuid", "parameter: uuid");
    }

    return geoqik_internal::execute_with_error_handling(
        [&]() -> geoqik_error_code_t {
            auto coreUuid = core::UUID::generate().to_array();

            assert(coreUuid.size() == uuidByteCount);
            for (std::size_t i = 0; i < uuidByteCount; ++i) {
                uuid->value[i] = coreUuid[i];
            }
            return GEOQIK_SUCCESS;
        },
        "geoqik_generate_uuid");
}

geoqik_result_t geoqik_add_point(double x, double y, double z) {
    return geoqik_add_point_opts(x, y, z, nullptr);
}

geoqik_result_t geoqik_add_point_with_color(double x, double y, double z, float r, float g, float b, float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                            "geoqik_add_point_with_color",
                                            "parameters: r, g, b, a");
    }

    const float color[ColorChannelCount] = {r, g, b, a};
    geoqik_add_points_options_t opts{};
    opts.color = color;
    opts.colorCount = ColorChannelCount;
    return geoqik_add_point_opts(x, y, z, &opts);
}

geoqik_result_t geoqik_add_point_opts(double x, double y, double z, geoqik_add_points_options_t* options) {
    if (!geoqik_internal::validate_finite_coords(x, y, z)) {
        return geoqik_internal::fail_result(ApiDiagnosticId::NonFiniteCoordinate,
                                            "geoqik_add_point_opts",
                                            "parameters: x, y, z");
    }

    std::vector<float> colorsCopy;

    if (options != nullptr) {
        if (!geoqik_internal::validate_point_color_count(options->colorCount, 1)) {
            return geoqik_internal::wrong_color_size_result("geoqik_add_point_opts",
                                                            "parameter: options->colorCount; expected 0 or 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                                "geoqik_add_point_opts",
                                                "parameter: options->color");
        }
        if (options->colorCount > 0) {
            colorsCopy.assign(options->color, options->color + options->colorCount);
        }
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_result_t {
            core::UUID reqId = core::UUID::generate();
            GeoQikMessageCommonData commonData;
            commonData.geometryId = reqId;

            if (options != nullptr) {
                if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil()) {
                    commonData.idempotencyId = idem;
                }

                commonData.rgba = std::move(colorsCopy);
            }

            auto enqueueResult = enqueue(GeoQikMessage{AddPointWithOpts{static_cast<float>(x),
                                                                        static_cast<float>(y),
                                                                        static_cast<float>(z),
                                                                        std::move(commonData)}});
            return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
        },
        "geoqik_add_point_opts");
}

geoqik_result_t geoqik_add_points_opts(const double* points, size_t size, geoqik_add_points_options_t* options) {
    if (points == nullptr || size == 0 || size % coordinateCount != 0) {
        return geoqik_internal::invalid_parameter_result(
            "geoqik_add_points_opts",
            "parameters: points, size; expected non-null points and size multiple of 3");
    }

    std::vector<float> pointsCopy(size);
    std::vector<float> colorsCopy;
    const std::size_t pointCount = size / coordinateCount;

    for (size_t i = 0; i < size; i += coordinateCount) {
        const double& x = points[i];
        const double& y = points[i + 1];
        const double& z = points[i + 2];
        if (!geoqik_internal::validate_finite_coords(x, y, z)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::NonFiniteCoordinate,
                                                "geoqik_add_points_opts",
                                                "parameter: points");
        }
        pointsCopy[i] = static_cast<float>(x);
        pointsCopy[i + 1] = static_cast<float>(y);
        pointsCopy[i + 2] = static_cast<float>(z);
    }

    if (options != nullptr && options->color != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_point_color_count(options->colorCount, pointCount)) {
            return geoqik_internal::wrong_color_size_result(
                "geoqik_add_points_opts",
                "parameter: options->colorCount; expected 0, 4, or point_count * 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                                "geoqik_add_points_opts",
                                                "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    } else if (options != nullptr && options->colorCount > 0) {
        return geoqik_internal::fail_result(ApiDiagnosticId::NullInputPointer,
                                            "geoqik_add_points_opts",
                                            "parameter: options->color; colorCount is non-zero");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_result_t {
            core::UUID reqId = core::UUID::generate();
            GeoQikMessageCommonData commonData;
            commonData.geometryId = reqId;

            if (options != nullptr) {
                if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil()) {
                    commonData.idempotencyId = idem;
                }

                commonData.rgba = std::move(colorsCopy);
            }

            auto enqueueResult =
                enqueue(GeoQikMessage{AddPointsWithOpts{std::move(pointsCopy), std::move(commonData)}});
            return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
        },
        "geoqik_add_points_opts");
}

geoqik_error_code_t geoqik_update_point(const geoqik_uuid_t* geometryId, double x, double y, double z) {
    return geoqik_update_point_opts(geometryId, x, y, z, nullptr);
}

geoqik_error_code_t geoqik_update_point_with_color(const geoqik_uuid_t* geometryId,
                                                   double x,
                                                   double y,
                                                   double z,
                                                   float r,
                                                   float g,
                                                   float b,
                                                   float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_update_point_with_color",
                                     "parameters: r, g, b, a");
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
                                             geoqik_update_points_options_t* options) {
    if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(x, y, z)) {
        return geoqik_internal::invalid_parameter(
            "geoqik_update_point_opts",
            "parameters: geometryId, x, y, z; expected non-null geometryId and finite coordinates");
    }

    std::vector<float> colorsCopy;
    if (options != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_point_color_count(options->colorCount, 1)) {
            return geoqik_internal::wrong_color_size("geoqik_update_point_opts",
                                                     "parameter: options->colorCount; expected 0 or 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                         "geoqik_update_point_opts",
                                         "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{UpdatePointWithOpts{handle,
                                                             static_cast<float>(x),
                                                             static_cast<float>(y),
                                                             static_cast<float>(z),
                                                             std::move(colorsCopy)}});
        },
        "geoqik_update_point_opts");
}

geoqik_error_code_t geoqik_update_points_opts(const geoqik_uuid_t* geometryId,
                                              const double* points,
                                              size_t size,
                                              geoqik_update_points_options_t* options) {
    if (geometryId == nullptr || points == nullptr || size == 0 || size % coordinateCount != 0) {
        return geoqik_internal::invalid_parameter(
            "geoqik_update_points_opts",
            "parameters: geometryId, points, size; expected non-null pointers and size multiple of 3");
    }

    std::vector<float> pointsCopy(size);
    std::vector<float> colorsCopy;
    const std::size_t pointCount = size / coordinateCount;

    for (std::size_t i = 0; i < size; i += coordinateCount) {
        const double px = points[i + 0];
        const double py = points[i + 1];
        const double pz = points[i + 2];
        if (!geoqik_internal::validate_finite_coords(px, py, pz)) {
            return geoqik_internal::fail(ApiDiagnosticId::NonFiniteCoordinate,
                                         "geoqik_update_points_opts",
                                         "parameter: points");
        }
        pointsCopy[i + 0] = static_cast<float>(px);
        pointsCopy[i + 1] = static_cast<float>(py);
        pointsCopy[i + 2] = static_cast<float>(pz);
    }

    if (options != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_point_color_count(options->colorCount, pointCount)) {
            return geoqik_internal::wrong_color_size(
                "geoqik_update_points_opts",
                "parameter: options->colorCount; expected 0, 4, or point_count * 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                         "geoqik_update_points_opts",
                                         "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{UpdatePointsWithOpts{handle, std::move(pointsCopy), std::move(colorsCopy)}});
        },
        "geoqik_update_points_opts");
}

geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullInputPointer, "geoqik_remove_point", "parameter: geometryId");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{RemovePoint{handle}});
        },
        "geoqik_remove_point");
}

geoqik_error_code_t geoqik_add_line(double x1, double y1, double z1, double x2, double y2, double z2) {
    return geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, nullptr).err;
}

geoqik_error_code_t geoqik_add_line_with_color(double x1,
                                               double y1,
                                               double z1,
                                               double x2,
                                               double y2,
                                               double z2,
                                               float r,
                                               float g,
                                               float b,
                                               float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_add_line_with_color",
                                     "parameters: r, g, b, a");
    }

    const float color[ColorChannelCount] = {r, g, b, a};
    geoqik_add_line_opts_t opts{};
    opts.color = color;
    opts.colorCount = ColorChannelCount;
    return geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, &opts).err;
}

geoqik_result_t geoqik_add_line_opts(double x1,
                                     double y1,
                                     double z1,
                                     double x2,
                                     double y2,
                                     double z2,
                                     geoqik_add_line_opts_t* options) {
    if (!geoqik_internal::validate_finite_coords(x1, y1, z1) || !geoqik_internal::validate_finite_coords(x2, y2, z2)) {
        return geoqik_internal::fail_result(ApiDiagnosticId::NonFiniteCoordinate,
                                            "geoqik_add_line_opts",
                                            "parameters: x1, y1, z1, x2, y2, z2");
    }

    std::vector<float> colorsCopy;

    if (options != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_line_color_count(options->colorCount, 1)) {
            return geoqik_internal::wrong_color_size_result("geoqik_add_line_opts",
                                                            "parameter: options->colorCount; expected 0 or 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                                "geoqik_add_line_opts",
                                                "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_result_t {
            core::UUID reqId = core::UUID::generate();
            GeoQikMessageCommonData commonData;
            commonData.geometryId = reqId;

            if (options != nullptr) {
                if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil()) {
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
        },
        "geoqik_add_line_opts");
}

geoqik_result_t geoqik_add_lines_opts(const double* lines, size_t size, geoqik_add_line_opts_t* options) {
    if (lines == nullptr || size == 0 || size % lineCoordinateCount != 0) {
        return geoqik_internal::invalid_parameter_result(
            "geoqik_add_lines_opts",
            "parameters: lines, size; expected non-null lines and size multiple of 6");
    }

    // size is the total number of doubles in the array; each line occupies 6 values (x1,y1,z1,x2,y2,z2)
    std::vector<float> linesCopy(size);
    std::vector<float> colorsCopy;
    const std::size_t lineCount = size / lineCoordinateCount;

    for (size_t i = 0; i < size; i += lineCoordinateCount) {
        const size_t base = i;
        const double lx1 = lines[base + 0];
        const double ly1 = lines[base + 1];
        const double lz1 = lines[base + 2];
        const double lx2 = lines[base + 3];
        const double ly2 = lines[base + 4];
        const double lz2 = lines[base + lineCoordinateCount - 1];
        if (!geoqik_internal::validate_finite_coords(lx1, ly1, lz1) ||
            !geoqik_internal::validate_finite_coords(lx2, ly2, lz2)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::NonFiniteCoordinate,
                                                "geoqik_add_lines_opts",
                                                "parameter: lines");
        }
        linesCopy[base + 0] = static_cast<float>(lx1);
        linesCopy[base + 1] = static_cast<float>(ly1);
        linesCopy[base + 2] = static_cast<float>(lz1);
        linesCopy[base + 3] = static_cast<float>(lx2);
        linesCopy[base + 4] = static_cast<float>(ly2);
        linesCopy[base + lineCoordinateCount - 1] = static_cast<float>(lz2);
    }

    if (options != nullptr && options->color != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_line_color_count(options->colorCount, lineCount)) {
            return geoqik_internal::wrong_color_size_result(
                "geoqik_add_lines_opts",
                "parameter: options->colorCount; expected 0, 4, or line_count * 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                                "geoqik_add_lines_opts",
                                                "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    } else if (options != nullptr && options->colorCount > 0) {
        return geoqik_internal::fail_result(ApiDiagnosticId::NullInputPointer,
                                            "geoqik_add_lines_opts",
                                            "parameter: options->color; colorCount is non-zero");
    }

    return geoqik_internal::execute_if_initialized([&]() -> geoqik_result_t {
        core::UUID reqId = core::UUID::generate();
        GeoQikMessageCommonData commonData;
        commonData.geometryId = reqId;

        if (options != nullptr) {
            if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil()) {
                commonData.idempotencyId = idem;
            }

            commonData.rgba = std::move(colorsCopy);
        }

        auto enqueueResult = enqueue(GeoQikMessage{AddLinesWithOpts{std::move(linesCopy), std::move(commonData)}});
        return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
    });
}

geoqik_error_code_t
geoqik_update_line(const geoqik_uuid_t* geometryId, double x1, double y1, double z1, double x2, double y2, double z2) {
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
                                                  float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_update_line_with_color",
                                     "parameters: r, g, b, a");
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
                                            geoqik_update_line_opts_t* options) {
    if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(x1, y1, z1) ||
        !geoqik_internal::validate_finite_coords(x2, y2, z2)) {
        return geoqik_internal::invalid_parameter(
            "geoqik_update_line_opts",
            "parameters: geometryId, x1, y1, z1, x2, y2, z2; expected non-null geometryId and finite coordinates");
    }

    std::vector<float> colorsCopy;
    if (options != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_line_color_count(options->colorCount, 1)) {
            return geoqik_internal::wrong_color_size("geoqik_update_line_opts",
                                                     "parameter: options->colorCount; expected 0 or 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                         "geoqik_update_line_opts",
                                         "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{UpdateLineWithOpts{handle,
                                                            static_cast<float>(x1),
                                                            static_cast<float>(y1),
                                                            static_cast<float>(z1),
                                                            static_cast<float>(x2),
                                                            static_cast<float>(y2),
                                                            static_cast<float>(z2),
                                                            std::move(colorsCopy)}});
        },
        "geoqik_update_line_opts");
}

geoqik_error_code_t geoqik_update_lines_opts(const geoqik_uuid_t* geometryId,
                                             const double* lines,
                                             size_t size,
                                             geoqik_update_line_opts_t* options) {
    if (geometryId == nullptr || lines == nullptr || size == 0 || size % lineCoordinateCount != 0) {
        return geoqik_internal::invalid_parameter(
            "geoqik_update_lines_opts",
            "parameters: geometryId, lines, size; expected non-null pointers and size multiple of 6");
    }

    std::vector<float> linesCopy(size);
    std::vector<float> colorsCopy;
    const std::size_t lineCount = size / lineCoordinateCount;

    for (std::size_t i = 0; i < size; i += lineCoordinateCount) {
        const double lx1 = lines[i + 0];
        const double ly1 = lines[i + 1];
        const double lz1 = lines[i + 2];
        const double lx2 = lines[i + 3];
        const double ly2 = lines[i + 4];
        const double lz2 = lines[i + lineCoordinateCount - 1];
        if (!geoqik_internal::validate_finite_coords(lx1, ly1, lz1) ||
            !geoqik_internal::validate_finite_coords(lx2, ly2, lz2)) {
            return geoqik_internal::fail(ApiDiagnosticId::NonFiniteCoordinate,
                                         "geoqik_update_lines_opts",
                                         "parameter: lines");
        }
        linesCopy[i + 0] = static_cast<float>(lx1);
        linesCopy[i + 1] = static_cast<float>(ly1);
        linesCopy[i + 2] = static_cast<float>(lz1);
        linesCopy[i + 3] = static_cast<float>(lx2);
        linesCopy[i + 4] = static_cast<float>(ly2);
        linesCopy[i + lineCoordinateCount - 1] = static_cast<float>(lz2);
    }

    if (options != nullptr && options->colorCount > 0) {
        if (!geoqik_internal::validate_line_color_count(options->colorCount, lineCount)) {
            return geoqik_internal::wrong_color_size(
                "geoqik_update_lines_opts",
                "parameter: options->colorCount; expected 0, 4, or line_count * 4");
        }
        if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
            return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                         "geoqik_update_lines_opts",
                                         "parameter: options->color");
        }
        colorsCopy.assign(options->color, options->color + options->colorCount);
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{UpdateLinesWithOpts{handle, std::move(linesCopy), std::move(colorsCopy)}});
        },
        "geoqik_update_lines_opts");
}

geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullInputPointer, "geoqik_remove_line", "parameter: geometryId");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{RemoveLine{handle}});
        },
        "geoqik_remove_line");
}

namespace {

constexpr float kDefaultSegmentLineWidth = 1.0F;
constexpr float kDefaultVertexPointSize = 3.0F;

[[nodiscard]]
geoqik_error_code_t apply_mesh_overlay_opts(geoqik::AddMeshWithOpts& message, const geoqik_add_mesh_opts_t* options) {
    if (options == nullptr) {
        return GEOQIK_SUCCESS;
    }

    if (options->segmentIndices != nullptr && options->segmentIndexCount > 0) {
        if (options->segmentIndexCount % 2 != 0) {
            return GEOQIK_ERROR_INVALID_PARAMETER;
        }
        message.segmentIndices.assign(options->segmentIndices, options->segmentIndices + options->segmentIndexCount);
    }
    if (options->segmentColor != nullptr) {
        message.segmentColors.assign(options->segmentColor, options->segmentColor + 4);
    }
    message.segmentLineWidth =
        (options->segmentLineWidth > 0.0F) ? options->segmentLineWidth : kDefaultSegmentLineWidth;
    message.showSegments = (options->showSegments != 0);

    if (options->vertexColor != nullptr) {
        message.vertexColors.assign(options->vertexColor, options->vertexColor + 4);
    }
    message.vertexPointSize = (options->vertexPointSize > 0.0F) ? options->vertexPointSize : kDefaultVertexPointSize;
    message.showVertices = (options->showVertices != 0);

    return GEOQIK_SUCCESS;
}

} // namespace

geoqik_result_t geoqik_add_mesh_opts(const float* vertices,
                                     size_t vertexCount,
                                     const uint32_t* triangleIndices,
                                     size_t triangleCount,
                                     geoqik_add_mesh_opts_t* options) {
    if (vertices == nullptr || vertexCount == 0) {
        return geoqik_internal::invalid_parameter_result(
            "geoqik_add_mesh_opts",
            "parameters: vertices, vertexCount; expected non-null vertices and vertexCount > 0");
    }
    if (triangleIndices == nullptr || triangleCount == 0) {
        return geoqik_internal::invalid_parameter_result(
            "geoqik_add_mesh_opts",
            "parameters: triangleIndices, triangleCount; expected non-null indices and triangleCount > 0");
    }

    std::vector<float> verticesCopy(vertices, vertices + (vertexCount * 3));
    std::vector<std::uint32_t> indicesCopy(triangleIndices, triangleIndices + (triangleCount * 3));
    std::vector<float> normalsCopy;
    std::vector<float> colorsCopy;

    if (options != nullptr) {
        if (options->normalsCount > 0 && options->normals != nullptr) {
            normalsCopy.assign(options->normals, options->normals + options->normalsCount);
        }
        if (options->colorCount > 0 && options->color != nullptr) {
            if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
                return geoqik_internal::fail_result(ApiDiagnosticId::InvalidColorRange,
                                                    "geoqik_add_mesh_opts",
                                                    "parameter: options->color");
            }
            colorsCopy.assign(options->color, options->color + options->colorCount);
        }
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_result_t {
            core::UUID reqId = core::UUID::generate();
            GeoQikMessageCommonData commonData;
            commonData.geometryId = reqId;
            commonData.rgba = std::move(colorsCopy);

            if (options != nullptr) {
                if (core::UUID idem = convert_to_core_uuid(options->idempotencyKey); !idem.is_nil()) {
                    commonData.idempotencyId = idem;
                }
            }

            geoqik::AddMeshWithOpts message;
            message.vertices = std::move(verticesCopy);
            message.normals = std::move(normalsCopy);
            message.triangleIndices = std::move(indicesCopy);
            message.commonData = std::move(commonData);

            if (const geoqik_error_code_t overlayErr = apply_mesh_overlay_opts(message, options);
                overlayErr != GEOQIK_SUCCESS) {
                return geoqik_result_t{overlayErr, {}};
            }

            auto enqueueResult = enqueue(GeoQikMessage{std::move(message)});
            return geoqik_result_t{enqueueResult, convert_to_geoqik_uuid(reqId)};
        },
        "geoqik_add_mesh_opts");
}

geoqik_error_code_t geoqik_remove_mesh(const geoqik_uuid_t* geometryId) {
    if (geometryId == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullInputPointer, "geoqik_remove_mesh", "parameter: geometryId");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{geoqik::RemoveMesh{handle}});
        },
        "geoqik_remove_mesh");
}

geoqik_error_code_t geoqik_update_mesh_opts(const geoqik_uuid_t* geometryId,
                                            const float* vertices,
                                            size_t vertexCount,
                                            geoqik_update_mesh_opts_t* options) {
    if (geometryId == nullptr || vertices == nullptr || vertexCount == 0) {
        return geoqik_internal::invalid_parameter(
            "geoqik_update_mesh_opts",
            "parameters: geometryId, vertices, vertexCount; expected non-null pointers and vertexCount > 0");
    }

    std::vector<float> verticesCopy(vertices, vertices + (vertexCount * 3));
    std::vector<float> normalsCopy;
    std::vector<float> colorsCopy;

    if (options != nullptr) {
        if (options->normalsCount > 0 && options->normals != nullptr) {
            normalsCopy.assign(options->normals, options->normals + options->normalsCount);
        }
        if (options->colorCount > 0 && options->color != nullptr) {
            if (!geoqik_internal::validate_color_values(options->color, options->colorCount)) {
                return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                             "geoqik_update_mesh_opts",
                                             "parameter: options->color");
            }
            colorsCopy.assign(options->color, options->color + options->colorCount);
        }
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            geoqik::UpdateMeshWithOpts message;
            message.handle = handle;
            message.vertices = std::move(verticesCopy);
            message.normals = std::move(normalsCopy);
            message.colors = std::move(colorsCopy);
            return enqueue(GeoQikMessage{std::move(message)});
        },
        "geoqik_update_mesh_opts");
}

geoqik_error_code_t geoqik_set_mesh_overlay_opts(const geoqik_uuid_t* geometryId,
                                                 const geoqik_mesh_overlay_opts_t* opts) {
    if (geometryId == nullptr || opts == nullptr) {
        return geoqik_internal::invalid_parameter("geoqik_set_mesh_overlay_opts",
                                                  "parameters: geometryId, opts; expected non-null pointers");
    }

    return geoqik_internal::execute_if_initialized([&]() -> geoqik_error_code_t {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        geoqik::SetMeshOverlayOpts message;
        message.handle = handle;
        message.showSegments = (opts->showSegments > 0);
        message.showVertices = (opts->showVertices > 0);
        return enqueue(GeoQikMessage{message});
    });
}

geoqik_error_code_t geoqik_set_mesh_rendering_opts(const geoqik_uuid_t* geometryId,
                                                   const geoqik_mesh_rendering_opts_t* options) {
    if (geometryId == nullptr || options == nullptr) {
        return geoqik_internal::invalid_parameter("geoqik_set_mesh_rendering_opts",
                                                  "parameters: geometryId, options; expected non-null pointers");
    }

    return geoqik_internal::execute_if_initialized([&]() -> geoqik_error_code_t {
        core::UUID handle = convert_to_core_uuid(*geometryId);
        geoqik::SetMeshRenderingOpts message;
        message.handle = handle;
        message.surfaceVisible = (options->surfaceVisible != 0);

        switch (options->cullMode) {
        case GEOQIK_MESH_CULL_FRONT: message.cullMode = geoqik::MeshCullMode::front; break;
        case GEOQIK_MESH_CULL_NONE:  message.cullMode = geoqik::MeshCullMode::none; break;
        case GEOQIK_MESH_CULL_BACK:
        default:                     message.cullMode = geoqik::MeshCullMode::back; break;
        }

        return enqueue(GeoQikMessage{message});
    });
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_all_geometry() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{RemoveAllGeometry{}}); },
        "geoqik_remove_all_geometry");
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_translate_geometry(const geoqik_uuid_t* geometryId,
                                                            double dx,
                                                            double dy,
                                                            double dz) {
    if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(dx, dy, dz)) {
        return geoqik_internal::invalid_parameter(
            "geoqik_translate_geometry",
            "parameters: geometryId, dx, dy, dz; expected non-null geometryId and finite translation");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{
                TranslateGeometry{handle, static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz)}});
        },
        "geoqik_translate_geometry");
}

GEOQIK_EXPORT geoqik_error_code_t geoqik_rotate_geometry(const geoqik_uuid_t* geometryId,
                                                         double centerX,
                                                         double centerY,
                                                         double centerZ,
                                                         double axisX,
                                                         double axisY,
                                                         double axisZ,
                                                         double angle) {
    if (geometryId == nullptr || !geoqik_internal::validate_finite_coords(centerX, centerY, centerZ) ||
        !geoqik_internal::validate_finite_coords(axisX, axisY, axisZ) || !std::isfinite(angle)) {
        return geoqik_internal::invalid_parameter(
            "geoqik_rotate_geometry",
            "parameters: geometryId, center, axis, angle; expected non-null geometryId and finite rotation values");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            core::UUID handle = convert_to_core_uuid(*geometryId);
            return enqueue(GeoQikMessage{RotateGeometry{handle,
                                                        static_cast<float>(centerX),
                                                        static_cast<float>(centerY),
                                                        static_cast<float>(centerZ),
                                                        static_cast<float>(axisX),
                                                        static_cast<float>(axisY),
                                                        static_cast<float>(axisZ),
                                                        static_cast<float>(angle)}});
        },
        "geoqik_rotate_geometry");
}

geoqik_error_code_t geoqik_draw() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{Draw{}}); },
        "geoqik_draw");
}

geoqik_error_code_t geoqik_stop_drawing() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StopDraw{}}); },
        "geoqik_stop_drawing");
}

geoqik_error_code_t geoqik_save_log(const char* path, geoqik_log_format_t format) {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        if (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON) {
            return geoqik_internal::fail(ApiDiagnosticId::UnsupportedFormat, "geoqik_save_log", "parameter: format");
        }
        return geoqik_internal::invalid_parameter("geoqik_save_log",
                                                  "parameter: path; expected non-null non-empty path");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
            std::future<geoqik_error_code_t> future = promise->get_future();

            auto enqueueResult =
                enqueue(GeoQikMessage{SaveLog{[promise, pathCopy = std::string(path), format](Context& context) {
                    promise->set_value(context.save_log(pathCopy.c_str(), format));
                }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            const geoqik_error_code_t result = future.get();
            if (result == GEOQIK_ERROR_UNKNOWN) {
                return geoqik_internal::fail(ApiDiagnosticId::IoFailure, "geoqik_save_log", path);
            }
            return result;
        },
        "geoqik_save_log");
}

geoqik_error_code_t geoqik_load_log(const char* path, geoqik_log_format_t format) {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        if (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON) {
            return geoqik_internal::fail(ApiDiagnosticId::UnsupportedFormat, "geoqik_load_log", "parameter: format");
        }
        return geoqik_internal::invalid_parameter("geoqik_load_log",
                                                  "parameter: path; expected non-null non-empty path");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
            std::future<geoqik_error_code_t> future = promise->get_future();

            auto enqueueResult =
                enqueue(GeoQikMessage{LoadLog{[promise, pathCopy = std::string(path), format](Context& context) {
                    promise->set_value(context.load_log(pathCopy.c_str(), format));
                }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            const geoqik_error_code_t result = future.get();
            if (result == GEOQIK_ERROR_UNKNOWN) {
                return geoqik_internal::fail(ApiDiagnosticId::IoFailure, "geoqik_load_log", path);
            }
            return result;
        },
        "geoqik_load_log");
}

geoqik_error_code_t
geoqik_replay_log(const char* path, geoqik_log_format_t format, const geoqik_replay_options_t* options) {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        if (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON) {
            return geoqik_internal::fail(ApiDiagnosticId::UnsupportedFormat, "geoqik_replay_log", "parameter: format");
        }
        return geoqik_internal::invalid_parameter("geoqik_replay_log",
                                                  "parameter: path; expected non-null non-empty path");
    }

    geoqik::ReplayOptions replayOptions;
    if (!geoqik_internal::convert_replay_options(options, replayOptions)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidReplayOptions, "geoqik_replay_log", "parameter: options");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
            std::future<geoqik_error_code_t> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                ReplayLog{[promise, pathCopy = std::string(path), format, replayOptions](Context& context) {
                    promise->set_value(context.replay_log(pathCopy.c_str(), format, replayOptions));
                }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            const geoqik_error_code_t result = future.get();
            if (result == GEOQIK_ERROR_UNKNOWN) {
                return geoqik_internal::fail(ApiDiagnosticId::IoFailure, "geoqik_replay_log", path);
            }
            return result;
        },
        "geoqik_replay_log");
}

geoqik_error_code_t geoqik_replay_current_log(const geoqik_replay_options_t* options) {
    geoqik::ReplayOptions replayOptions;
    if (!geoqik_internal::convert_replay_options(options, replayOptions)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidReplayOptions,
                                     "geoqik_replay_current_log",
                                     "parameter: options");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<geoqik_error_code_t>>();
            std::future<geoqik_error_code_t> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{ReplayCurrentLog{[promise, replayOptions](Context& context) {
                promise->set_value(context.replay_current_log(replayOptions));
            }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            return future.get();
        },
        "geoqik_replay_current_log");
}

geoqik_error_code_t geoqik_cancel_replay() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            request_replay_cancel();
            return GEOQIK_SUCCESS;
        },
        "geoqik_cancel_replay");
}

geoqik_error_code_t geoqik_pause_replay() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{PauseReplay{}}); },
        "geoqik_pause_replay");
}

geoqik_error_code_t geoqik_resume_replay() {
    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{ResumeReplay{}}); },
        "geoqik_resume_replay");
}

geoqik_error_code_t geoqik_step_replay() {
    return geoqik_step_replay_n(1);
}

geoqik_error_code_t geoqik_step_replay_n(size_t count) {
    if (count == 0) {
        return geoqik_internal::invalid_parameter("geoqik_step_replay_n", "parameter: count; expected > 0");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StepReplay{count}}); },
        "geoqik_step_replay_n");
}

geoqik_error_code_t geoqik_step_replay_backward() {
    return geoqik_step_replay_backward_n(1);
}

geoqik_error_code_t geoqik_step_replay_backward_n(size_t count) {
    if (count == 0) {
        return geoqik_internal::invalid_parameter("geoqik_step_replay_backward_n", "parameter: count; expected > 0");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{StepReplayBackward{count}}); },
        "geoqik_step_replay_backward_n");
}

geoqik_error_code_t geoqik_get_replay_state(geoqik_replay_state_t* state) {
    if (state == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer, "geoqik_get_replay_state", "parameter: state");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<geoqik_replay_state_t>>();
            std::future<geoqik_replay_state_t> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetReplayState{[promise](Context& context) { promise->set_value(context.get_replay_state()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            *state = future.get();
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_replay_state");
}

geoqik_error_code_t geoqik_get_replay_progress(size_t* currentEntry, size_t* totalEntries) {
    if (currentEntry == nullptr || totalEntries == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer,
                                     "geoqik_get_replay_progress",
                                     "parameters: currentEntry, totalEntries");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<std::pair<std::size_t, std::size_t>>>();
            std::future<std::pair<std::size_t, std::size_t>> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetReplayProgress{[promise](Context& context) { promise->set_value(context.get_replay_progress()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            const auto [current, total] = future.get();
            *currentEntry = current;
            *totalEntries = total;
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_replay_progress");
}

geoqik_error_code_t geoqik_set_point_size(float pointSize) {
    if (pointSize <= 0.0F) {
        return geoqik_internal::invalid_parameter("geoqik_set_point_size", "parameter: pointSize; expected > 0");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetPointSize{pointSize}}); },
        "geoqik_set_point_size");
}

geoqik_error_code_t geoqik_get_point_size(float* result) {
    if (result == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer, "geoqik_get_point_size", "parameter: result");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<float>>();
            std::future<float> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetPointSize{[promise](Context& context) { promise->set_value(context.get_point_size()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            *result = future.get();
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_point_size");
}

geoqik_error_code_t geoqik_set_point_color(float r, float g, float b, float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_set_point_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetPointColor{Color{r, g, b, a}}}); },
        "geoqik_set_point_color");
}

geoqik_error_code_t geoqik_get_point_color(float* r, float* g, float* b, float* a) {
    if (r == nullptr || g == nullptr || b == nullptr || a == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer,
                                     "geoqik_get_point_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<Color>>();
            std::future<Color> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetPointColor{[promise](Context& context) { promise->set_value(context.get_point_color()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            auto color = future.get();
            *r = color[0];
            *g = color[1];
            *b = color[2];
            *a = color[3];
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_point_color");
}

geoqik_error_code_t geoqik_set_line_width(float lineWidth) {
    if (lineWidth <= 0.0F) {
        return geoqik_internal::invalid_parameter("geoqik_set_line_width", "parameter: lineWidth; expected > 0");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t { return enqueue(GeoQikMessage{SetLineWidth{lineWidth}}); },
        "geoqik_set_line_width");
}

geoqik_error_code_t geoqik_get_line_width(float* result) {
    if (result == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer, "geoqik_get_line_width", "parameter: result");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<float>>();
            std::future<float> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetLineWidth{[promise](Context& context) { promise->set_value(context.get_line_width()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            *result = future.get();
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_line_width");
}

geoqik_error_code_t geoqik_set_line_color(float r, float g, float b, float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_set_line_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            Color colorData{r, g, b, a};
            return enqueue(GeoQikMessage{SetLineColor{colorData}});
        },
        "geoqik_set_line_color");
}

geoqik_error_code_t geoqik_get_line_color(float* r, float* g, float* b, float* a) {
    if (r == nullptr || g == nullptr || b == nullptr || a == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer,
                                     "geoqik_get_line_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<Color>>();
            std::future<Color> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetLineColor{[promise](Context& context) { promise->set_value(context.get_line_color()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            auto color = future.get();
            *r = color[0];
            *g = color[1];
            *b = color[2];
            *a = color[3];
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_line_color");
}

geoqik_error_code_t geoqik_set_mesh_color(float r, float g, float b, float a) {
    if (!geoqik_internal::validate_color(r, g, b, a)) {
        return geoqik_internal::fail(ApiDiagnosticId::InvalidColorRange,
                                     "geoqik_set_mesh_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            Color colorData{r, g, b, a};
            return enqueue(GeoQikMessage{SetMeshColor{colorData}});
        },
        "geoqik_set_mesh_color");
}

geoqik_error_code_t geoqik_get_mesh_color(float* r, float* g, float* b, float* a) {
    if (r == nullptr || g == nullptr || b == nullptr || a == nullptr) {
        return geoqik_internal::fail(ApiDiagnosticId::NullOutputPointer,
                                     "geoqik_get_mesh_color",
                                     "parameters: r, g, b, a");
    }

    return geoqik_internal::execute_if_initialized(
        [&]() -> geoqik_error_code_t {
            auto promise = std::make_shared<std::promise<Color>>();
            std::future<Color> future = promise->get_future();

            auto enqueueResult = enqueue(GeoQikMessage{
                GetMeshColor{[promise](Context& context) { promise->set_value(context.get_mesh_color()); }}});
            if (enqueueResult != GEOQIK_SUCCESS) {
                return enqueueResult;
            }

            auto color = future.get();
            *r = color[0];
            *g = color[1];
            *b = color[2];
            *a = color[3];
            return GEOQIK_SUCCESS;
        },
        "geoqik_get_mesh_color");
}

geoqik_error_code_t geoqik_wait_for_exit_and_cleanup() {
    return geoqik_internal::execute_if_initialized([&]() -> geoqik_error_code_t {
        auto& renderThread = geoqik_internal::get_render_thread();
        if (renderThread.joinable()) {
            renderThread.join();
        }

        api_is_initialized_storage().store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
    });
}

geoqik_error_code_t geoqik_cleanup() {
    return geoqik_internal::execute_if_initialized([&]() -> geoqik_error_code_t {
        get_message_queue().try_enqueue(GeoQikMessage{Cleanup{}});

        auto& renderThread = geoqik_internal::get_render_thread();
        if (renderThread.joinable()) {
            renderThread.join(); // Wait for the render thread to finish
        }

        api_is_initialized_storage().store(false, std::memory_order_release);
        return GEOQIK_SUCCESS;
    });
}
