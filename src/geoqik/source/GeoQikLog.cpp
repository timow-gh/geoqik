#include "GeoQikLog.hpp"
#include "GeoQikMessageSerialization.hpp"
#include "GeoQikMessageJsonSerialization.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace geoqik
{

namespace
{

constexpr std::array<char, 8> logMagic{'G', 'Q', 'K', 'L', 'O', 'G', '0', '1'};
constexpr std::uint32_t logVersion = 1;

template <typename T>
void write_pod(std::ostream& stream, const T& value)
{
  static_assert(std::is_trivially_copyable_v<T>);
  const auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(value);
  stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log");
  }
}

template <typename T>
T read_pod(std::istream& stream)
{
  static_assert(std::is_trivially_copyable_v<T>);
  std::array<char, sizeof(T)> bytes{};
  stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!stream)
  {
    throw std::runtime_error("Failed to read GeoQik log");
  }
  return std::bit_cast<T>(bytes);
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
                      std::is_same_v<T, RotateGeometry> ||
                      std::is_same_v<T, SetMeshColor> ||
                      std::is_same_v<T, AddMeshWithOpts> ||
                      std::is_same_v<T, RemoveMesh> ||
                      std::is_same_v<T, UpdateMeshWithOpts> ||
                      std::is_same_v<T, SetMeshOverlayOpts> ||
                      std::is_same_v<T, SetMeshRenderingOpts>)
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

  stream.write(logMagic.data(), static_cast<std::streamsize>(logMagic.size()));
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log header");
  }

  write_pod(stream, logVersion);
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

  std::array<char, logMagic.size()> magic{};
  stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
  if (!stream || magic != logMagic)
  {
    throw std::runtime_error("Invalid GeoQik log header");
  }

  if (read_pod<std::uint32_t>(stream) != logVersion)
  {
    throw std::runtime_error("Unsupported GeoQik log version");
  }

  const auto entryCount = read_pod<std::uint64_t>(stream);
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

void save_log_json(const std::filesystem::path& path, std::span<const GeoQikLogEntry> entries)
{
  std::ofstream stream(path, std::ios::binary);
  if (!stream)
  {
    throw std::runtime_error("Failed to open GeoQik log for writing");
  }

  nlohmann::json jsonArray = nlohmann::json::array();
  for (const GeoQikLogEntry& entry: entries)
  {
    jsonArray.push_back(to_json(entry));
  }

  stream << jsonArray;
  if (!stream)
  {
    throw std::runtime_error("Failed to write GeoQik log");
  }
}

std::vector<GeoQikLogEntry> load_log_json(const std::filesystem::path& path)
{
  std::ifstream stream(path, std::ios::binary);
  if (!stream)
  {
    throw std::runtime_error("Failed to open GeoQik log for reading");
  }

  nlohmann::json jsonArray;
  try
  {
    jsonArray = nlohmann::json::parse(stream);
  }
  catch (const nlohmann::json::parse_error& e)
  {
    throw std::runtime_error(std::string("Failed to parse GeoQik JSON log: ") + e.what());
  }

  if (!jsonArray.is_array())
  {
    throw std::runtime_error("Invalid GeoQik JSON log: expected a top-level array");
  }

  std::vector<GeoQikLogEntry> entries;
  entries.reserve(jsonArray.size());
  for (const auto& element: jsonArray)
  {
    entries.push_back(from_json(element));
  }

  return entries;
}

} // namespace geoqik
