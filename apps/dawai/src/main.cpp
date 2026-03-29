#include "daemon_server.hpp"
#include "dawai/advisory_layer/api_key_store.hpp"
#include "dawai/advisory_layer/local_brain_adviser.hpp"
#include "dawai/aifr3d_core/system_contract.hpp"
#include "dawai/aifr3d_core/version.hpp"
#include "dawai/reference_engine/pools/benchmark_pool.hpp"
#include "dawai/reference_engine/profile_cache.hpp"
#include "dawai/reference_engine/reference_profiler.hpp"
#include "dawai/ui/main_view.hpp"

#include <juce_gui_extra/juce_gui_extra.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <functional>
#include <vector>

namespace
{

struct FirstRunConfig
{
    std::string audioDevice = "Default";
    std::vector<std::string> vst3Paths;
    std::string referencePoolFolder;
};

std::filesystem::path standaloneWindowStatePath()
{
    const auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getFullPathName()
                          .toStdString();
    return std::filesystem::path(base) / "AIFR3D" / "standalone_window_state_2_2_4.json";
}

juce::Rectangle<int> loadStandaloneWindowBounds()
{
    constexpr int kDefaultWidth = 1400;
    constexpr int kDefaultHeight = 820;
    const auto path = standaloneWindowStatePath();
    if (!std::filesystem::exists(path))
    {
        return {0, 0, kDefaultWidth, kDefaultHeight};
    }

    try
    {
        std::ifstream in(path);
        const auto parsed = nlohmann::json::parse(in);
        const int width = juce::jlimit(1180, 2200, parsed.value("width", kDefaultWidth));
        const int height = juce::jlimit(720, 1400, parsed.value("height", kDefaultHeight));
        const int x = parsed.value("x", 0);
        const int y = parsed.value("y", 0);
        return {x, y, width, height};
    }
    catch (...)
    {
        return {0, 0, kDefaultWidth, kDefaultHeight};
    }
}

void saveStandaloneWindowBounds(juce::Rectangle<int> bounds)
{
    const auto path = standaloneWindowStatePath();
    std::filesystem::create_directories(path.parent_path());
    nlohmann::json payload{
        {"x", bounds.getX()},
        {"y", bounds.getY()},
        {"width", bounds.getWidth()},
        {"height", bounds.getHeight()}
    };
    std::ofstream out(path);
    out << payload.dump(2);
}

void writeFirstRunConfig(const std::filesystem::path& path, const FirstRunConfig& config)
{
    nlohmann::json j;
    j["audioDevice"] = config.audioDevice;
    j["vst3Paths"] = config.vst3Paths;
    j["referencePoolFolder"] = config.referencePoolFolder;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << j.dump(2);
}

std::filesystem::path homeDirectory()
{
#if JUCE_WINDOWS
    const char* value = std::getenv("USERPROFILE");
#else
    const char* value = std::getenv("HOME");
#endif
    if (value == nullptr)
    {
        return {};
    }
    return std::filesystem::path(value);
}

std::filesystem::path expandUserPath(std::string path)
{
    if (path == "~")
    {
        return homeDirectory();
    }
    if (path.rfind("~/", 0) == 0 || path.rfind("~\\", 0) == 0)
    {
        auto home = homeDirectory();
        if (home.empty())
        {
            return std::filesystem::path(path);
        }
        return home / path.substr(2);
    }
    return std::filesystem::path(std::move(path));
}

[[maybe_unused]] std::filesystem::path defaultMusicFolder()
{
#if JUCE_WINDOWS
    return homeDirectory() / "Music";
#else
    return homeDirectory() / "Music";
#endif
}

std::string slugify(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    bool previousDash = false;
    for (char ch : text)
    {
        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (std::isalnum(lower) != 0)
        {
            out.push_back(lower);
            previousDash = false;
        }
        else if (!previousDash)
        {
            out.push_back('-');
            previousDash = true;
        }
    }

    while (!out.empty() && out.front() == '-')
    {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '-')
    {
        out.pop_back();
    }

    if (out.empty())
    {
        return "ref";
    }
    return out;
}

std::filesystem::path canonicalCuratedReferencePoolRoot()
{
    const std::filesystem::path preferred{"assets/reference_pools/pro_25x6"};
    if (std::filesystem::exists(preferred))
    {
        return preferred;
    }
    return std::filesystem::path("assets/reference_pools/canonical_25x5");
}

std::filesystem::path safeCanonicalPath(const std::filesystem::path& path)
{
    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(path, ec);
    if (ec)
    {
        resolved = path.lexically_normal();
    }
    return resolved;
}

bool pathStartsWith(const std::filesystem::path& path, const std::filesystem::path& prefix)
{
    auto pathIt = path.begin();
    auto prefixIt = prefix.begin();
    for (; prefixIt != prefix.end(); ++prefixIt, ++pathIt)
    {
        if (pathIt == path.end() || *pathIt != *prefixIt)
        {
            return false;
        }
    }
    return true;
}

bool isAudioFile(const std::filesystem::path& path)
{
    static const std::set<std::string> extensions = {".wav",  ".wave", ".aif", ".aiff",
                                                     ".flac", ".mp3",  ".ogg"};
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extensions.contains(ext);
}

std::vector<std::filesystem::path> collectAudioFiles(const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(root))
    {
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::skip_permission_denied))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        if (isAudioFile(entry.path()))
        {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

bool readAudioFileAsBlock(const std::filesystem::path& sourcePath,
                          dawai::metering::AudioBlock& outBlock, double& outSampleRate,
                          std::string& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    const auto absolutePath = std::filesystem::absolute(sourcePath);
    const juce::File file(absolutePath.string());
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
    {
        error = "Unsupported or unreadable file format";
        return false;
    }

    constexpr int64_t kMaxSamples = 48000 * 30; // Cap profiling to first 30 seconds.
    const int64_t fileSamples = std::max<int64_t>(0, reader->lengthInSamples);
    const int64_t sampleCount64 = std::min<int64_t>(fileSamples, kMaxSamples);

    if (sampleCount64 <= 0 || sampleCount64 > static_cast<int64_t>(std::numeric_limits<int>::max()))
    {
        error = "File contains no readable audio samples";
        return false;
    }

    const int channels = juce::jlimit(1, 2, static_cast<int>(reader->numChannels));
    const int sampleCount = static_cast<int>(sampleCount64);

    juce::AudioBuffer<float> buffer(channels, sampleCount);
    if (!reader->read(&buffer, 0, sampleCount, 0, true, true))
    {
        error = "Failed reading PCM data";
        return false;
    }

    outBlock.channels.resize(static_cast<std::size_t>(channels));
    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* src = buffer.getReadPointer(channel);
        auto& dst = outBlock.channels[static_cast<std::size_t>(channel)];
        dst.assign(src, src + sampleCount);
    }

    outSampleRate = reader->sampleRate;
    return true;
}

juce::String valueAfterFlag(const juce::StringArray& args, const juce::String& flag,
                            const juce::String& fallback)
{
    const int idx = args.indexOf(flag);
    if (idx >= 0 && idx + 1 < args.size())
    {
        return args[idx + 1];
    }
    return fallback;
}

std::vector<std::filesystem::path> parseAttachmentList(const juce::String& csv)
{
    std::vector<std::filesystem::path> files;
    std::stringstream ss(csv.toStdString());
    std::string item;
    while (std::getline(ss, item, ','))
    {
        if (item.empty())
        {
            continue;
        }
        files.push_back(expandUserPath(item));
    }
    return files;
}

juce::Image loadStandaloneSplashImage()
{
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    const std::array<juce::File, 9> candidates{
        cwd.getChildFile("assets/branding/mascot_splash_2_2_4_beta.png"),
        cwd.getChildFile("../assets/branding/mascot_splash_2_2_4_beta.png"),
        home.getChildFile(".local/share/aifr3d/assets/mascot_splash_2_2_4_beta.png"),
        cwd.getChildFile("apps/website/assets/brand/mascot_app_icon.png"),
        cwd.getChildFile("assets/branding/north3rnlight3r_mascot.png"),
        cwd.getChildFile("../assets/branding/north3rnlight3r_mascot.png"),
        cwd.getChildFile("assets/icons/aifred_logo.png"),
        home.getChildFile("Pictures/North3rnLight3r_Brand_Assets/North3rnLight3rrMascot.png"),
        home.getChildFile(".local/share/icons/hicolor/512x512/apps/aifr3d.png"),
    };

    for (const auto& candidate : candidates)
    {
        if (!candidate.existsAsFile())
        {
            continue;
        }
        if (auto image = juce::ImageFileFormat::loadFrom(candidate); image.isValid())
        {
            return image;
        }
    }

    return {};
}

bool buildReferencePoolFromDirectory(const std::filesystem::path& inputRoot,
                                     const std::filesystem::path& outputRoot,
                                     const std::string& poolName, const std::string& genre,
                                     std::size_t maxFiles)
{
    const auto normalizedOutputRoot = safeCanonicalPath(outputRoot);
    const auto curatedRoot = safeCanonicalPath(canonicalCuratedReferencePoolRoot());
    if (pathStartsWith(normalizedOutputRoot, curatedRoot))
    {
        juce::Logger::writeToLog(
            "Refusing to write generated references into the canonical curated pool.");
        return false;
    }

    auto files = collectAudioFiles(inputRoot);
    if (files.empty())
    {
        juce::Logger::writeToLog("No audio files found for reference pool build.");
        return false;
    }

    if (maxFiles > 0 && files.size() > maxFiles)
    {
        files.resize(maxFiles);
    }

    const auto profilesDir = outputRoot / "profiles";
    std::filesystem::create_directories(profilesDir);

    dawai::reference_engine::ReferenceProfiler profiler(2048, 32);
    dawai::reference_engine::ProfileCache cache;

    nlohmann::json poolJson;
    poolJson["name"] = poolName;
    poolJson["genre"] = genre;
    poolJson["entries"] = nlohmann::json::array();

    nlohmann::json manifest;
    manifest["schema_version"] = "refpool-1.0.0";
    manifest["pool_id"] = slugify(poolName);
    manifest["created_utc"] = juce::Time::getCurrentTime().toISO8601(true).toStdString();
    manifest["genres"] = nlohmann::json::array({genre});
    manifest["references"] = nlohmann::json::array();

    std::size_t successCount = 0;
    std::size_t skippedCount = 0;
    std::size_t index = 0;

    for (const auto& sourcePath : files)
    {
        dawai::metering::AudioBlock block;
        double sampleRate = 0.0;
        std::string error;

        if (!readAudioFileAsBlock(sourcePath, block, sampleRate, error))
        {
            ++skippedCount;
            juce::Logger::writeToLog("Skipping " + sourcePath.string() + ": " + error);
            continue;
        }

        const std::string stem = sourcePath.stem().string();
        const std::string id = "ref_" + std::to_string(index + 1) + "_" + slugify(stem);
        const auto profile =
            profiler.buildProfile(id, stem, sourcePath.string(), block, sampleRate);

        const auto profilePath = profilesDir / (id + ".json");
        if (!cache.save(profilePath, {profile}))
        {
            ++skippedCount;
            juce::Logger::writeToLog("Skipping " + sourcePath.string() +
                                     ": failed to write profile.");
            continue;
        }

        poolJson["entries"].push_back({{"profileId", id}, {"profilePath", profilePath.string()}});
        manifest["references"].push_back({{"reference_id", id},
                                          {"title", stem},
                                          {"genre", genre},
                                          {"source_path", sourcePath.string()},
                                          {"profile_path", std::filesystem::relative(profilePath, outputRoot).string()},
                                          {"integrated_lufs", profile.integratedLufs},
                                          {"short_term_lufs", profile.shortTermLufs},
                                          {"true_peak_dbtp", profile.truePeakDbtp},
                                          {"rms_db", profile.rmsDb},
                                          {"peak_dbfs", profile.peakDbfs},
                                          {"crest_factor_db", profile.crestFactorDb},
                                          {"spectral_tilt_db", profile.spectralTiltDb},
                                          {"low_band_db", profile.lowBandDb},
                                          {"low_mid_band_db", profile.lowMidBandDb},
                                          {"presence_band_db", profile.presenceBandDb},
                                          {"air_band_db", profile.airBandDb},
                                          {"correlation", profile.correlation},
                                          {"stereo_width", profile.stereoWidth},
                                          {"mid_energy", profile.midEnergy},
                                          {"side_energy", profile.sideEnergy},
                                          {"phase_risk", profile.phaseRisk},
                                          {"sub_mono_integrity", profile.subMonoIntegrity},
                                          {"transient_density", profile.transientDensity}});

        ++successCount;
        ++index;
    }

    if (successCount == 0)
    {
        juce::Logger::writeToLog("Reference pool build failed: no profiles were created.");
        return false;
    }

    std::filesystem::create_directories(outputRoot);
    const auto poolPath = outputRoot / "pool.json";
    {
        std::ofstream out(poolPath);
        out << poolJson.dump(2);
    }
    {
        std::ofstream out(outputRoot / "pool_manifest.json");
        out << manifest.dump(2);
    }

    const bool cacheOk = dawai::reference_engine::pools::buildReferenceCache(
        poolPath, outputRoot / "reference_cache.json");
    juce::Logger::writeToLog(
        "Reference pool build complete. profiles=" + std::to_string(successCount) +
        ", skipped=" + std::to_string(skippedCount));
    return cacheOk;
}

bool ensureFirstRunSetup(const juce::String& appDir)
{
    const auto configPath =
        std::filesystem::path(appDir.toStdString()) / "config" / "first_run.json";
    if (std::filesystem::exists(configPath))
    {
        return true;
    }

    juce::AlertWindow setup(
        "First Run Setup",
        "Choose default options in Settings later. Initial defaults will be created now.",
        juce::AlertWindow::InfoIcon);
    setup.addButton("Continue", 1, juce::KeyPress(juce::KeyPress::returnKey));
    if (setup.runModalLoop() == 0)
    {
        return false;
    }

    FirstRunConfig config;
#if JUCE_WINDOWS
    config.vst3Paths = {"C:/Program Files/Common Files/VST3"};
#elif JUCE_LINUX
    config.vst3Paths = {"/usr/lib/vst3", "/usr/local/lib/vst3"};
#else
    config.vst3Paths = {"/Library/Audio/Plug-Ins/VST3"};
#endif
    juce::FileChooser vstChooser("Select VST3 folder", juce::File(), "*");
    if (vstChooser.browseForDirectory())
    {
        config.vst3Paths = {vstChooser.getResult().getFullPathName().toStdString()};
    }

    juce::FileChooser refChooser("Select reference pool folder", juce::File(), "*");
    if (refChooser.browseForDirectory())
    {
        config.referencePoolFolder = refChooser.getResult().getFullPathName().toStdString();
    }
    else
    {
        config.referencePoolFolder =
            (std::filesystem::path(appDir.toStdString()) / "reference_pools").string();
    }

    writeFirstRunConfig(configPath, config);
    return true;
}

} // namespace

class DawaiApplication : public juce::JUCEApplication
{
  public:
    DawaiApplication() = default;

    const juce::String getApplicationName() override
    {
        return "AIFR3D Beta 2.2.4";
    }
    const juce::String getApplicationVersion() override
    {
        return dawai::aifr3d_core::kCanonicalVersion;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise(const juce::String& commandLine) override
    {
        std::filesystem::create_directories("logs");
        m_logger.reset(
            juce::FileLogger::createDateStampedLogger("logs", "aifr3d", ".log", "AIFR3D startup"));
        juce::Logger::setCurrentLogger(m_logger.get());

        juce::StringArray args;
        args.addTokens(commandLine, true);

        if (args.contains("--help") || args.contains("-h"))
        {
            juce::Logger::writeToLog("AIFR3D CLI options:");
            juce::Logger::writeToLog("  --daemon [--daemon-port <port>]");
            juce::Logger::writeToLog("  --no-daemon (disable background daemon in GUI mode)");
            juce::Logger::writeToLog("  --openai-key-set <key> (stores encrypted local API key)");
            juce::Logger::writeToLog("  --openai-key-clear (removes stored local API key)");
            juce::Logger::writeToLog("  --build-reference-cache <pool.json>");
            juce::Logger::writeToLog(
                "  --build-reference-pool <audio-dir> [--out <pool-dir>] [--pool-name <name>] "
                "[--genre <genre>] [--max-files <N>]");
            juce::Logger::writeToLog(
                "  --chat-ask <prompt> [--analysis <dir>] [--genre <name>] [--conversation <id>] "
                "[--attach file1,file2]");
            quit();
            return;
        }

        const int daemonPort =
            juce::jlimit(1, 65535, valueAfterFlag(args, "--daemon-port", "7777").getIntValue());

        const int setKeyIndex = args.indexOf("--openai-key-set");
        if (setKeyIndex >= 0)
        {
            const std::string key = (setKeyIndex + 1 < args.size()) ? args[setKeyIndex + 1].toStdString() : "";
            const bool ok = dawai::advisory_layer::ApiKeyStore::saveOpenAiKey(key);
            juce::Logger::writeToLog(ok ? "OpenAI API key stored in encrypted local key store."
                                        : "OpenAI API key store failed.");
            quit();
            return;
        }
        if (args.contains("--openai-key-clear"))
        {
            const bool ok = dawai::advisory_layer::ApiKeyStore::clearOpenAiKey();
            juce::Logger::writeToLog(ok ? "OpenAI API key cleared from local key store."
                                        : "No stored OpenAI API key found.");
            quit();
            return;
        }

        if (args.contains("--daemon"))
        {
            m_daemon = std::make_unique<dawai::DaemonServer>(daemonPort);
            const int exitCode = m_daemon->runBlocking();
            m_daemon.reset();
            juce::Logger::writeToLog("Daemon exited with code " + std::to_string(exitCode));
            quit();
            return;
        }

        const int cacheIndex = args.indexOf("--build-reference-cache");
        if (cacheIndex >= 0 && cacheIndex + 1 < args.size())
        {
            const std::filesystem::path poolPath(args[cacheIndex + 1].toStdString());
            const auto outputPath = std::filesystem::path("analysis") / "reference_cache.json";
            const bool ok =
                dawai::reference_engine::pools::buildReferenceCache(poolPath, outputPath);
            juce::Logger::writeToLog(ok ? "Reference cache built."
                                        : "Reference cache build failed.");
            quit();
            return;
        }

        const int poolIndex = args.indexOf("--build-reference-pool");
        if (poolIndex >= 0)
        {
            std::filesystem::path inputRoot;
            if (poolIndex + 1 < args.size())
            {
                inputRoot = expandUserPath(args[poolIndex + 1].toStdString());
            }

            if (inputRoot.empty())
            {
                juce::Logger::writeToLog(
                    "Missing input folder. Provide --build-reference-pool <licensed-audio-dir>.");
                quit();
                return;
            }

            const auto outValue =
                valueAfterFlag(args, "--out", "analysis/reference_pools/generated");
            const auto poolName =
                valueAfterFlag(args, "--pool-name", "Generated Reference Pool").toStdString();
            const auto genre = valueAfterFlag(args, "--genre", "Unknown").toStdString();
            const auto maxFilesText = valueAfterFlag(args, "--max-files", "0");
            const auto maxFiles =
                static_cast<std::size_t>(juce::jmax(0, maxFilesText.getIntValue()));
            const auto outputRoot = expandUserPath(outValue.toStdString());

            juce::Logger::writeToLog("Building reference pool from: " + inputRoot.string());
            const bool ok =
                buildReferencePoolFromDirectory(inputRoot, outputRoot, poolName, genre, maxFiles);
            juce::Logger::writeToLog(ok ? "Reference pool built." : "Reference pool build failed.");
            quit();
            return;
        }

        const int chatIndex = args.indexOf("--chat-ask");
        if (chatIndex >= 0 && chatIndex + 1 < args.size())
        {
            const auto analysisDir =
                expandUserPath(valueAfterFlag(args, "--analysis", "analysis").toStdString());
            const auto prompt = args[chatIndex + 1].toStdString();
            const auto genre = valueAfterFlag(args, "--genre", "Unknown").toStdString();
            const auto conversationId =
                valueAfterFlag(args, "--conversation", "desktop-main").toStdString();
            const auto requestedModel = valueAfterFlag(args, "--model", "").toStdString();
            const auto attachments = parseAttachmentList(valueAfterFlag(args, "--attach", ""));

            auto adviser = dawai::advisory_layer::LocalBrainAdviser::createFromEnvironment();
            if (!adviser)
            {
                juce::Logger::writeToLog("Adviser unavailable.");
                quit();
                return;
            }

            const dawai::advisory_layer::SessionContext context{
                genre,
                prompt,
                std::string("contract=") + dawai::aifr3d_core::contract::kSystemId +
                    "; metrics=" + dawai::aifr3d_core::contract::kMetricMathPack,
                conversationId,
                prompt,
                requestedModel,
                attachments,
                {},
                {}};

            const auto output = adviser->advise(analysisDir, context);
            if (!output)
            {
                juce::Logger::writeToLog("No advisory output generated.");
                quit();
                return;
            }

            juce::Logger::writeToLog("Summary: " + output->summary30s);
            for (const auto& fix : output->top3Fixes)
            {
                juce::Logger::writeToLog("Fix: " + fix);
            }
            juce::Logger::writeToLog("Safe: " + output->safePath);
            juce::Logger::writeToLog("Bold: " + output->boldPath);
            juce::Logger::writeToLog("Details: " + output->details);
            quit();
            return;
        }

        if (!args.contains("--no-daemon"))
        {
            m_daemon = std::make_unique<dawai::DaemonServer>(daemonPort);
            if (!m_daemon->start())
            {
                juce::Logger::writeToLog(
                    "Background daemon failed to start. App will continue without daemon.");
                m_daemon.reset();
            }
        }

        const auto appDir = juce::File::getCurrentWorkingDirectory().getFullPathName();
        m_splashWindow = std::make_unique<SplashWindow>(
            [this, appDir]
            {
                if (!ensureFirstRunSetup(appDir))
                {
                    m_splashWindow.reset();
                    quit();
                    return;
                }
                m_splashWindow.reset();
                m_mainWindow = std::make_unique<MainWindow>(getApplicationName());
            });
    }

    void shutdown() override
    {
        if (m_daemon)
        {
            m_daemon->stop();
            m_daemon.reset();
        }
        m_mainWindow.reset();
        m_splashWindow.reset();
        juce::Logger::setCurrentLogger(nullptr);
        m_logger.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    class MainWindow : public juce::DocumentWindow
    {
      public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(std::move(name),
                             juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                                 juce::ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new dawai::ui::MainView(), true);
            setResizable(true, true);
            setResizeLimits(1180, 720, 2200, 1400);
            const auto savedBounds = loadStandaloneWindowBounds();
            if (savedBounds.getX() == 0 && savedBounds.getY() == 0)
            {
                centreWithSize(savedBounds.getWidth(), savedBounds.getHeight());
            }
            else
            {
                setBounds(savedBounds);
            }
            setVisible(true);
        }

        ~MainWindow() override
        {
            saveStandaloneWindowBounds(getBounds());
        }

        void closeButtonPressed() override
        {
            saveStandaloneWindowBounds(getBounds());
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    class SplashContent : public juce::Component
    {
      public:
        SplashContent() : m_image(loadStandaloneSplashImage()) {}

        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            juce::ColourGradient bg(juce::Colour::fromRGB(10, 12, 16).brighter(0.02f), bounds.getX(),
                                    bounds.getY(), juce::Colour::fromRGB(6, 8, 12), bounds.getX(),
                                    bounds.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(bounds.reduced(4.0f), 24.0f);
            g.setColour(juce::Colour::fromRGBA(196, 218, 255, 48));
            g.drawRoundedRectangle(bounds.reduced(5.0f), 24.0f, 1.2f);

            auto content = getLocalBounds().reduced(30, 24);
            auto imageArea = content.removeFromTop(360).reduced(30, 0).toFloat();
            if (m_image.isValid())
            {
                juce::ColourGradient glow(juce::Colour(0xff47d7ff).withAlpha(0.28f), imageArea.getCentreX(),
                                          imageArea.getCentreY(), juce::Colours::transparentBlack,
                                          imageArea.getX(), imageArea.getBottom(), true);
                g.setGradientFill(glow);
                g.fillEllipse(imageArea.reduced(18.0f).expanded(10.0f));
                juce::Path clipPath;
                clipPath.addEllipse(imageArea.reduced(18.0f));
                g.saveState();
                g.reduceClipRegion(clipPath);
                g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
                auto drawArea = imageArea.expanded(static_cast<int>(imageArea.getWidth() * 0.08f),
                                                   static_cast<int>(imageArea.getHeight() * 0.08f));
                g.drawImageWithin(m_image, drawArea.toNearestInt().getX(), drawArea.toNearestInt().getY(),
                                  drawArea.toNearestInt().getWidth(), drawArea.toNearestInt().getHeight(),
                                  juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
                g.restoreState();
            }

            g.setColour(juce::Colour::fromRGB(238, 242, 250));
            g.setFont(juce::Font(juce::FontOptions("Rajdhani", 30.0f, juce::Font::bold)));
            g.drawText("AIFR3D 2.2.4 Beta", content.removeFromTop(34), juce::Justification::centred);
            g.setColour(juce::Colour::fromRGB(176, 186, 206));
            g.setFont(juce::Font(juce::FontOptions("Inter", 16.0f, juce::Font::plain)));
            g.drawFittedText("North3rnLight3r — updated halo, candlestick meters, Mix Tips tabs, and OpenAI-only advisory flow.",
                             content.removeFromTop(48), juce::Justification::centred, 3);
        }

      private:
        juce::Image m_image;
    };

    class SplashWindow : public juce::DocumentWindow, private juce::Timer
    {
      public:
        explicit SplashWindow(std::function<void()> onDone)
            : DocumentWindow("Aifr3dVST3", juce::Colours::transparentBlack, juce::DocumentWindow::closeButton),
              m_onDone(std::move(onDone))
        {
            setUsingNativeTitleBar(false);
            setResizable(false, false);
            setTitleBarHeight(0);
            setContentOwned(new SplashContent(), true);
            centreWithSize(560, 520);
            setAlwaysOnTop(true);
            setVisible(true);
            setAlpha(0.0f);
            startTimerHz(60);
        }

        void closeButtonPressed() override
        {
            finish();
        }

      private:
        void timerCallback() override
        {
            ++m_frame;
            const float fadeIn = juce::jlimit(0.0f, 1.0f, static_cast<float>(m_frame) / 18.0f);
            setAlpha(fadeIn);
            if (m_frame >= 96)
            {
                finish();
            }
        }

        void finish()
        {
            stopTimer();
            setVisible(false);
            auto onDone = std::move(m_onDone);
            juce::MessageManager::callAsync(
                [callback = std::move(onDone)]
                {
                    if (callback)
                    {
                        callback();
                    }
                });
        }

        std::function<void()> m_onDone;
        int m_frame = 0;
    };

  private:
    std::unique_ptr<MainWindow> m_mainWindow;
    std::unique_ptr<SplashWindow> m_splashWindow;
    std::unique_ptr<dawai::DaemonServer> m_daemon;
    std::unique_ptr<juce::FileLogger> m_logger;
};

START_JUCE_APPLICATION(DawaiApplication)
