// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace silkreed
{

struct Preset
{
    juce::String name;
    std::vector<std::pair<const char*, float>> values;   // real (denormalised) values; others -> default
};

const std::vector<Preset>& getFactoryPresets();

// Instrument (0 soprano, 1 alto, 2 tenor) a preset explicitly selects, or -1 if it does not set one
int getPresetInstrument (const Preset& preset);

// Resets every parameter to its default and then applies the preset's values
void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset);

} // namespace silkreed
