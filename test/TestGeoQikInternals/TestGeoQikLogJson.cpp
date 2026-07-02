#include "GeoQikLog.hpp"
#include "GeoQikMessageJsonSerialization.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {

core::UUID make_uuid(std::uint8_t seed) {
    std::array<std::uint8_t, 16> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(seed + i);
    }
    return core::UUID(bytes);
}

std::filesystem::path make_temp_path(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

} // namespace

TEST(GeoQikLogTest, JsonRoundTripPreservesRepresentativeEntries) {
    using namespace geoqik;

    const GeoQikMessageCommonData commonData{make_uuid(1), make_uuid(20), {0.1f, 0.2f, 0.3f, 1.0f}};
    const std::vector<GeoQikLogEntry> entries{
        AddPointWithOpts{1.0f, 2.0f, 3.0f, commonData},
        AddPointsWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
        UpdatePointWithOpts{make_uuid(15), 7.0f, 8.0f, 9.0f, {0.2f, 0.3f, 0.4f, 1.0f}},
        UpdatePointsWithOpts{make_uuid(16), {7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f}, {0.3f, 0.4f, 0.5f, 1.0f}},
        RemovePoint{make_uuid(2)},
        SetPointSize{7.0f},
        SetPointColor{Color{0.4f, 0.5f, 0.6f, 1.0f}},
        AddLineWithOpts{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, commonData},
        AddLinesWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
        UpdateLineWithOpts{make_uuid(17), 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, {0.6f, 0.7f, 0.8f, 1.0f}},
        UpdateLinesWithOpts{make_uuid(18), {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, {0.7f, 0.8f, 0.9f, 1.0f}},
        RemoveLine{make_uuid(3)},
        SetLineWidth{8.0f},
        SetLineColor{Color{0.7f, 0.8f, 0.9f, 1.0f}},
        RemoveAllGeometry{},
        TranslateGeometry{make_uuid(4), 1.0f, 2.0f, 3.0f},
        RotateGeometry{make_uuid(5), 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 90.0f}};

    const std::filesystem::path path = make_temp_path("geoqik_log_roundtrip.gqklog.json");
    save_log_json(path, entries);

    const std::vector<GeoQikLogEntry> loadedEntries = load_log_json(path);
    EXPECT_EQ(entries, loadedEntries);

    (void)std::filesystem::remove(path);
}

TEST(GeoQikLogTest, JsonToFromJsonRoundTrip) {
    using namespace geoqik;

    const GeoQikMessageCommonData commonData{make_uuid(10), make_uuid(40), {0.1f, 0.2f, 0.3f, 1.0f}};
    const std::vector<GeoQikLogEntry> entries{
        AddPointWithOpts{1.0f, 2.0f, 3.0f, commonData},
        AddPointsWithOpts{{1.0f, 2.0f, 3.0f}, commonData},
        UpdatePointWithOpts{make_uuid(20), 4.0f, 5.0f, 6.0f, {0.2f, 0.3f, 0.4f, 1.0f}},
        UpdatePointsWithOpts{make_uuid(21), {4.0f, 5.0f, 6.0f}, {0.3f, 0.4f, 0.5f, 1.0f}},
        RemovePoint{make_uuid(11)},
        SetPointSize{9.0f},
        SetPointColor{Color{0.2f, 0.3f, 0.4f, 1.0f}},
        AddLineWithOpts{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, commonData},
        AddLinesWithOpts{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, commonData},
        UpdateLineWithOpts{make_uuid(22), 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, {0.4f, 0.5f, 0.6f, 1.0f}},
        UpdateLinesWithOpts{make_uuid(23), {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}, {0.5f, 0.6f, 0.7f, 1.0f}},
        RemoveLine{make_uuid(12)},
        SetLineWidth{3.0f},
        SetLineColor{Color{0.5f, 0.6f, 0.7f, 1.0f}},
        RemoveAllGeometry{},
        TranslateGeometry{make_uuid(13), 1.0f, 2.0f, 3.0f},
        RotateGeometry{make_uuid(14), 1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 45.0f}};

    for (const GeoQikLogEntry& entry: entries) {
        const GeoQikLogEntry roundTripped = from_json(to_json(entry));
        EXPECT_EQ(entry, roundTripped);
    }
}

TEST(GeoQikLogTest, JsonMeshMessagesRoundTrip) {
    using namespace geoqik;

    const std::vector<float> verts = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const std::vector<std::uint32_t> idx = {0, 1, 2};
    GeoQikMessageCommonData common;
    common.geometryId = make_uuid(50);
    common.idempotencyId = make_uuid(60);
    common.rgba = {0.2f, 0.4f, 0.6f, 1.0f};

    const core::UUID meshHandle = make_uuid(70);

    AddMeshWithOpts meshWithOverlay;
    meshWithOverlay.vertices = verts;
    meshWithOverlay.normals = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
    meshWithOverlay.colors = {1.f, 1.f, 1.f, 1.f};
    meshWithOverlay.triangleIndices = idx;
    meshWithOverlay.commonData = common;
    meshWithOverlay.segmentIndices = {0, 1, 1, 2};
    meshWithOverlay.segmentColors = {1.f, 0.f, 0.f, 1.f};
    meshWithOverlay.segmentLineWidth = 2.5f;
    meshWithOverlay.showSegments = true;
    meshWithOverlay.vertexColors = {0.f, 1.f, 0.f, 1.f};
    meshWithOverlay.vertexPointSize = 4.0f;
    meshWithOverlay.showVertices = true;

    const std::vector<GeoQikLogEntry> entries{meshWithOverlay,
                                              RemoveMesh{meshHandle},
                                              UpdateMeshWithOpts{meshHandle, verts, {}, {}},
                                              SetMeshColor{Color{0.1f, 0.2f, 0.3f, 1.0f}},
                                              SetMeshOverlayOpts{make_uuid(80), true, false},
                                              SetMeshRenderingOpts{make_uuid(90), MeshCullMode::front, true}};

    const std::filesystem::path path = make_temp_path("geoqik_log_mesh_roundtrip.gqklog.json");
    save_log_json(path, entries);

    const std::vector<GeoQikLogEntry> loadedEntries = load_log_json(path);
    EXPECT_EQ(entries, loadedEntries);

    (void)std::filesystem::remove(path);
}

TEST(GeoQikLogTest, JsonLoadRejectsMissingFile) {
    EXPECT_THROW(
        {
            auto entries = geoqik::load_log_json(make_temp_path("geoqik_missing_log_file.gqklog.json"));
            (void)entries;
        },
        std::runtime_error);
}

TEST(GeoQikLogTest, JsonLoadRejectsMalformedJson) {
    const std::filesystem::path notJsonPath = make_temp_path("geoqik_not_json.gqklog.json");
    {
        std::ofstream stream(notJsonPath, std::ios::binary);
        stream << "not json";
    }
    // Unparseable JSON: load_log_json wraps nlohmann::json::parse_error as std::runtime_error
    // for a uniform contract with load_log_binary (which always throws std::runtime_error).
    EXPECT_THROW((void)geoqik::load_log_json(notJsonPath), std::runtime_error);

    const std::filesystem::path bogusTypePath = make_temp_path("geoqik_bogus_type.gqklog.json");
    {
        std::ofstream stream(bogusTypePath, std::ios::binary);
        stream << R"([{"type":"Bogus"}])";
    }
    // Syntactically valid JSON, unknown "type": from_json's explicit check throws std::runtime_error.
    EXPECT_THROW((void)geoqik::load_log_json(bogusTypePath), std::runtime_error);

    (void)std::filesystem::remove(notJsonPath);
    (void)std::filesystem::remove(bogusTypePath);
}
