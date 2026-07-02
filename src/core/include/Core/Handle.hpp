#ifndef CORE_HANDLE_HPP
#define CORE_HANDLE_HPP

#include <compare>
#include <cstdint>
#include <limits>

namespace core {

class Handle {
  public:
    using value_type = std::uint64_t;
    static constexpr value_type invalidHandle = std::numeric_limits<value_type>::max();
    static constexpr value_type nullHandle = value_type{};

    constexpr Handle() noexcept = default;
    constexpr Handle(const Handle&) noexcept = default;
    constexpr Handle& operator=(const Handle&) noexcept = default;
    constexpr Handle(Handle&&) noexcept = default;
    constexpr Handle& operator=(Handle&&) noexcept = default;
    ~Handle() = default;

    constexpr explicit Handle(value_type handle) noexcept
        : m_value(handle) {}

    constexpr std::strong_ordering operator<=>(const Handle& rhs) const noexcept = default;

    Handle& operator++() noexcept {
        ++m_value;
        return *this;
    }

    Handle operator++(int) noexcept {
        Handle temp = *this;
        ++(*this);
        return temp;
    }

    [[nodiscard]]
    constexpr value_type get_value() const noexcept {
        return m_value;
    }
    [[nodiscard]]
    constexpr bool is_valid() const noexcept {
        return m_value != Handle::invalidHandle;
    }

  private:
    value_type m_value{invalidHandle};
};

} // namespace core

#endif // CORE_HANDLE_HPP
