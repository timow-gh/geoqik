#ifndef GEOQIKMESSAGES_HPP
#define GEOQIKMESSAGES_HPP

#include "Core/UUID.hpp"
#include "GeometryBuffers/MeshBuffer.hpp"
#include <Renderer/Color.hpp>
#include <functional>
#include <variant>
#include <vector>

namespace geoqik {

using renderer::Color;

class Context;

struct GeoQikMessageCommonData {
    core::UUID geometryId;
    core::UUID idempotencyId;
    std::vector<float> rgba;

    [[nodiscard]]
    bool operator==(const GeoQikMessageCommonData&) const = default;
};

struct AddPointWithOpts {
    float x, y, z;
    GeoQikMessageCommonData commonData;

    [[nodiscard]]
    bool operator==(const AddPointWithOpts&) const = default;
};

struct AddPointsWithOpts {
    std::vector<float> points;
    GeoQikMessageCommonData commonData;

    [[nodiscard]]
    bool operator==(const AddPointsWithOpts&) const = default;
};

struct UpdatePointWithOpts {
    core::UUID handle;
    float x, y, z;
    std::vector<float> rgba;

    [[nodiscard]]
    bool operator==(const UpdatePointWithOpts&) const = default;
};

struct UpdatePointsWithOpts {
    core::UUID handle;
    std::vector<float> points;
    std::vector<float> rgba;

    [[nodiscard]]
    bool operator==(const UpdatePointsWithOpts&) const = default;
};

struct RemovePoint {
    core::UUID handle;

    [[nodiscard]]
    bool operator==(const RemovePoint&) const = default;
};

struct SetPointSize {
    float size;

    [[nodiscard]]
    bool operator==(const SetPointSize&) const = default;
};

struct SetPointColor {
    Color color;

    [[nodiscard]]
    bool operator==(const SetPointColor&) const = default;
};

struct AddLineWithOpts {
    float x1, y1, z1;
    float x2, y2, z2;
    GeoQikMessageCommonData commonData;

    [[nodiscard]]
    bool operator==(const AddLineWithOpts&) const = default;
};

struct AddLinesWithOpts {
    std::vector<float> lines;
    GeoQikMessageCommonData commonData;

    [[nodiscard]]
    bool operator==(const AddLinesWithOpts&) const = default;
};

struct UpdateLineWithOpts {
    core::UUID handle;
    float x1, y1, z1;
    float x2, y2, z2;
    std::vector<float> rgba;

    [[nodiscard]]
    bool operator==(const UpdateLineWithOpts&) const = default;
};

struct UpdateLinesWithOpts {
    core::UUID handle;
    std::vector<float> lines;
    std::vector<float> rgba;

    [[nodiscard]]
    bool operator==(const UpdateLinesWithOpts&) const = default;
};

struct RemoveLine {
    core::UUID handle;

    [[nodiscard]]
    bool operator==(const RemoveLine&) const = default;
};

struct SetLineWidth {
    float width;

    [[nodiscard]]
    bool operator==(const SetLineWidth&) const = default;
};

struct SetLineColor {
    Color color;

    [[nodiscard]]
    bool operator==(const SetLineColor&) const = default;
};

struct SetMeshColor {
    Color color;

    [[nodiscard]]
    bool operator==(const SetMeshColor&) const = default;
};

struct GetMeshColor {
    std::function<void(Context& context)> callback;
};

struct AddMeshWithOpts {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    std::vector<std::uint32_t> triangleIndices;
    GeoQikMessageCommonData commonData;
    std::vector<std::uint32_t> segmentIndices; // empty = auto-derive; non-empty = user-supplied pairs
    std::vector<float> segmentColors;          // single RGBA [4 floats] or empty (use default black)
    float segmentLineWidth{1.0f};
    bool showSegments{false};
    std::vector<float> vertexColors; // single RGBA [4 floats] or empty (use default white)
    float vertexPointSize{3.0f};
    bool showVertices{false};

    [[nodiscard]]
    bool operator==(const AddMeshWithOpts&) const = default;
};

struct RemoveMesh {
    core::UUID handle;

    [[nodiscard]]
    bool operator==(const RemoveMesh&) const = default;
};

struct UpdateMeshWithOpts {
    core::UUID handle;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;

    [[nodiscard]]
    bool operator==(const UpdateMeshWithOpts&) const = default;
};

struct SetMeshOverlayOpts {
    core::UUID handle;
    bool showSegments{false};
    bool showVertices{false};

    [[nodiscard]]
    bool operator==(const SetMeshOverlayOpts&) const = default;
};

struct SetMeshRenderingOpts {
    core::UUID handle;
    MeshCullMode cullMode{MeshCullMode::back};
    bool surfaceVisible{true};

    bool operator==(const SetMeshRenderingOpts&) const = default;
};

struct RemoveAllGeometry {
    [[nodiscard]]
    bool operator==(const RemoveAllGeometry&) const = default;
};

struct TranslateGeometry {
    core::UUID handle;
    float dx, dy, dz;

    [[nodiscard]]
    bool operator==(const TranslateGeometry&) const = default;
};

struct RotateGeometry {
    core::UUID handle;
    float centerX, centerY, centerZ;
    float axisX, axisY, axisZ;
    float angle;

    [[nodiscard]]
    bool operator==(const RotateGeometry&) const = default;
};

struct Draw {};

struct StopDraw {};

struct SaveLog {
    std::function<void(Context& context)> callback;
};

struct LoadLog {
    std::function<void(Context& context)> callback;
};

struct ReplayLog {
    std::function<void(Context& context)> callback;
};

struct ReplayCurrentLog {
    std::function<void(Context& context)> callback;
};

struct PauseReplay {};

struct ResumeReplay {};

struct StepReplay {
    std::size_t count;
};

struct StepReplayBackward {
    std::size_t count;
};

struct GetReplayState {
    std::function<void(Context& context)> callback;
};

struct GetReplayProgress {
    std::function<void(Context& context)> callback;
};

struct GetPointSize {
    std::function<void(Context& context)> callback;
};

struct GetPointColor {
    std::function<void(Context& context)> callback;
};

struct GetLineWidth {
    std::function<void(Context& context)> callback;
};

struct GetLineColor {
    std::function<void(Context& context)> callback;
};

struct Cleanup {};

using GeoQikLogEntry = std::variant<AddPointWithOpts,
                                    AddPointsWithOpts,
                                    UpdatePointWithOpts,
                                    UpdatePointsWithOpts,
                                    RemovePoint,
                                    SetPointSize,
                                    SetPointColor,
                                    AddLineWithOpts,
                                    AddLinesWithOpts,
                                    UpdateLineWithOpts,
                                    UpdateLinesWithOpts,
                                    RemoveLine,
                                    SetLineWidth,
                                    SetLineColor,
                                    RemoveAllGeometry,
                                    TranslateGeometry,
                                    RotateGeometry,
                                    SetMeshColor,
                                    AddMeshWithOpts,
                                    RemoveMesh,
                                    UpdateMeshWithOpts,
                                    SetMeshOverlayOpts,
                                    SetMeshRenderingOpts>;

using GeoQikMessage = std::variant<AddPointWithOpts,
                                   AddPointsWithOpts,
                                   UpdatePointWithOpts,
                                   UpdatePointsWithOpts,
                                   RemovePoint,
                                   SetPointSize,
                                   SetPointColor,
                                   AddLineWithOpts,
                                   AddLinesWithOpts,
                                   UpdateLineWithOpts,
                                   UpdateLinesWithOpts,
                                   RemoveLine,
                                   SetLineWidth,
                                   SetLineColor,
                                   SetMeshColor,
                                   RemoveAllGeometry,
                                   TranslateGeometry,
                                   RotateGeometry,
                                   AddMeshWithOpts,
                                   RemoveMesh,
                                   UpdateMeshWithOpts,
                                   SetMeshOverlayOpts,
                                   SetMeshRenderingOpts,
                                   Draw,
                                   StopDraw,
                                   SaveLog,
                                   LoadLog,
                                   ReplayLog,
                                   ReplayCurrentLog,
                                   PauseReplay,
                                   ResumeReplay,
                                   StepReplay,
                                   StepReplayBackward,
                                   GetReplayState,
                                   GetReplayProgress,
                                   GetPointSize,
                                   GetPointColor,
                                   GetLineWidth,
                                   GetLineColor,
                                   GetMeshColor,
                                   Cleanup>;

} // namespace geoqik

#endif // GEOQIKMESSAGES_HPP
