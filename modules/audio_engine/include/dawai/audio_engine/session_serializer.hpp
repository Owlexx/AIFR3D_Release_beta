#pragma once

#include "dawai/audio_engine/track_model.hpp"

#include <filesystem>
#include <optional>

namespace dawai::audio_engine
{

class SessionSerializer
{
  public:
    bool save(const std::filesystem::path& path, const SessionState& state) const;
    [[nodiscard]] std::optional<SessionState> load(const std::filesystem::path& path) const;
};

} // namespace dawai::audio_engine
