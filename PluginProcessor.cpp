#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <limits>

namespace
{
    constexpr std::array<const char*, 12> noteIds {
        "note_C", "note_Cs", "note_D", "note_Ds", "note_E", "note_F",
        "note_Fs", "note_G", "note_Gs", "note_A", "note_As", "note_B"
    };

    constexpr std::array<const char*, 12> noteNames {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
}

SpotTwoAudioProcessor::SpotTwoAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SPOTTWO_STATE", createParameterLayout())
{
    for (size_t i = 0; i < noteIds.size(); ++i)
        noteParameters[i] = apvts.getRawParameterValue (noteIds[i]);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpotTwoAudioProcessor::createParameterLayout()
{
    using FloatParameter = juce::AudioParameterFloat;
    using BoolParameter = juce::AudioParameterBool;
    using ParameterID = juce::ParameterID;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<FloatParameter> (ParameterID { "amount", 1 }, "Amount",
                                                  juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    layout.add (std::make_unique<FloatParameter> (ParameterID { "speed", 1 }, "Speed",
                                                  juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 68.0f));
    layout.add (std::make_unique<BoolParameter> (ParameterID { "bypass", 1 }, "Bypass", false));

    for (size_t i = 0; i < noteIds.size(); ++i)
        layout.add (std::make_unique<BoolParameter> (ParameterID { noteIds[i], 1 }, noteNames[i], true));

    return layout;
}

void SpotTwoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = juce::jmax (8000.0, sampleRate);
    pitchTracker.prepare (currentSampleRate);
    pitchShifter.reset();
    setLatencySamples (DualGrainPitchShifter::reportedLatencySamples);

    desiredCorrectionSemitones = 0.0f;
    smoothedCorrectionSemitones = 0.0f;
    voicedHoldSamples = 0;
    currentPitchMidi.store (-1.0f);
    currentFrequencyHz.store (0.0f);
    appliedCorrectionSemitones.store (0.0f);
}

void SpotTwoAudioProcessor::releaseResources()
{
    pitchShifter.reset();
}

bool SpotTwoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
        && output == input;
}

void SpotTwoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channelCount = juce::jmin (2, getTotalNumInputChannels(), buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();

    for (int channel = channelCount; channel < buffer.getNumChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    if (channelCount <= 0)
        return;

    const auto* amountParameter = apvts.getRawParameterValue ("amount");
    const auto* speedParameter = apvts.getRawParameterValue ("speed");
    const auto* bypassParameter = apvts.getRawParameterValue ("bypass");

    const float amount = amountParameter != nullptr ? amountParameter->load() * 0.01f : 1.0f;
    const float speed = speedParameter != nullptr ? juce::jlimit (0.0f, 1.0f, speedParameter->load() * 0.01f) : 0.68f;
    const bool bypassed = bypassParameter != nullptr && bypassParameter->load() >= 0.5f;

    // Low speed gives a relaxed correction; high speed approaches instant note snapping.
    const float smoothingSeconds = 0.008f + (1.0f - speed) * (1.0f - speed) * 0.42f;
    const float smoothingCoefficient = 1.0f - std::exp (-1.0f / (smoothingSeconds * static_cast<float> (currentSampleRate)));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float left = buffer.getSample (0, sample);
        const float right = channelCount > 1 ? buffer.getSample (1, sample) : left;
        const float monoInput = channelCount > 1 ? 0.5f * (left + right) : left;

        float detectedFrequency = 0.0f;
        if (pitchTracker.processSample (monoInput, detectedFrequency))
        {
            if (detectedFrequency > 0.0f)
            {
                const float midiNote = 69.0f + 12.0f * std::log2 (detectedFrequency / 440.0f);
                currentPitchMidi.store (midiNote, std::memory_order_relaxed);
                currentFrequencyHz.store (detectedFrequency, std::memory_order_relaxed);
                const float targetNote = findNearestAllowedNote (midiNote);
                desiredCorrectionSemitones = targetNote - midiNote;
                voicedHoldSamples = static_cast<int> (currentSampleRate * 0.10);
            }
            else
            {
                currentPitchMidi.store (-1.0f, std::memory_order_relaxed);
                currentFrequencyHz.store (0.0f, std::memory_order_relaxed);
                if (voicedHoldSamples == 0)
                    desiredCorrectionSemitones = 0.0f;
            }
        }

        if (voicedHoldSamples > 0)
            --voicedHoldSamples;
        else
            desiredCorrectionSemitones = 0.0f;

        smoothedCorrectionSemitones += smoothingCoefficient * (desiredCorrectionSemitones - smoothedCorrectionSemitones);
        const float correction = bypassed ? 0.0f : smoothedCorrectionSemitones * amount;
        appliedCorrectionSemitones.store (correction, std::memory_order_relaxed);

        const float ratio = std::exp2 (correction / 12.0f);
        float frame[2] { left, right };
        pitchShifter.processFrame (frame, channelCount, ratio);

        // Bypass keeps the delay path active at a unity ratio, so reported plug-in
        // latency remains consistent with the processed signal.
        buffer.setSample (0, sample, frame[0]);
        if (channelCount > 1)
            buffer.setSample (1, sample, frame[1]);
    }
}

float SpotTwoAudioProcessor::findNearestAllowedNote (float midiNote) const noexcept
{
    const int roundedMidi = static_cast<int> (std::lround (midiNote));
    float nearest = midiNote;
    float smallestDistance = std::numeric_limits<float>::max();
    bool foundEnabledNote = false;

    for (int candidate = roundedMidi - 12; candidate <= roundedMidi + 12; ++candidate)
    {
        const int pitchClass = (candidate % 12 + 12) % 12;
        if (! isNoteEnabled (pitchClass))
            continue;

        const float distance = std::abs (static_cast<float> (candidate) - midiNote);
        if (distance < smallestDistance)
        {
            smallestDistance = distance;
            nearest = static_cast<float> (candidate);
            foundEnabledNote = true;
        }
    }

    return foundEnabledNote ? nearest : midiNote;
}

bool SpotTwoAudioProcessor::isNoteEnabled (int note) const noexcept
{
    const int index = (note % 12 + 12) % 12;
    const auto* parameter = noteParameters[static_cast<size_t> (index)];
    return parameter != nullptr && parameter->load (std::memory_order_relaxed) >= 0.5f;
}

void SpotTwoAudioProcessor::setNoteEnabled (int note, bool enabled)
{
    const int index = (note % 12 + 12) % 12;
    if (auto* parameter = apvts.getParameter (noteIds[static_cast<size_t> (index)]))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (enabled ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}

void SpotTwoAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}

void SpotTwoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
// A compact YIN-style monophonic pitch detector. Input is averaged and downsampled
// to roughly 12 kHz before analysis to keep the per-frame CPU cost bounded.

void SpotTwoAudioProcessor::YinPitchTracker::prepare (double hostSampleRate) noexcept
{
    decimationFactor = juce::jlimit (1, 16, static_cast<int> (std::lround (hostSampleRate / 12000.0)));
    analysisSampleRate = juce::jmax (2000.0, hostSampleRate / static_cast<double> (decimationFactor));
    reset();
}

void SpotTwoAudioProcessor::YinPitchTracker::reset() noexcept
{
    history.fill (0.0f);
    difference.fill (0.0f);
    cumulativeMeanNormalizedDifference.fill (1.0f);
    writeIndex = 0;
    filledSamples = 0;
    samplesSinceAnalysis = 0;
    decimationCounter = 0;
    decimationAccumulator = 0.0f;
}

bool SpotTwoAudioProcessor::YinPitchTracker::processSample (float input, float& frequencyHz) noexcept
{
    decimationAccumulator += input;
    ++decimationCounter;

    if (decimationCounter < decimationFactor)
        return false;

    const float downsampled = decimationAccumulator / static_cast<float> (decimationFactor);
    decimationAccumulator = 0.0f;
    decimationCounter = 0;

    history[static_cast<size_t> (writeIndex)] = downsampled;
    writeIndex = (writeIndex + 1) % analysisSize;
    filledSamples = juce::jmin (analysisSize, filledSamples + 1);
    ++samplesSinceAnalysis;

    if (samplesSinceAnalysis < analysisInterval || filledSamples < analysisSize)
        return false;

    samplesSinceAnalysis = 0;
    frequencyHz = 0.0f;
    analyse (frequencyHz);
    return true;
}

bool SpotTwoAudioProcessor::YinPitchTracker::analyse (float& frequencyHz) noexcept
{
    double meanSquare = 0.0;
    std::array<float, analysisSize> ordered {};

    for (int i = 0; i < analysisSize; ++i)
    {
        const float value = history[static_cast<size_t> ((writeIndex + i) % analysisSize)];
        ordered[static_cast<size_t> (i)] = value;
        meanSquare += static_cast<double> (value) * static_cast<double> (value);
    }

    meanSquare /= static_cast<double> (analysisSize);
    if (meanSquare < 1.0e-7)
        return false;

    difference.fill (0.0f);
    cumulativeMeanNormalizedDifference.fill (1.0f);
    double runningDifference = 0.0;

    for (int lag = 1; lag <= maximumLag; ++lag)
    {
        double sum = 0.0;
        for (int i = 0; i < analysisSize - lag; ++i)
        {
            const double delta = static_cast<double> (ordered[static_cast<size_t> (i)]
                                                     - ordered[static_cast<size_t> (i + lag)]);
            sum += delta * delta;
        }

        difference[static_cast<size_t> (lag)] = static_cast<float> (sum);
        runningDifference += sum;
        cumulativeMeanNormalizedDifference[static_cast<size_t> (lag)] =
            static_cast<float> (runningDifference > 0.0 ? sum * static_cast<double> (lag) / runningDifference : 1.0);
    }

    int bestLag = -1;
    constexpr float threshold = 0.18f;

    for (int lag = minimumLag; lag < maximumLag; ++lag)
    {
        const float value = cumulativeMeanNormalizedDifference[static_cast<size_t> (lag)];
        if (value < threshold)
        {
            bestLag = lag;
            while (bestLag + 1 < maximumLag
                   && cumulativeMeanNormalizedDifference[static_cast<size_t> (bestLag + 1)] < value)
            {
                ++bestLag;
            }
            break;
        }
    }

    if (bestLag < 0)
    {
        float bestValue = 1.0f;
        for (int lag = minimumLag; lag <= maximumLag; ++lag)
        {
            const float value = cumulativeMeanNormalizedDifference[static_cast<size_t> (lag)];
            if (value < bestValue)
            {
                bestValue = value;
                bestLag = lag;
            }
        }
        if (bestLag < 0 || bestValue > 0.38f)
            return false;
    }

    const float previous = cumulativeMeanNormalizedDifference[static_cast<size_t> (juce::jmax (minimumLag, bestLag - 1))];
    const float current = cumulativeMeanNormalizedDifference[static_cast<size_t> (bestLag)];
    const float next = cumulativeMeanNormalizedDifference[static_cast<size_t> (juce::jmin (maximumLag, bestLag + 1))];
    const float denominator = previous - 2.0f * current + next;
    const float offset = std::abs (denominator) > 1.0e-6f
                       ? juce::jlimit (-0.5f, 0.5f, 0.5f * (previous - next) / denominator)
                       : 0.0f;
    const float refinedLag = static_cast<float> (bestLag) + offset;
    if (refinedLag <= 0.0f)
        return false;

    frequencyHz = static_cast<float> (analysisSampleRate / static_cast<double> (refinedLag));
    return frequencyHz >= 45.0f && frequencyHz <= 650.0f;
}

//==============================================================================
// Two cross-faded delay grains provide a lightweight real-time pitch shifter.
// It is intentionally simple and tuned for monophonic vocal correction rather
// than polyphonic instrument processing.

void SpotTwoAudioProcessor::DualGrainPitchShifter::reset() noexcept
{
    for (auto& line : delayLines)
        line.fill (0.0f);
    writePosition = 0;
    phase = 0.0f;
}

float SpotTwoAudioProcessor::DualGrainPitchShifter::readInterpolated (int channel, float delaySamples) const noexcept
{
    const auto& line = delayLines[static_cast<size_t> (channel)];
    float readPosition = static_cast<float> (writePosition) - delaySamples;
    while (readPosition < 0.0f)
        readPosition += static_cast<float> (ringSize);
    while (readPosition >= static_cast<float> (ringSize))
        readPosition -= static_cast<float> (ringSize);

    const int indexA = static_cast<int> (readPosition);
    const int indexB = (indexA + 1) % ringSize;
    const float fraction = readPosition - static_cast<float> (indexA);
    const float a = line[static_cast<size_t> (indexA)];
    const float b = line[static_cast<size_t> (indexB)];
    return a + fraction * (b - a);
}

void SpotTwoAudioProcessor::DualGrainPitchShifter::processFrame (float* samples, int channels, float ratio) noexcept
{
    ratio = juce::jlimit (0.5f, 2.0f, ratio);

    for (int channel = 0; channel < channels; ++channel)
        delayLines[static_cast<size_t> (channel)][static_cast<size_t> (writePosition)] = samples[channel];

    const float phaseA = phase;
    float phaseB = phase + 0.5f;
    if (phaseB >= 1.0f)
        phaseB -= 1.0f;

    const float delayA = baseDelay + (1.0f - ratio) * static_cast<float> (grainSize) * (phaseA - 0.5f);
    const float delayB = baseDelay + (1.0f - ratio) * static_cast<float> (grainSize) * (phaseB - 0.5f);
    const float weightA = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phaseA);
    const float weightB = 1.0f - weightA;

    for (int channel = 0; channel < channels; ++channel)
    {
        const float a = readInterpolated (channel, delayA);
        const float b = readInterpolated (channel, delayB);
        samples[channel] = a * weightA + b * weightB;
    }

    writePosition = (writePosition + 1) % ringSize;
    phase += 1.0f / static_cast<float> (grainSize);
    if (phase >= 1.0f)
        phase -= 1.0f;
}

juce::AudioProcessorEditor* SpotTwoAudioProcessor::createEditor()
{
    return new SpotTwoAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpotTwoAudioProcessor();
}
