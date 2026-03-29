#pragma once

#include <juce_graphics/juce_graphics.h>

namespace dawai::ui::theme
{

inline juce::String pickTypeface(const std::initializer_list<const char*> candidates,
                                 const juce::String& fallback)
{
    const auto installed = juce::Font::findAllTypefaceNames();
    for (const auto* candidate : candidates)
    {
        if (installed.contains(candidate))
        {
            return candidate;
        }
    }
    return fallback;
}

struct Palette
{
    static juce::Colour matteBlack()
    {
        return juce::Colour::fromRGB(8, 10, 14);
    }
    static juce::Colour panel()
    {
        return juce::Colour::fromRGB(16, 20, 28);
    }
    static juce::Colour panelAlt()
    {
        return juce::Colour::fromRGB(12, 16, 23);
    }
    static juce::Colour accent()
    {
        return juce::Colour::fromRGB(0, 198, 186);
    }
    static juce::Colour accentMuted()
    {
        return juce::Colour::fromRGB(28, 122, 214);
    }
    static juce::Colour accentWarm()
    {
        return juce::Colour::fromRGB(220, 168, 62);
    }
    static juce::Colour accentPurple()
    {
        return juce::Colour::fromRGB(122, 92, 228);
    }
    static juce::Colour danger()
    {
        return juce::Colour::fromRGB(224, 82, 82);
    }
    static juce::Colour textPrimary()
    {
        return juce::Colour::fromRGB(238, 242, 250);
    }
    static juce::Colour textSecondary()
    {
        return juce::Colour::fromRGB(176, 186, 206);
    }
    static juce::Colour stroke()
    {
        return juce::Colour::fromRGBA(196, 218, 255, 42);
    }
};

inline juce::Font headingFont()
{
    return juce::Font(juce::FontOptions(
        pickTypeface({"Rajdhani SemiBold", "Rajdhani", "Space Grotesk", "Inter Semi Bold", "Inter"},
                     juce::Font::getDefaultSansSerifFontName()),
        19.0F, juce::Font::bold));
}

inline juce::Font bodyFont()
{
    return juce::Font(juce::FontOptions(
        pickTypeface({"Inter", "IBM Plex Sans", "Source Sans 3", "Space Grotesk"},
                     juce::Font::getDefaultSansSerifFontName()),
        15.0F, juce::Font::plain));
}

inline juce::Font monoFont()
{
    return juce::Font(juce::FontOptions(
        pickTypeface({"JetBrains Mono", "IBM Plex Mono", "Source Code Pro", "Menlo"},
                     juce::Font::getDefaultMonospacedFontName()),
        13.0F, juce::Font::plain));
}

} // namespace dawai::ui::theme
