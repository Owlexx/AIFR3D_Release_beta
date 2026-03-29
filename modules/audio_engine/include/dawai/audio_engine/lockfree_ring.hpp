#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace dawai::audio_engine
{

template <typename T, std::size_t Capacity> class LockFreeRing
{
  public:
    static_assert(Capacity > 1, "Capacity must be > 1");

    bool push(const T& value)
    {
        const auto write = m_write.load(std::memory_order_relaxed);
        const auto next = increment(write);
        if (next == m_read.load(std::memory_order_acquire))
        {
            return false;
        }

        m_data[write] = value;
        m_write.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out)
    {
        const auto read = m_read.load(std::memory_order_relaxed);
        if (read == m_write.load(std::memory_order_acquire))
        {
            return false;
        }

        out = m_data[read];
        m_read.store(increment(read), std::memory_order_release);
        return true;
    }

  private:
    static constexpr std::size_t increment(std::size_t index) noexcept
    {
        return (index + 1) % Capacity;
    }

    std::array<T, Capacity> m_data{};
    std::atomic<std::size_t> m_write{0};
    std::atomic<std::size_t> m_read{0};
};

} // namespace dawai::audio_engine
