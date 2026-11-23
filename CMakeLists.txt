cmake_minimum_required(VERSION 3.22)

project(InsideOut VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# AU only exists on macOS
if(APPLE)
    set(INSIDEOUT_FORMATS AU VST3 Standalone)
else()
    set(INSIDEOUT_FORMATS VST3 Standalone)
endif()

juce_add_plugin(InsideOut
    COMPANY_NAME                "Philip Jin Plugins"
    BUNDLE_ID                   "com.philipjin.insideout"
    PLUGIN_MANUFACTURER_CODE    PhJn
    PLUGIN_CODE                 InsO
    FORMATS                     ${INSIDEOUT_FORMATS}
    PRODUCT_NAME                "InsideOut"
    DESCRIPTION                 "Dynamischer Sidechain-EQ, der Platz fuer Vocals macht"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE                kAudioUnitType_Effect
    VST3_CATEGORIES             Fx EQ Dynamics
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

juce_add_binary_data(InsideOutAssets
    SOURCES
        Resources/Fonts/UnifrakturCook-Bold.ttf
        Resources/Fonts/DMSans-Regular.ttf
        Resources/Fonts/DMSans-Medium.ttf
        Resources/Fonts/DMSans-Bold.ttf
        Resources/Fonts/DMMono-Regular.ttf
        Resources/Fonts/DMMono-Medium.ttf
)

target_sources(InsideOut PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SpectralDucker.cpp
)

target_include_directories(InsideOut PRIVATE Source)

target_compile_definitions(InsideOut PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(InsideOut
    PRIVATE
        InsideOutAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Offline DSP test (no GUI), run with: ctest or ./InsideOutDspTest
option(INSIDEOUT_BUILD_TESTS "Build the DSP test" OFF)
if(INSIDEOUT_BUILD_TESTS)
    juce_add_console_app(InsideOutDspTest PRODUCT_NAME "InsideOutDspTest")
    target_sources(InsideOutDspTest PRIVATE Tests/DspTest.cpp Source/SpectralDucker.cpp)
    target_include_directories(InsideOutDspTest PRIVATE Source)
    target_compile_definitions(InsideOutDspTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(InsideOutDspTest PRIVATE juce::juce_dsp juce::juce_audio_basics
        PUBLIC juce::juce_recommended_config_flags)
endif()

# Renders the editor to a PNG (used to check the UI), needs a display (xvfb-run on Linux)
if(INSIDEOUT_BUILD_TESTS)
    juce_add_console_app(InsideOutSnapshot PRODUCT_NAME "InsideOutSnapshot")
    target_sources(InsideOutSnapshot PRIVATE Tests/Snapshot.cpp Source/PluginProcessor.cpp
                   Source/PluginEditor.cpp Source/SpectralDucker.cpp)
    target_include_directories(InsideOutSnapshot PRIVATE Source)
    target_compile_definitions(InsideOutSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1 JucePlugin_Name="InsideOut")
    target_link_libraries(InsideOutSnapshot PRIVATE InsideOutAssets juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
