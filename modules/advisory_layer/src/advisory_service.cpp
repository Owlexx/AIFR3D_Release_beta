#include "dawai/advisory_layer/advisory_service.hpp"

namespace dawai::advisory_layer
{

AdvisoryService::AdvisoryService(std::unique_ptr<IAdviser> adviser)
    : m_adviser(std::move(adviser)), m_worker([this] { workerLoop(); })
{
}

AdvisoryService::~AdvisoryService()
{
    {
        std::scoped_lock lock(m_mutex);
        m_stopping = true;
    }
    m_cv.notify_all();

    if (m_worker.joinable())
    {
        m_worker.join();
    }
}

void AdvisoryService::submit(AdvisoryJob job)
{
    {
        std::scoped_lock lock(m_mutex);
        constexpr std::size_t kMaxPendingJobs = 2;
        while (m_jobs.size() >= kMaxPendingJobs)
        {
            m_jobs.pop();
        }
        m_jobs.push(std::move(job));
    }
    m_cv.notify_one();
}

void AdvisoryService::workerLoop()
{
    while (true)
    {
        AdvisoryJob job;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [&] { return m_stopping || !m_jobs.empty(); });
            if (m_stopping && m_jobs.empty())
            {
                break;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop();
        }

        if (!m_adviser)
        {
            if (job.callback)
            {
                job.callback(std::nullopt);
            }
            continue;
        }

        auto result = m_adviser->advise(job.analysisDir, job.context);
        if (job.callback)
        {
            job.callback(std::move(result));
        }
    }
}

} // namespace dawai::advisory_layer
