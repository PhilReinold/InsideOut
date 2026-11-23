#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace theme
{
    // Miso Choir, light variant: pearl cream, petrol, soft green
    inline const juce::Colour bg         { 0xfff3eee3 };
    inline const juce::Colour panel      { 0xfffbf8f1 };
    inline const juce::Colour border     { 0xffcfc8b8 };
    inline const juce::Colour ink        { 0xff1d2b2c };
    inline const juce::Colour muted      { 0xff55625f };
    inline const juce::Colour petrol     { 0xff1f5a60 };
    inline const juce::Colour petrolDark { 0xff123c40 };
    inline const juce::Colour green      { 0xff6f9f55 };
    inline const juce::Colour track      { 0xffddd6c7 };
    inline const juce::Colour ring       { 0xff8a948f };
    inline const juce::Colour protect    { 0xffebe5d8 };
    inline const juce::Colour grid       { 0xffd9d2c3 };
    inline const juce::Colour gridSoft   { 0xffe6e0d3 };
    inline const juce::Colour scOk       { 0xff4f8a3f };
    inline const juce::Colour scOff      { 0xffb8300f };
    inline const juce::Colour scIdle     { 0xffb9a77a };

    struct Fonts
    {
        juce::Typeface::Ptr title, sans, sansMedium, sansBold, mono, monoMedium;

        static Fonts& get()
        {
            static Fonts f;
            return f;
        }

    private:
        Fonts()
        {
            auto load = [] (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); };
            title      = load (BinaryData::UnifrakturCookBold_ttf, BinaryData::UnifrakturCookBold_ttfSize);
            sans       = load (BinaryData::DMSansRegular_ttf,      BinaryData::DMSansRegular_ttfSize);
            sansMedium = load (BinaryData::DMSansMedium_ttf,       BinaryData::DMSansMedium_ttfSize);
            sansBold   = load (BinaryData::DMSansBold_ttf,         BinaryData::DMSansBold_ttfSize);
            mono       = load (BinaryData::DMMonoRegular_ttf,      BinaryData::DMMonoRegular_ttfSize);
            monoMedium = load (BinaryData::DMMonoMedium_ttf,       BinaryData::DMMonoMedium_ttfSize);
        }
    };

    inline juce::Font font (const juce::Typeface::Ptr& tf, float height)
    {
        return juce::Font (juce::FontOptions (tf).withHeight (height));
    }

    inline juce::Font titleFont (float h)  { return font (Fonts::get().title, h); }
    inline juce::Font sans (float h)       { return font (Fonts::get().sans, h); }
    inline juce::Font sansMedium (float h) { return font (Fonts::get().sansMedium, h); }
    inline juce::Font sansBold (float h)   { return font (Fonts::get().sansBold, h); }
    inline juce::Font mono (float h)       { return font (Fonts::get().mono, h); }

    /** Small caps-style label: bold, upper case, spaced. */
    inline juce::Font label (float h)      { return sansBold (h).withExtraKerningFactor (0.08f); }

    inline float textWidth (const juce::Font& f, const juce::String& s)
    {
        return juce::GlyphArrangement::getStringWidth (f, s);
    }
}
