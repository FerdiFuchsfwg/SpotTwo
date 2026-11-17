#include "PluginEditor.h"
#include <cmath>
#include <utility>

namespace
{
    constexpr std::array<const char*, 12> noteLabels {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    struct ThemePreset
    {
        const char* name;
        juce::Colour primary;
        juce::Colour secondary;
        juce::Colour keyboard;
        juce::Colour meter;
    };

    const std::array<ThemePreset, 10> themePresets {{
        { "Lagoon",     juce::Colour (0xff53d6c7), juce::Colour (0xff73a6ff), juce::Colour (0xff4de0ad), juce::Colour (0xff9cebe2) },
        { "Ultraviolet",juce::Colour (0xffb18cff), juce::Colour (0xff6d8cff), juce::Colour (0xffd29aff), juce::Colour (0xffa5b5ff) },
        { "Ember",      juce::Colour (0xffff966b), juce::Colour (0xffffc46b), juce::Colour (0xffff765e), juce::Colour (0xffffba87) },
        { "Arctic",     juce::Colour (0xff79c9ff), juce::Colour (0xffa1e4ff), juce::Colour (0xff77baff), juce::Colour (0xffb2ebff) },
        { "Orchid",     juce::Colour (0xffe18acb), juce::Colour (0xffa58bff), juce::Colour (0xffe78fbd), juce::Colour (0xffd9a6f0) },
        { "Limewire",   juce::Colour (0xffb7e36e), juce::Colour (0xff70d6aa), juce::Colour (0xffa7df52), juce::Colour (0xffd1ef91) },
        { "Solar",      juce::Colour (0xffffcc68), juce::Colour (0xffff9a75), juce::Colour (0xffffc14f), juce::Colour (0xffffdf8a) },
        { "Cobalt",     juce::Colour (0xff668bff), juce::Colour (0xff64c6fa), juce::Colour (0xff5477f5), juce::Colour (0xff92b1ff) },
        { "Rosewood",   juce::Colour (0xffff7f9d), juce::Colour (0xffffb1c2), juce::Colour (0xfff46b98), juce::Colour (0xffffa6b9) },
        { "Mono",       juce::Colour (0xffd3d8e2), juce::Colour (0xff929cab), juce::Colour (0xffc0c7d3), juce::Colour (0xffe4e8ef) }
    }};

    juce::Colour backgroundColour() { return juce::Colour (0xff11151b); }
    juce::Colour panelColour()      { return juce::Colour (0xff1a2029); }
    juce::Colour mutedColour()      { return juce::Colour (0xff7f8b9b); }

    juce::String colourPropertyName (int index)
    {
        static const std::array<const char*, 4> propertyNames {
            "spotTwoPrimary", "spotTwoSecondary", "spotTwoKeyboard", "spotTwoMeter"
        };
        return propertyNames[static_cast<size_t> (juce::jlimit (0, 3, index))];
    }

}

//==============================================================================
SpotTwoAudioProcessorEditor::SpotTwoAudioProcessorEditor (SpotTwoAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      pitchScope (p),
      noteKeyboard (p)
{
    setSize (760, 520);
    setResizable (false, false);

    loadPaletteFromState();

    amountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amountSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
    amountSlider.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (amountSlider);

    speedSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    speedSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    speedSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
    speedSlider.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (speedSlider);

    amountLabel.setText ("AMOUNT", juce::dontSendNotification);
    amountLabel.setJustificationType (juce::Justification::centred);
    amountLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    amountLabel.setColour (juce::Label::textColourId, juce::Colour (0xffdfe6ef));
    addAndMakeVisible (amountLabel);

    speedLabel.setText ("SPEED", juce::dontSendNotification);
    speedLabel.setJustificationType (juce::Justification::centred);
    speedLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    speedLabel.setColour (juce::Label::textColourId, juce::Colour (0xffdfe6ef));
    addAndMakeVisible (speedLabel);

    amountValue.setJustificationType (juce::Justification::centred);
    amountValue.setFont (juce::Font (juce::FontOptions (12.0f)));
    amountValue.setColour (juce::Label::textColourId, juce::Colour (0xffaab5c3));
    addAndMakeVisible (amountValue);

    speedValue.setJustificationType (juce::Justification::centred);
    speedValue.setFont (juce::Font (juce::FontOptions (12.0f)));
    speedValue.setColour (juce::Label::textColourId, juce::Colour (0xffaab5c3));
    addAndMakeVisible (speedValue);

    bypassButton.setButtonText ("BYPASS");
    bypassButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffc3ccd7));
    addAndMakeVisible (bypassButton);

    settingsButton.onClick = [this] { showSettings (true); };
    settingsButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe8edf4));
    addAndMakeVisible (settingsButton);

    backButton.onClick = [this] { showSettings (false); };
    backButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe8edf4));
    addAndMakeVisible (backButton);

    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "amount", amountSlider);
    speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "speed", speedSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "bypass", bypassButton);

    rootCombo.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    rootCombo.setSelectedId (1, juce::dontSendNotification);
    rootCombo.onChange = [this] { applyScaleFromControls(); };
    addAndMakeVisible (rootCombo);

    scaleCombo.addItemList (juce::StringArray { "Chromatic", "Major", "Natural Minor", "Dorian", "Major Pentatonic",
                                               "Minor Pentatonic", "Blues", "Custom" }, 1);
    scaleCombo.setSelectedId (1, juce::dontSendNotification);
    scaleCombo.onChange = [this] { applyScaleFromControls(); };
    addAndMakeVisible (scaleCombo);

    themeCombo.addItemList (juce::StringArray { "Lagoon", "Ultraviolet", "Ember", "Arctic", "Orchid", "Limewire",
                                               "Solar", "Cobalt", "Rosewood", "Mono", "Custom" }, 1);
    themeCombo.onChange = [this]
    {
        if (themeCombo.getSelectedId() >= 1 && themeCombo.getSelectedId() <= 10)
            applyPresetTheme (themeCombo.getSelectedId() - 1, true);
    };
    addAndMakeVisible (themeCombo);

    for (int i = 0; i < static_cast<int> (colourButtons.size()); ++i)
    {
        auto& button = colourButtons[static_cast<size_t> (i)];
        button.setButtonText (colourNames[static_cast<size_t> (i)]);
        button.onClick = [this, i] { openColourPicker (i); };
        addAndMakeVisible (button);
    }

    pitchScope.setPalette (palette);
    noteKeyboard.setPalette (palette);
    noteKeyboard.onManualNoteChange = [this]
    {
        applyingScale = true;
        scaleCombo.setSelectedId (8, juce::dontSendNotification);
        applyingScale = false;
    };
    addAndMakeVisible (pitchScope);
    addAndMakeVisible (noteKeyboard);

    const int themeIndex = static_cast<int> (processor.apvts.state.getProperty ("spotTwoThemeIndex", 0));
    themeCombo.setSelectedId (juce::jlimit (0, 10, themeIndex) + 1, juce::dontSendNotification);

    applyPalette();
    updateValueLabels();
    showSettings (false);
    startTimerHz (30);
}

SpotTwoAudioProcessorEditor::~SpotTwoAudioProcessorEditor()
{
    stopTimer();
    amountSlider.setLookAndFeel (nullptr);
    speedSlider.setLookAndFeel (nullptr);
}

void SpotTwoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour());

    auto bounds = getLocalBounds().toFloat();
    g.setColour (panelColour());
    g.fillRoundedRectangle (bounds.reduced (12.0f), 18.0f);

    g.setColour (juce::Colour (0xff2a3441));
    g.drawRoundedRectangle (bounds.reduced (12.5f), 18.0f, 1.0f);

    if (! settingsVisible)
    {
        g.setColour (juce::Colour (0xfff3f6fa));
        g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
        g.drawText ("SPOTTWO", 34, 20, 220, 34, juce::Justification::centredLeft);

        g.setColour (mutedColour());
        g.setFont (juce::Font (juce::FontOptions (10.5f)));
        g.drawText ("VOCAL PITCH CORRECTION", 36, 51, 240, 16, juce::Justification::centredLeft);

        g.setColour (palette.primary.withAlpha (0.7f));
        g.fillRoundedRectangle (34.0f, 76.0f, 692.0f, 1.0f, 0.5f);

        g.setColour (mutedColour());
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText ("TARGET NOTES", 34, 264, 115, 18, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (juce::Colour (0xfff3f6fa));
        g.setFont (juce::Font (juce::FontOptions (25.0f, juce::Font::bold)));
        g.drawText ("COLOUR STUDIO", 34, 23, 360, 34, juce::Justification::centredLeft);

        g.setColour (mutedColour());
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText ("Choose a preset, then tune every accent independently.", 36, 57, 520, 18,
                    juce::Justification::centredLeft);

        g.setColour (palette.primary.withAlpha (0.75f));
        g.fillRoundedRectangle (34.0f, 88.0f, 692.0f, 1.0f, 0.5f);
    }
}

void SpotTwoAudioProcessorEditor::resized()
{
    if (! settingsVisible)
    {
        amountLabel.setBounds (154, 92, 160, 22);
        amountSlider.setBounds (148, 112, 172, 142);
        amountValue.setBounds (164, 240, 140, 20);

        speedLabel.setBounds (446, 92, 160, 22);
        speedSlider.setBounds (440, 112, 172, 142);
        speedValue.setBounds (456, 240, 140, 20);

        bypassButton.setBounds (546, 29, 92, 24);
        settingsButton.setBounds (647, 24, 82, 30);
        backButton.setBounds (647, 24, 82, 30);

        rootCombo.setBounds (34, 282, 122, 30);
        scaleCombo.setBounds (166, 282, 176, 30);
        pitchScope.setBounds (34, 323, 692, 66);
        noteKeyboard.setBounds (34, 402, 692, 82);

        themeCombo.setBounds (0, 0, 0, 0);
        for (auto& button : colourButtons)
            button.setBounds (0, 0, 0, 0);
    }
    else
    {
        bypassButton.setBounds (0, 0, 0, 0);
        amountSlider.setBounds (0, 0, 0, 0);
        speedSlider.setBounds (0, 0, 0, 0);
        amountLabel.setBounds (0, 0, 0, 0);
        speedLabel.setBounds (0, 0, 0, 0);
        amountValue.setBounds (0, 0, 0, 0);
        speedValue.setBounds (0, 0, 0, 0);
        rootCombo.setBounds (0, 0, 0, 0);
        scaleCombo.setBounds (0, 0, 0, 0);
        pitchScope.setBounds (0, 0, 0, 0);
        noteKeyboard.setBounds (0, 0, 0, 0);

        settingsButton.setBounds (0, 0, 0, 0);
        backButton.setBounds (647, 24, 82, 30);
        themeCombo.setBounds (40, 139, 250, 34);

        const int swatchWidth = 145;
        const int swatchHeight = 48;
        const int gap = 22;
        const int startX = 40;
        const int startY = 222;
        for (int i = 0; i < static_cast<int> (colourButtons.size()); ++i)
        {
            const int row = i / 2;
            const int column = i % 2;
            colourButtons[static_cast<size_t> (i)].setBounds (startX + column * (swatchWidth + gap),
                                                              startY + row * (swatchHeight + 18),
                                                              swatchWidth, swatchHeight);
        }
    }
}

void SpotTwoAudioProcessorEditor::timerCallback()
{
    updateValueLabels();
    pitchScope.repaint();
    noteKeyboard.repaint();
}

void SpotTwoAudioProcessorEditor::updateValueLabels()
{
    amountValue.setText (juce::String (static_cast<int> (std::round (amountSlider.getValue()))) + "%",
                         juce::dontSendNotification);
    speedValue.setText (juce::String (static_cast<int> (std::round (speedSlider.getValue()))) + "%",
                        juce::dontSendNotification);
}

void SpotTwoAudioProcessorEditor::showSettings (bool shouldShow)
{
    settingsVisible = shouldShow;
    settingsButton.setVisible (! settingsVisible);
    backButton.setVisible (settingsVisible);

    amountSlider.setVisible (! settingsVisible);
    speedSlider.setVisible (! settingsVisible);
    amountLabel.setVisible (! settingsVisible);
    speedLabel.setVisible (! settingsVisible);
    amountValue.setVisible (! settingsVisible);
    speedValue.setVisible (! settingsVisible);
    bypassButton.setVisible (! settingsVisible);
    rootCombo.setVisible (! settingsVisible);
    scaleCombo.setVisible (! settingsVisible);
    pitchScope.setVisible (! settingsVisible);
    noteKeyboard.setVisible (! settingsVisible);

    themeCombo.setVisible (settingsVisible);
    for (auto& button : colourButtons)
        button.setVisible (settingsVisible);

    resized();
    repaint();
}

void SpotTwoAudioProcessorEditor::applyScaleFromControls()
{
    if (applyingScale || scaleCombo.getSelectedId() == 8)
        return;

    const int root = juce::jlimit (0, 11, rootCombo.getSelectedId() - 1);
    const int mode = scaleCombo.getSelectedId();

    static const std::array<std::array<bool, 12>, 7> scalePatterns {{
        { true, true, true, true, true, true, true, true, true, true, true, true },
        { true, false, true, false, true, true, false, true, false, true, false, true },
        { true, false, true, true, false, true, false, true, true, false, true, false },
        { true, false, true, true, false, true, false, true, false, true, true, false },
        { true, false, true, false, true, false, false, true, false, true, false, false },
        { true, false, false, true, false, true, false, true, false, false, true, false },
        { true, false, false, true, false, true, true, true, false, false, true, false }
    }};

    if (mode < 1 || mode > 7)
        return;

    applyingScale = true;
    const auto& pattern = scalePatterns[static_cast<size_t> (mode - 1)];
    for (int note = 0; note < 12; ++note)
        processor.setNoteEnabled (note, pattern[static_cast<size_t> ((note - root + 12) % 12)]);
    applyingScale = false;
    noteKeyboard.repaint();
}

void SpotTwoAudioProcessorEditor::applyPresetTheme (int themeIndex, bool persist)
{
    const int index = juce::jlimit (0, static_cast<int> (themePresets.size()) - 1, themeIndex);
    const auto& preset = themePresets[static_cast<size_t> (index)];
    palette = { preset.primary, preset.secondary, preset.keyboard, preset.meter };

    if (persist)
    {
        processor.apvts.state.setProperty ("spotTwoThemeIndex", index, nullptr);
        persistPaletteToState();
    }

    applyPalette();
    themeCombo.setSelectedId (index + 1, juce::dontSendNotification);
}

void SpotTwoAudioProcessorEditor::applyPalette()
{
    lookAndFeel.setPalette (palette);
    pitchScope.setPalette (palette);
    noteKeyboard.setPalette (palette);
    amountSlider.repaint();
    speedSlider.repaint();

    for (int i = 0; i < static_cast<int> (colourButtons.size()); ++i)
    {
        auto& button = colourButtons[static_cast<size_t> (i)];
        const std::array<juce::Colour, 4> colours { palette.primary, palette.secondary, palette.keyboard, palette.meter };
        button.setColour (juce::TextButton::buttonColourId, colours[static_cast<size_t> (i)]);
        button.setColour (juce::TextButton::buttonOnColourId, colours[static_cast<size_t> (i)].brighter (0.08f));
        button.setColour (juce::TextButton::textColourOffId,
                          colours[static_cast<size_t> (i)].getPerceivedBrightness() > 0.57f
                              ? juce::Colour (0xff11151b) : juce::Colour (0xfff5f7fa));
    }

    repaint();
}

void SpotTwoAudioProcessorEditor::loadPaletteFromState()
{
    const auto& state = processor.apvts.state;
    const int savedTheme = static_cast<int> (state.getProperty ("spotTwoThemeIndex", 0));
    const int presetIndex = juce::jlimit (0, 9, savedTheme);
    const auto& preset = themePresets[static_cast<size_t> (presetIndex)];
    palette = { preset.primary, preset.secondary, preset.keyboard, preset.meter };

    std::array<juce::Colour*, 4> destinations { &palette.primary, &palette.secondary, &palette.keyboard, &palette.meter };
    for (int i = 0; i < 4; ++i)
    {
        const auto property = state.getProperty (colourPropertyName (i), destinations[static_cast<size_t> (i)]->toString()).toString();
        *destinations[static_cast<size_t> (i)] = juce::Colour::fromString (property);
    }
}

void SpotTwoAudioProcessorEditor::persistPaletteToState()
{
    processor.apvts.state.setProperty (colourPropertyName (0), palette.primary.toString(), nullptr);
    processor.apvts.state.setProperty (colourPropertyName (1), palette.secondary.toString(), nullptr);
    processor.apvts.state.setProperty (colourPropertyName (2), palette.keyboard.toString(), nullptr);
    processor.apvts.state.setProperty (colourPropertyName (3), palette.meter.toString(), nullptr);
}

void SpotTwoAudioProcessorEditor::openColourPicker (int colourIndex)
{
    const std::array<juce::Colour, 4> colours { palette.primary, palette.secondary, palette.keyboard, palette.meter };
    auto content = std::make_unique<ColourPickerContent> (colours[static_cast<size_t> (colourIndex)],
        [this, colourIndex] (juce::Colour chosen)
        {
            switch (colourIndex)
            {
                case 0: palette.primary = chosen; break;
                case 1: palette.secondary = chosen; break;
                case 2: palette.keyboard = chosen; break;
                case 3: palette.meter = chosen; break;
                default: break;
            }

            processor.apvts.state.setProperty ("spotTwoThemeIndex", 10, nullptr);
            persistPaletteToState();
            themeCombo.setSelectedId (11, juce::dontSendNotification);
            applyPalette();
        });

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                            colourButtons[static_cast<size_t> (colourIndex)].getScreenBounds(),
                                            this);
}

SpotTwoAudioProcessorEditor::ColourPickerContent::ColourPickerContent(
    juce::Colour initialColour,
    std::function<void(juce::Colour)> callback)
    : selector (
          juce::ColourSelector::showColourAtTop
        | juce::ColourSelector::showSliders
        | juce::ColourSelector::showColourspace),
      onColourChanged (std::move(callback))
{
    selector.setCurrentColour(initialColour, juce::dontSendNotification);

    addAndMakeVisible(selector);
    selector.addChangeListener(this);

    setSize(310, 360);
}

void SpotTwoAudioProcessorEditor::ColourPickerContent::resized()
{
    selector.setBounds (getLocalBounds().reduced (6));
}

void SpotTwoAudioProcessorEditor::ColourPickerContent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (onColourChanged != nullptr)
        onColourChanged (selector.getCurrentColour());
}

//==============================================================================
void SpotTwoAudioProcessorEditor::SpotTwoLookAndFeel::drawRotarySlider (
    juce::Graphics& g, int x, int y, int width, int height, float sliderPosition,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider&)
{
    const float diameter = static_cast<float> (juce::jmin (width, height)) * 0.82f;
    const float radius = diameter * 0.5f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.48f;
    const auto dial = juce::Rectangle<float> (centreX - radius, centreY - radius, diameter, diameter);

    g.setColour (juce::Colour (0xff10151c));
    g.fillEllipse (dial.expanded (4.0f));
    g.setColour (juce::Colour (0xff303b49));
    g.drawEllipse (dial.expanded (4.0f), 1.0f);
    g.setColour (juce::Colour (0xff26303b));
    g.fillEllipse (dial);

    juce::Path track;
    track.addCentredArc (centreX, centreY, radius - 8.0f, radius - 8.0f, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff414c5b));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float angle = rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc (centreX, centreY, radius - 8.0f, radius - 8.0f, 0.0f,
                            rotaryStartAngle, angle, true);
    g.setColour (palette.primary);
    g.strokePath (valueArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float pointerLength = radius * 0.55f;
    const juce::Point<float> pointerEnd (centreX + std::sin (angle) * pointerLength,
                                         centreY - std::cos (angle) * pointerLength);
    g.setColour (juce::Colour (0xfff3f6fa));
    g.drawLine (centreX, centreY, pointerEnd.x, pointerEnd.y, 3.0f);
    g.setColour (palette.secondary);
    g.fillEllipse (centreX - 5.0f, centreY - 5.0f, 10.0f, 10.0f);
}

void SpotTwoAudioProcessorEditor::PitchScope::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff131922));
    g.fillRoundedRectangle (area, 10.0f);
    g.setColour (juce::Colour (0xff2b3644));
    g.drawRoundedRectangle (area.reduced (0.5f), 10.0f, 1.0f);

    g.setColour (mutedColour());
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText ("LIVE PITCH", 13, 7, 100, 14, juce::Justification::centredLeft);

    const float left = 16.0f;
    const float top = 29.0f;
    const float width = getWidth() - 32.0f;
    const float cellWidth = width / 12.0f;

    for (int note = 0; note < 12; ++note)
    {
        const float x = left + static_cast<float> (note) * cellWidth;
        const bool enabled = processor.isNoteEnabled (note);
        const bool blackKey = note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
        g.setColour (enabled ? palette.meter.withAlpha (0.16f) : juce::Colour (0xff202833));
        g.fillRoundedRectangle (x + 1.0f, top, cellWidth - 2.0f, 23.0f, 4.0f);
        g.setColour (enabled ? (blackKey ? palette.secondary : palette.meter) : mutedColour().withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (10.0f, enabled ? juce::Font::bold : juce::Font::plain)));
        g.drawText (noteLabels[static_cast<size_t> (note)],
                    juce::Rectangle<float> (x, top + 1.0f, cellWidth, 21.0f).toNearestInt(),
                    juce::Justification::centred);
    }

    const float midi = processor.getCurrentPitchMidi();
    if (midi >= 0.0f)
    {
        float pitchClass = std::fmod (midi, 12.0f);
        if (pitchClass < 0.0f)
            pitchClass += 12.0f;
        const float markerX = left + (pitchClass / 12.0f) * width;
        g.setColour (palette.primary);
        g.fillRoundedRectangle (markerX - 1.5f, top - 4.0f, 3.0f, 31.0f, 1.5f);

        const int roundedNote = (static_cast<int> (std::lround (midi)) % 12 + 12) % 12;
        g.setColour (palette.primary);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (juce::String (noteLabels[static_cast<size_t> (roundedNote)]) + "  "
                    + juce::String (static_cast<int> (std::round (processor.getCurrentFrequencyHz()))) + " Hz",
                    getWidth() - 118, 6, 106, 15, juce::Justification::centredRight);
    }
    else
    {
        g.setColour (mutedColour());
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        g.drawText ("WAITING FOR VOICE", getWidth() - 145, 6, 132, 15, juce::Justification::centredRight);
    }
}

void SpotTwoAudioProcessorEditor::NoteKeyboard::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff131922));
    g.fillRoundedRectangle (area, 10.0f);
    g.setColour (juce::Colour (0xff2b3644));
    g.drawRoundedRectangle (area.reduced (0.5f), 10.0f, 1.0f);

    const float keyTop = 8.0f;
    const float keyHeight = static_cast<float> (getHeight()) - 16.0f;
    const float whiteWidth = static_cast<float> (getWidth()) / 7.0f;
    const std::array<int, 7> whiteNotes { 0, 2, 4, 5, 7, 9, 11 };

    for (int white = 0; white < 7; ++white)
    {
        const float x = static_cast<float> (white) * whiteWidth;
        const int note = whiteNotes[static_cast<size_t> (white)];
        const bool enabled = processor.isNoteEnabled (note);
        auto key = juce::Rectangle<float> (x + 1.0f, keyTop, whiteWidth - 2.0f, keyHeight);
        g.setColour (enabled ? palette.keyboard.withAlpha (0.82f) : juce::Colour (0xffd9dee6));
        g.fillRoundedRectangle (key, 4.0f);
        g.setColour (enabled ? palette.keyboard.brighter (0.15f) : juce::Colour (0xff7e8998));
        g.drawRoundedRectangle (key, 4.0f, 1.0f);
        g.setColour (enabled ? juce::Colour (0xff10151b) : juce::Colour (0xff526071));
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (noteLabels[static_cast<size_t> (note)],
                    juce::Rectangle<float> (x, keyTop + keyHeight - 18.0f, whiteWidth, 14.0f).toNearestInt(),
                    juce::Justification::centred);
    }

    const std::array<int, 5> blackNotes { 1, 3, 6, 8, 10 };
    const std::array<int, 5> precedingWhiteIndices { 0, 1, 3, 4, 5 };
    const float blackWidth = whiteWidth * 0.58f;
    const float blackHeight = keyHeight * 0.62f;

    for (size_t i = 0; i < blackNotes.size(); ++i)
    {
        const float x = whiteWidth * static_cast<float> (precedingWhiteIndices[i] + 1) - blackWidth * 0.5f;
        const bool enabled = processor.isNoteEnabled (blackNotes[i]);
        auto key = juce::Rectangle<float> (x, keyTop, blackWidth, blackHeight);
        g.setColour (enabled ? palette.primary : juce::Colour (0xff252e39));
        g.fillRoundedRectangle (key, 3.0f);
        g.setColour (juce::Colour (0xff0b0f14).withAlpha (0.65f));
        g.drawRoundedRectangle (key, 3.0f, 1.0f);
    }
}

int SpotTwoAudioProcessorEditor::NoteKeyboard::noteAtPosition (juce::Point<float> position) const noexcept
{
    const float whiteWidth = static_cast<float> (getWidth()) / 7.0f;
    const float keyTop = 8.0f;
    const float keyHeight = static_cast<float> (getHeight()) - 16.0f;
    const float blackWidth = whiteWidth * 0.58f;
    const float blackHeight = keyHeight * 0.62f;
    const std::array<int, 5> blackNotes { 1, 3, 6, 8, 10 };
    const std::array<int, 5> precedingWhiteIndices { 0, 1, 3, 4, 5 };

    if (position.y >= keyTop && position.y <= keyTop + blackHeight)
    {
        for (size_t i = 0; i < blackNotes.size(); ++i)
        {
            const float x = whiteWidth * static_cast<float> (precedingWhiteIndices[i] + 1) - blackWidth * 0.5f;
            if (position.x >= x && position.x <= x + blackWidth)
                return blackNotes[i];
        }
    }

    if (position.y < keyTop || position.y > keyTop + keyHeight)
        return -1;

    const int whiteIndex = juce::jlimit (0, 6, static_cast<int> (position.x / whiteWidth));
    static constexpr std::array<int, 7> whiteNotes { 0, 2, 4, 5, 7, 9, 11 };
    return whiteNotes[static_cast<size_t> (whiteIndex)];
}

void SpotTwoAudioProcessorEditor::NoteKeyboard::mouseDown (const juce::MouseEvent& event)
{
    const int note = noteAtPosition (event.position);
    if (note < 0)
        return;

    processor.setNoteEnabled (note, ! processor.isNoteEnabled (note));
    if (onManualNoteChange != nullptr)
        onManualNoteChange();
    repaint();
}
