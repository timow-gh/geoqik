#include "GeoQikMessageSerialization.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace geoqik
{

namespace
{

enum class SerializedMessageType : std::uint32_t
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
  RotateGeometry = 13
};

template <typename T>
void write_pod(std::ostream& stream, const T& value)
{
  stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik message");
  }
}

template <typename T>
T read_pod(std::istream& stream)
{
  T value{};
  stream.read(reinterpret_cast<char*>(&value), sizeof(T));
  if (!stream)
  {
    throw std::runtime_error("Failed to read GeoQik message");
  }
  return value;
}

void write_uuid(std::ostream& stream, const core::UUID& uuid)
{
  const auto bytes = uuid.to_array();
  stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik message UUID");
  }
}

core::UUID read_uuid(std::istream& stream)
{
  std::array<std::uint8_t, 16> bytes{};
  stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  if (!stream)
  {
    throw std::runtime_error("Failed to read GeoQik message UUID");
  }
  return core::UUID(bytes);
}

void write_float_vector(std::ostream& stream, const std::vector<float>& values)
{
  const std::uint64_t count = static_cast<std::uint64_t>(values.size());
  write_pod(stream, count);
  if (!values.empty())
  {
    stream.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(float)));
    if (!stream)
    {
      throw std::runtime_error("Failed to write GeoQik message float vector");
    }
  }
}

std::vector<float> read_float_vector(std::istream& stream)
{
  const std::uint64_t count = read_pod<std::uint64_t>(stream);
  if (count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
  {
    throw std::runtime_error("GeoQik message vector is too large");
  }

  std::vector<float> values(static_cast<std::size_t>(count));
  if (!values.empty())
  {
    stream.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(float)));
    if (!stream)
    {
      throw std::runtime_error("Failed to read GeoQik message float vector");
    }
  }
  return values;
}

void write_color(std::ostream& stream, const Color& color)
{
  for (const float channel: color)
  {
    write_pod(stream, channel);
  }
}

Color read_color(std::istream& stream)
{
  Color color{};
  for (float& channel: color)
  {
    channel = read_pod<float>(stream);
  }
  return color;
}

void write_common_data(std::ostream& stream, const GeoQikMessageCommonData& commonData)
{
  write_uuid(stream, commonData.geometryId);
  write_uuid(stream, commonData.idempotencyId);
  write_float_vector(stream, commonData.rgba);
}

GeoQikMessageCommonData read_common_data(std::istream& stream)
{
  GeoQikMessageCommonData commonData;
  commonData.geometryId = read_uuid(stream);
  commonData.idempotencyId = read_uuid(stream);
  commonData.rgba = read_float_vector(stream);
  return commonData;
}

} // namespace

MessageWriter::MessageWriter(std::ostream& stream)
    : m_stream(stream)
{
}

void MessageWriter::write(const GeoQikLogEntry& message)
{
  std::visit(
      [this](const auto& value)
      {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, AddPointWithOpts>)
        {
          write_pod(m_stream, SerializedMessageType::AddPointWithOpts);
          write_pod(m_stream, value.x);
          write_pod(m_stream, value.y);
          write_pod(m_stream, value.z);
          write_common_data(m_stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, AddPointsWithOpts>)
        {
          write_pod(m_stream, SerializedMessageType::AddPointsWithOpts);
          write_float_vector(m_stream, value.points);
          write_common_data(m_stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, RemovePoint>)
        {
          write_pod(m_stream, SerializedMessageType::RemovePoint);
          write_uuid(m_stream, value.handle);
        }
        else if constexpr (std::is_same_v<T, SetPointSize>)
        {
          write_pod(m_stream, SerializedMessageType::SetPointSize);
          write_pod(m_stream, value.size);
        }
        else if constexpr (std::is_same_v<T, SetPointColor>)
        {
          write_pod(m_stream, SerializedMessageType::SetPointColor);
          write_color(m_stream, value.color);
        }
        else if constexpr (std::is_same_v<T, AddLineWithOpts>)
        {
          write_pod(m_stream, SerializedMessageType::AddLineWithOpts);
          write_pod(m_stream, value.x1);
          write_pod(m_stream, value.y1);
          write_pod(m_stream, value.z1);
          write_pod(m_stream, value.x2);
          write_pod(m_stream, value.y2);
          write_pod(m_stream, value.z2);
          write_common_data(m_stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, AddLinesWithOpts>)
        {
          write_pod(m_stream, SerializedMessageType::AddLinesWithOpts);
          write_float_vector(m_stream, value.lines);
          write_common_data(m_stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, RemoveLine>)
        {
          write_pod(m_stream, SerializedMessageType::RemoveLine);
          write_uuid(m_stream, value.handle);
        }
        else if constexpr (std::is_same_v<T, SetLineWidth>)
        {
          write_pod(m_stream, SerializedMessageType::SetLineWidth);
          write_pod(m_stream, value.width);
        }
        else if constexpr (std::is_same_v<T, SetLineColor>)
        {
          write_pod(m_stream, SerializedMessageType::SetLineColor);
          write_color(m_stream, value.color);
        }
        else if constexpr (std::is_same_v<T, RemoveAllGeometry>)
        {
          write_pod(m_stream, SerializedMessageType::RemoveAllGeometry);
        }
        else if constexpr (std::is_same_v<T, TranslateGeometry>)
        {
          write_pod(m_stream, SerializedMessageType::TranslateGeometry);
          write_uuid(m_stream, value.handle);
          write_pod(m_stream, value.dx);
          write_pod(m_stream, value.dy);
          write_pod(m_stream, value.dz);
        }
        else if constexpr (std::is_same_v<T, RotateGeometry>)
        {
          write_pod(m_stream, SerializedMessageType::RotateGeometry);
          write_uuid(m_stream, value.handle);
          write_pod(m_stream, value.centerX);
          write_pod(m_stream, value.centerY);
          write_pod(m_stream, value.centerZ);
          write_pod(m_stream, value.axisX);
          write_pod(m_stream, value.axisY);
          write_pod(m_stream, value.axisZ);
          write_pod(m_stream, value.angle);
        }
      },
      message);
}

MessageReader::MessageReader(std::istream& stream)
    : m_stream(stream)
{
}

GeoQikLogEntry MessageReader::read()
{
  switch (read_pod<SerializedMessageType>(m_stream))
  {
  case SerializedMessageType::AddPointWithOpts:
    return AddPointWithOpts{read_pod<float>(m_stream), read_pod<float>(m_stream), read_pod<float>(m_stream), read_common_data(m_stream)};
  case SerializedMessageType::AddPointsWithOpts: return AddPointsWithOpts{read_float_vector(m_stream), read_common_data(m_stream)};
  case SerializedMessageType::RemovePoint: return RemovePoint{read_uuid(m_stream)};
  case SerializedMessageType::SetPointSize: return SetPointSize{read_pod<float>(m_stream)};
  case SerializedMessageType::SetPointColor: return SetPointColor{read_color(m_stream)};
  case SerializedMessageType::AddLineWithOpts:
    return AddLineWithOpts{read_pod<float>(m_stream),
                           read_pod<float>(m_stream),
                           read_pod<float>(m_stream),
                           read_pod<float>(m_stream),
                           read_pod<float>(m_stream),
                           read_pod<float>(m_stream),
                           read_common_data(m_stream)};
  case SerializedMessageType::AddLinesWithOpts: return AddLinesWithOpts{read_float_vector(m_stream), read_common_data(m_stream)};
  case SerializedMessageType::RemoveLine: return RemoveLine{read_uuid(m_stream)};
  case SerializedMessageType::SetLineWidth: return SetLineWidth{read_pod<float>(m_stream)};
  case SerializedMessageType::SetLineColor: return SetLineColor{read_color(m_stream)};
  case SerializedMessageType::RemoveAllGeometry: return RemoveAllGeometry{};
  case SerializedMessageType::TranslateGeometry:
    return TranslateGeometry{read_uuid(m_stream), read_pod<float>(m_stream), read_pod<float>(m_stream), read_pod<float>(m_stream)};
  case SerializedMessageType::RotateGeometry:
    return RotateGeometry{read_uuid(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream),
                          read_pod<float>(m_stream)};
  default: throw std::runtime_error("Unknown GeoQik message type");
  }
}

} // namespace geoqik
