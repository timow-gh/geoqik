#ifndef MPSCQUEUE_HPP
#define MPSCQUEUE_HPP

#include "Core/Assert.hpp"
#include <atomic>
#include <optional>
#include <cassert>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <semaphore>

namespace geoqik
{

template <typename TMessage>
class ConcurrentQueue
{
public:
  using value_type = TMessage;

private:
  mutable std::mutex m_mutex;
  std::queue<value_type> m_queue;
  std::counting_semaphore<> m_slotsFree;
  std::counting_semaphore<> m_slotsUsed;

public:
  explicit ConcurrentQueue(std::int64_t capacity = 100000)
      : m_slotsFree(capacity)
      , m_slotsUsed(0)
  {
    CORE_ASSERT(capacity > 0);
  }

  ConcurrentQueue(const ConcurrentQueue&) = delete;
  ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;
  ConcurrentQueue(ConcurrentQueue&& other)
  {
    std::scoped_lock lock(m_mutex, other.m_mutex);
    assert(other.m_queue.empty());
    assert(!other.m_slotsUsed.try_acquire()); // Ensure the other queue is not being used.
    assert(m_queue.empty());
    assert(!m_slotsUsed.try_acquire());       // Ensure this queue is not being used.
    m_queue = std::move(other.m_queue);
  }
  ConcurrentQueue& operator=(ConcurrentQueue&& other)
  {
    if (this != &other)
    {
      std::scoped_lock lock(m_mutex, other.m_mutex);
      assert(other.m_queue.empty());
      assert(!other.m_slotsUsed.try_acquire()); // Ensure the other queue is not being used.
      assert(m_queue.empty());
      assert(!m_slotsUsed.try_acquire());       // Ensure this queue is not being used.
      m_queue = std::move(other.m_queue);
    }
    return *this;
  }
  ~ConcurrentQueue() = default;

  std::size_t size() const
  {
    std::scoped_lock lock(m_mutex);
    return m_queue.size();
  }

  bool empty() const
  {
    std::scoped_lock lock(m_mutex);
    return m_queue.empty();
  }

  void enqueue(value_type&& message)
  {
    m_slotsFree.acquire();
    try
    {
      std::scoped_lock lock(m_mutex);
      m_queue.push(std::move(message));
    }
    catch (...)
    {
      m_slotsFree.release();
      throw;
    }
    m_slotsUsed.release();
  }

  bool try_enqueue(value_type&& message)
  {
    if (!m_slotsFree.try_acquire())
    {
      return false;
    }

    try
    {
      std::scoped_lock lock(m_mutex);
      m_queue.push(std::move(message));
    }
    catch (...)
    {
      m_slotsFree.release();
      throw;
    }

    m_slotsUsed.release();
    return true;
  }

  std::optional<value_type> dequeue()
  {
    m_slotsUsed.acquire();
    std::optional<value_type> message;
    {
      std::scoped_lock lock(m_mutex);
      if (m_queue.empty())
      {
        message = std::nullopt;
      }
      else
      {
        message = std::move(m_queue.front());
        m_queue.pop();
      }
    }
    m_slotsFree.release();

    return std::move(message);
  }

  const value_type* peek() const
  {
    std::scoped_lock lock(m_mutex);
    if (m_queue.empty())
    {
      return nullptr;
    }
    return &m_queue.front();
  }

  void clear()
  {
    std::scoped_lock lock(m_mutex);
    while (!m_queue.empty())
    {
      m_queue.pop();
      m_slotsFree.release();
    }
    // Set used slots to 0
    while (m_slotsUsed.try_acquire())
    {
      // No need to do anything here, just acquire all used slots to set the count to 0.
    } 
  }

};

} // namespace geoqik

#endif // MPSCQUEUE_HPP
