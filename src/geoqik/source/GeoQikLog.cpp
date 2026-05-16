#include "GeoQikLog.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace geoqik
{

namespace
{

constexpr std::array<char, 8> LogMagic{'G', 'Q', 'K', 'L', 'O', 'G', '0', '1'};
constexpr std::uint32_t LogVersion = 1;

enum class LogEntryType : std::uint32_t
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

GeoQikLogCommonData copy_common_data(const GeoQikMessageData::CommonMessageData& commonData)
{
  GeoQikLogCommonData result;
  result.geometryId = commonData.geometryId;
  result.idempotencyId = commonData.idempotencyId;
  if (commonData.rgba && commonData.rgbaCount > 0)
  {
    result.rgba.assign(commonData.rgba, commonData.rgba + commonData.rgbaCount);
  }
  return result;
}

template <typename T>
void write_pod(std::ostream& stream, const T& value)
{
  stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log");
  }
}

template <typename T>
T read_pod(std::istream& stream)
{
  T value{};
  stream.read(reinterpret_cast<char*>(&value), sizeof(T));
  if (!stream)
  {
    throw std::runtime_error("Failed to read GeoQik log");
  }
  return value;
}

void write_uuid(std::ostream& stream, const core::UUID& uuid)
{
  const auto bytes = uuid.to_array();
  stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log UUID");
  }
}

core::UUID read_uuid(std::istream& stream)
{
  std::array<std::uint8_t, 16> bytes{};
  stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  if (!stream)
  {
    throw std::runtime_error("Failed to read GeoQik log UUID");
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
      throw std::runtime_error("Failed to write GeoQik log float vector");
    }
  }
}

std::vector<float> read_float_vector(std::istream& stream)
{
  const std::uint64_t count = read_pod<std::uint64_t>(stream);
  if (count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
  {
    throw std::runtime_error("GeoQik log vector is too large");
  }

  std::vector<float> values(static_cast<std::size_t>(count));
  if (!values.empty())
  {
    stream.read(reinterpret_cast<char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(float)));
    if (!stream)
    {
      throw std::runtime_error("Failed to read GeoQik log float vector");
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

void write_common_data(std::ostream& stream, const GeoQikLogCommonData& commonData)
{
  write_uuid(stream, commonData.geometryId);
  write_uuid(stream, commonData.idempotencyId);
  write_float_vector(stream, commonData.rgba);
}

GeoQikLogCommonData read_common_data(std::istream& stream)
{
  GeoQikLogCommonData commonData;
  commonData.geometryId = read_uuid(stream);
  commonData.idempotencyId = read_uuid(stream);
  commonData.rgba = read_float_vector(stream);
  return commonData;
}

void write_entry(std::ostream& stream, const GeoQikLogEntry& entry)
{
  std::visit(
      [&](const auto& value)
      {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, GeoQikLogAddPointWithOpts>)
        {
          write_pod(stream, LogEntryType::AddPointWithOpts);
          write_pod(stream, value.x);
          write_pod(stream, value.y);
          write_pod(stream, value.z);
          write_common_data(stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogAddPointsWithOpts>)
        {
          write_pod(stream, LogEntryType::AddPointsWithOpts);
          write_float_vector(stream, value.points);
          write_common_data(stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogRemovePoint>)
        {
          write_pod(stream, LogEntryType::RemovePoint);
          write_uuid(stream, value.handle);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogSetPointSize>)
        {
          write_pod(stream, LogEntryType::SetPointSize);
          write_pod(stream, value.size);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogSetPointColor>)
        {
          write_pod(stream, LogEntryType::SetPointColor);
          write_color(stream, value.color);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogAddLineWithOpts>)
        {
          write_pod(stream, LogEntryType::AddLineWithOpts);
          write_pod(stream, value.x1);
          write_pod(stream, value.y1);
          write_pod(stream, value.z1);
          write_pod(stream, value.x2);
          write_pod(stream, value.y2);
          write_pod(stream, value.z2);
          write_common_data(stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogAddLinesWithOpts>)
        {
          write_pod(stream, LogEntryType::AddLinesWithOpts);
          write_float_vector(stream, value.lines);
          write_common_data(stream, value.commonData);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogRemoveLine>)
        {
          write_pod(stream, LogEntryType::RemoveLine);
          write_uuid(stream, value.handle);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogSetLineWidth>)
        {
          write_pod(stream, LogEntryType::SetLineWidth);
          write_pod(stream, value.width);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogSetLineColor>)
        {
          write_pod(stream, LogEntryType::SetLineColor);
          write_color(stream, value.color);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogRemoveAllGeometry>)
        {
          write_pod(stream, LogEntryType::RemoveAllGeometry);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogTranslateGeometry>)
        {
          write_pod(stream, LogEntryType::TranslateGeometry);
          write_uuid(stream, value.handle);
          write_pod(stream, value.dx);
          write_pod(stream, value.dy);
          write_pod(stream, value.dz);
        }
        else if constexpr (std::is_same_v<T, GeoQikLogRotateGeometry>)
        {
          write_pod(stream, LogEntryType::RotateGeometry);
          write_uuid(stream, value.handle);
          write_pod(stream, value.centerX);
          write_pod(stream, value.centerY);
          write_pod(stream, value.centerZ);
          write_pod(stream, value.axisX);
          write_pod(stream, value.axisY);
          write_pod(stream, value.axisZ);
          write_pod(stream, value.angle);
        }
      },
      entry);
}

GeoQikLogEntry read_entry(std::istream& stream)
{
  switch (read_pod<LogEntryType>(stream))
  {
  case LogEntryType::AddPointWithOpts:
    return GeoQikLogAddPointWithOpts{read_pod<float>(stream), read_pod<float>(stream), read_pod<float>(stream), read_common_data(stream)};
  case LogEntryType::AddPointsWithOpts: return GeoQikLogAddPointsWithOpts{read_float_vector(stream), read_common_data(stream)};
  case LogEntryType::RemovePoint: return GeoQikLogRemovePoint{read_uuid(stream)};
  case LogEntryType::SetPointSize: return GeoQikLogSetPointSize{read_pod<float>(stream)};
  case LogEntryType::SetPointColor: return GeoQikLogSetPointColor{read_color(stream)};
  case LogEntryType::AddLineWithOpts:
    return GeoQikLogAddLineWithOpts{read_pod<float>(stream),
                                    read_pod<float>(stream),
                                    read_pod<float>(stream),
                                    read_pod<float>(stream),
                                    read_pod<float>(stream),
                                    read_pod<float>(stream),
                                    read_common_data(stream)};
  case LogEntryType::AddLinesWithOpts: return GeoQikLogAddLinesWithOpts{read_float_vector(stream), read_common_data(stream)};
  case LogEntryType::RemoveLine: return GeoQikLogRemoveLine{read_uuid(stream)};
  case LogEntryType::SetLineWidth: return GeoQikLogSetLineWidth{read_pod<float>(stream)};
  case LogEntryType::SetLineColor: return GeoQikLogSetLineColor{read_color(stream)};
  case LogEntryType::RemoveAllGeometry: return GeoQikLogRemoveAllGeometry{};
  case LogEntryType::TranslateGeometry:
    return GeoQikLogTranslateGeometry{read_uuid(stream), read_pod<float>(stream), read_pod<float>(stream), read_pod<float>(stream)};
  case LogEntryType::RotateGeometry:
    return GeoQikLogRotateGeometry{read_uuid(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream),
                                   read_pod<float>(stream)};
  default: throw std::runtime_error("Unknown GeoQik log entry type");
  }
}

} // namespace

std::optional<GeoQikLogEntry> create_log_entry(const GeoQikMessage& message)
{
  switch (message.type)
  {
  case GeoQikMessageType::ADD_POINT_WITH_OPTS:
  {
    const auto& data = message.data.pointWithOpts;
    return GeoQikLogAddPointWithOpts{data.x, data.y, data.z, copy_common_data(data.commonData)};
  }
  case GeoQikMessageType::ADD_POINTS_WITH_OPTS:
  {
    const auto& data = message.data.pointsWithOpts;
    GeoQikLogAddPointsWithOpts entry;
    if (data.points && data.size > 0)
    {
      entry.points.assign(data.points, data.points + data.size);
    }
    entry.commonData = copy_common_data(data.commonData);
    return entry;
  }
  case GeoQikMessageType::REMOVE_POINT: return GeoQikLogRemovePoint{message.data.removePoint.handle};
  case GeoQikMessageType::SET_POINT_SIZE: return GeoQikLogSetPointSize{message.data.pointSize.size};
  case GeoQikMessageType::SET_POINT_COLOR: return GeoQikLogSetPointColor{message.data.color};
  case GeoQikMessageType::ADD_LINE_WITH_OPTS:
  {
    const auto& data = message.data.lineWithOpts;
    return GeoQikLogAddLineWithOpts{data.x1, data.y1, data.z1, data.x2, data.y2, data.z2, copy_common_data(data.commonData)};
  }
  case GeoQikMessageType::ADD_LINES_WITH_OPTS:
  {
    const auto& data = message.data.linesWithOpts;
    GeoQikLogAddLinesWithOpts entry;
    if (data.lines && data.size > 0)
    {
      entry.lines.assign(data.lines, data.lines + data.size);
    }
    entry.commonData = copy_common_data(data.commonData);
    return entry;
  }
  case GeoQikMessageType::REMOVE_LINE: return GeoQikLogRemoveLine{message.data.removeLine.handle};
  case GeoQikMessageType::SET_LINE_WIDTH: return GeoQikLogSetLineWidth{message.data.lineWidth.width};
  case GeoQikMessageType::SET_LINE_COLOR: return GeoQikLogSetLineColor{message.data.color};
  case GeoQikMessageType::REMOVE_ALL_GEOMETRY: return GeoQikLogRemoveAllGeometry{};
  case GeoQikMessageType::TRANSLATE_GEOMETRY:
  {
    const auto& data = message.data.translateGeometry;
    return GeoQikLogTranslateGeometry{data.handle, data.dx, data.dy, data.dz};
  }
  case GeoQikMessageType::ROTATE_GEOMETRY:
  {
    const auto& data = message.data.rotateGeometry;
    return GeoQikLogRotateGeometry{data.handle, data.centerX, data.centerY, data.centerZ, data.axisX, data.axisY, data.axisZ, data.angle};
  }
  default: return std::nullopt;
  }
}

void save_log_binary(const std::filesystem::path& path, std::span<const GeoQikLogEntry> entries)
{
  std::ofstream stream(path, std::ios::binary);
  if (!stream)
  {
    throw std::runtime_error("Failed to open GeoQik log for writing");
  }

  stream.write(LogMagic.data(), LogMagic.size());
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log header");
  }

  write_pod(stream, LogVersion);
  write_pod(stream, static_cast<std::uint64_t>(entries.size()));
  for (const GeoQikLogEntry& entry: entries)
  {
    write_entry(stream, entry);
  }
}

std::vector<GeoQikLogEntry> load_log_binary(const std::filesystem::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
  {
    throw std::runtime_error("Failed to open GeoQik log for reading");
  }

  std::array<char, LogMagic.size()> magic{};
  stream.read(magic.data(), magic.size());
  if (!stream || magic != LogMagic)
  {
    throw std::runtime_error("Invalid GeoQik log header");
  }

  if (read_pod<std::uint32_t>(stream) != LogVersion)
  {
    throw std::runtime_error("Unsupported GeoQik log version");
  }

  const std::uint64_t entryCount = read_pod<std::uint64_t>(stream);
  if (entryCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
  {
    throw std::runtime_error("GeoQik log entry count is too large");
  }

  std::vector<GeoQikLogEntry> entries;
  entries.reserve(static_cast<std::size_t>(entryCount));
  for (std::uint64_t i = 0; i < entryCount; ++i)
  {
    entries.push_back(read_entry(stream));
  }

  return entries;
}

} // namespace geoqik
