#include "PluginEditor.h"

using namespace insideout;

namespace insideout_ui
{

//==============================================================================
// LookAndFeel
//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, theme::panel);
    setColour (juce::PopupMenu::textColourId, theme::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::petrol);
    setColour (juce::PopupMenu::highlightedTextColourId, theme::panel);
    setColour (juce::PopupMenu::headerTextColourId, theme::muted);
    setColour (juce::ComboBox::textColourId, theme::ink);
    setColour (juce::ComboBox::backgroundColourId, theme::panel);
    setColour (juce::ComboBox::outlineColourId, theme::ring);
    setColour (juce::ComboBox::arrowColourId, theme::ink);
    setColour (juce::TextButton::textColourOffId, theme::ink);
    setColour (juce::TextButton::textColourOnId, theme::panel);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float startAngle, float endAngle, juce::Slider& slider)
{
    const float size = (float) juce::jmin (w, h);
    const float cx = (float) x + (float) w * 0.5f;
    const float cy = (float) y + (float) h * 0.5f;
    const bool large = size > 80.0f;
    const float sw = large ? 6.0f : 4.0f;
    const float r = size * 0.5f - 6.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool active = slider.isMouseOverOrDragging();

    // body
    const float br = r - sw - 2.0f;
    g.setColour (theme::panel);
    g.fillEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f);
    g.setColour (active ? theme::ink : theme::ring);
    g.drawEllipse (cx - br, cy - br, br * 2.0f, br * 2.0f, 1.5f);

    // track
    juce::Path trackArc;
    trackArc.addCentredArc (cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour (theme::track);
    g.strokePath (trackArc, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value
    if (pos > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (cx, cy, r, r, 0.0f, startAngle, angle, true);
        g.setColour (theme::petrol);
        g.strokePath (valueArc, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // pointer
    const float pr = r - (large ? 14.0f : 10.0f);
    g.setColour (theme::ink);
    g.drawLine (cx, cy, cx + pr * std::sin (angle), cy - pr * std::cos (angle), large ? 2.5f : 2.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                        bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.75f);
    const float corner = r.getHeight() * 0.5f;

    if (b.getToggleState())
    {
        g.setColour (down ? theme::petrolDark : theme::petrol);
        g.fillRoundedRectangle (r, corner);
        return;
    }

    g.setColour (highlighted ? theme::panel.darker (0.03f) : theme::panel);
    g.fillRoundedRectangle (r, corner);
    g.setColour (highlighted ? theme::ink : theme::ring);
    g.drawRoundedRectangle (r, corner, 1.5f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    g.setFont (on ? theme::sansBold (12.5f) : theme::sansMedium (12.5f));
    g.setColour (on ? theme::panel : theme::ink);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.75f);
    const float corner = r.getHeight() * 0.5f;
    g.setColour (theme::panel);
    g.fillRoundedRectangle (r, corner);
    g.setColour (box.isMouseOver (true) ? theme::ink : theme::ring);
    g.drawRoundedRectangle (r, corner, 1.5f);

    juce::Path chevron;
    const float ax = (float) w - 22.0f, ay = (float) h * 0.5f - 2.0f;
    chevron.startNewSubPath (ax, ay);
    chevron.lineTo (ax + 4.5f, ay + 4.5f);
    chevron.lineTo (ax + 9.0f, ay);
    g.setColour (theme::ink);
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return theme::sansMedium (12.5f); }

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 38, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font LookAndFeel::getPopupMenuFont() { return theme::sans (14.0f); }

void LookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& text)
{
    g.setFont (theme::label (11.0f));
    g.setColour (theme::muted);
    g.drawText (text.toUpperCase(), area.reduced (12, 0).withTrimmedTop (4), juce::Justification::centredLeft, true);
}

//==============================================================================
// SegmentedControl
//==============================================================================
SegmentedControl::SegmentedControl (juce::RangedAudioParameter& param, juce::StringArray l, float fs)
    : labels (std::move (l)), fontSize (fs),
      attachment (param, [this] (float v) { selected = juce::roundToInt (v); repaint(); }, nullptr)
{
    setTitle (param.getName (64));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

int SegmentedControl::getIdealWidth() const
{
    float total = 6.0f + 2.0f * (float) (labels.size() - 1);
    for (auto& s : labels)
        total += theme::textWidth (theme::sansBold (fontSize), s) + 24.0f;
    return (int) std::ceil (total);
}

juce::Rectangle<float> SegmentedControl::segmentBounds (int index) const
{
    std::vector<float> widths;
    float sum = 0.0f;
    for (auto& s : labels)
    {
        widths.push_back (theme::textWidth (theme::sansBold (fontSize), s) + 24.0f);
        sum += widths.back();
    }
    const float avail = (float) getWidth() - 6.0f - 2.0f * (float) (labels.size() - 1);
    const float scale = sum > 0.0f ? avail / sum : 1.0f;

    float x = 3.0f;
    for (int i = 0; i < index; ++i)
        x += widths[(size_t) i] * scale + 2.0f;
    return { x, 3.0f, widths[(size_t) index] * scale, (float) getHeight() - 6.0f };
}

int SegmentedControl::segmentAt (juce::Point<float> p) const
{
    for (int i = 0; i < labels.size(); ++i)
        if (segmentBounds (i).expanded (1.0f, 3.0f).contains (p))
            return i;
    return -1;
}

void SegmentedControl::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.75f);
    g.setColour (theme::panel);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (theme::ring);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.5f);

    for (int i = 0; i < labels.size(); ++i)
    {
        auto s = segmentBounds (i);
        const bool on = i == selected;
        if (on)
        {
            g.setColour (theme::petrol);
            g.fillRoundedRectangle (s, s.getHeight() * 0.5f);
        }
        else if (i == hovered)
        {
            g.setColour (theme::track.withAlpha (0.6f));
            g.fillRoundedRectangle (s, s.getHeight() * 0.5f);
        }
        g.setFont (on ? theme::sansBold (fontSize) : theme::sansMedium (fontSize));
        g.setColour (on ? theme::panel : theme::ink);
        g.drawText (labels[i], s, juce::Justification::centred, false);
    }
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const int i = segmentAt (e.position);
    if (i < 0)
        return;

    if (onUserSelect)
        onUserSelect (i);
    else
        attachment.setValueAsCompleteGesture ((float) i);
}

void SegmentedControl::mouseMove (const juce::MouseEvent& e)
{
    const int i = segmentAt (e.position);
    if (i != hovered) { hovered = i; repaint(); }
}

void SegmentedControl::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

//==============================================================================
// Knob
//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text, int size)
    : param (apvts.getParameter (paramId)), labelText (text), knobSize (size)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setVelocityBasedMode (false);
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.setTitle (text);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
}

void Knob::resized()
{
    slider.setBounds ((getWidth() - knobSize) / 2, 0, knobSize, knobSize);
}

void Knob::paint (juce::Graphics& g)
{
    const bool large = knobSize > 80;
    auto area = getLocalBounds().withTrimmedTop (knobSize + 4);

    g.setFont (theme::label (large ? 12.0f : 11.0f));
    g.setColour (theme::muted);
    g.drawText (labelText.toUpperCase(), area.removeFromTop (16), juce::Justification::centred, false);

    g.setFont (theme::mono (large ? 15.0f : 13.0f));
    g.setColour (theme::ink);
    g.drawText (param->getCurrentValueAsText(), area.removeFromTop (18), juce::Justification::centred, false);
}

//==============================================================================
// SpectrumView
//==============================================================================
SpectrumView::SpectrumView (InsideOutProcessor& p)
    : processor (p),
      lowParam (*p.apvts.getParameter (ParamID::lowProtect)),
      highParam (*p.apvts.getParameter (ParamID::highProtect))
{
    dispMain.fill (-120.0f);
    dispSc.fill (-120.0f);
    dispRed.fill (0.0f);
    centres.fill (1000.0f);
    setTitle ("Spektrum");
    startTimerHz (30);
}

SpectrumView::~SpectrumView() { stopTimer(); }

float SpectrumView::xForHz (float hz) const
{
    return (float) getWidth() * std::log10 (juce::jmax (kMinFreq, hz) / kMinFreq) / 3.0f;
}

float SpectrumView::hzForX (float x) const
{
    return kMinFreq * std::pow (10.0f, 3.0f * x / (float) juce::jmax (1, getWidth()));
}

void SpectrumView::timerCallback()
{
    auto& m = processor.getMeter();
    numBands = juce::jlimit (4, kMaxBands, m.numBands.load());
    depthDb = m.depthDb.load();

    auto follow = [] (float& disp, float v, float fallRate)
    {
        disp = v > disp ? v : disp + (v - disp) * fallRate;
    };

    const bool scOn = processor.sidechainConnected.load();
    for (int b = 0; b < numBands; ++b)
    {
        centres[(size_t) b] = m.centreHz[(size_t) b].load();
        follow (dispMain[(size_t) b], m.mainDb[(size_t) b].load(), 0.2f);
        follow (dispSc[(size_t) b], scOn ? m.scDb[(size_t) b].load() : -120.0f, 0.2f);
        follow (dispRed[(size_t) b], m.redDb[(size_t) b].load(), 0.35f);
    }
    repaint();
}

juce::Path SpectrumView::curve (const std::array<float, kMaxBands>& values, int n,
                                std::function<float (float)> yFor) const
{
    juce::Path p;
    p.startNewSubPath (0.0f, yFor (values[0]));
    for (int b = 0; b < n; ++b)
        p.lineTo (xForHz (centres[(size_t) b]), yFor (values[(size_t) b]));
    p.lineTo ((float) getWidth(), yFor (values[(size_t) (n - 1)]));
    return p.createPathWithRoundedCorners (6.0f);
}

int SpectrumView::handleAt (float x) const
{
    const float lowHz = lowParam.convertFrom0to1 (lowParam.getValue());
    const float highHz = highParam.convertFrom0to1 (highParam.getValue());
    const float lx = juce::jmax (6.0f, lowHz > 20.5f ? xForHz (lowHz) : 0.0f);
    const float hx = juce::jmin ((float) getWidth() - 6.0f, highHz < 19990.0f ? xForHz (highHz) : (float) getWidth());

    if (std::abs (x - lx) < 10.0f) return 1;
    if (std::abs (x - hx) < 10.0f) return 2;
    return 0;
}

void SpectrumView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth(), h = bounds.getHeight();

    juce::Path frame;
    frame.addRoundedRectangle (bounds.reduced (0.5f), 14.0f);
    g.setColour (theme::panel);
    g.fillPath (frame);

    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (frame);

        // grid
        g.setColour (theme::gridSoft);
        for (float f : { 0.25f, 0.5f, 0.75f })
            g.drawHorizontalLine ((int) (h * f), 0.0f, w);

        const std::pair<float, const char*> lines[] = { { 50.0f, "50" }, { 100.0f, "100" }, { 200.0f, "200" }, { 500.0f, "500" },
                                                        { 1000.0f, "1k" }, { 2000.0f, "2k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" } };
        for (auto& [hz, text] : lines)
        {
            juce::ignoreUnused (text);
            g.setColour (theme::grid);
            g.drawVerticalLine ((int) xForHz (hz), 0.0f, h);
        }

        const int n = numBands;

        // mix level
        auto yLevel = [h] (float db) { return juce::jmap (juce::jlimit (-90.0f, 0.0f, db), -90.0f, 0.0f, h, h * 0.1f); };
        auto mainPath = curve (dispMain, n, yLevel);
        auto mainFill = mainPath;
        mainFill.lineTo (w, h);
        mainFill.lineTo (0.0f, h);
        mainFill.closeSubPath();
        g.setColour (theme::petrol.withAlpha (0.16f));
        g.fillPath (mainFill);
        g.setColour (theme::petrol);
        g.strokePath (mainPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // sidechain (dashed)
        if (processor.sidechainConnected.load())
        {
            auto scPath = curve (dispSc, n, yLevel);
            juce::Path dashed;
            const float dashes[] = { 5.0f, 4.0f };
            juce::PathStrokeType (2.0f).createDashedStroke (dashed, scPath, dashes, 2);
            g.setColour (theme::green);
            g.fillPath (dashed);
        }

        // reduction from the top
        const float depth = juce::jmax (1.0f, depthDb);
        auto yRed = [h, depth] (float db) { return juce::jlimit (0.0f, h, db / depth * h * 0.8f); };
        auto redPath = curve (dispRed, n, yRed);
        auto redFill = redPath;
        redFill.lineTo (w, 0.0f);
        redFill.lineTo (0.0f, 0.0f);
        redFill.closeSubPath();
        g.setColour (theme::petrol.withAlpha (0.55f));
        g.fillPath (redFill);
        g.setColour (theme::petrolDark);
        g.strokePath (redPath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // protected areas
        const float lowHz = lowParam.convertFrom0to1 (lowParam.getValue());
        const float highHz = highParam.convertFrom0to1 (highParam.getValue());
        const float lx = lowHz > 20.5f ? xForHz (lowHz) : 0.0f;
        const float hx = highHz < 19990.0f ? xForHz (highHz) : w;

        auto drawProtected = [&] (juce::Rectangle<float> r)
        {
            if (r.getWidth() <= 0.5f)
                return;
            g.setColour (theme::protect.withAlpha (0.85f));
            g.fillRect (r);
            juce::Graphics::ScopedSaveState s2 (g);
            g.reduceClipRegion (r.toNearestInt());
            g.setColour (theme::border.withAlpha (0.6f));
            for (float x = r.getX() - h; x < r.getRight() + h; x += 8.0f)
                g.drawLine (x, h, x + h, 0.0f, 2.0f);
        };
        drawProtected ({ 0.0f, 0.0f, lx, h });
        drawProtected ({ hx, 0.0f, w - hx, h });

        auto drawHandle = [&] (float x, bool hasLine)
        {
            if (hasLine)
            {
                g.setColour (theme::ink);
                g.fillRect (x - 1.0f, 0.0f, 2.0f, h);
            }
            const float hxPos = juce::jlimit (6.0f, w - 6.0f, x);
            juce::Rectangle<float> knob (hxPos - 6.0f, h * 0.5f - 18.0f, 12.0f, 36.0f);
            g.setColour (theme::panel);
            g.fillRoundedRectangle (knob, 6.0f);
            g.setColour (theme::ink);
            g.drawRoundedRectangle (knob, 6.0f, 2.0f);
        };
        drawHandle (lx, lx > 0.0f);
        drawHandle (hx, hx < w);

        // frequency labels on top of everything
        g.setFont (theme::mono (11.0f));
        g.setColour (theme::muted);
        for (auto& [hz, text] : lines)
            g.drawText (text, juce::Rectangle<float> (xForHz (hz) + 4.0f, h - 22.0f, 40.0f, 16.0f), juce::Justification::centredLeft, false);

        // labels
        g.setColour (theme::ink);
        g.setFont (theme::label (11.0f));
        g.drawText ("LOW PROTECT", juce::Rectangle<float> (10.0f, 26.0f, 120.0f, 16.0f), juce::Justification::centredLeft, false);
        g.setFont (theme::mono (12.0f));
        g.drawText (lowParam.getCurrentValueAsText(), juce::Rectangle<float> (10.0f, 42.0f, 120.0f, 16.0f), juce::Justification::centredLeft, false);

        const bool roomRight = w - hx > 110.0f;
        const auto just = roomRight ? juce::Justification::centredLeft : juce::Justification::centredRight;
        const float tx = roomRight ? hx + 10.0f : w - 130.0f;
        g.setFont (theme::label (11.0f));
        g.drawText ("HIGH PROTECT", juce::Rectangle<float> (tx, 26.0f, 120.0f, 16.0f), just, false);
        g.setFont (theme::mono (12.0f));
        g.drawText (highParam.getCurrentValueAsText(), juce::Rectangle<float> (tx, 42.0f, 120.0f, 16.0f), just, false);

        // sidechain hint
        juce::String hint;
        if (! processor.sidechainConnected.load())
            hint = juce::String (juce::CharPointer_UTF8 ("Kein Sidechain verbunden \xe2\x80\x93 w\xc3\xa4hle oben im Plugin-Fenster die Vocal-Spur als Side Chain."));
        else if (! processor.sidechainActive.load())
            hint = "Sidechain ist still: spiel die Vocals ab.";

        if (hint.isNotEmpty())
        {
            const auto f = theme::sans (12.5f);
            const float tw = theme::textWidth (f, hint) + 28.0f;
            juce::Rectangle<float> pill ((w - tw) * 0.5f, 14.0f, tw, 26.0f);
            g.setColour (theme::panel.withAlpha (0.95f));
            g.fillRoundedRectangle (pill, 13.0f);
            g.setColour (theme::border);
            g.drawRoundedRectangle (pill, 13.0f, 1.0f);
            g.setColour (theme::ink);
            g.setFont (f);
            g.drawText (hint, pill, juce::Justification::centred, false);
        }
    }

    g.setColour (theme::border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);
}

void SpectrumView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (handleAt (e.position.x) != 0 ? juce::MouseCursor::LeftRightResizeCursor
                                                  : juce::MouseCursor::NormalCursor);
}

void SpectrumView::mouseDown (const juce::MouseEvent& e)
{
    dragging = handleAt (e.position.x);
    if (dragging == 1) lowParam.beginChangeGesture();
    if (dragging == 2) highParam.beginChangeGesture();
}

void SpectrumView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == 0)
        return;

    auto& param = dragging == 1 ? lowParam : highParam;
    const auto& range = param.getNormalisableRange();
    const float hz = juce::jlimit (range.start, range.end, hzForX (e.position.x));
    param.setValueNotifyingHost (param.convertTo0to1 (hz));
}

void SpectrumView::mouseUp (const juce::MouseEvent&)
{
    if (dragging == 1) lowParam.endChangeGesture();
    if (dragging == 2) highParam.endChangeGesture();
    dragging = 0;
}

void SpectrumView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hdl = handleAt (e.position.x);
    if (hdl == 0)
        return;
    auto& param = hdl == 1 ? lowParam : highParam;
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.getDefaultValue());
    param.endChangeGesture();
}

} // namespace insideout_ui

//==============================================================================
// Editor
//==============================================================================
using namespace insideout_ui;

InsideOutEditor::Content::Content (InsideOutProcessor& p)
    : processor (p),
      modeControl (*p.apvts.getParameter (ParamID::mode), modeNames(), 12.5f),
      stereoControl (*p.apvts.getParameter (ParamID::stereo), stereoNames(), 12.0f),
      bandsControl (*p.apvts.getParameter (ParamID::bands), bandNames(), 12.0f),
      spectrum (p)
{
    setOpaque (true);

    modeControl.onUserSelect = [this] (int i) { processor.applyModeDefaults (i); };
    for (auto* c : { (juce::Component*) &modeControl, (juce::Component*) &stereoControl,
                     (juce::Component*) &bandsControl, (juce::Component*) &spectrum })
        addAndMakeVisible (c);

    rebuildPresetMenu();
    presetBox.setTitle ("Preset");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0)
            processor.applyPreset (id - 1);
    };
    addAndMakeVisible (presetBox);

    auto& s = processor.apvts;
    amount   = std::make_unique<Knob> (s, ParamID::amount,      "Amount",       120);
    attack   = std::make_unique<Knob> (s, ParamID::attack,      "Attack",       64);
    release  = std::make_unique<Knob> (s, ParamID::release,     "Release",      64);
    scLow    = std::make_unique<Knob> (s, ParamID::scLowCut,    "SC Low Cut",   64);
    scHigh   = std::make_unique<Knob> (s, ParamID::scHighCut,   "SC High Cut",  64);
    lowProt  = std::make_unique<Knob> (s, ParamID::lowProtect,  "Low Protect",  64);
    highProt = std::make_unique<Knob> (s, ParamID::highProtect, "High Protect", 64);
    dryWet   = std::make_unique<Knob> (s, ParamID::dryWet,      "Dry/Wet",      64);
    output   = std::make_unique<Knob> (s, ParamID::output,      "Output",       64);

    for (auto* k : { amount.get(), attack.get(), release.get(), scLow.get(), scHigh.get(),
                     lowProt.get(), highProt.get(), dryWet.get(), output.get() })
        addAndMakeVisible (k);

    for (auto* b : { &listenButton, &bypassButton })
    {
        b->setClickingTogglesState (true);
        b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (b);
    }
    listenButton.setTooltip ("Nur das hoeren, was entfernt wird");
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, ParamID::listen, listenButton);
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, ParamID::bypass, bypassButton);

    refreshStatus();
}

void InsideOutEditor::Content::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    juce::String lastCategory;
    const auto& presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (lastCategory != presets[i].category)
        {
            lastCategory = presets[i].category;
            presetBox.addSectionHeading (lastCategory);
        }
        presetBox.addItem (presets[i].name, (int) i + 1);
    }
}

void InsideOutEditor::Content::refreshStatus()
{
    const int counter = processor.presetChangeCounter.load();
    if (counter != lastPresetCounter)
    {
        lastPresetCounter = counter;
        presetBox.setSelectedId (processor.currentPreset + 1, juce::dontSendNotification);
    }

    const int newState = ! processor.sidechainConnected.load() ? 0 : (processor.sidechainActive.load() ? 2 : 1);
    const float newRed = processor.getMeter().maxRedDb.load();

    if (newState != scState || std::abs (newRed - maxRed) > 0.05f)
    {
        scState = newState;
        maxRed = newRed;
        repaint (0, 72, getWidth(), 30);
    }
}

void InsideOutEditor::Content::resized()
{
    const int right = InsideOutEditor::designWidth - 40;

    presetBox.setBounds (right - 190, 26, 190, 30);
    const int mw = modeControl.getIdealWidth();
    modeControl.setBounds (presetBox.getX() - 12 - mw, 26, mw, 30);

    spectrum.setBounds (40, 104, InsideOutEditor::designWidth - 80, 230);

    const int kw = 76, kh = Knob::heightFor (64), gapX = 14, row1 = 352, row2 = row1 + kh + 6;
    attack ->setBounds (40,            row1, kw, kh);
    release->setBounds (40 + kw + gapX, row1, kw, kh);
    scLow  ->setBounds (40,            row2, kw, kh);
    scHigh ->setBounds (40 + kw + gapX, row2, kw, kh);

    const int rx = right - 2 * kw - gapX;
    lowProt ->setBounds (rx,            row1, kw, kh);
    highProt->setBounds (rx + kw + gapX, row1, kw, kh);
    dryWet  ->setBounds (rx,            row2, kw, kh);
    output  ->setBounds (rx + kw + gapX, row2, kw, kh);

    const int aw = 150, ah = Knob::heightFor (120);
    amount->setBounds ((InsideOutEditor::designWidth - aw) / 2, row1 + (row2 + kh - row1 - ah) / 2, aw, ah);

    const int fy = 576, fh = 28;
    int x = 40;
    stereoControl.setBounds (x, fy, stereoControl.getIdealWidth(), fh); x = stereoControl.getRight() + 12;
    bandsControl.setBounds (x, fy, bandsControl.getIdealWidth(), fh);   x = bandsControl.getRight() + 12;
    listenButton.setBounds (x, fy, 70, fh);                              x = listenButton.getRight() + 10;
    bypassButton.setBounds (x, fy, 76, fh);
}

void InsideOutEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (theme::bg);
    const float W = (float) InsideOutEditor::designWidth;

    // title
    g.setColour (theme::petrol);
    g.setFont (theme::titleFont (46.0f));
    g.drawText ("InsideOut", juce::Rectangle<float> (40.0f, 14.0f, 320.0f, 54.0f), juce::Justification::centredLeft, false);

    // legend row
    const float ly = 78.0f, lh = 18.0f, cy = ly + lh * 0.5f;
    float x = 40.0f;
    g.setFont (theme::sans (12.5f));

    auto legendText = [&] (const juce::String& t)
    {
        g.setColour (theme::muted);
        const float tw = theme::textWidth (g.getCurrentFont(), t);
        g.drawText (t, juce::Rectangle<float> (x, ly, tw + 2.0f, lh), juce::Justification::centredLeft, false);
        x += tw + 20.0f;
    };

    g.setColour (theme::petrol);
    g.fillRect (x, cy - 1.0f, 22.0f, 2.0f);
    x += 28.0f;
    legendText ("Mix");

    {
        juce::Path line, dashed;
        line.startNewSubPath (x, cy);
        line.lineTo (x + 22.0f, cy);
        const float d[] = { 5.0f, 4.0f };
        juce::PathStrokeType (2.0f).createDashedStroke (dashed, line, d, 2);
        g.setColour (theme::green);
        g.fillPath (dashed);
        x += 28.0f;
        legendText ("Vocals (Sidechain)");
    }

    g.setColour (theme::petrol.withAlpha (0.55f));
    g.fillRoundedRectangle (x, cy - 5.0f, 14.0f, 10.0f, 2.0f);
    x += 20.0f;
    legendText ("Absenkung");

    g.setColour (theme::protect);
    g.fillRoundedRectangle (x, cy - 5.0f, 14.0f, 10.0f, 2.0f);
    g.setColour (theme::border);
    g.drawRoundedRectangle (x, cy - 5.0f, 14.0f, 10.0f, 2.0f, 1.0f);
    x += 20.0f;
    legendText (juce::String (juce::CharPointer_UTF8 ("Gesch\xc3\xbctzt")));

    // right side: max reduction + sidechain status
    g.setFont (theme::mono (12.5f));
    const auto redText = "max " + formatNumber (-maxRed, 1) + " dB";
    const float rw = theme::textWidth (g.getCurrentFont(), redText);
    float rx = W - 40.0f - rw;
    g.setColour (theme::ink);
    g.drawText (redText, juce::Rectangle<float> (rx, ly, rw + 2.0f, lh), juce::Justification::centredLeft, false);

    const juce::String scText = scState == 2 ? "Sidechain aktiv" : (scState == 1 ? "Sidechain still" : "Kein Sidechain");
    g.setFont (theme::sans (12.5f));
    const float sw = theme::textWidth (g.getCurrentFont(), scText);
    rx -= 24.0f + sw;
    g.drawText (scText, juce::Rectangle<float> (rx, ly, sw + 2.0f, lh), juce::Justification::centredLeft, false);
    g.setColour (scState == 2 ? theme::scOk : (scState == 1 ? theme::scIdle : theme::scOff));
    g.fillEllipse (rx - 14.0f, cy - 4.0f, 8.0f, 8.0f);

    // footer brand
    g.setFont (theme::titleFont (17.0f));
    const juce::String maker ("Philip Jin Plugins");
    const float mw = theme::textWidth (g.getCurrentFont(), maker);
    g.setColour (theme::petrol);
    g.drawText (maker, juce::Rectangle<float> (W - 40.0f - mw, 576.0f, mw + 2.0f, 28.0f), juce::Justification::centredLeft, false);

    g.setFont (theme::sans (12.5f));
    const juce::String brand ("Boy made of Soy");
    const float bw = theme::textWidth (g.getCurrentFont(), brand);
    g.setColour (theme::muted);
    g.drawText (brand, juce::Rectangle<float> (W - 40.0f - mw - 16.0f - bw, 576.0f, bw + 2.0f, 28.0f), juce::Justification::centredLeft, false);
}

//==============================================================================
InsideOutEditor::InsideOutEditor (InsideOutProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), content (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, designWidth, designHeight);

    setResizable (true, true);
    setResizeLimits (designWidth * 3 / 4, designHeight * 3 / 4, designWidth * 3 / 2, designHeight * 3 / 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designWidth / (double) designHeight);
    setSize (designWidth, designHeight);

    startTimerHz (15);
}

InsideOutEditor::~InsideOutEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void InsideOutEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::bg);
}

void InsideOutEditor::resized()
{
    const float scale = (float) getWidth() / (float) designWidth;
    content.setTransform (juce::AffineTransform::scale (scale));
}

void InsideOutEditor::timerCallback()
{
    content.refreshStatus();
}
