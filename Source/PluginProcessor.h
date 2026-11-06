// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "SaxEngine.h"
#include "FXChain.h"
#include "Presets.h"

class SilkReedProcessor : public juce::AudioProcessor
{
public:
    SilkReedProcessor();
    ~SilkReedProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SilkReed Sax"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;

    // Switches to an instrument by loading its preset: the one last used with that
    // instrument in this session, or its first factory preset.
    void selectInstrument (int instrument);
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // user presets
    bool saveUserPreset (const juce::File& file);
    bool loadUserPreset (const juce::File& file);
    static juce::File getUserPresetFolder();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    float getBreathMeter() const { return engine.getBreathMeter(); }
    float getOutputPeak() const  { return outputPeak.load(); }
    juce::String currentPresetName { "Soprano - Smooth Jazz Lead" };

private:
    silkreed::ParamCache params;
    silkreed::SaxEngine engine;
    silkreed::FXChain fx;
    silkreed::EngineParams engineParams;
    silkreed::FXParams fxParams;
    std::vector<float> mono;
    int currentProgram = 0;
    std::array<int, 3> lastProgramForInstrument { -1, -1, -1 };
    int preparedBlock = 512;
    std::atomic<float> outputPeak { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkReedProcessor)
};
