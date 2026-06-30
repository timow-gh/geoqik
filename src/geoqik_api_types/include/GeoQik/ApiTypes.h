#ifndef GEOQIK_API_TYPES_H
#define GEOQIK_API_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
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

typedef struct
{
  size_t struct_size;
  geoqik_error_code_t code;
  const char* operation;
  const char* what;
  const char* why;
  const char* action;
  const char* details;
} geoqik_error_info_t;

typedef struct
{
  uint8_t value[16];
} geoqik_uuid_t;

typedef struct
{
  geoqik_error_code_t err;
  geoqik_uuid_t geometryId;
} geoqik_result_t;

typedef struct
{
  geoqik_uuid_t idempotencyKey;
  const float* color;
  size_t colorCount;
} geoqik_add_points_options_t;

typedef struct
{
  const float* color;
  size_t colorCount;
} geoqik_update_points_options_t;

typedef struct
{
  geoqik_uuid_t idempotencyKey;
  const float* color;
  size_t colorCount;
} geoqik_add_line_opts_t;

typedef struct
{
  const float* color;
  size_t colorCount;
} geoqik_update_line_opts_t;

typedef enum
{
  GEOQIK_MESH_CULL_BACK = 0,
  GEOQIK_MESH_CULL_FRONT = 1,
  GEOQIK_MESH_CULL_NONE = 2
} geoqik_mesh_cull_mode_t;

typedef struct
{
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

typedef struct
{
  geoqik_uuid_t idempotencyKey;
  const float* normals;
  size_t normalsCount;
  const float* color;
  size_t colorCount;
} geoqik_update_mesh_opts_t;

typedef struct
{
  int showSegments;
  int showVertices;
} geoqik_mesh_overlay_opts_t;

typedef struct
{
  geoqik_mesh_cull_mode_t cullMode;
  int surfaceVisible;
} geoqik_mesh_rendering_opts_t;

#endif
