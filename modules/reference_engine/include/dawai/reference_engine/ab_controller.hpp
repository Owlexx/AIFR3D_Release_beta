#pragma once

#include <cstddef>
#include <utility>

namespace dawai::reference_engine
{

class ABController
{
  public:
    enum class Source
    {
        Mix,
        Reference
    };

    explicit ABController(std::size_t fadeSamples = 256);

    void setSource(Source source);
    void setCompensationDb(double db);
    void reset();

    [[nodiscard]] std::pair<float, float> nextGains();
    [[nodiscard]] Source currentSource() const noexcept
    {
        return m_targetSource;
    }

  private:
    std::size_t m_fadeSamples;
    std::size_t m_fadePosition = 0;
    Source m_currentSource = Source::Mix;
    Source m_targetSource = Source::Mix;
    float m_compensationLinear = 1.0f;
};

} // namespace dawai::reference_engine
