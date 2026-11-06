// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "PluginProcessor.h"
#include "PluginEditor.h"

SilkReedProcessor::SilkReedProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SilkReedState", silkreed::createParameterLayout())
{
    params.init (apvts);
    setCurrentProgram (0);
}

bool SilkReedProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SilkReedProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int maxBlock = juce::jmax (samplesPerBlock, 32);
    preparedBlock = maxBlock;
    mono.assign ((size_t) maxBlock, 0.0f);
    engine.prepare (sampleRate, maxBlock);
    fx.prepare (sampleRate, maxBlock);
    keyboardState.reset();
}

void SilkReedProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    // On-screen keyboard -> MIDI stream
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    params.fill (engineParams, fxParams);

    if ((int) mono.size() < n)   // host exceeded the announced block size: process in chunks
        mono.resize ((size_t) n);   // (rare; keeps us safe rather than glitching)

    engine.process (mono.data(), n, midi, engineParams);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // FX chain works in chunks of its prepared block size
    int pos = 0;
    static thread_local std::vector<float> tmpR;
    while (pos < n)
    {
        const int chunk = juce::jmin (n - pos, preparedBlock);
        if (R != nullptr)
            fx.process (mono.data() + pos, L + pos, R + pos, chunk, fxParams);
        else
        {
            if ((int) tmpR.size() < chunk) tmpR.resize ((size_t) chunk);
            fx.process (mono.data() + pos, L + pos, tmpR.data(), chunk, fxParams);
            for (int i = 0; i < chunk; ++i) L[pos + i] = 0.5f * (L[pos + i] + tmpR[(size_t) i]);
        }
        pos += chunk;
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);

    outputPeak.store (fx.getOutputPeak());
    midi.clear();
}

juce::AudioProcessorEditor* SilkReedProcessor::createEditor()
{
    return new SilkReedEditor (*this);
}

//==============================================================================
int SilkReedProcessor::getNumPrograms()
{
    return (int) silkreed::getFactoryPresets().size();
}

void SilkReedProcessor::setCurrentProgram (int index)
{
    const auto& presets = silkreed::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;
    silkreed::applyPreset (apvts, presets[(size_t) index]);
    currentPresetName = presets[(size_t) index].name;

    const int inst = silkreed::getPresetInstrument (presets[(size_t) index]);
    if (juce::isPositiveAndBelow (inst, 3))
        lastProgramForInstrument[(size_t) inst] = index;
}

void SilkReedProcessor::selectInstrument (int instrument)
{
    if (! juce::isPositiveAndBelow (instrument, 3)) return;
    const auto& presets = silkreed::getFactoryPresets();

    int index = lastProgramForInstrument[(size_t) instrument];
    if (index < 0)
        for (size_t i = 0; i < presets.size(); ++i)
            if (silkreed::getPresetInstrument (presets[i]) == instrument) { index = (int) i; break; }

    if (index >= 0)
        setCurrentProgram (index);
}

const juce::String SilkReedProcessor::getProgramName (int index)
{
    const auto& presets = silkreed::getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : juce::String();
}

//==============================================================================
void SilkReedProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", currentPresetName, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SilkReedProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPresetName = tree.getProperty ("presetName", currentPresetName).toString();
            currentProgram = (int) tree.getProperty ("program", currentProgram);
            apvts.replaceState (tree);
        }
}

juce::File SilkReedProcessor::getUserPresetFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                   .getChildFile ("SilkReed Sax").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

bool SilkReedProcessor::saveUserPreset (const juce::File& file)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", file.getFileNameWithoutExtension(), nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (file))
        {
            currentPresetName = file.getFileNameWithoutExtension();
            return true;
        }
    return false;
}

bool SilkReedProcessor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPresetName = file.getFileNameWithoutExtension();
            return true;
        }
    return false;
}

//==============================================================================
#if ! SILKREED_STANDALONE_TOOL
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SilkReedProcessor();
}
#endif
