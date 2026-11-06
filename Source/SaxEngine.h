// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cstdint>
#include "SaxProfiles.h"

namespace silkreed
{

//==============================================================================
// Snapshot of every parameter the synthesis engine needs (filled once per block)
struct EngineParams
{
    int   instrument   = 0;     // 0 soprano, 1 alto, 2 tenor
    bool  foldToRange  = false;
    float tuneCents    = 0.0f;
    int   transpose    = 0;
    int   voiceMode    = 0;     // 0 mono legato, 1 mono retrigger, 2 poly

    // breath / dynamics
    int   breathSource = 0;     // 0 vel, 1 CC2, 2 CC11, 3 aftertouch, 4 vel x CC11
    float breathCurve  = 0.0f;  // -1..1
    float breathSens   = 1.0f;  // 0..2
    float dynRange     = 0.7f;  // 0..1 timbre change with dynamics
    float attackMs     = 30.0f;
    float releaseMs    = 180.0f;
    float swell        = 0.3f;
    float swellMs      = 900.0f;

    // tone
    float brightness   = 0.55f;
    float reed         = 0.35f;
    float warmth       = 0.4f;
    float body         = 0.7f;
    float formantShift = 0.0f;  // -1..1
    float air          = 0.35f;
    float airTone      = 0.5f;
    float subtone      = 0.0f;

    // vibrato
    float vibRate      = 5.6f;
    float vibDepth     = 25.0f; // cents
    float vibDelayMs   = 300.0f;
    float vibFadeMs    = 600.0f;
    float vibModDepth  = 40.0f; // cents at full mod wheel
    float vibAmp       = 0.35f;
    float vibRandom    = 0.3f;

    // articulation
    float glideMs      = 50.0f;
    int   glideMode    = 0;     // 0 legato only, 1 always
    float scoop        = 0.3f;
    float scoopMs      = 90.0f;
    float tongue       = 0.4f;
    float fall         = 0.0f;
    int   pbRange      = 2;

    // character
    float growl        = 0.0f;
    float growlRate    = 55.0f;
    float humanize     = 0.35f;
};

//==============================================================================
// Live MIDI controller state shared by all voices
struct ExpressionState
{
    float pitchBend  = 0.0f;   // -1..1
    float modWheel   = 0.0f;   // 0..1
    float breathCC   = 0.0f;   // 0..1 (CC2)
    float exprCC     = 1.0f;   // 0..1 (CC11)
    float aftertouch = 0.0f;   // 0..1
};

//==============================================================================
// Slowly wandering random value in -1..1 (cosine-interpolated random targets)
struct SmoothRandom
{
    float from = 0.0f, to = 0.0f, pos = 1.0f, step = 0.01f;
    uint32_t seed = 12345u;

    float nextRand()
    {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        return (float) (seed & 0xffffff) / (float) 0x7fffff - 1.0f;
    }
    void init (uint32_t s) { seed = s ? s : 1u; from = 0; to = nextRand(); pos = 0; }
    float tick (float dt, float meanPeriod)
    {
        pos += step;
        if (pos >= 1.0f)
        {
            pos = 0.0f; from = to; to = nextRand();
            const float period = meanPeriod * (0.6f + 0.8f * (0.5f + 0.5f * nextRand()));
            step = dt / juce::jmax (0.01f, period);
        }
        const float w = 0.5f - 0.5f * std::cos (pos * juce::MathConstants<float>::pi);
        return from + (to - from) * w;
    }
};

//==============================================================================
// Topology-preserving state variable filter (Simper)
struct SVF
{
    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
    void set (float freq, float q, float sr)
    {
        freq = juce::jlimit (20.0f, sr * 0.45f, freq);
        g = std::tan (juce::MathConstants<float>::pi * freq / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }
    inline float bandpass (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        return v1;
    }
    inline float lowpass (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        return v2;
    }
};

//==============================================================================
// Pre-computed static spectral envelope (formants, warmth, body) on a log-frequency grid
class SpectralShape
{
public:
    static constexpr int kBins = 512;
    static constexpr float kMinHz = 40.0f, kMaxHz = 22000.0f;

    void rebuild (const SaxProfile& prof, float body, float formantShift, float warmth);
    inline float gainAt (float hz) const
    {
        const float x = (std::log2 (juce::jmax (kMinHz, hz)) - logMin) * binScale;
        const int i = juce::jlimit (0, kBins - 2, (int) x);
        const float f = juce::jlimit (0.0f, 1.0f, x - (float) i);
        return table[(size_t) i] + (table[(size_t) i + 1] - table[(size_t) i]) * f;
    }

private:
    std::array<float, kBins> table {};
    float logMin = std::log2 (kMinHz);
    float binScale = (float) (kBins - 1) / (std::log2 (kMaxHz) - std::log2 (kMinHz));
};

//==============================================================================
class SaxVoice
{
public:
    static constexpr int kMaxHarmonics = 64;
    static constexpr int kCtrlInterval = 16;

    void prepare (double sampleRate, uint32_t seed);
    void reset();

    // Starts a tongued note. glideFromNote < 0 -> no glide.
    void noteOn (float note, float velocity, float glideFromNote);
    // Changes pitch without re-articulating (slur)
    void legatoTo (float note, float velocity);
    void noteOff();
    void kill() { active = false; env = 0.0f; }

    bool  isActive() const      { return active; }
    bool  isGateOn() const      { return gate; }
    float getCurrentNote() const { return currentNote; }
    float getTargetNote() const  { return targetNote; }
    float getLevel() const      { return env; }
    int   getAge() const        { return age; }
    float getBreathDisplay() const { return breathSmooth * env; }

    void render (float* out, int numSamples, const EngineParams& p, const ExpressionState& ex,
                 const SpectralShape& shape, const SaxProfile& prof);

private:
    void updateControl (const EngineParams& p, const ExpressionState& ex,
                        const SpectralShape& shape, const SaxProfile& prof);
    float computeBreath (const EngineParams& p, const ExpressionState& ex) const;
    inline float white()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) (int32_t) rng * (1.0f / 2147483648.0f);
    }

    double sr = 48000.0;
    float ctrlDt = 16.0f / 48000.0f;
    uint32_t rng = 1;

    // state
    bool active = false, gate = false;
    int age = 0;
    float velocity = 0.8f;
    float currentNote = 60.0f, targetNote = 60.0f;
    float env = 0.0f;
    float breathSmooth = 0.0f;
    float noteTime = 0.0f;      // seconds since articulation
    float vibTime = 0.0f;       // seconds since last pitch change (vibrato onset)
    float releaseTime = 0.0f;
    float bloom = 1.0f;
    float scoopCents = 0.0f, scoopStart = 0.0f;
    float tongueEnv = 0.0f, legatoDip = 0.0f;
    float vibPhase = 0.0f, growlPhase = 0.0f;
    float phase = 0.0f;         // 0..1 fundamental phase
    float subPhase = 0.0f;
    float inc = 0.0f, incStep = 0.0f;

    bool freshNote = false, instantStart = false, slurPending = false;
    float tongueGain = 0.0f;
    int ctrlCounter = 0;
    int numActiveHarm = 0, prevHarm = 0;
    std::array<float, kMaxHarmonics> amp {}, ampStep {}, phaseOff {};
    std::array<SmoothRandom, kMaxHarmonics> harmJitter;
    SmoothRandom driftRnd, breathRnd, vibRnd, growlRnd;

    // per-sample ramps
    float harmGain = 0, harmGainStep = 0;
    float noiseGain = 0, noiseGainStep = 0;
    float drive = 0.3f, driveStep = 0;
    float growlDepth = 0, growlInc = 0;
    float subGain = 0, subGainStep = 0;

    SVF nzF1, nzF2, airF, airLP;
};

//==============================================================================
// Polyphony / note-priority management + MIDI handling for the voices
class SaxEngine
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare (double sampleRate, int maxBlock);
    void reset();

    // Renders mono output, consuming MIDI from the buffer sample-accurately
    void process (float* mono, int numSamples, const juce::MidiBuffer& midi, const EngineParams& p);

    float getBreathMeter() const { return breathMeter.load(); }
    int   getActiveVoiceCount() const;

private:
    void handleMidi (const juce::MidiMessage& m, const EngineParams& p);
    void noteOn (int note, float vel, const EngineParams& p);
    void noteOff (int note, const EngineParams& p);
    void releaseAll();
    float mapNote (int note, const EngineParams& p) const;
    void renderVoices (float* out, int n, const EngineParams& p);

    double sr = 48000.0;
    std::array<SaxVoice, kMaxVoices> voices;
    std::array<int, kMaxVoices> voiceNote {};   // MIDI note each voice is playing
    ExpressionState expr;
    SpectralShape shape;
    int shapeInstrument = -1;
    float shapeBody = -1, shapeShift = -99, shapeWarmth = -1;

    // mono note stack (last-note priority)
    std::array<int, 128> stack {};
    std::array<float, 128> stackVel {};
    int stackSize = 0;
    float lastNote = -1.0f;
    bool sustain = false;
    std::array<bool, 128> sustained {};
    int lastVoiceMode = -1;

    std::vector<float> scratch;
    std::atomic<float> breathMeter { 0.0f };
};

} // namespace silkreed
