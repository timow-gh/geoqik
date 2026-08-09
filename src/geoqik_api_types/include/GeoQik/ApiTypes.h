#ifndef GEOQIK_API_TYPES_H
#define GEOQIK_API_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GEOQIK_SUCCESS = 0,
    GEOQIK_ERROR_NOT_INITIALIZED = 1,
    GEOQIK_ERROR_ALREADY_INITIALIZED = 2,
    GEOQIK_ERROR_INVALID_PARAMETER = 3,
    GEOQIK_ERROR_WRONG_COLOR_SIZE = 4,
    GEOQIK_ERROR_MEMORY_ALLOCATION = 5,
    GEOQIK_ERROR_UNKNOWN = 6,
    GEOQIK_ERROR_RENDERER_INIT_FAILED = 7,
    GEOQIK_ERROR_IO = 8,
    GEOQIK_ERROR_UNSUPPORTED_FORMAT = 9,
    GEOQIK_ERROR_INVALID_STATE = 10
} geoqik_error_code_t;

typedef struct {
    size_t struct_size;
    geoqik_error_code_t code;
    const char* operation;
    const char* what;
    const char* why;
    const char* action;
    const char* details;
} geoqik_error_info_t;

typedef struct {
    uint8_t value[16];
} geoqik_uuid_t;

typedef struct {
    geoqik_error_code_t err;
    geoqik_uuid_t geometryId;
} geoqik_result_t;

typedef enum {
    GEOQIK_LINE_CAP_BUTT = 0,   /* flush with the endpoint, SVG default */
    GEOQIK_LINE_CAP_SQUARE = 1, /* extends half the line width past the endpoint */
    GEOQIK_LINE_CAP_ROUND = 2   /* semicircle centered on the endpoint */
} geoqik_line_cap_t;

typedef enum {
    GEOQIK_LINE_JOIN_MITER = 0, /* sharp pointed join, SVG default */
    GEOQIK_LINE_JOIN_BEVEL = 1, /* diagonal cutoff at the join */
    GEOQIK_LINE_JOIN_ROUND = 2  /* circular arc join */
} geoqik_line_join_t;

typedef enum {
    GEOQIK_DASH_SPACE_WORLD = 0, /* dash/gap lengths in world units (scale with zoom) */
    GEOQIK_DASH_SPACE_SCREEN = 1 /* dash/gap lengths in pixels (constant on screen) */
} geoqik_dash_space_t;

typedef enum {
    GEOQIK_LINE_TYPE_LINES = 0,      /* independent endpoint pairs (default) */
    GEOQIK_LINE_TYPE_LINE_STRIP = 1, /* connected polyline */
    GEOQIK_LINE_TYPE_LINE_LOOP = 2   /* closed polyline */
} geoqik_line_type_t;

typedef enum {
    /* Diameter in pixels; each sphere holds a roughly constant on-screen size, like glPointSize.
       GeoQik default
     * so opts size values feel like the legacy point size. */
    GEOQIK_SPHERE_SIZE_SPACE_SCREEN = 0,
    /* Radius in world units; spheres scale with zoom/distance. */
    GEOQIK_SPHERE_SIZE_SPACE_WORLD = 1
} geoqik_sphere_size_space_t;

/* Stroke styling for line drawables. A zero-initialized struct
   describes a solid line whose width falls back to the global default line width. */
typedef struct {
    float lineWidth;         /* <= 0 => fall back to the global default line width */
    geoqik_line_cap_t cap;   /* 0 = BUTT */
    geoqik_line_join_t join; /* 0 = MITER */
    float miterLimit;        /* <= 0 => plinth default (4.0); only used when join == MITER */

    const float* dashPattern; /* NULL/0 count => solid. SVG dasharray: even=dash, odd=gap */
    size_t dashPatternCount;
    float dashPhase;               /* marching-ants offset along the arc length */
    geoqik_dash_space_t dashSpace; /* 0 = WORLD */
} geoqik_stroke_style_t;

typedef struct {
    geoqik_uuid_t idempotencyKey;
    const float* color;
    size_t colorCount;

    /* Sphere sizes, interpreted as world radii or screen diameters according to sizeSpace.
       0 count => uniform
     * size from the global point size; 1 => uniform; N => per-point. */
    const float* radii;
    size_t radiusCount;
    geoqik_sphere_size_space_t sizeSpace; /* 0 = SCREEN (pixels, glPointSize-like) */
} geoqik_add_points_options_t;

typedef struct {
    const float* color;
    size_t colorCount;

    /* Same radius + size-space semantics as geoqik_add_points_options_t. */
    const float* radii;
    size_t radiusCount;
    geoqik_sphere_size_space_t sizeSpace; /* 0 = SCREEN (pixels, glPointSize-like) */
} geoqik_update_points_options_t;

typedef struct {
    geoqik_uuid_t idempotencyKey;
    const float* color;
    size_t colorCount;

    /* Full styling. When styleSet == 0 the style and lineType are ignored and current behaviour
       applies (default width, solid, GEOQIK_LINE_TYPE_LINES). */
    int styleSet;
    geoqik_stroke_style_t style;
    geoqik_line_type_t lineType;

    /* Per-vertex dash flags (0/1); length must match the vertex count. NULL/0 => not used. */
    const uint8_t* perVertexDashFlags;
    size_t perVertexDashFlagCount;
} geoqik_add_line_opts_t;

typedef struct {
    const float* color;
    size_t colorCount;

    /* Same style controls as geoqik_add_line_opts_t for in-place restyle on update. */
    int styleSet;
    geoqik_stroke_style_t style;
    geoqik_line_type_t lineType;
    const uint8_t* perVertexDashFlags;
    size_t perVertexDashFlagCount;
} geoqik_update_line_opts_t;

typedef enum {
    GEOQIK_MESH_CULL_BACK = 0,
    GEOQIK_MESH_CULL_FRONT = 1,
    GEOQIK_MESH_CULL_NONE = 2
} geoqik_mesh_cull_mode_t;

typedef struct {
    geoqik_uuid_t idempotencyKey;
    const float* normals;
    size_t normalsCount;
    const float* color;
    size_t colorCount;
    const uint32_t* segmentIndices;
    size_t segmentIndexCount;
    const float* segmentColor;
    int showSegments;
    float segmentLineWidth;
    const float* vertexColor;
    int showVertices;
    float vertexPointSize;
} geoqik_add_mesh_opts_t;

typedef struct {
    geoqik_uuid_t idempotencyKey;
    const float* normals;
    size_t normalsCount;
    const float* color;
    size_t colorCount;
} geoqik_update_mesh_opts_t;

typedef struct {
    int showSegments;
    int showVertices;
} geoqik_mesh_overlay_opts_t;

typedef struct {
    geoqik_mesh_cull_mode_t cullMode;
    int surfaceVisible;
} geoqik_mesh_rendering_opts_t;

#endif
