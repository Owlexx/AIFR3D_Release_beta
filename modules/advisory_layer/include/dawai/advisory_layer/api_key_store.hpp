#pragma once

#include <string>

namespace dawai::advisory_layer
{

class ApiKeyStore
{
  public:
    static std::string normalizeOpenAiKey(const std::string& apiKey);
    static bool saveOpenAiKey(const std::string& apiKey);
    static std::string loadOpenAiKey();
    static bool clearOpenAiKey();
};

} // namespace dawai::advisory_layer
