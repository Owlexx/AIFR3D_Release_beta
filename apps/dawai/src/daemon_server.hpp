#pragma once

#include "dawai/advisory_layer/adviser.hpp"

#include <juce_core/juce_core.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace dawai
{

class DaemonServer
{
  public:
    explicit DaemonServer(int port = 7777);
    ~DaemonServer();

    bool start();
    int runBlocking();
    void stop();
    [[nodiscard]] bool isRunning() const noexcept
    {
        return m_running.load();
    }

  private:
    void runLoop();
    void handleClient(std::unique_ptr<juce::StreamingSocket> client) const;

    static void ensureAutoDependencies();

    std::optional<dawai::advisory_layer::AdvisoryOutput>
    runAdviser(const std::filesystem::path& analysisDir,
               const dawai::advisory_layer::SessionContext& context) const;

    int m_port = 7777;
    std::atomic<bool> m_running{false};
    std::unique_ptr<juce::StreamingSocket> m_listener;
    std::thread m_thread;

    mutable std::mutex m_adviserMutex;
    mutable std::unique_ptr<dawai::advisory_layer::IAdviser> m_adviser;
};

} // namespace dawai
