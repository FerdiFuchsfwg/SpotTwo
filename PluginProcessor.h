#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class SpotTwoAudioProcessor final : public juce::AudioProcessor
{
public:
    SpotTwoAudioProcessor();
    ~SpotTwoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpotTwo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float getCurrentPitchMidi() const noexcept { return currentPitchMidi.load(); }
    float getCurrentFrequencyHz() const noexcept { return currentFrequencyHz.load(); }
    float getAppliedCorrectionSemitones() const noexcept { return appliedCorrectionSemitones.load(); }
    bool isNoteEnabled (int note) const noexcept;
    void setNoteEnabled (int note, bool enabled);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    class YinPitchTracker
    {
    public:
        void prepare (double hostSampleRate) noexcept;
        void reset() noexcept;
        // Returns true whenever a fresh analysis result is available. A non-positive
        // frequency means that the current frame was not confidently voiced.
        bool processSample (float input, float& frequencyHz) noexcept;

    private:
        bool analyse (float& frequencyHz) noexcept;

        static constexpr int analysisSize = 512;
        static constexpr int minimumLag = 18;
        static constexpr int maximumLag = 220;
        static constexpr int analysisInterval = 64;

        std::array<float, analysisSize> history {};
        std::array<float, maximumLag + 1> difference {};
        std::array<float, maximumLag + 1> cumulativeMeanNormalizedDifference {};
        int writeIndex = 0;
        int filledSamples = 0;
        int samplesSinceAnalysis = 0;
        int decimationCounter = 0;
        float decimationAccumulator = 0.0f;
        double analysisSampleRate = 12000.0;
        int decimationFactor = 4;
    };

    class DualGrainPitchShifter
    {
    public:
        void reset() noexcept;
        void processFrame (float* samples, int channels, float ratio) noexcept;

        static constexpr int reportedLatencySamples = 2048;

    private:
        static constexpr int ringSize = 8192;
        static constexpr int grainSize = 1024;
        static constexpr float baseDelay = static_cast<float> (reportedLatencySamples);

        std::array<std::array<float, ringSize>, 2> delayLines {};
        int writePosition = 0;
        float phase = 0.0f;

        float readInterpolated (int channel, float delaySamples) const noexcept;
    };

    float findNearestAllowedNote (float midiNote) const noexcept;

    YinPitchTracker pitchTracker;
    DualGrainPitchShifter pitchShifter;
    std::array<std::atomic<float>*, 12> noteParameters {};

    std::atomic<float> currentPitchMidi { -1.0f };
    std::atomic<float> currentFrequencyHz { 0.0f };
    std::atomic<float> appliedCorrectionSemitones { 0.0f };

    double currentSampleRate = 44100.0;
    float desiredCorrectionSemitones = 0.0f;
    float smoothedCorrectionSemitones = 0.0f;
    int voicedHoldSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpotTwoAudioProcessor)
};
