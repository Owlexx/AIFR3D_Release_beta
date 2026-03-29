#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace dawai::audio_engine
{

class ModuleProfiler
{
  public:
    class ScopedSample
    {
      public:
        ScopedSample(ModuleProfiler& profiler, std::string moduleName);
        ~ScopedSample();

      private:
        ModuleProfiler& m_profiler;
        std::string m_moduleName;
        std::chrono::steady_clock::time_point m_start;
    };

    void addSample(const std::string& module, double milliseconds);

    [[nodiscard]] std::unordered_map<std::string, double> snapshotCpuMs() const;

  private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, double> m_cpuByModuleMs;
};

} // namespace dawai::audio_engine
