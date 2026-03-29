#pragma once

#include "dawai/advisory_layer/adviser.hpp"

namespace dawai::advisory_layer
{

class OpenAiAdviser : public IAdviser
{
  public:
    OpenAiAdviser(std::string model, std::string apiKey, std::string endpoint = "https://api.openai.com/v1/responses")
        : m_model(std::move(model)), m_apiKey(std::move(apiKey)), m_endpoint(std::move(endpoint))
    {
    }

    std::optional<AdvisoryOutput> advise(const std::filesystem::path& analysisDir,
                                         const SessionContext& context) override;

    [[nodiscard]] const std::string& model() const noexcept
    {
        return m_model;
    }

  private:
    std::string m_model;
    std::string m_apiKey;
    std::string m_endpoint;
};

} // namespace dawai::advisory_layer
