#pragma once

#include "dawai/reference_engine/reference_profile.hpp"

#include <filesystem>
#include <vector>

namespace dawai::reference_engine
{

class ProfileCache
{
  public:
    bool save(const std::filesystem::path& path,
              const std::vector<ReferenceProfile>& profiles) const;
    [[nodiscard]] std::vector<ReferenceProfile> load(const std::filesystem::path& path) const;
};

} // namespace dawai::reference_engine
