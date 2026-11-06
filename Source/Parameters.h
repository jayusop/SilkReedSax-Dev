// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SaxEngine.h"
#include "FXChain.h"

namespace silkreed
{

namespace ID
{
    #define SR_PARAM(name) inline constexpr const char* name = #name;
    SR_PARAM (instrument)  SR_PARAM (rangeMode)  SR_PARAM (tune)       SR_PARAM (transpose)  SR_PARAM (voiceMode)
    SR_PARAM (breathSource) SR_PARAM (breathCurve) SR_PARAM (breathSens) SR_PARAM (dynRange)
    SR_PARAM (attack)      SR_PARAM (release)    SR_PARAM (swell)      SR_PARAM (swellTime)
    SR_PARAM (brightness)  SR_PARAM (reed)       SR_PARAM (warmth)     SR_PARAM (body)       SR_PARAM (formant)
    SR_PARAM (air)         SR_PARAM (airTone)    SR_PARAM (subtone)
    SR_PARAM (vibRate)     SR_PARAM (vibDepth)   SR_PARAM (vibDelay)   SR_PARAM (vibFade)    SR_PARAM (vibMod)
    SR_PARAM (vibAmp)      SR_PARAM (vibRandom)
    SR_PARAM (glide)       SR_PARAM (glideMode)  SR_PARAM (scoop)      SR_PARAM (scoopTime)  SR_PARAM (tongue)
    SR_PARAM (fall)        SR_PARAM (pbRange)
    SR_PARAM (growl)       SR_PARAM (growlRate)  SR_PARAM (humanize)
    SR_PARAM (eqLow)       SR_PARAM (eqMid)      SR_PARAM (eqMidFreq)  SR_PARAM (eqHigh)     SR_PARAM (comp)
    SR_PARAM (width)       SR_PARAM (delayMix)   SR_PARAM (delayTime)  SR_PARAM (delayFb)
    SR_PARAM (revMix)      SR_PARAM (revSize)    SR_PARAM (revDamp)    SR_PARAM (master)
    #undef SR_PARAM
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Caches raw parameter pointers for lock-free reads on the audio thread
class ParamCache
{
public:
    void init (juce::AudioProcessorValueTreeState& apvts);
    void fill (EngineParams& e, FXParams& f) const;

private:
    std::unordered_map<std::string, std::atomic<float>*> map;
    float get (const char* id) const { return map.at (id)->load (std::memory_order_relaxed); }
};

} // namespace silkreed
