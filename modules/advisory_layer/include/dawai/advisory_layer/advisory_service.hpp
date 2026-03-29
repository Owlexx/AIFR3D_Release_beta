#pragma once

#include "dawai/advisory_layer/adviser.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace dawai::advisory_layer
{

struct AdvisoryJob
{
    std::filesystem::path analysisDir;
    SessionContext context;
    AdvisoryCallback callback;
};

class AdvisoryService
{
  public:
    explicit AdvisoryService(std::unique_ptr<IAdviser> adviser);
    ~AdvisoryService();

    void submit(AdvisoryJob job);

  private:
    void workerLoop();

    std::unique_ptr<IAdviser> m_adviser;
    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<AdvisoryJob> m_jobs;
    bool m_stopping = false;
};

} // namespace dawai::advisory_layer
