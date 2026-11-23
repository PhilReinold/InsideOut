// Renders the InsideOut editor with simulated audio to a PNG.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <random>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String outPath = argc > 1 ? argv[1] : "snapshot.png";
    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 1.0f;

    InsideOutProcessor proc;
    const double sr = 48000.0;
    const int block = 512;
    proc.prepareToPlay (sr, block);

    juce::AudioBuffer<float> buf (4, block);
    juce::MidiBuffer midi;
    std::mt19937 rng (3);
    std::normal_distribution<float> nd (0.0f, 1.0f);
    float lp1 = 0, lp2 = 0;
    double phase = 0;

    for (int b = 0; b < 200; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            // band-limited noise for the instrument bus
            const float n = nd (rng);
            lp1 += 0.08f * (n - lp1);
            lp2 += 0.5f * (n - lp2);
            const float bus = 0.25f * lp1 + 0.05f * lp2;
            buf.setSample (0, i, bus);
            buf.setSample (1, i, bus * 0.9f);

            // vowel-ish vocal: harmonics of 196 Hz with formant peaks
            phase += 196.0 / sr;
            float v = 0.0f;
            for (int h = 1; h < 40; ++h)
            {
                const double f = 196.0 * h;
                const double env = std::exp (-std::pow ((f - 700.0) / 180.0, 2)) + 0.7 * std::exp (-std::pow ((f - 1200.0) / 250.0, 2))
                                 + 0.35 * std::exp (-std::pow ((f - 2700.0) / 400.0, 2)) + 0.05;
                v += (float) (env * std::sin (2.0 * juce::MathConstants<double>::pi * phase * h)) / (float) h * 2.0f;
            }
            buf.setSample (2, i, 0.12f * v);
            buf.setSample (3, i, 0.12f * v);
        }
        proc.processBlock (buf, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize ((int) (InsideOutEditor::designWidth * scale), (int) (InsideOutEditor::designHeight * scale));
    juce::MessageManager::getInstance()->runDispatchLoopUntil (700);

    auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (outPath));
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    png.writeImageToStream (img, os);
    std::printf ("wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
