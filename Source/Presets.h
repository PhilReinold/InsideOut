#pragma once

#include "Parameters.h"
#include <vector>

namespace insideout
{

struct Preset
{
    const char* category;
    const char* name;
    int   mode;        // index into modeNames()
    float amount;      // %
    float attack;      // ms
    float release;     // ms
    float scLowCut;    // Hz (20 = off)
    float scHighCut;   // Hz (20000 = off)
    float lowProtect;  // Hz (20 = off)
    float highProtect; // Hz (20000 = off)
    int   stereo;      // 0 Stereo, 1 Mid, 2 Side
    float dryWet = 100.0f;
    int   bands  = 1;  // 0 = 16, 1 = 32, 2 = 64
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets {
        // category              name                       mode  amt  att   rel   scLC   scHC     lowP   highP   stereo
        { "Mix Bus",             "Gentle Mix Bus",            0,  50, 10.0f, 320.0f, 120.0f, 12000.0f, 150.0f, 10000.0f, 1 },
        { "Mix Bus",             "Hard Carve",                0, 100,  5.0f, 200.0f, 120.0f, 12000.0f, 150.0f, 10000.0f, 1 },
        { "Vocals vs. Gitarren", "Vocal Room (Akustik)",      1,  55, 10.0f, 250.0f, 150.0f, 12000.0f, 100.0f, 20000.0f, 0 },
        { "Vocals vs. Gitarren", "Vocal Room (E-Gitarre)",    1,  65,  8.0f, 200.0f, 150.0f, 12000.0f, 120.0f, 20000.0f, 0 },
        { "Vocals vs. Gitarren", "Folk Space",                1,  45, 15.0f, 350.0f, 120.0f, 10000.0f, 100.0f, 16000.0f, 1 },
        { "Vocals vs. Keys",     "Piano Gap",                 2,  50, 15.0f, 400.0f, 120.0f, 12000.0f,  80.0f, 16000.0f, 0 },
        { "Vocals vs. Keys",     "Pad Carve",                 3,  60, 20.0f, 300.0f, 150.0f, 12000.0f, 100.0f, 20000.0f, 0 },
        { "Vocals vs. Keys",     "Synth Make-Way",            3,  70,  5.0f, 180.0f, 150.0f, 14000.0f,  80.0f, 20000.0f, 0 },
        { "Low End",             "Kick vs. Bass",             4,  60,  1.0f, 120.0f,  20.0f,  2000.0f,  20.0f,  5000.0f, 0 },
        { "Low End",             "Sub Duck",                  4,  80,  2.0f, 150.0f,  20.0f,  2000.0f,  20.0f,  5000.0f, 1 },
        { "Creative",            "Breathing Room",            4,  40, 30.0f, 800.0f, 150.0f, 12000.0f,  80.0f, 16000.0f, 0 },
        { "Creative",            "Side Only Space",           4,  70, 10.0f, 300.0f, 150.0f, 12000.0f, 100.0f, 20000.0f, 2 },
    };
    return presets;
}

/** Values a mode sets when it is picked in the UI. */
struct ModeDefaults
{
    float lowProtect;
    float highProtect;
    int stereo;
};

inline ModeDefaults modeDefaults (int mode)
{
    switch (mode)
    {
        case 0:  return { 150.0f, 10000.0f, 1 }; // Mix Bus
        case 1:  return { 100.0f, 20000.0f, 0 }; // Guitar
        case 2:  return {  80.0f, 16000.0f, 0 }; // Piano
        case 3:  return {  80.0f, 20000.0f, 0 }; // Synth
        default: return {  20.0f, 20000.0f, 0 }; // Universal
    }
}

} // namespace insideout
