#include "dawai/reference_engine/ab_controller.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::reference_engine
{

ABController::ABController(std::size_t fadeSamples)
    : m_fadeSamples(std::max<std::size_t>(fadeSamples, 1))
{
}

void ABController::setSource(Source source)
{
    if (source != m_targetSource)
    {
        m_targetSource = source;
        m_fadePosition = 0;
    }
}

void ABController::setCompensationDb(double db)
{
    m_compensationLinear = static_cast<float>(std::pow(10.0, db / 20.0));
}

void ABController::reset()
{
    m_currentSource = Source::Mix;
    m_targetSource = Source::Mix;
    m_fadePosition = 0;
    m_compensationLinear = 1.0f;
}

std::pair<float, float> ABController::nextGains()
{
    const bool switching = m_currentSource != m_targetSource;
    if (!switching)
    {
        return (m_currentSource == Source::Mix)
                   ? std::pair<float, float>{1.0f, 0.0f}
                   : std::pair<float, float>{0.0f, m_compensationLinear};
    }

    const float t = static_cast<float>(m_fadePosition) / static_cast<float>(m_fadeSamples);
    const float fadeOut = std::clamp(1.0f - t, 0.0f, 1.0f);
    const float fadeIn = std::clamp(t, 0.0f, 1.0f);

    std::pair<float, float> gains;
    if (m_targetSource == Source::Reference)
    {
        gains = {fadeOut, fadeIn * m_compensationLinear};
    }
    else
    {
        gains = {fadeIn, fadeOut * m_compensationLinear};
    }

    if (++m_fadePosition >= m_fadeSamples)
    {
        m_currentSource = m_targetSource;
        m_fadePosition = 0;
    }

    return gains;
}

} // namespace dawai::reference_engine
