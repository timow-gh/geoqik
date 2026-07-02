#ifndef SCOPED_ACTION_HPP
#define SCOPED_ACTION_HPP

#include "Core/Assert.hpp"
#include <functional>

namespace core {

class ScopedAction {
  public:
    explicit ScopedAction(std::function<void()> action)
        : m_action(std::move(action)) {}

    ScopedAction(ScopedAction&& other) noexcept
        : m_action(std::move(other.m_action)) {}
    ScopedAction& operator=(ScopedAction&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        m_action = std::move(other.m_action);
        other.m_action = nullptr;
        return *this;
    }

    ~ScopedAction() noexcept {
        CORE_ASSERT(m_action);
        if (m_action) {
            m_action();
        }
    }

    [[nodiscard]]
    bool is_active() const noexcept {
        return static_cast<bool>(m_action);
    }

    void deactivate() noexcept { m_action = nullptr; }

  private:
    std::function<void()> m_action;
};

inline ScopedAction make_scoped_action(std::function<void()> action) {
    CORE_ASSERT(action);
    return ScopedAction(std::move(action));
}

} // namespace core

#endif // SCOPED_ACTION_HPP
