#pragma once
#include <JuceHeader.h>

enum class RailMaterial {
    Iron = 0, Copper, Brass, Silver, Gold, Tungsten, Titanium, BogOak, Uranium, TotalMaterials
};

struct MaterialProperties {
    float feedbackBase;
    float brightness;
    float dispersion;
    float density;
};

static const MaterialProperties MaterialPresets[] = {
    { 0.97f,  0.85f, 0.15f, 7.8f  }, // Iron
    { 0.93f,  0.65f, 0.08f, 8.9f  }, // Copper
    { 0.95f,  0.75f, 0.12f, 8.5f  }, // Brass
    { 0.98f,  0.92f, 0.05f, 10.5f }, // Silver
    { 0.91f,  0.55f, 0.03f, 19.3f }, // Gold
    { 0.985f, 0.80f, 0.25f, 19.2f }, // Tungsten
    { 0.96f,  0.88f, 0.18f, 4.5f  }, // Titanium
    { 0.72f,  0.15f, 0.00f, 0.9f  }, // BogOak
    { 0.89f,  0.42f, 0.35f, 19.1f }  // Uranium
};

class RailDispersionFilter {
public:
    void clear() { delayedSample = 0.0f; }
    float process(float input, float coefficient) {
        float output = -coefficient * input + delayedSample;
        delayedSample = input + coefficient * output;
        return output;
    }
private:
    float delayedSample = 0.0f;
};

class RailReverbProcessor : public juce::AudioProcessor
{
public:
    RailReverbProcessor();
    ~RailReverbProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Rail Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    std::pair<float, float> getTailLevels() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    double currentSampleRate = 44100.0;
    int writePos = 0;
    juce::AudioBuffer<float> delayLinesL[4];
    juce::AudioBuffer<float> delayLinesR[4];
    float baseDelays[4];
    float lastFilterOutL[4];
    float lastFilterOutR[4];
    RailDispersionFilter dispersionFiltersL[4];
    RailDispersionFilter dispersionFiltersR[4];
    juce::dsp::LinkwitzRileyFilter<float> inputLowPass;
    float driftPhaseL = 0.0f, driftPhaseR = 0.0f;
    juce::Random noiseRandom;
    float currentGainComp = 1.0f;
    std::atomic<float> tailOutputL{ 0.0f }, tailOutputR{ 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RailReverbProcessor)
};