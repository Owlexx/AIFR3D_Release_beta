#pragma once

#include "dawai/reference_engine/reference_profile.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dawai::reference_engine
{

class ReferenceLibrary
{
  public:
    void addProfile(ReferenceProfile profile);
    [[nodiscard]] bool loadFolder(const std::filesystem::path& directory);

    [[nodiscard]] std::vector<ReferenceProfile> allProfiles() const;
    [[nodiscard]] std::optional<ReferenceProfile> findById(const std::string& id) const;

  private:
    std::vector<ReferenceProfile> m_profiles;
};

} // namespace dawai::reference_engine
