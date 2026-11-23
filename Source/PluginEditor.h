#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Theme.h"

namespace insideout_ui
{

//==============================================================================
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
};

//==============================================================================
/** Pill-shaped segmented switch bound to a choice parameter. */
class SegmentedControl : public juce::Component
{
public:
    SegmentedControl (juce::RangedAudioParameter& param, juce::StringArray labels, float fontSize = 12.0f);

    /** Called on a user click. If not set, the parameter is set directly. */
    std::function<void (int)> onUserSelect;

    int getIdealWidth() const;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    int segmentAt (juce::Point<float>) const;
    juce::Rectangle<float> segmentBounds (int index) const;

    juce::StringArray labels;
    float fontSize;
    int selected = 0, hovered = -1;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Rotary knob with label and value text underneath. */
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& labelText, int knobSize);

    void paint (juce::Graphics&) override;
    void resized() override;
    static int heightFor (int knobSize) { return knobSize + 40; }

private:
    juce::Slider slider;
    juce::RangedAudioParameter* param;
    juce::String labelText;
    int knobSize;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/** Live spectrum: mix, sidechain, reduction and draggable protect handles. */
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumView (InsideOutProcessor&);
    ~SpectrumView() override;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    float xForHz (float hz) const;
    float hzForX (float x) const;
    int handleAt (float x) const; // 0 none, 1 low, 2 high
    juce::Path curve (const std::array<float, insideout::kMaxBands>& values, int n,
                      std::function<float (float)> yFor) const;

    InsideOutProcessor& processor;
    juce::RangedAudioParameter& lowParam;
    juce::RangedAudioParameter& highParam;

    std::array<float, insideout::kMaxBands> dispMain {}, dispSc {}, dispRed {}, centres {};
    int numBands = 32;
    float depthDb = 6.0f;
    int dragging = 0;
};

} // namespace insideout_ui

//==============================================================================
class InsideOutEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit InsideOutEditor (InsideOutProcessor&);
    ~InsideOutEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int designWidth = 960;
    static constexpr int designHeight = 620;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (InsideOutProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;
        void refreshStatus();

    private:
        void rebuildPresetMenu();

        InsideOutProcessor& processor;
        insideout_ui::SegmentedControl modeControl, stereoControl, bandsControl;
        juce::ComboBox presetBox;
        insideout_ui::SpectrumView spectrum;

        std::unique_ptr<insideout_ui::Knob> amount, attack, release, scLow, scHigh, lowProt, highProt, dryWet, output;

        juce::TextButton listenButton { "Listen" }, bypassButton { "Bypass" };
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttach, bypassAttach;

        int lastPresetCounter = -1;
        int scState = -1;
        float maxRed = 0.0f;

        friend class InsideOutEditor;
    };

    void timerCallback() override;

    InsideOutProcessor& processorRef;
    insideout_ui::LookAndFeel lnf;
    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InsideOutEditor)
};
