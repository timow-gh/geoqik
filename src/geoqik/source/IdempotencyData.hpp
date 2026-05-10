#ifndef IDEMPOTENCYDATA_HPP
#define IDEMPOTENCYDATA_HPP

#include <Core/UUID.hpp>
#include <chrono>

namespace geoqik
{

struct IdempotencyData
{
  core::UUID key;
  std::chrono::high_resolution_clock::time_point timestamp;

  [[nodiscard]] bool operator==(const IdempotencyData& other) const noexcept
  {
    return key == other.key;
  }
  [[nodiscard]] bool operator!=(const IdempotencyData& other) const noexcept
  {
    return key != other.key;
  }

  [[nodiscard]] auto operator<(const IdempotencyData& other) const noexcept
  {
    return key < other.key;
  }
  
  // Add hash function as a member
  struct Hash
  {
    std::size_t operator()(const IdempotencyData& data) const noexcept
    {
      return std::hash<core::UUID>{}(data.key);
    }
  };
};

} // namespace geoqik

#endif // IDEMPOTENCYDATA_HPP