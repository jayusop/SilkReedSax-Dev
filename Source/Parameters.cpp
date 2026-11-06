// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "Parameters.h"

namespace silkreed
{

using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

namespace
{
    juce::AudioParameterFloatAttributes fmt (std::function<juce::String (float, int)> f)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (f));
    }
    auto pct()   { return fmt ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }); }
    auto bip()   { return fmt ([] (float v, int) { const int i = juce::roundToInt (v * 100.0f); return (i > 0 ? "+" : "") + juce::String (i); }); }
    auto ms()    { return fmt ([] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms"; }); }
    auto hz()    { return fmt ([] (float v, int) { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 20.0f ? 2 : 0) + " Hz"; }); }
    auto cents() { return fmt ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; }); }
    auto db()    { return fmt ([] (float v, int) { return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }); }

    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    void addFloat (juce::AudioProcessorValueTreeState::ParameterLayout& l, const char* id, const char* name,
                   Range r, float def, juce::AudioParameterFloatAttributes a)
    {
        l.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def, a));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    const Range unit (0.0f, 1.0f);

    // ---- instrument
    l.add (std::make_unique<APC> (juce::ParameterID { ID::instrument, 1 }, "Instrument",
                                  juce::StringArray { "Soprano", "Alto", "Tenor" }, 0));
    l.add (std::make_unique<APC> (juce::ParameterID { ID::rangeMode, 1 }, "Range",
                                  juce::StringArray { "Free", "Fold to Range" }, 0));
    addFloat (l, ID::tune, "Fine Tune", Range (-100.0f, 100.0f), 0.0f, cents());
    l.add (std::make_unique<API> (juce::ParameterID { ID::transpose, 1 }, "Transpose", -24, 24, 0));
    l.add (std::make_unique<APC> (juce::ParameterID { ID::voiceMode, 1 }, "Voice Mode",
                                  juce::StringArray { "Legato", "Retrigger", "Poly" }, 0));

    // ---- breath / dynamics
    l.add (std::make_unique<APC> (juce::ParameterID { ID::breathSource, 1 }, "Breath Source",
                                  juce::StringArray { "Velocity", "Breath CC2", "Expr CC11", "Aftertouch", "Vel x CC11" }, 0));
    addFloat (l, ID::breathCurve, "Breath Curve", Range (-1.0f, 1.0f), 0.0f, bip());
    addFloat (l, ID::breathSens, "Breath Sens", Range (0.25f, 2.0f), 1.0f, pct());
    addFloat (l, ID::dynRange, "Dyn Timbre", unit, 0.7f, pct());
    addFloat (l, ID::attack, "Attack", skewed (1.0f, 600.0f, 60.0f), 30.0f, ms());
    addFloat (l, ID::release, "Release", skewed (10.0f, 2500.0f, 250.0f), 180.0f, ms());
    addFloat (l, ID::swell, "Auto Swell", unit, 0.3f, pct());
    addFloat (l, ID::swellTime, "Swell Time", skewed (100.0f, 4000.0f, 900.0f), 1000.0f, ms());

    // ---- tone
    addFloat (l, ID::brightness, "Brightness", unit, 0.55f, pct());
    addFloat (l, ID::reed, "Reed Edge", unit, 0.35f, pct());
    addFloat (l, ID::warmth, "Warmth", unit, 0.4f, pct());
    addFloat (l, ID::body, "Body", Range (0.0f, 1.5f), 0.8f, pct());
    addFloat (l, ID::formant, "Formant", Range (-1.0f, 1.0f), 0.0f, bip());
    addFloat (l, ID::air, "Air", unit, 0.35f, pct());
    addFloat (l, ID::airTone, "Air Tone", unit, 0.5f, pct());
    addFloat (l, ID::subtone, "Subtone", unit, 0.0f, pct());

    // ---- vibrato
    addFloat (l, ID::vibRate, "Vib Rate", Range (2.0f, 9.0f), 5.6f, hz());
    addFloat (l, ID::vibDepth, "Vib Depth", Range (0.0f, 100.0f), 25.0f, cents());
    addFloat (l, ID::vibDelay, "Vib Delay", skewed (0.0f, 2000.0f, 400.0f), 300.0f, ms());
    addFloat (l, ID::vibFade, "Vib Fade", skewed (1.0f, 3000.0f, 600.0f), 600.0f, ms());
    addFloat (l, ID::vibMod, "Mod Wheel Vib", Range (0.0f, 100.0f), 40.0f, cents());
    addFloat (l, ID::vibAmp, "Vib Amp/Tone", unit, 0.35f, pct());
    addFloat (l, ID::vibRandom, "Vib Random", unit, 0.3f, pct());

    // ---- articulation
    addFloat (l, ID::glide, "Glide", skewed (0.0f, 800.0f, 80.0f), 50.0f, ms());
    l.add (std::make_unique<APC> (juce::ParameterID { ID::glideMode, 1 }, "Glide Mode",
                                  juce::StringArray { "Legato", "Always" }, 0));
    addFloat (l, ID::scoop, "Scoop", unit, 0.3f, pct());
    addFloat (l, ID::scoopTime, "Scoop Time", skewed (10.0f, 400.0f, 90.0f), 90.0f, ms());
    addFloat (l, ID::tongue, "Tongue", unit, 0.4f, pct());
    addFloat (l, ID::fall, "Fall", unit, 0.0f, pct());
    l.add (std::make_unique<API> (juce::ParameterID { ID::pbRange, 1 }, "Bend Range", 1, 12, 2));

    // ---- character
    addFloat (l, ID::growl, "Growl", unit, 0.0f, pct());
    addFloat (l, ID::growlRate, "Growl Rate", Range (20.0f, 120.0f), 55.0f, hz());
    addFloat (l, ID::humanize, "Humanize", unit, 0.35f, pct());

    // ---- effects
    addFloat (l, ID::eqLow, "Low", Range (-12.0f, 12.0f), 0.0f, db());
    addFloat (l, ID::eqMid, "Mid", Range (-12.0f, 12.0f), 0.0f, db());
    addFloat (l, ID::eqMidFreq, "Mid Freq", skewed (300.0f, 6000.0f, 1500.0f), 1500.0f, hz());
    addFloat (l, ID::eqHigh, "High", Range (-12.0f, 12.0f), 0.0f, db());
    addFloat (l, ID::comp, "Compressor", unit, 0.3f, pct());
    addFloat (l, ID::width, "Width", unit, 0.5f, pct());
    addFloat (l, ID::delayMix, "Delay Mix", unit, 0.12f, pct());
    addFloat (l, ID::delayTime, "Delay Time", skewed (40.0f, 1000.0f, 350.0f), 380.0f, ms());
    addFloat (l, ID::delayFb, "Feedback", Range (0.0f, 0.9f), 0.3f, pct());
    addFloat (l, ID::revMix, "Reverb Mix", unit, 0.25f, pct());
    addFloat (l, ID::revSize, "Reverb Size", unit, 0.6f, pct());
    addFloat (l, ID::revDamp, "Reverb Damp", unit, 0.4f, pct());
    addFloat (l, ID::master, "Master", Range (-36.0f, 6.0f), -3.0f, db());

    return l;
}

//==============================================================================
void ParamCache::init (juce::AudioProcessorValueTreeState& apvts)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* wp = dynamic_cast<juce::RangedAudioParameter*> (p))
            map[wp->getParameterID().toStdString()] = apvts.getRawParameterValue (wp->getParameterID());
}

void ParamCache::fill (EngineParams& e, FXParams& f) const
{
    e.instrument   = (int) get (ID::instrument);
    e.foldToRange  = get (ID::rangeMode) > 0.5f;
    e.tuneCents    = get (ID::tune);
    e.transpose    = (int) get (ID::transpose);
    e.voiceMode    = (int) get (ID::voiceMode);

    e.breathSource = (int) get (ID::breathSource);
    e.breathCurve  = get (ID::breathCurve);
    e.breathSens   = get (ID::breathSens);
    e.dynRange     = get (ID::dynRange);
    e.attackMs     = get (ID::attack);
    e.releaseMs    = get (ID::release);
    e.swell        = get (ID::swell);
    e.swellMs      = get (ID::swellTime);

    e.brightness   = get (ID::brightness);
    e.reed         = get (ID::reed);
    e.warmth       = get (ID::warmth);
    e.body         = get (ID::body);
    e.formantShift = get (ID::formant);
    e.air          = get (ID::air);
    e.airTone      = get (ID::airTone);
    e.subtone      = get (ID::subtone);

    e.vibRate      = get (ID::vibRate);
    e.vibDepth     = get (ID::vibDepth);
    e.vibDelayMs   = get (ID::vibDelay);
    e.vibFadeMs    = get (ID::vibFade);
    e.vibModDepth  = get (ID::vibMod);
    e.vibAmp       = get (ID::vibAmp);
    e.vibRandom    = get (ID::vibRandom);

    e.glideMs      = get (ID::glide);
    e.glideMode    = (int) get (ID::glideMode);
    e.scoop        = get (ID::scoop);
    e.scoopMs      = get (ID::scoopTime);
    e.tongue       = get (ID::tongue);
    e.fall         = get (ID::fall);
    e.pbRange      = (int) get (ID::pbRange);

    e.growl        = get (ID::growl);
    e.growlRate    = get (ID::growlRate);
    e.humanize     = get (ID::humanize);

    f.eqLowDb  = get (ID::eqLow);
    f.eqMidDb  = get (ID::eqMid);
    f.eqMidHz  = get (ID::eqMidFreq);
    f.eqHighDb = get (ID::eqHigh);
    f.comp     = get (ID::comp);
    f.width    = get (ID::width);
    f.delayMix = get (ID::delayMix);
    f.delayMs  = get (ID::delayTime);
    f.delayFb  = get (ID::delayFb);
    f.revMix   = get (ID::revMix);
    f.revSize  = get (ID::revSize);
    f.revDamp  = get (ID::revDamp);
    f.masterDb = get (ID::master);
}

} // namespace silkreed
