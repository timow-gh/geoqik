#include "GeoQikMessageSerialization.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace geoqik {

namespace {

constexpr std::size_t uuidByteCount = 16;

enum class SerializedMessageType : std::uint32_t // NOLINT(performance-enum-size): persisted binary log format uses
                                                 // uint32_t.
{
    AddPointWithOpts = 1,
    AddPointsWithOpts = 2,
    RemovePoint = 3,
    SetPointSize = 4,
    SetPointColor = 5,
    AddLineWithOpts = 6,
    AddLinesWithOpts = 7,
    RemoveLine = 8,
    SetLineWidth = 9,
    SetLineColor = 10,
    RemoveAllGeometry = 11,
    TranslateGeometry = 12,
    RotateGeometry = 13,
    UpdatePointWithOpts = 14,
    UpdatePointsWithOpts = 15,
    UpdateLineWithOpts = 16,
    UpdateLinesWithOpts = 17,
    SetMeshColor = 18,
    AddMeshWithOpts = 19,
    RemoveMesh = 20,
    UpdateMeshWithOpts = 21,
    SetMeshOverlayOpts = 22,
    SetMeshRenderingOpts = 23
    // Next free ID: 24
};

constexpr auto serialized_message_type_value(SerializedMessageType type) {
    return static_cast<std::underlying_type_t<SerializedMessageType>>(type);
}

template <typename T>
void write_pod(std::ostream& stream, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!stream) {
        throw std::runtime_error("Failed to write GeoQik message");
    }
}

template <typename T>
T read_pod(std::istream& stream) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::array<char, sizeof(T)> bytes{};
    stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!stream) {
        throw std::runtime_error("Failed to read GeoQik message");
    }
    return std::bit_cast<T>(bytes);
}

void write_uuid(std::ostream& stream, const core::UUID& uuid) {
    const auto bytes = uuid.to_array();
    for (const std::uint8_t byte: bytes) {
        stream.put(static_cast<char>(byte));
        if (!stream) {
            throw std::runtime_error("Failed to write GeoQik message UUID");
        }
    }
}

core::UUID read_uuid(std::istream& stream) {
    std::array<std::uint8_t, uuidByteCount> bytes{};
    for (std::uint8_t& byte: bytes) {
        char value{};
        stream.get(value);
        if (!stream) {
            throw std::runtime_error("Failed to read GeoQik message UUID");
        }
        byte = static_cast<std::uint8_t>(value);
    }
    return core::UUID(bytes);
}

void write_float_vector(std::ostream& stream, const std::vector<float>& values) {
    const std::uint64_t count = values.size();
    write_pod(stream, count);
    if (!values.empty()) {
        for (const float value: values) {
            write_pod(stream, value);
        }
    }
}

std::vector<float> read_float_vector(std::istream& stream) {
    const auto count = read_pod<std::uint64_t>(stream);
    if (count > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("GeoQik message vector is too large");
    }

    auto values = std::vector<float>(static_cast<std::size_t>(count));
    for (float& value: values) {
        value = read_pod<float>(stream);
    }
    return values;
}

void write_color(std::ostream& stream, const Color& color) {
    for (const float channel: color) {
        write_pod(stream, channel);
    }
}

Color read_color(std::istream& stream) {
    Color color{};
    for (float& channel: color) {
        channel = read_pod<float>(stream);
    }
    return color;
}

void write_common_data(std::ostream& stream, const GeoQikMessageCommonData& commonData) {
    write_uuid(stream, commonData.geometryId);
    write_uuid(stream, commonData.idempotencyId);
    write_float_vector(stream, commonData.rgba);
}

GeoQikMessageCommonData read_common_data(std::istream& stream) {
    GeoQikMessageCommonData commonData;
    commonData.geometryId = read_uuid(stream);
    commonData.idempotencyId = read_uuid(stream);
    commonData.rgba = read_float_vector(stream);
    return commonData;
}

} // namespace

MessageWriter::MessageWriter(std::ostream& stream)
    : m_stream(stream) {
}

void MessageWriter::write(const GeoQikLogEntry& message) {
    std::visit(
        [this](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AddPointWithOpts>) {
                write_pod(m_stream, SerializedMessageType::AddPointWithOpts);
                write_pod(m_stream, value.x);
                write_pod(m_stream, value.y);
                write_pod(m_stream, value.z);
                write_common_data(m_stream, value.commonData);
            } else if constexpr (std::is_same_v<T, AddPointsWithOpts>) {
                write_pod(m_stream, SerializedMessageType::AddPointsWithOpts);
                write_float_vector(m_stream, value.points);
                write_common_data(m_stream, value.commonData);
            } else if constexpr (std::is_same_v<T, UpdatePointWithOpts>) {
                write_pod(m_stream, SerializedMessageType::UpdatePointWithOpts);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, value.x);
                write_pod(m_stream, value.y);
                write_pod(m_stream, value.z);
                write_float_vector(m_stream, value.rgba);
            } else if constexpr (std::is_same_v<T, UpdatePointsWithOpts>) {
                write_pod(m_stream, SerializedMessageType::UpdatePointsWithOpts);
                write_uuid(m_stream, value.handle);
                write_float_vector(m_stream, value.points);
                write_float_vector(m_stream, value.rgba);
            } else if constexpr (std::is_same_v<T, RemovePoint>) {
                write_pod(m_stream, SerializedMessageType::RemovePoint);
                write_uuid(m_stream, value.handle);
            } else if constexpr (std::is_same_v<T, SetPointSize>) {
                write_pod(m_stream, SerializedMessageType::SetPointSize);
                write_pod(m_stream, value.size);
            } else if constexpr (std::is_same_v<T, SetPointColor>) {
                write_pod(m_stream, SerializedMessageType::SetPointColor);
                write_color(m_stream, value.color);
            } else if constexpr (std::is_same_v<T, AddLineWithOpts>) {
                write_pod(m_stream, SerializedMessageType::AddLineWithOpts);
                write_pod(m_stream, value.x1);
                write_pod(m_stream, value.y1);
                write_pod(m_stream, value.z1);
                write_pod(m_stream, value.x2);
                write_pod(m_stream, value.y2);
                write_pod(m_stream, value.z2);
                write_common_data(m_stream, value.commonData);
            } else if constexpr (std::is_same_v<T, AddLinesWithOpts>) {
                write_pod(m_stream, SerializedMessageType::AddLinesWithOpts);
                write_float_vector(m_stream, value.lines);
                write_common_data(m_stream, value.commonData);
            } else if constexpr (std::is_same_v<T, UpdateLineWithOpts>) {
                write_pod(m_stream, SerializedMessageType::UpdateLineWithOpts);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, value.x1);
                write_pod(m_stream, value.y1);
                write_pod(m_stream, value.z1);
                write_pod(m_stream, value.x2);
                write_pod(m_stream, value.y2);
                write_pod(m_stream, value.z2);
                write_float_vector(m_stream, value.rgba);
            } else if constexpr (std::is_same_v<T, UpdateLinesWithOpts>) {
                write_pod(m_stream, SerializedMessageType::UpdateLinesWithOpts);
                write_uuid(m_stream, value.handle);
                write_float_vector(m_stream, value.lines);
                write_float_vector(m_stream, value.rgba);
            } else if constexpr (std::is_same_v<T, RemoveLine>) {
                write_pod(m_stream, SerializedMessageType::RemoveLine);
                write_uuid(m_stream, value.handle);
            } else if constexpr (std::is_same_v<T, SetLineWidth>) {
                write_pod(m_stream, SerializedMessageType::SetLineWidth);
                write_pod(m_stream, value.width);
            } else if constexpr (std::is_same_v<T, SetLineColor>) {
                write_pod(m_stream, SerializedMessageType::SetLineColor);
                write_color(m_stream, value.color);
            } else if constexpr (std::is_same_v<T, RemoveAllGeometry>) {
                write_pod(m_stream, SerializedMessageType::RemoveAllGeometry);
            } else if constexpr (std::is_same_v<T, TranslateGeometry>) {
                write_pod(m_stream, SerializedMessageType::TranslateGeometry);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, value.dx);
                write_pod(m_stream, value.dy);
                write_pod(m_stream, value.dz);
            } else if constexpr (std::is_same_v<T, RotateGeometry>) {
                write_pod(m_stream, SerializedMessageType::RotateGeometry);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, value.centerX);
                write_pod(m_stream, value.centerY);
                write_pod(m_stream, value.centerZ);
                write_pod(m_stream, value.axisX);
                write_pod(m_stream, value.axisY);
                write_pod(m_stream, value.axisZ);
                write_pod(m_stream, value.angle);
            } else if constexpr (std::is_same_v<T, SetMeshColor>) {
                write_pod(m_stream, SerializedMessageType::SetMeshColor);
                write_color(m_stream, value.color);
            } else if constexpr (std::is_same_v<T, AddMeshWithOpts>) {
                write_pod(m_stream, SerializedMessageType::AddMeshWithOpts);
                write_common_data(m_stream, value.commonData);
                write_float_vector(m_stream, value.vertices);
                write_float_vector(m_stream, value.normals);
                write_pod(m_stream, static_cast<std::uint64_t>(value.triangleIndices.size()));
                for (std::uint32_t idx: value.triangleIndices) {
                    write_pod(m_stream, idx);
                }
                // Segment overlay (added for overlay replay support)
                write_pod(m_stream, static_cast<std::uint64_t>(value.segmentIndices.size()));
                for (std::uint32_t idx: value.segmentIndices) {
                    write_pod(m_stream, idx);
                }
                write_float_vector(m_stream, value.segmentColors);
                write_pod(m_stream, value.segmentLineWidth);
                write_pod(m_stream, static_cast<std::uint8_t>(value.showSegments ? 1 : 0));
                // Vertex overlay
                write_float_vector(m_stream, value.vertexColors);
                write_pod(m_stream, value.vertexPointSize);
                write_pod(m_stream, static_cast<std::uint8_t>(value.showVertices ? 1 : 0));
            } else if constexpr (std::is_same_v<T, RemoveMesh>) {
                write_pod(m_stream, SerializedMessageType::RemoveMesh);
                write_uuid(m_stream, value.handle);
            } else if constexpr (std::is_same_v<T, UpdateMeshWithOpts>) {
                write_pod(m_stream, SerializedMessageType::UpdateMeshWithOpts);
                write_uuid(m_stream, value.handle);
                write_float_vector(m_stream, value.vertices);
                write_float_vector(m_stream, value.normals);
                write_float_vector(m_stream, value.colors);
            } else if constexpr (std::is_same_v<T, SetMeshOverlayOpts>) {
                write_pod(m_stream, SerializedMessageType::SetMeshOverlayOpts);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, static_cast<std::uint8_t>(value.showSegments ? 1 : 0));
                write_pod(m_stream, static_cast<std::uint8_t>(value.showVertices ? 1 : 0));
            } else if constexpr (std::is_same_v<T, SetMeshRenderingOpts>) {
                write_pod(m_stream, SerializedMessageType::SetMeshRenderingOpts);
                write_uuid(m_stream, value.handle);
                write_pod(m_stream, static_cast<std::uint8_t>(value.cullMode));
                write_pod(m_stream, static_cast<std::uint8_t>(value.surfaceVisible ? 1 : 0));
            }
        },
        message);
}

MessageReader::MessageReader(std::istream& stream)
    : m_stream(stream) {
}

GeoQikLogEntry MessageReader::read() {
    const auto messageType = read_pod<std::underlying_type_t<SerializedMessageType>>(m_stream);
    switch (messageType) {
    case serialized_message_type_value(SerializedMessageType::AddPointWithOpts):
        return AddPointWithOpts{read_pod<float>(m_stream),
                                read_pod<float>(m_stream),
                                read_pod<float>(m_stream),
                                read_common_data(m_stream)};
    case serialized_message_type_value(SerializedMessageType::AddPointsWithOpts):
        return AddPointsWithOpts{read_float_vector(m_stream), read_common_data(m_stream)};
    case serialized_message_type_value(SerializedMessageType::UpdatePointWithOpts):
        return UpdatePointWithOpts{read_uuid(m_stream),
                                   read_pod<float>(m_stream),
                                   read_pod<float>(m_stream),
                                   read_pod<float>(m_stream),
                                   read_float_vector(m_stream)};
    case serialized_message_type_value(SerializedMessageType::UpdatePointsWithOpts):
        return UpdatePointsWithOpts{read_uuid(m_stream), read_float_vector(m_stream), read_float_vector(m_stream)};
    case serialized_message_type_value(SerializedMessageType::RemovePoint): return RemovePoint{read_uuid(m_stream)};
    case serialized_message_type_value(SerializedMessageType::SetPointSize):
        return SetPointSize{read_pod<float>(m_stream)};
    case serialized_message_type_value(SerializedMessageType::SetPointColor):
        return SetPointColor{read_color(m_stream)};
    case serialized_message_type_value(SerializedMessageType::AddLineWithOpts):
        return AddLineWithOpts{read_pod<float>(m_stream),
                               read_pod<float>(m_stream),
                               read_pod<float>(m_stream),
                               read_pod<float>(m_stream),
                               read_pod<float>(m_stream),
                               read_pod<float>(m_stream),
                               read_common_data(m_stream)};
    case serialized_message_type_value(SerializedMessageType::AddLinesWithOpts):
        return AddLinesWithOpts{read_float_vector(m_stream), read_common_data(m_stream)};
    case serialized_message_type_value(SerializedMessageType::UpdateLineWithOpts):
        return UpdateLineWithOpts{read_uuid(m_stream),
                                  read_pod<float>(m_stream),
                                  read_pod<float>(m_stream),
                                  read_pod<float>(m_stream),
                                  read_pod<float>(m_stream),
                                  read_pod<float>(m_stream),
                                  read_pod<float>(m_stream),
                                  read_float_vector(m_stream)};
    case serialized_message_type_value(SerializedMessageType::UpdateLinesWithOpts):
        return UpdateLinesWithOpts{read_uuid(m_stream), read_float_vector(m_stream), read_float_vector(m_stream)};
    case serialized_message_type_value(SerializedMessageType::RemoveLine): return RemoveLine{read_uuid(m_stream)};
    case serialized_message_type_value(SerializedMessageType::SetLineWidth):
        return SetLineWidth{read_pod<float>(m_stream)};
    case serialized_message_type_value(SerializedMessageType::SetLineColor):      return SetLineColor{read_color(m_stream)};
    case serialized_message_type_value(SerializedMessageType::RemoveAllGeometry): return RemoveAllGeometry{};
    case serialized_message_type_value(SerializedMessageType::TranslateGeometry):
        return TranslateGeometry{read_uuid(m_stream),
                                 read_pod<float>(m_stream),
                                 read_pod<float>(m_stream),
                                 read_pod<float>(m_stream)};
    case serialized_message_type_value(SerializedMessageType::RotateGeometry):
        return RotateGeometry{read_uuid(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream),
                              read_pod<float>(m_stream)};
    case serialized_message_type_value(SerializedMessageType::SetMeshColor): {
        Color color;
        color[0] = read_pod<float>(m_stream);
        color[1] = read_pod<float>(m_stream);
        color[2] = read_pod<float>(m_stream);
        color[3] = read_pod<float>(m_stream);
        return SetMeshColor{color};
    }
    case serialized_message_type_value(SerializedMessageType::AddMeshWithOpts): {
        GeoQikMessageCommonData common = read_common_data(m_stream);
        std::vector<float> verts = read_float_vector(m_stream);
        std::vector<float> norms = read_float_vector(m_stream);
        const auto idxCount = read_pod<std::uint64_t>(m_stream);
        std::vector<std::uint32_t> idxs(static_cast<std::size_t>(idxCount));
        for (auto& i: idxs) {
            i = read_pod<std::uint32_t>(m_stream);
        }
        AddMeshWithOpts msg;
        msg.vertices = std::move(verts);
        msg.normals = std::move(norms);
        msg.triangleIndices = std::move(idxs);
        msg.commonData = std::move(common);
        // Segment and vertex overlay fields (present in new-format logs only)
        if (m_stream.peek() != std::char_traits<char>::eof()) {
            const auto segIdxCount = read_pod<std::uint64_t>(m_stream);
            msg.segmentIndices.resize(static_cast<std::size_t>(segIdxCount));
            for (auto& i: msg.segmentIndices) {
                i = read_pod<std::uint32_t>(m_stream);
            }
            msg.segmentColors = read_float_vector(m_stream);
            msg.segmentLineWidth = read_pod<float>(m_stream);
            msg.showSegments = (read_pod<std::uint8_t>(m_stream) != 0);
            msg.vertexColors = read_float_vector(m_stream);
            msg.vertexPointSize = read_pod<float>(m_stream);
            msg.showVertices = (read_pod<std::uint8_t>(m_stream) != 0);
        }
        return msg;
    }
    case serialized_message_type_value(SerializedMessageType::RemoveMesh):         return RemoveMesh{read_uuid(m_stream)};
    case serialized_message_type_value(SerializedMessageType::UpdateMeshWithOpts): {
        auto handle = read_uuid(m_stream);
        std::vector<float> verts = read_float_vector(m_stream);
        std::vector<float> norms = read_float_vector(m_stream);
        std::vector<float> colors = read_float_vector(m_stream);
        return UpdateMeshWithOpts{handle, std::move(verts), std::move(norms), std::move(colors)};
    }
    case serialized_message_type_value(SerializedMessageType::SetMeshOverlayOpts): {
        SetMeshOverlayOpts msg;
        msg.handle = read_uuid(m_stream);
        msg.showSegments = (read_pod<std::uint8_t>(m_stream) != 0);
        msg.showVertices = (read_pod<std::uint8_t>(m_stream) != 0);
        return msg;
    }
    case serialized_message_type_value(SerializedMessageType::SetMeshRenderingOpts): {
        SetMeshRenderingOpts msg;
        msg.handle = read_uuid(m_stream);
        msg.cullMode = static_cast<MeshCullMode>(read_pod<std::uint8_t>(m_stream));
        msg.surfaceVisible = (read_pod<std::uint8_t>(m_stream) != 0);
        return msg;
    }
    default: throw std::runtime_error("Unknown GeoQik message type");
    }
}

} // namespace geoqik
