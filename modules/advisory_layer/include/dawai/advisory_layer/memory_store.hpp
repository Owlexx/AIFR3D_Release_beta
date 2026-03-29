#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace dawai::advisory_layer
{

class MemoryStore
{
  public:
    explicit MemoryStore(std::filesystem::path root = "analysis/chat_memory")
        : m_root(std::move(root))
    {
    }

    [[nodiscard]] std::string loadConversationContext(const std::string& conversationId,
                                                      std::size_t maxTurns = 16,
                                                      std::size_t maxBytes = 32768) const;
    [[nodiscard]] std::string
    loadAttachmentContext(const std::vector<std::filesystem::path>& attachments,
                          std::size_t maxBytesPerFile = 65536) const;
    void appendTurn(const std::string& conversationId, const std::string& prompt,
                    const std::string& response,
                    const std::vector<std::filesystem::path>& attachments) const;

  private:
    std::filesystem::path conversationPath(const std::string& conversationId) const;
    static std::string sanitizeConversationId(const std::string& raw);
    static std::size_t maxStoredTurns();
    std::filesystem::path m_root;
};

} // namespace dawai::advisory_layer
