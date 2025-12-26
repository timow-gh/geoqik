#ifndef CORE_UUID_HPP
#define CORE_UUID_HPP

#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"

#include <array>
#include <string>
#include <cstdint>
#include <compare>

namespace core
{

class UUID
{
  uuids::uuid m_uuid;

public:

  /** \brief Generate a new UUID using the operating system. */
  [[nodiscard]] static UUID generate()
  {
    return UUID{ uuids::uuid_system_generator{}() };
  }

  [[nodiscard]] static UUID nil()
  {
    return UUID{};
  }

  constexpr UUID()
      : m_uuid(uuids::uuid{})
  {
  }
  constexpr explicit UUID(const std::array<uint8_t, 16>& bytes) : m_uuid(uuids::uuid{ bytes }) {}
  explicit UUID(const uuids::uuid& uuid) : m_uuid(uuid) {}

  [[nodiscard]] std::string to_string() const
  {
    return uuids::to_string(m_uuid);
  }

  [[nodiscard]] bool is_nil() const
  {
    return m_uuid.is_nil();
  }

  [[nodiscard]] std::array<uint8_t, 16> to_array() const
  {
    std::array<uint8_t, 16> bytes;
    auto spanBytes = m_uuid.as_bytes();
    for (size_t i = 0; i < 16; ++i)
    {
      bytes[i] = static_cast<uint8_t>(spanBytes[i]);
    }
    return bytes;
  }

  const uuids::uuid& get_internal_uuid() const { return m_uuid; }

  [[nodiscard]] bool operator==(const UUID& other) const noexcept
  {
    return m_uuid == other.m_uuid;
  }

  [[nodiscard]] bool operator!=(const UUID& other) const noexcept
  {
    return m_uuid != other.m_uuid;
  }

  [[nodiscard]] auto operator<(const UUID& other) const noexcept
  {
    return m_uuid < other.m_uuid;
  }
};

} // namespace core

namespace std
{
template <>
struct hash<core::UUID>
{
  std::size_t operator()(const core::UUID& uuid) const noexcept { return std::hash<uuids::uuid>{}(uuid.get_internal_uuid()); }
};

} // namespace std

#endif // CORE_UUID_HPP