#ifndef GEOQIK_HPP
#define GEOQIK_HPP

#include "GeoQik/geoqik_export.h"
#include <stddef.h>
#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

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
 * geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f); // Following calls that add points will use this color.
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
    size_t initialMeshCapacity;          /* Initial capacity for the mesh vertex buffer */
    float defaultMeshColor[4];           /* Default color for meshes (RGBA) */
    float meshHeadLightColor[3];         /* Camera headlight color (RGB) */
    float meshHeadLightIntensity;        /* Camera headlight intensity */
    float meshFillLightDirection[3];     /* World-space direction from mesh surface toward the fill light */
    float meshFillLightColor[3];         /* Directional fill light color (RGB) */
    float meshFillLightIntensity;        /* Directional fill light intensity */
    float meshAmbientColor[3];           /* Mesh ambient light color (RGB) */
    float meshAmbientIntensity;          /* Mesh ambient light intensity */
    float meshShininess;                 /* Mesh Phong specular exponent */
    size_t capacityGrowthFactor;         /* Growth factor for the geometry buffers */
    float defaultPointSize;              /* Default size for points */
    float defaultLineWidth;              /* Default width for lines */
    float defaultPointColor[4];          /* Default color for points (RGBA) */
    float defaultLineColor[4];           /* Default color for lines (RGBA) */
    float backgroundColor[4];            /* Background color for the window (RGBA) */
    double cameraFarPlaneMultiplier;     /* Far plane multiplier for camera */
    int autoFitCameraEnabled;            /* Whether camera auto-fit runs after scene changes */
    int autoFitZoomInEnabled;            /* Whether auto-fit may zoom in when geometry is very small */
    double autoFitZoomOutPadding;        /* Padding used when auto-fit zooms out */
    double autoFitMinViewportOccupancy;  /* Occupancy below which auto-fit may zoom in */
    double autoFitTargetViewportOccupancy; /* Target occupancy when auto-fit zooms in */
    int64_t autoFitSuppressAfterUserCameraInteractionMs; /* Suppress zoom-in after user camera movement */
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

  typedef enum
  {
    GEOQIK_LOG_FORMAT_BINARY = 0
  } geoqik_log_format_t;

  typedef enum
  {
    GEOQIK_KEY_UNKNOWN = -1,
    GEOQIK_KEY_SPACE = 32,
    GEOQIK_KEY_APOSTROPHE = 39,
    GEOQIK_KEY_COMMA = 44,
    GEOQIK_KEY_MINUS = 45,
    GEOQIK_KEY_PERIOD = 46,
    GEOQIK_KEY_SLASH = 47,
    GEOQIK_KEY_0 = 48,
    GEOQIK_KEY_1 = 49,
    GEOQIK_KEY_2 = 50,
    GEOQIK_KEY_3 = 51,
    GEOQIK_KEY_4 = 52,
    GEOQIK_KEY_5 = 53,
    GEOQIK_KEY_6 = 54,
    GEOQIK_KEY_7 = 55,
    GEOQIK_KEY_8 = 56,
    GEOQIK_KEY_9 = 57,
    GEOQIK_KEY_SEMICOLON = 59,
    GEOQIK_KEY_EQUAL = 61,
    GEOQIK_KEY_A = 65,
    GEOQIK_KEY_B = 66,
    GEOQIK_KEY_C = 67,
    GEOQIK_KEY_D = 68,
    GEOQIK_KEY_E = 69,
    GEOQIK_KEY_F = 70,
    GEOQIK_KEY_G = 71,
    GEOQIK_KEY_H = 72,
    GEOQIK_KEY_I = 73,
    GEOQIK_KEY_J = 74,
    GEOQIK_KEY_K = 75,
    GEOQIK_KEY_L = 76,
    GEOQIK_KEY_M = 77,
    GEOQIK_KEY_N = 78,
    GEOQIK_KEY_O = 79,
    GEOQIK_KEY_P = 80,
    GEOQIK_KEY_Q = 81,
    GEOQIK_KEY_R = 82,
    GEOQIK_KEY_S = 83,
    GEOQIK_KEY_T = 84,
    GEOQIK_KEY_U = 85,
    GEOQIK_KEY_V = 86,
    GEOQIK_KEY_W = 87,
    GEOQIK_KEY_X = 88,
    GEOQIK_KEY_Y = 89,
    GEOQIK_KEY_Z = 90,
    GEOQIK_KEY_LEFT_BRACKET = 91,
    GEOQIK_KEY_BACKSLASH = 92,
    GEOQIK_KEY_RIGHT_BRACKET = 93,
    GEOQIK_KEY_GRAVE_ACCENT = 96,
    GEOQIK_KEY_WORLD_1 = 161,
    GEOQIK_KEY_WORLD_2 = 162,
    GEOQIK_KEY_ESCAPE = 256,
    GEOQIK_KEY_ENTER = 257,
    GEOQIK_KEY_TAB = 258,
    GEOQIK_KEY_BACKSPACE = 259,
    GEOQIK_KEY_INSERT = 260,
    GEOQIK_KEY_DELETE = 261,
    GEOQIK_KEY_RIGHT = 262,
    GEOQIK_KEY_LEFT = 263,
    GEOQIK_KEY_DOWN = 264,
    GEOQIK_KEY_UP = 265,
    GEOQIK_KEY_PAGE_UP = 266,
    GEOQIK_KEY_PAGE_DOWN = 267,
    GEOQIK_KEY_HOME = 268,
    GEOQIK_KEY_END = 269,
    GEOQIK_KEY_CAPS_LOCK = 280,
    GEOQIK_KEY_SCROLL_LOCK = 281,
    GEOQIK_KEY_NUM_LOCK = 282,
    GEOQIK_KEY_PRINT_SCREEN = 283,
    GEOQIK_KEY_PAUSE = 284,
    GEOQIK_KEY_F1 = 290,
    GEOQIK_KEY_F2 = 291,
    GEOQIK_KEY_F3 = 292,
    GEOQIK_KEY_F4 = 293,
    GEOQIK_KEY_F5 = 294,
    GEOQIK_KEY_F6 = 295,
    GEOQIK_KEY_F7 = 296,
    GEOQIK_KEY_F8 = 297,
    GEOQIK_KEY_F9 = 298,
    GEOQIK_KEY_F10 = 299,
    GEOQIK_KEY_F11 = 300,
    GEOQIK_KEY_F12 = 301,
    GEOQIK_KEY_F13 = 302,
    GEOQIK_KEY_F14 = 303,
    GEOQIK_KEY_F15 = 304,
    GEOQIK_KEY_F16 = 305,
    GEOQIK_KEY_F17 = 306,
    GEOQIK_KEY_F18 = 307,
    GEOQIK_KEY_F19 = 308,
    GEOQIK_KEY_F20 = 309,
    GEOQIK_KEY_F21 = 310,
    GEOQIK_KEY_F22 = 311,
    GEOQIK_KEY_F23 = 312,
    GEOQIK_KEY_F24 = 313,
    GEOQIK_KEY_F25 = 314,
    GEOQIK_KEY_KP_0 = 320,
    GEOQIK_KEY_KP_1 = 321,
    GEOQIK_KEY_KP_2 = 322,
    GEOQIK_KEY_KP_3 = 323,
    GEOQIK_KEY_KP_4 = 324,
    GEOQIK_KEY_KP_5 = 325,
    GEOQIK_KEY_KP_6 = 326,
    GEOQIK_KEY_KP_7 = 327,
    GEOQIK_KEY_KP_8 = 328,
    GEOQIK_KEY_KP_9 = 329,
    GEOQIK_KEY_KP_DECIMAL = 330,
    GEOQIK_KEY_KP_DIVIDE = 331,
    GEOQIK_KEY_KP_MULTIPLY = 332,
    GEOQIK_KEY_KP_SUBTRACT = 333,
    GEOQIK_KEY_KP_ADD = 334,
    GEOQIK_KEY_KP_ENTER = 335,
    GEOQIK_KEY_KP_EQUAL = 336,
    GEOQIK_KEY_LEFT_SHIFT = 340,
    GEOQIK_KEY_LEFT_CONTROL = 341,
    GEOQIK_KEY_LEFT_ALT = 342,
    GEOQIK_KEY_LEFT_SUPER = 343,
    GEOQIK_KEY_RIGHT_SHIFT = 344,
    GEOQIK_KEY_RIGHT_CONTROL = 345,
    GEOQIK_KEY_RIGHT_ALT = 346,
    GEOQIK_KEY_RIGHT_SUPER = 347,
    GEOQIK_KEY_MENU = 348
  } geoqik_key_t;

  typedef struct
  {
    double entriesPerSecond;                 /* 0 = default 60.0 */
    double speedMultiplier;                  /* 0 = default 1.0 */
    size_t maxEntriesPerFrame;               /* 0 = default 1024 */
    int startPaused;                         /* 0 = start playing, non-zero = start paused */
    size_t entriesPerStep;                   /* 0 = default 1 */
    const geoqik_key_t* stepKeys;            /* null or empty = default: right arrow and D */
    size_t stepKeyCount;
    const geoqik_key_t* backwardStepKeys;    /* null or empty = default: left arrow and A */
    size_t backwardStepKeyCount;
    const geoqik_key_t* resumeKeys;          /* null or empty = default: space */
    size_t resumeKeyCount;
    const geoqik_key_t* pauseKeys;           /* null or empty = default: space */
    size_t pauseKeyCount;
    const geoqik_key_t* increaseEntriesPerStepKeys;   /* null or empty = default: up arrow and W */
    size_t increaseEntriesPerStepKeyCount;
    const geoqik_key_t* decreaseEntriesPerStepKeys;   /* null or empty = default: down arrow and S */
    size_t decreaseEntriesPerStepKeyCount;
  } geoqik_replay_options_t;

  typedef enum
  {
    GEOQIK_REPLAY_INACTIVE = 0,
    GEOQIK_REPLAY_PLAYING = 1,
    GEOQIK_REPLAY_PAUSED = 2
  } geoqik_replay_state_t;

  typedef struct
  {
    uint8_t value[16];
  } geoqik_uuid_t;

  typedef struct
  {
    geoqik_error_code_t err;  /* The result code */
    geoqik_uuid_t geometryId; /* The geometry ID, if applicable (zeroed otherwise) */
  } geoqik_result_t;

  /** \brief Initializes the GeoQik library.
   *
   * This function initializes the GeoQik library.
   * \attention Must be called before any other GeoQik functions.
   */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_init();

  GEOQIK_EXPORT void geoqik_create_default_settings(geoqik_settings_t* settings);
  GEOQIK_EXPORT void geoqik_init_default_window_settings(geoqik_window_settings_t* settings);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_init_with_settings(const geoqik_settings_t* geoqikSettings, const geoqik_window_settings_t* windowSettings);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_is_api_initialized(bool* isInitialized);

  /* Get error message for result code */
  GEOQIK_EXPORT const char* geoqik_get_error_string(geoqik_error_code_t result);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_generate_uuid(geoqik_uuid_t* uuid);

  /** \brief Sets the size used for all points */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_point_size(float pointSize);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_point_size(float* pointSize);

  /** \brief Sets the color used for all points that don't specify their color. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_point_color(float r, float g, float b, float a);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_point_color(float* r, float* g, float* b, float* a);

  GEOQIK_EXPORT geoqik_result_t geoqik_add_point(double x, double y, double z);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_point_with_color(double x, double y, double z, float r, float g, float b, float a);

  typedef struct
  {
    geoqik_uuid_t idempotencyKey; /**< Optional idempotency key for the point, ignored if the uuid is null. */
    const float* color;           /**< Optional color used for the point (RGBA), ignored if the pointer is null. */
    size_t colorCount;            /**< Number of floats in the color array, must be 0, 4, or number of points * 4 */
  } geoqik_add_points_options_t;

  GEOQIK_EXPORT geoqik_result_t geoqik_add_point_opts(double x, double y, double z, geoqik_add_points_options_t* options);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_points_opts(const double* points, size_t size, geoqik_add_points_options_t* options);

  typedef struct
  {
    const float* color; /**< Optional replacement color (RGBA), ignored if the pointer is null. */
    size_t colorCount;  /**< Number of floats in the color array, must be 0, 4, or number of points * 4 */
  } geoqik_update_points_options_t;

  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_point(const geoqik_uuid_t* geometryId, double x, double y, double z);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_point_with_color(const geoqik_uuid_t* geometryId, double x, double y, double z, float r, float g, float b, float a);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_point_opts(const geoqik_uuid_t* geometryId, double x, double y, double z, geoqik_update_points_options_t* options);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_points_opts(const geoqik_uuid_t* geometryId, const double* points, size_t size, geoqik_update_points_options_t* options);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_point(const geoqik_uuid_t* geometryId);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_add_line(double x1, double y1, double z1, double x2, double y2, double z2);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_add_line_with_color(double x1, double y1, double z1, double x2, double y2, double z2, float r, float g, float b, float a);

  /** \brief Sets the width used for all lines. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_line_width(float lineWidth);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_line_width(float* lineWidth);

  /** \brief Sets the color used for all lines that don't specify their color. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_line_color(float r, float g, float b, float a);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_line_color(float* r, float* g, float* b, float* a);

  /** \brief Sets the default color used for meshes that don't specify their color. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_set_mesh_color(float r, float g, float b, float a);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_mesh_color(float* r, float* g, float* b, float* a);

  typedef struct
  {
    geoqik_uuid_t idempotencyKey; /**< Optional idempotency key for the line, ignored if the uuid is zeroed. */
    const float* color;           /**< Optional color used for the line (RGBA), ignored if the pointer is null. */
    size_t colorCount;            /**< Number of floats in the color array, must be 0, 4, or number of lines * 4 */
  } geoqik_add_line_opts_t;

  GEOQIK_EXPORT geoqik_result_t geoqik_add_line_opts(double x1, double y1, double z1, double x2, double y2, double z2, geoqik_add_line_opts_t* options);
  GEOQIK_EXPORT geoqik_result_t geoqik_add_lines_opts(const double* lines, size_t size, geoqik_add_line_opts_t* options);

  typedef struct
  {
    const float* color; /**< Optional replacement color (RGBA), ignored if the pointer is null. */
    size_t colorCount;  /**< Number of floats in the color array, must be 0, 4, or number of lines * 4 */
  } geoqik_update_line_opts_t;

  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_line(const geoqik_uuid_t* geometryId, double x1, double y1, double z1, double x2, double y2, double z2);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_line_with_color(const geoqik_uuid_t* geometryId,
                                                                  double x1,
                                                                  double y1,
                                                                  double z1,
                                                                  double x2,
                                                                  double y2,
                                                                  double z2,
                                                                  float r,
                                                                  float g,
                                                                  float b,
                                                                  float a);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_line_opts(const geoqik_uuid_t* geometryId,
                                                            double x1,
                                                            double y1,
                                                            double z1,
                                                            double x2,
                                                            double y2,
                                                            double z2,
                                                            geoqik_update_line_opts_t* options);
  GEOQIK_EXPORT geoqik_error_code_t geoqik_update_lines_opts(const geoqik_uuid_t* geometryId, const double* lines, size_t size, geoqik_update_line_opts_t* options);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_line(const geoqik_uuid_t* geometryId);

  typedef struct
  {
    geoqik_uuid_t idempotencyKey; /**< Optional idempotency key for the mesh, ignored if the uuid is zeroed. */
    const float* normals;         /**< Optional per-vertex normals (XYZ interleaved), ignored if null. Size must be 0 or vertexCount*3. */
    size_t normalsCount;          /**< Number of floats in the normals array. */
    const float* color;           /**< Optional color (RGBA), ignored if null. Size must be 0, 4, or vertexCount*4. */
    size_t colorCount;            /**< Number of floats in the color array. */
  } geoqik_add_mesh_opts_t;

  GEOQIK_EXPORT geoqik_result_t geoqik_add_mesh_opts(const float* vertices,
                                                       size_t vertexCount,
                                                       const uint32_t* triangleIndices,
                                                       size_t triangleCount,
                                                       geoqik_add_mesh_opts_t* options);

  GEOQIK_EXPORT geoqik_error_code_t geoqik_remove_mesh(const geoqik_uuid_t* geometryId);

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

  /** \brief Saves the current geometry event log to a file. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_save_log(const char* path, geoqik_log_format_t format);

  /** \brief Loads a geometry event log from a file, replacing current geometry and the in-memory log. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_load_log(const char* path, geoqik_log_format_t format);

  /** \brief Replays a geometry event log from a file over time, replacing current geometry and the in-memory log. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_replay_log(const char* path, geoqik_log_format_t format, const geoqik_replay_options_t* options);

  /** \brief Replays the current in-memory geometry event log over time. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_replay_current_log(const geoqik_replay_options_t* options);

  /** \brief Cancels an active replay, leaving the scene at its current replayed state. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_cancel_replay();

  /** \brief Pauses an active replay. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_pause_replay();

  /** \brief Resumes a paused replay. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_resume_replay();

  /** \brief Advances an active replay by one log entry and leaves it paused. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_step_replay();

  /** \brief Advances an active replay by count log entries and leaves it paused. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_step_replay_n(size_t count);

  /** \brief Moves an active replay backward by one log entry and leaves it paused. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_step_replay_backward();

  /** \brief Moves an active replay backward by count log entries and leaves it paused. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_step_replay_backward_n(size_t count);

  /** \brief Gets the current replay state. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_replay_state(geoqik_replay_state_t* state);

  /** \brief Gets the current replay progress as current and total log entries. */
  GEOQIK_EXPORT geoqik_error_code_t geoqik_get_replay_progress(size_t* currentEntry, size_t* totalEntries);

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
