#include "GeoQikMessageJsonSerialization.hpp"
#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace geoqik {

namespace {

std::string to_hex_string(const core::UUID& uuid) {
    return uuid.to_string();
}

core::UUID from_hex_string(const std::string& text) {
    std::array<std::uint8_t, 16> bytes{};
    std::size_t byteIndex = 0;
    std::string hexDigits;
    hexDigits.reserve(32);
    for (const char c: text) {
        if (c == '-') {
            continue;
        }
        hexDigits.push_back(c);
    }

    if (hexDigits.size() != 32) {
        throw std::runtime_error("Invalid GeoQik JSON log UUID string: " + text);
    }

    for (byteIndex = 0; byteIndex < 16; ++byteIndex) {
        const std::string byteText = hexDigits.substr(byteIndex * 2, 2);
        bytes[byteIndex] = static_cast<std::uint8_t>(std::stoul(byteText, nullptr, 16));
    }

    return core::UUID(bytes);
}

void write_uuid(nlohmann::json& json, const char* key, const core::UUID& uuid) {
    json[key] = to_hex_string(uuid);
}

core::UUID read_uuid(const nlohmann::json& json, const char* key) {
    return from_hex_string(json.at(key).get<std::string>());
}

nlohmann::json common_data_to_json(const GeoQikMessageCommonData& commonData) {
    nlohmann::json json;
    json["geometryId"] = to_hex_string(commonData.geometryId);
    json["idempotencyId"] = to_hex_string(commonData.idempotencyId);
    json["rgba"] = commonData.rgba;
    return json;
}

GeoQikMessageCommonData common_data_from_json(const nlohmann::json& json) {
    GeoQikMessageCommonData commonData;
    commonData.geometryId = from_hex_string(json.at("geometryId").get<std::string>());
    commonData.idempotencyId = from_hex_string(json.at("idempotencyId").get<std::string>());
    commonData.rgba = json.at("rgba").get<std::vector<float>>();
    return commonData;
}

nlohmann::json color_to_json(const Color& color) {
    nlohmann::json json = nlohmann::json::array();
    for (const float channel: color) {
        json.push_back(channel);
    }
    return json;
}

Color color_from_json(const nlohmann::json& json) {
    Color color{};
    const auto values = json.get<std::vector<float>>();
    if (values.size() != color.size()) {
        throw std::runtime_error("Invalid GeoQik JSON log color array size");
    }
    for (std::size_t i = 0; i < color.size(); ++i) {
        color[i] = values[i];
    }
    return color;
}

} // namespace

nlohmann::json to_json(const GeoQikLogEntry& message) {
    nlohmann::json json;
    std::visit(
        [&json](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AddPointWithOpts>) {
                json["type"] = "AddPointWithOpts";
                json["x"] = value.x;
                json["y"] = value.y;
                json["z"] = value.z;
                json["commonData"] = common_data_to_json(value.commonData);
            } else if constexpr (std::is_same_v<T, AddPointsWithOpts>) {
                json["type"] = "AddPointsWithOpts";
                json["points"] = value.points;
                json["commonData"] = common_data_to_json(value.commonData);
            } else if constexpr (std::is_same_v<T, UpdatePointWithOpts>) {
                json["type"] = "UpdatePointWithOpts";
                write_uuid(json, "handle", value.handle);
                json["x"] = value.x;
                json["y"] = value.y;
                json["z"] = value.z;
                json["rgba"] = value.rgba;
            } else if constexpr (std::is_same_v<T, UpdatePointsWithOpts>) {
                json["type"] = "UpdatePointsWithOpts";
                write_uuid(json, "handle", value.handle);
                json["points"] = value.points;
                json["rgba"] = value.rgba;
            } else if constexpr (std::is_same_v<T, RemovePoint>) {
                json["type"] = "RemovePoint";
                write_uuid(json, "handle", value.handle);
            } else if constexpr (std::is_same_v<T, SetPointSize>) {
                json["type"] = "SetPointSize";
                json["size"] = value.size;
            } else if constexpr (std::is_same_v<T, SetPointColor>) {
                json["type"] = "SetPointColor";
                json["color"] = color_to_json(value.color);
            } else if constexpr (std::is_same_v<T, AddLineWithOpts>) {
                json["type"] = "AddLineWithOpts";
                json["x1"] = value.x1;
                json["y1"] = value.y1;
                json["z1"] = value.z1;
                json["x2"] = value.x2;
                json["y2"] = value.y2;
                json["z2"] = value.z2;
                json["commonData"] = common_data_to_json(value.commonData);
            } else if constexpr (std::is_same_v<T, AddLinesWithOpts>) {
                json["type"] = "AddLinesWithOpts";
                json["lines"] = value.lines;
                json["commonData"] = common_data_to_json(value.commonData);
            } else if constexpr (std::is_same_v<T, UpdateLineWithOpts>) {
                json["type"] = "UpdateLineWithOpts";
                write_uuid(json, "handle", value.handle);
                json["x1"] = value.x1;
                json["y1"] = value.y1;
                json["z1"] = value.z1;
                json["x2"] = value.x2;
                json["y2"] = value.y2;
                json["z2"] = value.z2;
                json["rgba"] = value.rgba;
            } else if constexpr (std::is_same_v<T, UpdateLinesWithOpts>) {
                json["type"] = "UpdateLinesWithOpts";
                write_uuid(json, "handle", value.handle);
                json["lines"] = value.lines;
                json["rgba"] = value.rgba;
            } else if constexpr (std::is_same_v<T, RemoveLine>) {
                json["type"] = "RemoveLine";
                write_uuid(json, "handle", value.handle);
            } else if constexpr (std::is_same_v<T, SetLineWidth>) {
                json["type"] = "SetLineWidth";
                json["width"] = value.width;
            } else if constexpr (std::is_same_v<T, SetLineColor>) {
                json["type"] = "SetLineColor";
                json["color"] = color_to_json(value.color);
            } else if constexpr (std::is_same_v<T, RemoveAllGeometry>) {
                json["type"] = "RemoveAllGeometry";
            } else if constexpr (std::is_same_v<T, TranslateGeometry>) {
                json["type"] = "TranslateGeometry";
                write_uuid(json, "handle", value.handle);
                json["dx"] = value.dx;
                json["dy"] = value.dy;
                json["dz"] = value.dz;
            } else if constexpr (std::is_same_v<T, RotateGeometry>) {
                json["type"] = "RotateGeometry";
                write_uuid(json, "handle", value.handle);
                json["centerX"] = value.centerX;
                json["centerY"] = value.centerY;
                json["centerZ"] = value.centerZ;
                json["axisX"] = value.axisX;
                json["axisY"] = value.axisY;
                json["axisZ"] = value.axisZ;
                json["angle"] = value.angle;
            } else if constexpr (std::is_same_v<T, SetMeshColor>) {
                json["type"] = "SetMeshColor";
                json["color"] = color_to_json(value.color);
            } else if constexpr (std::is_same_v<T, AddMeshWithOpts>) {
                json["type"] = "AddMeshWithOpts";
                json["commonData"] = common_data_to_json(value.commonData);
                json["vertices"] = value.vertices;
                json["normals"] = value.normals;
                json["colors"] = value.colors;
                json["triangleIndices"] = value.triangleIndices;
                json["segmentIndices"] = value.segmentIndices;
                json["segmentColors"] = value.segmentColors;
                json["segmentLineWidth"] = value.segmentLineWidth;
                json["showSegments"] = value.showSegments;
                json["vertexColors"] = value.vertexColors;
                json["vertexPointSize"] = value.vertexPointSize;
                json["showVertices"] = value.showVertices;
            } else if constexpr (std::is_same_v<T, RemoveMesh>) {
                json["type"] = "RemoveMesh";
                write_uuid(json, "handle", value.handle);
            } else if constexpr (std::is_same_v<T, UpdateMeshWithOpts>) {
                json["type"] = "UpdateMeshWithOpts";
                write_uuid(json, "handle", value.handle);
                json["vertices"] = value.vertices;
                json["normals"] = value.normals;
                json["colors"] = value.colors;
            } else if constexpr (std::is_same_v<T, SetMeshOverlayOpts>) {
                json["type"] = "SetMeshOverlayOpts";
                write_uuid(json, "handle", value.handle);
                json["showSegments"] = value.showSegments;
                json["showVertices"] = value.showVertices;
            } else if constexpr (std::is_same_v<T, SetMeshRenderingOpts>) {
                json["type"] = "SetMeshRenderingOpts";
                write_uuid(json, "handle", value.handle);
                json["cullMode"] = static_cast<std::uint8_t>(value.cullMode);
                json["surfaceVisible"] = value.surfaceVisible;
            }
        },
        message);
    return json;
}

GeoQikLogEntry from_json(const nlohmann::json& value) {
    const std::string typeStr = value.at("type").get<std::string>();

    if (typeStr == "AddPointWithOpts") {
        AddPointWithOpts msg;
        msg.x = value.at("x").get<float>();
        msg.y = value.at("y").get<float>();
        msg.z = value.at("z").get<float>();
        msg.commonData = common_data_from_json(value.at("commonData"));
        return msg;
    }
    if (typeStr == "AddPointsWithOpts") {
        AddPointsWithOpts msg;
        msg.points = value.at("points").get<std::vector<float>>();
        msg.commonData = common_data_from_json(value.at("commonData"));
        return msg;
    }
    if (typeStr == "UpdatePointWithOpts") {
        UpdatePointWithOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.x = value.at("x").get<float>();
        msg.y = value.at("y").get<float>();
        msg.z = value.at("z").get<float>();
        msg.rgba = value.at("rgba").get<std::vector<float>>();
        return msg;
    }
    if (typeStr == "UpdatePointsWithOpts") {
        UpdatePointsWithOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.points = value.at("points").get<std::vector<float>>();
        msg.rgba = value.at("rgba").get<std::vector<float>>();
        return msg;
    }
    if (typeStr == "RemovePoint") {
        RemovePoint msg;
        msg.handle = read_uuid(value, "handle");
        return msg;
    }
    if (typeStr == "SetPointSize") {
        SetPointSize msg;
        msg.size = value.at("size").get<float>();
        return msg;
    }
    if (typeStr == "SetPointColor") {
        SetPointColor msg;
        msg.color = color_from_json(value.at("color"));
        return msg;
    }
    if (typeStr == "AddLineWithOpts") {
        AddLineWithOpts msg;
        msg.x1 = value.at("x1").get<float>();
        msg.y1 = value.at("y1").get<float>();
        msg.z1 = value.at("z1").get<float>();
        msg.x2 = value.at("x2").get<float>();
        msg.y2 = value.at("y2").get<float>();
        msg.z2 = value.at("z2").get<float>();
        msg.commonData = common_data_from_json(value.at("commonData"));
        return msg;
    }
    if (typeStr == "AddLinesWithOpts") {
        AddLinesWithOpts msg;
        msg.lines = value.at("lines").get<std::vector<float>>();
        msg.commonData = common_data_from_json(value.at("commonData"));
        return msg;
    }
    if (typeStr == "UpdateLineWithOpts") {
        UpdateLineWithOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.x1 = value.at("x1").get<float>();
        msg.y1 = value.at("y1").get<float>();
        msg.z1 = value.at("z1").get<float>();
        msg.x2 = value.at("x2").get<float>();
        msg.y2 = value.at("y2").get<float>();
        msg.z2 = value.at("z2").get<float>();
        msg.rgba = value.at("rgba").get<std::vector<float>>();
        return msg;
    }
    if (typeStr == "UpdateLinesWithOpts") {
        UpdateLinesWithOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.lines = value.at("lines").get<std::vector<float>>();
        msg.rgba = value.at("rgba").get<std::vector<float>>();
        return msg;
    }
    if (typeStr == "RemoveLine") {
        RemoveLine msg;
        msg.handle = read_uuid(value, "handle");
        return msg;
    }
    if (typeStr == "SetLineWidth") {
        SetLineWidth msg;
        msg.width = value.at("width").get<float>();
        return msg;
    }
    if (typeStr == "SetLineColor") {
        SetLineColor msg;
        msg.color = color_from_json(value.at("color"));
        return msg;
    }
    if (typeStr == "RemoveAllGeometry") {
        return RemoveAllGeometry{};
    }
    if (typeStr == "TranslateGeometry") {
        TranslateGeometry msg;
        msg.handle = read_uuid(value, "handle");
        msg.dx = value.at("dx").get<float>();
        msg.dy = value.at("dy").get<float>();
        msg.dz = value.at("dz").get<float>();
        return msg;
    }
    if (typeStr == "RotateGeometry") {
        RotateGeometry msg;
        msg.handle = read_uuid(value, "handle");
        msg.centerX = value.at("centerX").get<float>();
        msg.centerY = value.at("centerY").get<float>();
        msg.centerZ = value.at("centerZ").get<float>();
        msg.axisX = value.at("axisX").get<float>();
        msg.axisY = value.at("axisY").get<float>();
        msg.axisZ = value.at("axisZ").get<float>();
        msg.angle = value.at("angle").get<float>();
        return msg;
    }
    if (typeStr == "SetMeshColor") {
        SetMeshColor msg;
        msg.color = color_from_json(value.at("color"));
        return msg;
    }
    if (typeStr == "AddMeshWithOpts") {
        AddMeshWithOpts msg;
        msg.commonData = common_data_from_json(value.at("commonData"));
        msg.vertices = value.at("vertices").get<std::vector<float>>();
        msg.normals = value.at("normals").get<std::vector<float>>();
        msg.colors = value.at("colors").get<std::vector<float>>();
        msg.triangleIndices = value.at("triangleIndices").get<std::vector<std::uint32_t>>();
        msg.segmentIndices = value.at("segmentIndices").get<std::vector<std::uint32_t>>();
        msg.segmentColors = value.at("segmentColors").get<std::vector<float>>();
        msg.segmentLineWidth = value.at("segmentLineWidth").get<float>();
        msg.showSegments = value.at("showSegments").get<bool>();
        msg.vertexColors = value.at("vertexColors").get<std::vector<float>>();
        msg.vertexPointSize = value.at("vertexPointSize").get<float>();
        msg.showVertices = value.at("showVertices").get<bool>();
        return msg;
    }
    if (typeStr == "RemoveMesh") {
        RemoveMesh msg;
        msg.handle = read_uuid(value, "handle");
        return msg;
    }
    if (typeStr == "UpdateMeshWithOpts") {
        UpdateMeshWithOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.vertices = value.at("vertices").get<std::vector<float>>();
        msg.normals = value.at("normals").get<std::vector<float>>();
        msg.colors = value.at("colors").get<std::vector<float>>();
        return msg;
    }
    if (typeStr == "SetMeshOverlayOpts") {
        SetMeshOverlayOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.showSegments = value.at("showSegments").get<bool>();
        msg.showVertices = value.at("showVertices").get<bool>();
        return msg;
    }
    if (typeStr == "SetMeshRenderingOpts") {
        SetMeshRenderingOpts msg;
        msg.handle = read_uuid(value, "handle");
        msg.cullMode = static_cast<MeshCullMode>(value.at("cullMode").get<std::uint8_t>());
        msg.surfaceVisible = value.at("surfaceVisible").get<bool>();
        return msg;
    }

    throw std::runtime_error("Unknown GeoQik JSON log message type: " + typeStr);
}

} // namespace geoqik
