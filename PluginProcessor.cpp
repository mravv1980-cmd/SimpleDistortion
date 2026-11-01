#include "PluginProcessor.h"

DistortionProcessor::DistortionProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    driveParam = apvts.getRawParameterValue ("drive");
    toneParam  = apvts.getRawParameterValue ("tone");
    mixParam   = apvts.getRawParameterValue ("mix");
    levelParam = apvts.getRawParameterValue ("level");
}

juce::AudioProcessorValueTreeState::ParameterLayout DistortionProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "drive", 1 }, "Drive",
        NormalisableRange<float> (0.0f, 40.0f, 0.1f), 12.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "tone", 1 }, "Tone",
        NormalisableRange<float> (500.0f, 20000.0f, 1.0f, 0.3f), 8000.0f,
        AudioParameterFloatAttributes().withLabel ("Hz")));

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "level", 1 }, "Level",
        NormalisableRange<float> (-24.0f, 6.0f, 0.1f), -6.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    return { p.begin(), p.end() };
}

void DistortionProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    driveSm.reset (sampleRate, 0.02);
    mixSm.reset   (sampleRate, 0.02);
    levelSm.reset (sampleRate, 0.02);

    driveSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    mixSm.setCurrentAndTargetValue   (mixParam->load());
    levelSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (levelParam->load()));

    lpState[0] = lpState[1] = 0.0f;
}

bool DistortionProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void DistortionProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (getTotalNumInputChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    driveSm.setTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    mixSm.setTargetValue   (mixParam->load());
    levelSm.setTargetValue (juce::Decibels::decibelsToGain (levelParam->load()));

    // Однополюсный ФНЧ (регулятор Tone)
    const float fc = toneParam->load();
    const float a  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);

    for (int i = 0; i < numSamples; ++i)
    {
        const float g   = driveSm.getNextValue();
        const float mix = mixSm.getNextValue();
        const float lvl = levelSm.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float dry = data[i];

            float wet = std::tanh (dry * g);       // мягкий клиппинг
            lpState[ch] += a * (wet - lpState[ch]);
            wet = lpState[ch];

            data[i] = (dry * (1.0f - mix) + wet * mix) * lvl;
        }
    }
}

juce::AudioProcessorEditor* DistortionProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void DistortionProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DistortionProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DistortionProcessor();
}
