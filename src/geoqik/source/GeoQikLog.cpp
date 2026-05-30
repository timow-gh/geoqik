#include "GeoQikLog.hpp"
#include "GeoQikMessageSerialization.hpp"
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

} // namespace

std::optional<GeoQikLogEntry> create_log_entry(const GeoQikMessage& message)
{
  return std::visit(
      [](const auto& value) -> std::optional<GeoQikLogEntry>
      {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, AddPointWithOpts> ||
                      std::is_same_v<T, AddPointsWithOpts> ||
                      std::is_same_v<T, UpdatePointWithOpts> ||
                      std::is_same_v<T, UpdatePointsWithOpts> ||
                      std::is_same_v<T, RemovePoint> ||
                      std::is_same_v<T, SetPointSize> ||
                      std::is_same_v<T, SetPointColor> ||
                      std::is_same_v<T, AddLineWithOpts> ||
                      std::is_same_v<T, AddLinesWithOpts> ||
                      std::is_same_v<T, UpdateLineWithOpts> ||
                      std::is_same_v<T, UpdateLinesWithOpts> ||
                      std::is_same_v<T, RemoveLine> ||
                      std::is_same_v<T, SetLineWidth> ||
                      std::is_same_v<T, SetLineColor> ||
                      std::is_same_v<T, RemoveAllGeometry> ||
                      std::is_same_v<T, TranslateGeometry> ||
                      std::is_same_v<T, RotateGeometry>)
        {
          return GeoQikLogEntry{value};
        }
        else
        {
          return std::nullopt;
        }
      },
      message);
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
  const std::uint64_t entrySize = entries.size();
  write_pod(stream, entrySize);

  MessageWriter writer(stream);
  for (const GeoQikLogEntry& entry: entries)
  {
    writer.write(entry);
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
  if (entryCount > std::numeric_limits<std::size_t>::max())
  {
    throw std::runtime_error("GeoQik log entry count is too large");
  }

  MessageReader reader(stream);
  std::vector<GeoQikLogEntry> entries;
  entries.reserve(static_cast<std::size_t>(entryCount));
  for (std::uint64_t i = 0; i < entryCount; ++i)
  {
    entries.push_back(reader.read());
  }

  return entries;
}

} // namespace geoqik
