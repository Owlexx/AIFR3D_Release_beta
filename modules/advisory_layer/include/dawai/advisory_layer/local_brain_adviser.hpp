#pragma once

#include "dawai/advisory_layer/adviser.hpp"
#include "dawai/advisory_layer/memory_store.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dawai::advisory_layer
{

class LocalBrainAdviser : public IAdviser
{
  public:
    LocalBrainAdviser(std::vector<std::string> modelNames,
                      std::string endpoint = "https://api.openai.com/v1/responses");

    std::optional<AdvisoryOutput> advise(const std::filesystem::path& analysisDir,
                                         const SessionContext& context) override;

    static std::unique_ptr<IAdviser> createFromEnvironment();

  private:
    struct AdviserEntry
    {
        std::string label;
        std::unique_ptr<IAdviser> adviser;
    };

    std::vector<AdviserEntry> m_chain;
    MemoryStore m_memory;
};

} // namespace dawai::advisory_layer
