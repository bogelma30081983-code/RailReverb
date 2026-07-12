#include "PluginProcessor.h"
#include "PluginEditor.h"

// Конструктор процесора
RailReverbProcessor::RailReverbProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    noiseRandom.setSeedRandomly();
}

RailReverbProcessor::~RailReverbProcessor()
{
}

// Підготовка DSP до роботи
void RailReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    const int maxDelaySamples = static_cast<int>(sampleRate * 0.5); // Максимум 500мс
    for (int i = 0; i < 4; ++i)
    {
        delayLinesL[i].setSize(1, maxDelaySamples);
        delayLinesR[i].setSize(1, maxDelaySamples);
        delayLinesL[i].clear();
        delayLinesR[i].clear();

        dispersionFiltersL[i].clear();
        dispersionFiltersR[i].clear();

        baseDelays[i] = 0.030f + (i * 0.017f);
        lastFilterOutL[i] = 0.0f;
        lastFilterOutR[i] = 0.0f;
    }

    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    inputLowPass.prepare(spec);
    inputLowPass.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);

    driftPhaseL = 0.0f;
    driftPhaseR = 0.5f;
    writePos = 0;
    // Заздалегідь виділяємо залізне місце в пам'яті під потрібну кількість каналів та семплів
    wetBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
}

void RailReverbProcessor::releaseResources()
{
}

bool RailReverbProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

// Головний цикл обробки звуку
void RailReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Завантаження параметрів
    const float railSize = apvts.getRawParameterValue("SIZE")->load();
    const int materialIdx = static_cast<int>(apvts.getRawParameterValue("MATERIAL")->load());
    const bool driftOn = apvts.getRawParameterValue("DRIFT")->load() > 0.5f;
    const bool noiseOn = apvts.getRawParameterValue("NOISE")->load() > 0.5f;
    const float detuneVal = apvts.getRawParameterValue("DETUNE")->load();
    const float cutoff = apvts.getRawParameterValue("CUTOFF")->load();
    const float mix = apvts.getRawParameterValue("MIX")->load();
    const bool autoComp = apvts.getRawParameterValue("AUTO_COMP")->load() > 0.5f;

    MaterialProperties props = MaterialPresets[materialIdx];
    inputLowPass.setCutoffFrequency(cutoff);

    /*juce::AudioBuffer<float> wetBuffer;
    wetBuffer.makeCopyOf(buffer);*/

    // Рядки 75-76 тепер виглядатимуть так:
    // Переконуємося, що розмір збігається (якщо він той самий, JUCE не буде чіпати пам'ять)
    wetBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);

    // Безпечно копіюємо аудіо з основного буфера в наш постійний wetBuffer
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        wetBuffer.copyFrom(channel, 0, buffer.getReadPointer(channel), buffer.getNumSamples());
    }

    juce::dsp::AudioBlock<float> block(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    inputLowPass.process(context);

    float* wetL = wetBuffer.getWritePointer(0);
    float* wetR = wetBuffer.getWritePointer(1);
    const float* dryL = buffer.getReadPointer(0);
    const float* dryR = buffer.getReadPointer(1);

    float dryEnergy = 0.0f;
    float totalOutputEnergy = 0.0f;

    int totalDelaySamples = delayLinesL[0].getNumSamples();
    if (totalDelaySamples == 0) return;

    // ГОЛОВНИЙ ЦИКЛ ОБРОБКИ СЕМПЛІВ
    for (int sample = 0; sample < numSamples; ++sample)
    {
        dryEnergy += (dryL[sample] * dryL[sample]) + (dryR[sample] * dryR[sample]);

        float currentDriftL = 0.0f, currentDriftR = 0.0f;
        if (driftOn)
        {
            float speedFactor = 1.0f / (props.density * 0.2f);
            driftPhaseL += 0.00005f * speedFactor;
            driftPhaseR += 0.00007f * speedFactor;

            if (driftPhaseL > juce::MathConstants<float>::twoPi) driftPhaseL -= juce::MathConstants<float>::twoPi;
            if (driftPhaseR > juce::MathConstants<float>::twoPi) driftPhaseR -= juce::MathConstants<float>::twoPi;

            currentDriftL = std::sin(driftPhaseL) * 0.003f;
            currentDriftR = std::cos(driftPhaseR) * 0.003f;
        }

        float noise = noiseOn ? (noiseRandom.nextFloat() * 2.0f - 1.0f) * 0.005f : 0.0f;
        float inL = wetL[sample] + noise;
        float inR = wetR[sample] + noise;

        float outCombL = 0.0f;
        float outCombR = 0.0f;
        float sizeFactor = 0.25f + (railSize * 0.75f);

        // Обробка резонаторів рейки
        for (int i = 0; i < 4; ++i)
        {
            float detuneOffset = (i + 1) * detuneVal * 0.0008f;
            float delayTimeL = (baseDelays[i] * sizeFactor) + currentDriftL - detuneOffset;
            float delayTimeR = (baseDelays[i] * sizeFactor) + currentDriftR + detuneOffset;

            int delaySamplesL = std::clamp(static_cast<int>(delayTimeL * currentSampleRate), 10, totalDelaySamples - 1);
            int delaySamplesR = std::clamp(static_cast<int>(delayTimeR * currentSampleRate), 10, totalDelaySamples - 1);

            int readPosL = (writePos - delaySamplesL + totalDelaySamples) % totalDelaySamples;
            int readPosR = (writePos - delaySamplesR + totalDelaySamples) % totalDelaySamples;

            float tapL = delayLinesL[i].getSample(0, readPosL);
            float tapR = delayLinesR[i].getSample(0, readPosR);

            tapL = dispersionFiltersL[i].process(tapL, props.dispersion);
            tapR = dispersionFiltersR[i].process(tapR, props.dispersion);

            lastFilterOutL[i] = tapL * props.brightness + lastFilterOutL[i] * (1.0f - props.brightness);
            lastFilterOutR[i] = tapR * props.brightness + lastFilterOutR[i] * (1.0f - props.brightness);

            float feedbackL = lastFilterOutL[i] * props.feedbackBase;
            float feedbackR = lastFilterOutR[i] * props.feedbackBase;

            float crossSpread = std::clamp(detuneVal / 10.0f, 0.0f, 0.25f);
            float finalFeedL = (feedbackL * (1.0f - crossSpread)) + (feedbackR * crossSpread);
            float finalFeedR = (feedbackR * (1.0f - crossSpread)) + (feedbackL * crossSpread);

            delayLinesL[i].setSample(0, writePos, inL + finalFeedL);
            delayLinesR[i].setSample(0, writePos, inR + finalFeedR);

            outCombL += tapL;
            outCombR += tapR;
        }

        outCombL *= 0.25f;
        outCombR *= 0.25f;

        tailOutputL.store(outCombL, std::memory_order_relaxed);
        tailOutputR.store(outCombR, std::memory_order_relaxed);

        float finalL = (dryL[sample] * (1.0f - mix)) + (outCombL * mix);
        float finalR = (dryR[sample] * (1.0f - mix)) + (outCombR * mix);

        buffer.setSample(0, sample, finalL);
        buffer.setSample(1, sample, finalR);

        totalOutputEnergy += (finalL * finalL) + (finalR * finalR);

        // !! ВАЖЛИВИЙ ФІКС: Рухаємо вказівник запису вперед для КОЖНОГО сэмпла !!
        if (++writePos >= totalDelaySamples)
            writePos = 0;
    }

    
    // Автокомпенсація гучності (Виправлений розумний компандер)
    if (autoComp && numSamples > 0)
    {
        float dryRMS = std::sqrt(dryEnergy / numSamples);
        float outRMS = std::sqrt(totalOutputEnergy / numSamples);

        // ФІКС: Стискаємо звук ТІЛЬКИ якщо на вході є активний сигнал (dryRMS > 0.01f)
        if (dryRMS > 0.01f && outRMS > dryRMS)
        {
            currentGainComp = 0.95f * currentGainComp + 0.05f * (dryRMS / outRMS);
        }
        else
        {
            // Якщо інструмент замовк, плавно повертаємо коефіцієнт до 1.0 (Реліз компресора)
            currentGainComp = 0.95f * currentGainComp + 0.05f;
        }

        if (std::isnan(currentGainComp) || std::isinf(currentGainComp))
            currentGainComp = 1.0f;

        buffer.applyGain(currentGainComp);
    }
}
void RailReverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RailReverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
// Зв'язок з вікном інтерфейсу
juce::AudioProcessorEditor* RailReverbProcessor::createEditor()
{
    return new RailReverbEditor(*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

// Створення дерева параметрів плагіна JUCE
juce::AudioProcessorValueTreeState::ParameterLayout RailReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "SIZE", 1 },
        "Rail Size", 0.0f, 1.0f, 0.6f));

    juce::StringArray mats{ "Iron", "Copper", "Brass", "Silver", "Gold", "Tungsten",
        "Titanium", "Bog Oak", "Uranium" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "MATERIAL", 1 }, 
        "Material", mats, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "DRIFT", 1 },
        "Analog Drift", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "NOISE", 1 }, 
        "Analog Noise", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "DETUNE", 1 }, 
        "Detune", 0.0f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "CUTOFF", 1 }, 
        "Filter Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 5000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ "MIX", 1 }, 
        "Mix", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "AUTO_COMP", 1 }, 
        "Auto Volume Comp", true));

    return { params.begin(), params.end() };


    

}
std::pair<float, float> RailReverbProcessor::getTailLevels() const
{
    return { tailOutputL.load(), tailOutputR.load() };
}
// Ця функція обов'язково має бути в кінці PluginProcessor.cpp
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RailReverbProcessor();
}