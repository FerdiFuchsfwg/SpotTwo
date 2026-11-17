#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>

class SpotTwoAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit SpotTwoAudioProcessorEditor (SpotTwoAudioProcessor&);
    ~SpotTwoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Palette
    {
        juce::Colour primary;
        juce::Colour secondary;
        juce::Colour keyboard;
        juce::Colour meter;
    };

    class SpotTwoLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void setPalette (const Palette& newPalette) { palette = newPalette; }
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPosition, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider&) override;

    private:
        Palette palette;
    };

    class PitchScope final : public juce::Component
    {
    public:
        explicit PitchScope (SpotTwoAudioProcessor& p) : processor (p) {}
        void setPalette (const Palette& newPalette) { palette = newPalette; repaint(); }
        void paint (juce::Graphics&) override;

    private:
        SpotTwoAudioProcessor& processor;
        Palette palette;
    };

    class NoteKeyboard final : public juce::Component
    {
    public:
        explicit NoteKeyboard (SpotTwoAudioProcessor& p) : processor (p) {}
        void setPalette (const Palette& newPalette) { palette = newPalette; repaint(); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        std::function<void()> onManualNoteChange;

    private:
        int noteAtPosition (juce::Point<float>) const noexcept;
        SpotTwoAudioProcessor& processor;
        Palette palette;
    };

    class ColourPickerContent final : public juce::Component,
                                      private juce::ChangeListener
    {
    public:
        ColourPickerContent (juce::Colour initialColour, std::function<void (juce::Colour)> callback);
        void resized() override;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override;
        juce::ColourSelector selector;
        std::function<void (juce::Colour)> onColourChanged;
    };

    void timerCallback() override;
    void showSettings (bool shouldShow);
    void applyScaleFromControls();
    void applyPresetTheme (int themeIndex, bool persist);
    void applyPalette();
    void loadPaletteFromState();
    void persistPaletteToState();
    void openColourPicker (int colourIndex);
    void updateValueLabels();

    SpotTwoAudioProcessor& processor;
    SpotTwoLookAndFeel lookAndFeel;
    Palette palette;

    juce::Slider amountSlider;
    juce::Slider speedSlider;
    juce::Label amountLabel;
    juce::Label speedLabel;
    juce::Label amountValue;
    juce::Label speedValue;
    juce::ToggleButton bypassButton;
    juce::TextButton settingsButton { "SETTINGS" };
    juce::TextButton backButton { "BACK" };

    juce::ComboBox rootCombo;
    juce::ComboBox scaleCombo;
    juce::ComboBox themeCombo;
    std::array<juce::TextButton, 4> colourButtons;
    std::array<juce::String, 4> colourNames { "Primary", "Secondary", "Keyboard", "Meter" };

    PitchScope pitchScope;
    NoteKeyboard noteKeyboard;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    bool settingsVisible = false;
    bool applyingScale = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpotTwoAudioProcessorEditor)
};
