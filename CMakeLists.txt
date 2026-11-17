cmake_minimum_required(VERSION 3.22)

project(SpotTwo VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)

# JUCE is fetched at configure time. Pinning the release keeps builds reproducible.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SpotTwo
    COMPANY_NAME "Open Audio Projects"
    BUNDLE_ID "audio.openaudio.spottwo"
    PLUGIN_MANUFACTURER_CODE OaPr
    PLUGIN_CODE SpT2
    PRODUCT_NAME "SpotTwo"
    DESCRIPTION "Monophonic vocal pitch correction"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3
)

juce_generate_juce_header(SpotTwo)

target_sources(SpotTwo
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_features(SpotTwo PRIVATE cxx_std_17)

target_compile_definitions(SpotTwo
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SpotTwo
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (MSVC)
    target_compile_options(SpotTwo PRIVATE /W4 /permissive-)
else()
    target_compile_options(SpotTwo PRIVATE -Wall -Wextra -Wpedantic)
endif()
