#include "GeoQikLog.hpp"
#include "GeoQikMessageSerialization.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
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

template <typename T>
void write_binary_value(std::ostream& stream, const T& value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

} // namespace

TEST(GeoQikLogTest, BinaryRoundTripPreservesRepresentativeEntries) {
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

    const std::filesystem::path path = make_temp_path("geoqik_log_roundtrip.gqklog");
    save_log_binary(path, entries);

    const std::vector<GeoQikLogEntry> loadedEntries = load_log_binary(path);
    EXPECT_EQ(entries, loadedEntries);

    (void)std::filesystem::remove(path);
}

TEST(GeoQikLogTest, MessageReaderWriterRoundTripPreservesRepresentativeEntries) {
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

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    MessageWriter writer(stream);
    for (const GeoQikLogEntry& entry: entries) {
        writer.write(entry);
    }

    stream.seekg(0);
    MessageReader reader(stream);
    std::vector<GeoQikLogEntry> loadedEntries;
    loadedEntries.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        loadedEntries.push_back(reader.read());
    }

    EXPECT_EQ(entries, loadedEntries);
}

TEST(GeoQikLogTest, BinaryLoadRejectsMissingFile) {
    EXPECT_THROW(
        {
            auto entries = geoqik::load_log_binary(make_temp_path("geoqik_missing_log_file.gqklog"));
            (void)entries;
        },
        std::runtime_error);
}

TEST(GeoQikLogTest, CreateLogEntryFiltersNonLogMessages) {
    EXPECT_TRUE(geoqik::create_log_entry(geoqik::GeoQikMessage{geoqik::SetPointSize{3.0f}}).has_value());
    EXPECT_FALSE(geoqik::create_log_entry(geoqik::GeoQikMessage{geoqik::Draw{}}).has_value());
    EXPECT_FALSE(geoqik::create_log_entry(geoqik::GeoQikMessage{geoqik::StopDraw{}}).has_value());
    EXPECT_FALSE(geoqik::create_log_entry(geoqik::GeoQikMessage{geoqik::Cleanup{}}).has_value());
}

TEST(GeoQikLogTest, BinaryLoadRejectsCorruptHeadersAndVersion) {
    const std::filesystem::path badHeaderPath = make_temp_path("geoqik_bad_header.gqklog");
    {
        std::ofstream stream(badHeaderPath, std::ios::binary);
        stream.write("not-log", 7);
    }
    EXPECT_THROW((void)geoqik::load_log_binary(badHeaderPath), std::runtime_error);

    const std::filesystem::path badVersionPath = make_temp_path("geoqik_bad_version.gqklog");
    {
        std::ofstream stream(badVersionPath, std::ios::binary);
        stream.write("GQKLOG01", 8);
        write_binary_value<std::uint32_t>(stream, 99);
        write_binary_value<std::uint64_t>(stream, 0);
    }
    EXPECT_THROW((void)geoqik::load_log_binary(badVersionPath), std::runtime_error);

    const std::filesystem::path truncatedPath = make_temp_path("geoqik_truncated.gqklog");
    {
        std::ofstream stream(truncatedPath, std::ios::binary);
        stream.write("GQKLOG01", 8);
    }
    EXPECT_THROW((void)geoqik::load_log_binary(truncatedPath), std::runtime_error);

    (void)std::filesystem::remove(badHeaderPath);
    (void)std::filesystem::remove(badVersionPath);
    (void)std::filesystem::remove(truncatedPath);
}

TEST(GeoQikLogTest, MeshMessages_RoundTrip) {
    using namespace geoqik;

    const std::vector<float> verts = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const std::vector<std::uint32_t> idx = {0, 1, 2};
    GeoQikMessageCommonData common;
    common.geometryId = make_uuid(50);
    common.idempotencyId = make_uuid(60);
    common.rgba = {0.2f, 0.4f, 0.6f, 1.0f};

    const core::UUID meshHandle = make_uuid(70);

    const std::vector<GeoQikLogEntry> entries{AddMeshWithOpts{verts, {}, {}, idx, common, {}, {}, 1.0f, false, {}},
                                              RemoveMesh{meshHandle},
                                              UpdateMeshWithOpts{meshHandle, verts, {}, {}},
                                              SetMeshColor{Color{0.1f, 0.2f, 0.3f, 1.0f}}};

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    MessageWriter writer(stream);
    for (const GeoQikLogEntry& entry: entries) {
        writer.write(entry);
    }

    stream.seekg(0);
    MessageReader reader(stream);
    std::vector<GeoQikLogEntry> loaded;
    loaded.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        loaded.push_back(reader.read());
    }

    EXPECT_EQ(entries, loaded);
}

TEST(GeoQikLogTest, MeshMessages_CreateLogEntry_IsRecorded) {
    using namespace geoqik;

    const std::vector<float> verts = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    const std::vector<std::uint32_t> idx = {0, 1, 2};
    GeoQikMessageCommonData common;
    common.geometryId = make_uuid(80);

    EXPECT_TRUE(create_log_entry(GeoQikMessage{AddMeshWithOpts{verts, {}, {}, idx, common, {}, {}, 1.0f, false, {}}})
                    .has_value());
    EXPECT_TRUE(create_log_entry(GeoQikMessage{RemoveMesh{make_uuid(81)}}).has_value());
    EXPECT_TRUE(create_log_entry(GeoQikMessage{UpdateMeshWithOpts{make_uuid(82), verts, {}, {}}}).has_value());
    EXPECT_TRUE(create_log_entry(GeoQikMessage{SetMeshColor{Color{0.5f, 0.5f, 0.5f, 1.0f}}}).has_value());
}

TEST(GeoQikLogTest, MessageWriterReaderRoundTrip_SetMeshOverlayOpts) {
    using namespace geoqik;

    const std::vector<GeoQikLogEntry> entries{SetMeshOverlayOpts{make_uuid(50), true, false},
                                              SetMeshOverlayOpts{make_uuid(51), false, true}};

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    MessageWriter writer(stream);
    for (const GeoQikLogEntry& entry: entries) {
        writer.write(entry);
    }

    stream.seekg(0);
    MessageReader reader(stream);
    for (const GeoQikLogEntry& expected: entries) {
        const GeoQikLogEntry actual = reader.read();
        EXPECT_EQ(actual, expected);
    }
}

TEST(GeoQikLogTest, MessageWriterReaderRoundTrip_SetMeshRenderingOpts) {
    using namespace geoqik;

    const std::vector<GeoQikLogEntry> entries{SetMeshRenderingOpts{make_uuid(60), MeshCullMode::none, false},
                                              SetMeshRenderingOpts{make_uuid(61), MeshCullMode::front, true}};

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    MessageWriter writer(stream);
    for (const GeoQikLogEntry& entry: entries) {
        writer.write(entry);
    }

    stream.seekg(0);
    MessageReader reader(stream);
    for (const GeoQikLogEntry& expected: entries) {
        const GeoQikLogEntry actual = reader.read();
        EXPECT_EQ(actual, expected);
    }
}

TEST(GeoQikLogTest, MessageWriterReaderRoundTrip_AddMeshWithOpts_OverlayFields) {
    using namespace geoqik;

    const GeoQikMessageCommonData commonData{make_uuid(70), make_uuid(71), {1.0f, 0.0f, 0.0f, 1.0f}};
    AddMeshWithOpts msg;
    msg.vertices = {0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f};
    msg.normals = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};
    msg.triangleIndices = {0, 1, 2};
    msg.commonData = commonData;
    msg.segmentIndices = {0, 1, 1, 2};
    msg.segmentColors = {1.f, 0.f, 0.f, 1.f};
    msg.segmentLineWidth = 2.5f;
    msg.showSegments = true;
    msg.vertexColors = {0.f, 1.f, 0.f, 1.f};
    msg.vertexPointSize = 4.0f;
    msg.showVertices = true;

    const std::vector<GeoQikLogEntry> entries{msg};

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    MessageWriter writer(stream);
    for (const GeoQikLogEntry& entry: entries) {
        writer.write(entry);
    }

    stream.seekg(0);
    MessageReader reader(stream);
    const GeoQikLogEntry actual = reader.read();
    EXPECT_EQ(actual, GeoQikLogEntry{msg});
}
