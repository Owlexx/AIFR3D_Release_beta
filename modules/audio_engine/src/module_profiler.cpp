#include "dawai/audio_engine/module_profiler.hpp"

namespace dawai::audio_engine
{

ModuleProfiler::ScopedSample::ScopedSample(ModuleProfiler& profiler, std::string moduleName)
    : m_profiler(profiler), m_moduleName(std::move(moduleName)),
      m_start(std::chrono::steady_clock::now())
{
}

ModuleProfiler::ScopedSample::~ScopedSample()
{
    const auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration<double, std::milli>(end - m_start).count();
    m_profiler.addSample(m_moduleName, ms);
}

void ModuleProfiler::addSample(const std::string& module, double milliseconds)
{
    std::scoped_lock lock(m_mutex);
    m_cpuByModuleMs[module] += milliseconds;
}

std::unordered_map<std::string, double> ModuleProfiler::snapshotCpuMs() const
{
    std::scoped_lock lock(m_mutex);
    return m_cpuByModuleMs;
}

} // namespace dawai::audio_engine
