#ifndef GEOQIK_HPP
#define GEOQIK_HPP

#include "GeoQik/geoqik_export.h"
#include <stddef.h>
#include <stdint.h>

/** Description:
 *
 * 'A picture is worth a thousand words' - This library aims to provide a simple way to visualize geometry for debugging purposes.
 *
 *
 * Thread Safety:
 * All functions in the GeoQik C API are thread-safe and can be called concurrently from multiple threads. Except for the geoqik_init()
 * function which must be called only once before any other GeoQik functions.
 *
 * Calls to the GeoQik API use locking internally, keep in mind that this may lead to contention if multiple threads
 * access the API simultaneously.
 *
 *
 * Usage Example:
 *
 * \code
 * geoqik_error_code_t result = geoqik_init(); // Initialize the GeoQik library
 * if (result != GEOQIK_SUCCESS) {
 *     printf("Failed to initialize: %s\n", geoqik_get_error_string(result));
 *     return -1;
 * }
 *
 * geoqik_set_point_size(5.0f); // Following calls that add points will use this size.
 * geoqik_set_line_width(2.0f); // Following calls that add lines will use this width.
 * geoqik_set_point_color(1.0f, 0.0f, 0.0f); // Following calls that add points will use this color.
 *
 * // Geometry won't be drawn yet, it will be drawn when geoqik_draw() is called.
 * geoqik_add_point(1.0, 0.0, 0.0);
 * geoqik_add_line(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
 *
 * geoqik_draw(); // Draw all added geometry. Geometry added after this call will be drawn as soon as possible.
 *
 * geoqik_add_point(0.0, 1.0, 0.0); // This point will be drawn as soon as possible since geoqik_draw() was already called.
 *
 * geoqik_wait_for_exit_and_cleanup(); // Blocking, wait for user to close the window and clean up resources.
 * \endcode
 */

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    int64_t maxMessageQueueSize;         /* Maximum size of the message queue */
    size_t initialPointCapacity;         /* Initial capacity for the point buffer */
    size_t initialLineCapacity;          /* Initial capacity for the line buffer */
    size_t capacityGrowthFactor;         /* Growth factor for the geometry buffers */
    float defaultPointSize;              /* Default size for points */
    float defaultLineWidth;              /* Default width for lines */
    float defaultPointColor[3];          /* Default color for points (RGB) */
    float defaultLineColor[3];           /* Default color for lines (RGB) */
    float backgroundColor[3];            /* Background color for the window (RGB) */
    double cameraFarPlaneMultiplier;     /* Far plane multiplier for camera */
    int64_t minGeometryProcessingTimeMs; /* Minimum time for geometry processing per frame (ms) */
    int64_t maxFrameProcessingTimeMs;    /* Maximum time for frame processing (ms) */
    size_t updateSceneFrequency;         /* Frequency of updating the scene */
  } geoqik_settings_t;

  typedef struct
  {
    const char* title;           /* Title of the window */
    uint32_t width;              /* Width of the window */
    uint32_t height;             /* Height of the window */
    int red_bits;                /* Number of bits for the red channel */
    int green_bits;              /* Number of bits for the green channel */
    int blue_bits;               /* Number of bits for the blue channel */
    int alpha_bits;              /* Number of bits for the alpha channel */
    int depth_bits;              /* Number of bits for the depth buffer */
    int stencil_bits;            /* Number of bits for the stencil buffer */
    int accum_red_bits;          /* Number of bits for the red channel in accumulation buffer */
    int accum_green_bits;        /* Number of bits for the green channel in accumulation buffer */
    int accum_blue_bits;         /* Number of bits for the blue channel in accumulation buffer */
    int accum_alpha_bits;        /* Number of bits for the alpha channel in accumulation buffer */
    int aux_buffers;             /* Number of auxiliary buffers */
    int samples;                 /* Number of samples for multisampling */
    int refresh_rate;            /* Refresh rate of the window, -1 for default */
    int stereo;                  /* Whether to use stereo rendering (0 = false, 1 = true) */
    int srgb_capable;            /* Whether the framebuffer should be sRGB capable */
    int double_buffer;           /* Whether to use double buffering */
    int resizable;               /* Whether the window is resizable */
    int visible;                 /* Whether the window is initially visible */
    int decorated;               /* Whether the window has decorations */
    int focused;                 /* Whether the window is initially focused */
    int auto_iconify;            /* Whether to auto iconify when switching from fullscreen */
    int floating;                /* Whether the window is always on top */
    int maximized;               /* Whether the window is maximized */
    int center_cursor;           /* Whether the cursor is centered over the window */
    int transparent_framebuffer; /* Whether the framebuffer should be transparent */
    int focus_on_show;           /* Whether the window should be focused when shown */
    int scale_to_monitor;        /* Whether to scale based on monitor content scale */
  } geoqik_window_settings_t;

  GEOQIK_EXPORT void geoqik_init_default_settings(geoqik_settings_t* settings);
  GEOQIK_EXPORT void geoqik_init_default_window_settings(geoqik_window_settings_t* settings);

  typedef enum
  {
    GEOQIK_SUCCESS = 0,
    GEOQIK_ERROR_NOT_INITIALIZED = 1,
    GEOQIK_ERROR_ALREADY_INITIALIZED = 2,
    GEOQIK_ERROR_INVALID_PARAMETER = 3,
    GEOQIK_ERROR_WRONG_COLOR_SIZE = 4,
    GEOQIK_ERROR_MEMORY_ALLOCATION = 5,
    GEOQIK_ERROR_UNKNOWN = 6
  } geoqik_error_code_t;

  typedef struct
  {
    uint8_t value[16];
  } geoqik_uuid_t;

  typedef struct
  {
    geoqik_error_code_t err;  /* The result code */
    geoqik_uuid_t geometryId; /* The geometry ID, if applicable (zeroed otherwise) */
  } geoqik_result_t;

  /* Get error message for result code */
  GEOQIK_EXPORT const char* geoqik_get_error_string(geoqik_error_code_t result);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_generate_uuid(geoqik_uuid_t* uuid);

  /** \brief Initializes the GeoQik library.
   *
   * This function initializes the GeoQik library.
   * \attention Must be called before any other GeoQik functions.
   */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_init();
  GEOQIK_EXPORT geoqik_error_code_t geoqik_init_with_settings(const geoqik_settings_t* geoqikSettings, const geoqik_window_settings_t* windowSettings);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized);

  /** \brief Sets the size used for all points */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_point_size(float pointSize);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_point_size(float* pointSize);

  /** \brief Sets the color used for all points that don't specify their color. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_point_color(float r, float g, float b);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_point_color(float* r, float* g, float* b);

  GEOQIK_EXPORT geoqik_result_t geoqik_add_point(double x, double y, double z);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_point_with_color(double x, double y, double z, float r, float g, float b);

  typedef struct
  {
    geoqik_uuid_t idempotencyKey; /**< Optional idempotency key for the point, ignored if the uuid is null. */
    const float* color;           /**< Optional color used for the point (RGB), ignored if the pointer is null. */
    size_t colorCount;            /**< Number of floats in the color array, must be 0, 3 or the same as the number of points */
  } geoqik_add_points_options_t;

  GEOQIK_EXPORT geoqik_result_t geoqik_add_point_opts(double x, double y, double z, geoqik_add_points_options_t* options);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_points_opts(const double* points, size_t size, geoqik_add_points_options_t* options);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_add_line(double x1, double y1, double z1, double x2, double y2, double z2);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_add_line_with_color(double x1, double y1, double z1, double x2, double y2, double z2, float r, float g, float b);

  /** \brief Sets the width used for all lines. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_line_width(float lineWidth);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_line_width(float* lineWidth);

  /** \brief Sets the color used for all lines that don't specify their color. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_line_color(float r, float g, float b);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_line_color(float* r, float* g, float* b);

  typedef struct
  {
    geoqik_uuid_t idempotencyKey; /**< Optional idempotency key for the line, ignored if the uuid is zeroed. */
    const float* color;           /**< Optional color used for the line (RGB), ignored if the pointer is null. */
    size_t colorCount;            /**< Number of floats in the color array, must be 0, 3 or the same as the number of lines * 3 */
  } geoqik_add_line_opts_t;

  GEOQIK_EXPORT geoqik_result_t geoqik_add_line_opts(double x1, double y1, double z1, double x2, double y2, double z2, geoqik_add_line_opts_t* options);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_lines_opts(const double* lines, size_t size, geoqik_add_line_opts_t* options);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_all_geometry();

  GEOQIK_EXPORT geoqik_error_code_t geoqik_translate_geometry(const geoqik_uuid_t* geometryId, double dx, double dy, double dz);
  /** \brief Rotates the specified geometry around the given center, axis and angle */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_rotate_geometry(const geoqik_uuid_t* geometryId,
                                                           double centerX,
                                                           double centerY,
                                                           double centerZ,
                                                           double axisX,
                                                           double axisY,
                                                           double axisZ,
                                                           double angle);

  /** \brief Starts drawing geometry.
   *
   * Starts the drawing process for all geometry added so far. Geometry added after this call will be drawn as soon as possible.
   */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_draw();
  GEOQIK_EXPORT geoqik_error_code_t geoqik_stop_drawing();

  /** \brief Waits for user to close the window and then cleans up resources.
   *
   * This function blocks until the user closes the window, then automatically
   * cleans up all resources used by the GeoQik library.
   * Calling other GeoQik functions after this function returns is undefined.
   */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_wait_for_exit_and_cleanup();

  /** \brief Destroys the Window and cleans up resources.
   *
   * This function cleans up all resources used by the GeoQik library.
   * Calling other GeoQik functions after destroy was called is undefined.
   */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_cleanup();

#ifdef __cplusplus
}
#endif

#endif // GEOQIK_HPP