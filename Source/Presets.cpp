// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "Presets.h"
#include "Parameters.h"

namespace silkreed
{

const std::vector<Preset>& getFactoryPresets()
{
    using namespace ID;
    static const std::vector<Preset> presets =
    {
        // ------------------------------------------------------------------ SOPRANO
        { "Soprano - Smooth Jazz Lead", {
            { instrument, 0 }, { brightness, 0.64f }, { reed, 0.30f }, { warmth, 0.38f }, { body, 0.85f },
            { formant, 0.05f }, { air, 0.42f }, { airTone, 0.62f }, { subtone, 0.0f },
            { dynRange, 0.65f }, { attack, 28.0f }, { release, 220.0f }, { swell, 0.45f }, { swellTime, 1400.0f },
            { vibRate, 5.7f }, { vibDepth, 30.0f }, { vibDelay, 260.0f }, { vibFade, 700.0f }, { vibAmp, 0.45f }, { vibRandom, 0.35f },
            { glide, 55.0f }, { scoop, 0.35f }, { scoopTime, 110.0f }, { tongue, 0.30f }, { fall, 0.0f },
            { humanize, 0.35f }, { eqLow, -1.5f }, { eqMid, 1.5f }, { eqMidFreq, 2400.0f }, { eqHigh, 1.5f },
            { comp, 0.40f }, { width, 0.55f }, { delayMix, 0.14f }, { delayTime, 390.0f }, { delayFb, 0.30f },
            { revMix, 0.30f }, { revSize, 0.70f }, { revDamp, 0.35f } } },

        { "Soprano - Silk Ballad", {
            { instrument, 0 }, { brightness, 0.52f }, { reed, 0.20f }, { warmth, 0.55f }, { body, 0.9f },
            { air, 0.58f }, { airTone, 0.55f }, { subtone, 0.12f },
            { dynRange, 0.75f }, { attack, 70.0f }, { release, 420.0f }, { swell, 0.6f }, { swellTime, 2000.0f },
            { vibRate, 5.2f }, { vibDepth, 34.0f }, { vibDelay, 380.0f }, { vibFade, 1100.0f }, { vibAmp, 0.5f }, { vibRandom, 0.4f },
            { glide, 90.0f }, { scoop, 0.45f }, { scoopTime, 160.0f }, { tongue, 0.15f }, { fall, 0.1f },
            { humanize, 0.4f }, { eqLow, -1.0f }, { eqHigh, 0.5f }, { comp, 0.35f }, { width, 0.6f },
            { delayMix, 0.18f }, { delayTime, 480.0f }, { delayFb, 0.35f },
            { revMix, 0.40f }, { revSize, 0.82f }, { revDamp, 0.45f } } },

        { "Soprano - Bright Pop Lead", {
            { instrument, 0 }, { brightness, 0.74f }, { reed, 0.50f }, { warmth, 0.25f }, { body, 0.8f },
            { formant, 0.12f }, { air, 0.30f }, { airTone, 0.7f },
            { dynRange, 0.6f }, { attack, 15.0f }, { release, 150.0f }, { swell, 0.25f },
            { vibRate, 6.0f }, { vibDepth, 22.0f }, { vibDelay, 220.0f }, { vibFade, 450.0f }, { vibAmp, 0.35f },
            { glide, 35.0f }, { scoop, 0.25f }, { scoopTime, 70.0f }, { tongue, 0.45f },
            { eqMid, 2.0f }, { eqMidFreq, 3000.0f }, { eqHigh, 2.5f }, { comp, 0.55f }, { width, 0.5f },
            { delayMix, 0.12f }, { delayTime, 330.0f }, { revMix, 0.22f }, { revSize, 0.55f }, { master, -6.0f } } },

        { "Soprano - Intimate Dry", {
            { instrument, 0 }, { brightness, 0.58f }, { reed, 0.28f }, { warmth, 0.45f }, { air, 0.45f },
            { vibRate, 5.5f }, { vibDepth, 26.0f }, { vibDelay, 300.0f }, { vibFade, 650.0f },
            { glide, 50.0f }, { scoop, 0.3f }, { comp, 0.25f }, { width, 0.25f },
            { delayMix, 0.0f }, { revMix, 0.06f }, { revSize, 0.25f } } },

        { "Soprano - EWI Breath Control", {
            { instrument, 0 }, { breathSource, 1 }, { breathCurve, 0.25f }, { breathSens, 1.1f },
            { brightness, 0.62f }, { reed, 0.32f }, { warmth, 0.4f }, { air, 0.40f }, { airTone, 0.6f },
            { dynRange, 0.8f }, { attack, 4.0f }, { release, 45.0f }, { swell, 0.0f },
            { vibRate, 5.7f }, { vibDepth, 8.0f }, { vibDelay, 350.0f }, { vibMod, 45.0f }, { vibAmp, 0.4f },
            { glide, 30.0f }, { scoop, 0.15f }, { tongue, 0.25f }, { pbRange, 2 },
            { comp, 0.35f }, { width, 0.55f }, { delayMix, 0.12f }, { revMix, 0.28f }, { revSize, 0.68f } } },

        // ------------------------------------------------------------------ ALTO
        { "Alto - Warm Ballad", {
            { instrument, 1 }, { brightness, 0.48f }, { reed, 0.25f }, { warmth, 0.6f }, { body, 0.9f },
            { air, 0.5f }, { airTone, 0.5f }, { subtone, 0.15f },
            { dynRange, 0.75f }, { attack, 55.0f }, { release, 350.0f }, { swell, 0.5f }, { swellTime, 1600.0f },
            { vibRate, 5.3f }, { vibDepth, 28.0f }, { vibDelay, 350.0f }, { vibFade, 900.0f }, { vibAmp, 0.45f },
            { glide, 70.0f }, { scoop, 0.35f }, { scoopTime, 130.0f }, { tongue, 0.2f },
            { eqLow, 0.5f }, { comp, 0.35f }, { width, 0.55f },
            { delayMix, 0.12f }, { delayTime, 420.0f }, { revMix, 0.32f }, { revSize, 0.72f } } },

        { "Alto - Smooth R&B Lead", {
            { instrument, 1 }, { brightness, 0.60f }, { reed, 0.38f }, { warmth, 0.42f }, { air, 0.38f },
            { dynRange, 0.65f }, { attack, 25.0f }, { release, 200.0f }, { swell, 0.35f },
            { vibRate, 5.8f }, { vibDepth, 24.0f }, { vibDelay, 260.0f }, { vibFade, 600.0f },
            { glide, 45.0f }, { scoop, 0.4f }, { scoopTime, 90.0f }, { tongue, 0.35f }, { fall, 0.15f },
            { eqMid, 1.5f }, { eqMidFreq, 1800.0f }, { comp, 0.45f },
            { delayMix, 0.1f }, { revMix, 0.25f }, { revSize, 0.6f } } },

        { "Alto - Funk Edge", {
            { instrument, 1 }, { brightness, 0.78f }, { reed, 0.75f }, { warmth, 0.2f }, { body, 0.75f },
            { air, 0.22f }, { growl, 0.12f }, { growlRate, 48.0f },
            { dynRange, 0.55f }, { attack, 6.0f }, { release, 90.0f }, { swell, 0.0f },
            { vibRate, 6.2f }, { vibDepth, 14.0f }, { vibDelay, 380.0f }, { vibFade, 300.0f }, { vibAmp, 0.2f },
            { voiceMode, 1 }, { glide, 15.0f }, { scoop, 0.2f }, { scoopTime, 50.0f }, { tongue, 0.7f }, { fall, 0.3f },
            { eqLow, -2.0f }, { eqMid, 3.0f }, { eqMidFreq, 1600.0f }, { eqHigh, 1.5f }, { comp, 0.65f },
            { width, 0.35f }, { delayMix, 0.0f }, { revMix, 0.14f }, { revSize, 0.35f }, { master, -6.0f } } },

        { "Alto - EWI Breath Control", {
            { instrument, 1 }, { breathSource, 1 }, { breathCurve, 0.25f }, { breathSens, 1.1f },
            { brightness, 0.58f }, { reed, 0.35f }, { warmth, 0.45f }, { air, 0.38f },
            { dynRange, 0.8f }, { attack, 4.0f }, { release, 45.0f }, { swell, 0.0f },
            { vibDepth, 8.0f }, { vibMod, 45.0f }, { glide, 30.0f }, { scoop, 0.15f }, { tongue, 0.25f },
            { comp, 0.35f }, { delayMix, 0.1f }, { revMix, 0.26f } } },

        // ------------------------------------------------------------------ TENOR
        { "Tenor - Breathy Ballad", {
            { instrument, 2 }, { brightness, 0.42f }, { reed, 0.22f }, { warmth, 0.65f }, { body, 0.95f },
            { air, 0.65f }, { airTone, 0.42f }, { subtone, 0.40f },
            { dynRange, 0.7f }, { attack, 80.0f }, { release, 450.0f }, { swell, 0.55f }, { swellTime, 1800.0f },
            { vibRate, 5.0f }, { vibDepth, 26.0f }, { vibDelay, 420.0f }, { vibFade, 1100.0f }, { vibAmp, 0.5f },
            { glide, 80.0f }, { scoop, 0.4f }, { scoopTime, 150.0f }, { tongue, 0.15f }, { fall, 0.1f },
            { eqLow, 1.0f }, { eqHigh, -1.0f }, { comp, 0.35f }, { width, 0.6f },
            { delayMix, 0.1f }, { delayTime, 450.0f }, { revMix, 0.34f }, { revSize, 0.75f }, { revDamp, 0.5f } } },

        { "Tenor - Smooth Lead", {
            { instrument, 2 }, { brightness, 0.56f }, { reed, 0.40f }, { warmth, 0.48f }, { air, 0.42f },
            { subtone, 0.1f }, { dynRange, 0.7f }, { attack, 30.0f }, { release, 220.0f }, { swell, 0.35f },
            { vibRate, 5.4f }, { vibDepth, 22.0f }, { vibDelay, 300.0f }, { vibFade, 700.0f },
            { glide, 50.0f }, { scoop, 0.35f }, { scoopTime, 100.0f }, { tongue, 0.35f },
            { comp, 0.4f }, { delayMix, 0.1f }, { revMix, 0.26f }, { revSize, 0.62f } } },

        { "Tenor - Gritty Rock", {
            { instrument, 2 }, { brightness, 0.72f }, { reed, 0.85f }, { warmth, 0.3f }, { body, 0.8f },
            { air, 0.3f }, { growl, 0.45f }, { growlRate, 62.0f },
            { dynRange, 0.5f }, { attack, 8.0f }, { release, 120.0f }, { swell, 0.0f },
            { vibRate, 5.8f }, { vibDepth, 18.0f }, { vibDelay, 350.0f }, { vibFade, 400.0f },
            { voiceMode, 1 }, { glide, 20.0f }, { scoop, 0.3f }, { scoopTime, 60.0f }, { tongue, 0.65f }, { fall, 0.35f },
            { eqLow, -1.0f }, { eqMid, 3.0f }, { eqMidFreq, 1200.0f }, { eqHigh, 2.0f }, { comp, 0.7f },
            { width, 0.4f }, { delayMix, 0.06f }, { revMix, 0.16f }, { revSize, 0.4f }, { master, -6.0f } } },

        { "Tenor - EWI Breath Control", {
            { instrument, 2 }, { breathSource, 1 }, { breathCurve, 0.25f }, { breathSens, 1.1f },
            { brightness, 0.52f }, { reed, 0.38f }, { warmth, 0.5f }, { air, 0.45f }, { subtone, 0.1f },
            { dynRange, 0.8f }, { attack, 4.0f }, { release, 50.0f }, { swell, 0.0f },
            { vibDepth, 8.0f }, { vibMod, 45.0f }, { glide, 30.0f }, { scoop, 0.15f }, { tongue, 0.25f },
            { comp, 0.35f }, { delayMix, 0.08f }, { revMix, 0.26f } } },

        // ------------------------------------------------------------------ INIT
        { "Init (Default)", {} },
    };
    return presets;
}

int getPresetInstrument (const Preset& preset)
{
    for (const auto& [id, value] : preset.values)
        if (juce::String (id) == ID::instrument)
            return juce::roundToInt (value);
    return -1;
}

void applyPreset (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

} // namespace silkreed
