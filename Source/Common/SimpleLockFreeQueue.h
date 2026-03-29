#pragma once

#include <array>
#include <atomic>

template <typename T, size_t Capacity> class SimpleLockFreeQueue
{
  public:
    bool push(const T& v)
    {
        auto w = writeIndex.load(std::memory_order_relaxed);
        auto n = (w + 1) % Capacity;
        if (n == readIndex.load(std::memory_order_acquire))
            return false;

        buffer[w] = v;
        writeIndex.store(n, std::memory_order_release);
        return true;
    }

    bool pop(T& out)
    {
        auto r = readIndex.load(std::memory_order_relaxed);
        if (r == writeIndex.load(std::memory_order_acquire))
            return false;

        out = buffer[r];
        readIndex.store((r + 1) % Capacity, std::memory_order_release);
        return true;
    }

  private:
    std::array<T, Capacity> buffer{};
    std::atomic<size_t> writeIndex{0};
    std::atomic<size_t> readIndex{0};
};
