// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "SaxEngine.h"
#include <cmath>

namespace silkreed
{

//==============================================================================
namespace
{
    constexpr int kSinSize = 4096;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    struct SinTable
    {
        float t[kSinSize + 1];
        SinTable()
        {
            for (int i = 0; i <= kSinSize; ++i)
                t[i] = std::sin (twoPi * (float) i / (float) kSinSize);
        }
    };
    const SinTable& sinTable() { static const SinTable s; return s; }

    // phase in [0,1)
    inline float sinLUT (const float* tab, float ph)
    {
        const float x = ph * (float) kSinSize;
        const int i = (int) x;
        const float f = x - (float) i;
        return tab[i] + (tab[i + 1] - tab[i]) * f;
    }

    inline float wrap01 (float x) { return x - std::floor (x); }

    inline float fastTanh (float x)
    {
        x = juce::jlimit (-3.0f, 3.0f, x);
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }

    inline float smoothstep (float x)
    {
        x = juce::jlimit (0.0f, 1.0f, x);
        return x * x * (3.0f - 2.0f * x);
    }

    inline float onePoleCoef (float dt, float tauSeconds)
    {
        return 1.0f - std::exp (-dt / juce::jmax (1.0e-5f, tauSeconds));
    }

    struct LogKTable
    {
        float v[SaxVoice::kMaxHarmonics + 1];
        LogKTable() { for (int k = 0; k <= SaxVoice::kMaxHarmonics; ++k) v[k] = std::log ((float) juce::jmax (1, k)); }
    };
    const LogKTable& logK() { static const LogKTable t; return t; }
}

//==============================================================================
void SpectralShape::rebuild (const SaxProfile& prof, float body, float formantShift, float warmth)
{
    const float shift = std::pow (2.0f, formantShift * 0.4f);   // +-0.4 octave
    for (int i = 0; i < kBins; ++i)
    {
        const float hz = std::pow (2.0f, logMin + (float) i / binScale);
        float db = 0.0f;

        for (const auto& f : prof.formants)
        {
            const float d = std::log2 (hz / (f.freq * shift)) / f.widthOct;
            db += body * f.gainDb * std::exp (-0.5f * d * d);
        }

        // Warmth: low-register body boost and a softened upper-mid "edge" region
        const float lowRef = 480.0f * shift * (prof.cutoffScale > 1.2f ? 1.4f : (prof.cutoffScale < 0.9f ? 0.75f : 1.0f));
        db += warmth * 6.0f / (1.0f + (hz / lowRef) * (hz / lowRef));
        const float e = std::log2 (hz / (3600.0f * shift)) / 0.6f;
        db -= warmth * 4.5f * std::exp (-0.5f * e * e);

        table[(size_t) i] = dbToGain (db);
    }
}

//==============================================================================
void SaxVoice::prepare (double sampleRate, uint32_t seed)
{
    sr = sampleRate;
    ctrlDt = (float) kCtrlInterval / (float) sr;
    rng = seed * 2654435761u + 1u;

    // Fixed (instrument-wide) harmonic phase offsets -> consistent waveform per voice
    uint32_t ps = 0x9E3779B9u;
    for (int k = 0; k < kMaxHarmonics; ++k)
    {
        ps ^= ps << 13; ps ^= ps >> 17; ps ^= ps << 5;
        phaseOff[(size_t) k] = (k == 0) ? 0.0f : (float) (ps & 0xffff) / 65536.0f;
        harmJitter[(size_t) k].init (seed * 977u + (uint32_t) k * 7919u + 3u);
    }
    driftRnd.init (seed * 31u + 101u);
    breathRnd.init (seed * 37u + 211u);
    vibRnd.init (seed * 41u + 307u);
    growlRnd.init (seed * 43u + 401u);
    reset();
}

void SaxVoice::reset()
{
    active = gate = false;
    env = 0; breathSmooth = 0;
    amp.fill (0); ampStep.fill (0);
    numActiveHarm = 0;
    harmGain = noiseGain = subGain = 0;
    harmGainStep = noiseGainStep = subGainStep = driveStep = 0;
    ctrlCounter = 0;
    nzF1.reset(); nzF2.reset(); airF.reset(); airLP.reset();
}

void SaxVoice::noteOn (float note, float vel, float glideFromNote)
{
    const bool wasSilent = ! active || env < 0.002f;
    active = true;
    gate = true;
    velocity = vel;
    targetNote = note;
    currentNote = (glideFromNote >= 0.0f) ? glideFromNote : note;
    noteTime = vibTime = releaseTime = 0.0f;
    bloom = 0.3f;
    freshNote = true;
    instantStart = wasSilent;
    vibPhase = 0.0f;
    if (wasSilent)
    {
        env = 0.0f;
        breathSmooth = 0.0f;
        phase = 0.0f; subPhase = 0.0f;
        nzF1.reset(); nzF2.reset(); airF.reset();
        ctrlCounter = 0;    // force control update on the first sample
    }
    ++age;
}

void SaxVoice::legatoTo (float note, float vel)
{
    if (! active) { noteOn (note, vel, -1.0f); return; }
    gate = true;
    velocity = vel;
    targetNote = note;
    vibTime = 0.0f;
    releaseTime = 0.0f;
    slurPending = true;
    bloom = juce::jmin (bloom, 0.85f);
    ++age;
}

void SaxVoice::noteOff()
{
    gate = false;
    releaseTime = 0.0f;
}

//==============================================================================
float SaxVoice::computeBreath (const EngineParams& p, const ExpressionState& ex) const
{
    float raw = velocity;
    switch (p.breathSource)
    {
        case 1:  raw = ex.breathCC; break;
        case 2:  raw = ex.exprCC; break;
        case 3:  raw = juce::jmax (ex.aftertouch, velocity * 0.45f); break;
        case 4:  raw = velocity * ex.exprCC; break;
        default: raw = velocity; break;
    }

    float x = juce::jlimit (0.0f, 1.0f, raw * p.breathSens);
    x = std::pow (x, std::pow (2.0f, -p.breathCurve * 1.3f));

    // Auto-swell for non-continuous sources: long notes bloom like a real player
    if (p.breathSource == 0 || p.breathSource == 3 || p.breathSource == 4)
    {
        const float s = p.swell * 0.4f;
        x *= (1.0f - s) + s * smoothstep (noteTime * 1000.0f / juce::jmax (50.0f, p.swellMs));
    }
    return x;
}

//==============================================================================
void SaxVoice::updateControl (const EngineParams& p, const ExpressionState& ex,
                              const SpectralShape& shape, const SaxProfile& prof)
{
    const float dt = ctrlDt;
    noteTime += dt;
    vibTime += dt;

    if (freshNote)
    {
        freshNote = false;
        scoopStart = -p.scoop * (55.0f + 90.0f * (1.0f - velocity));
        tongueEnv = p.tongue * (0.35f + 0.65f * velocity);
        legatoDip = 0.0f;
    }
    if (slurPending)
    {
        slurPending = false;
        legatoDip = 0.06f + 0.12f * p.tongue;
        tongueEnv = juce::jmax (tongueEnv, p.tongue * 0.12f);
    }

    // ---- breath & envelope -------------------------------------------------
    float B = computeBreath (p, ex);
    B *= 1.0f + p.humanize * 0.05f * breathRnd.tick (dt, 0.35f);
    breathSmooth += (B - breathSmooth) * onePoleCoef (dt, 0.008f);
    if (instantStart && p.breathSource == 0) breathSmooth = B;

    if (gate)
        env += (1.0f - env) * onePoleCoef (dt, p.attackMs * 0.001f / 3.0f);
    else
    {
        env += (0.0f - env) * onePoleCoef (dt, p.releaseMs * 0.001f / 3.0f);
        releaseTime += dt;
    }
    bloom += (1.0f - bloom) * onePoleCoef (dt, 0.02f + p.attackMs * 0.0006f);
    tongueEnv *= std::exp (-dt / 0.018f);
    legatoDip *= std::exp (-dt / 0.035f);

    if (! gate && env < 1.0e-4f)
    {
        active = false;
        return;
    }

    const float d = juce::jlimit (0.0f, 1.0f, breathSmooth);
    const float level = env * std::pow (d, 1.25f);

    // ---- pitch ---------------------------------------------------------------
    if (p.glideMs > 1.0f)
        currentNote += (targetNote - currentNote) * onePoleCoef (dt, p.glideMs * 0.001f / 3.0f);
    else
        currentNote = targetNote;

    const float sNorm = juce::jlimit (0.0f, 1.0f, noteTime * 1000.0f / juce::jmax (5.0f, p.scoopMs));
    scoopCents = scoopStart * (1.0f - sNorm) * (1.0f - sNorm);

    const float vibOnset = smoothstep ((vibTime * 1000.0f - p.vibDelayMs) / juce::jmax (1.0f, p.vibFadeMs));
    const float rateVar = 1.0f + p.vibRandom * 0.12f * vibRnd.tick (dt, 0.8f);
    vibPhase = wrap01 (vibPhase + p.vibRate * rateVar * dt);
    const float vibSin = std::sin (twoPi * vibPhase);
    const float vibDepthC = (p.vibDepth * vibOnset + ex.modWheel * p.vibModDepth) * (1.0f + p.vibRandom * 0.1f * vibRnd.from);
    const float vibCents = vibDepthC * vibSin;
    const float vibNorm = juce::jlimit (0.0f, 1.5f, vibDepthC / 35.0f);

    const float driftCents = p.humanize * 6.0f * driftRnd.tick (dt, 1.3f);

    float fallCents = 0.0f;
    if (! gate && p.fall > 0.0f)
    {
        const float r = juce::jmin (1.0f, releaseTime / juce::jmax (0.05f, p.releaseMs * 0.001f));
        fallCents = -p.fall * 320.0f * std::pow (r, 1.6f);
    }

    const float growlRateHz = p.growlRate * (1.0f + 0.15f * growlRnd.tick (dt, 0.4f));
    growlInc = growlRateHz / (float) sr;
    const float growlFm = p.growl * 10.0f * std::sin (twoPi * growlPhase);

    const float note = currentNote + ex.pitchBend * (float) p.pbRange
                     + (p.tuneCents + scoopCents + vibCents + driftCents + fallCents + growlFm) * 0.01f;
    float f0 = 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
    f0 = juce::jlimit (20.0f, (float) sr * 0.45f, f0);

    const float newInc = f0 / (float) sr;
    if (instantStart) { inc = newInc; incStep = 0.0f; }
    else incStep = (newInc - inc) / (float) kCtrlInterval;

    // ---- timbre --------------------------------------------------------------
    float t = p.brightness + p.dynRange * (d - 0.6f) * 0.9f + p.reed * 0.12f - p.subtone * 0.45f;
    t += p.vibAmp * 0.06f * vibNorm * vibSin;
    t = juce::jlimit (0.0f, 1.0f, t) * (0.3f + 0.7f * bloom);

    const float slope = juce::jmax (0.25f, 1.85f - 1.35f * t - 0.25f * p.reed);
    const float fc = prof.cutoffScale * (650.0f + 6200.0f * std::pow (t, 1.4f));
    const float maxF = juce::jmin ((float) sr * 0.45f, 18000.0f);
    const int N = juce::jlimit (1, kMaxHarmonics, (int) (maxF / f0));

    const float fundBoost = dbToGain (prof.fundamentalDb + p.subtone * 6.0f + p.warmth * 1.5f);
    const float secondBoost = dbToGain (prof.secondDb);
    const float taperStart = (float) sr * 0.36f, taperWidth = (float) sr * 0.09f;

    float target[kMaxHarmonics];
    float sumSq = 0.0f;
    const auto& lk = logK();
    for (int k = 1; k <= N; ++k)
    {
        const float fk = f0 * (float) k;
        float a = std::exp (-slope * lk.v[k]);
        const float x = fk / fc;
        const float x4 = x * x * x * x;
        a *= 1.0f / std::sqrt (1.0f + x4);
        a *= 1.0f / (1.0f + 0.6f * (1.0f - t) * x4);
        a *= shape.gainAt (fk);
        if (k == 1) a *= fundBoost;
        else if (k == 2) a *= secondBoost;
        a *= 1.0f + p.humanize * (0.08f + 0.25f * juce::jmin (1.0f, (float) k / 20.0f))
                    * harmJitter[(size_t) k - 1].tick (dt, 0.25f);
        if (fk > taperStart) a *= juce::jmax (0.0f, 1.0f - (fk - taperStart) / taperWidth);
        a = juce::jmax (0.0f, a);
        target[k - 1] = a;
        sumSq += a * a;
    }
    const float norm = sumSq > 1.0e-12f ? 1.0f / std::sqrt (sumSq) : 0.0f;
    const int activeCount = juce::jmax (N, numActiveHarm);
    const float invCtrl = 1.0f / (float) kCtrlInterval;
    for (int k = 0; k < activeCount; ++k)
    {
        const float tg = (k < N) ? target[k] * norm : 0.0f;
        if (instantStart) { amp[(size_t) k] = tg; ampStep[(size_t) k] = 0.0f; }
        else ampStep[(size_t) k] = (tg - amp[(size_t) k]) * invCtrl;
    }
    numActiveHarm = activeCount;
    prevHarm = N;

    // ---- gains ---------------------------------------------------------------
    const float ampVib = 1.0f + p.vibAmp * 0.22f * vibNorm * vibSin;
    const float trim = dbToGain (prof.levelTrimDb);   // per-instrument loudness balance
    const float hTarget = trim * level * ampVib * (1.0f - p.subtone * 0.35f) * (1.0f - legatoDip);
    const float nTarget = trim * p.air * prof.breathiness * env * (0.22f * std::pow (d, 0.6f) + 0.035f)
                        * (1.0f + 1.6f * p.subtone) * (1.0f + 1.2f * p.growl);
    const float dTarget = 0.15f + 0.85f * p.reed * std::pow (d, 1.2f) + 0.25f * p.growl;
    const float sTarget = p.growl * 0.10f * level;

    if (instantStart)
    {
        harmGain = hTarget; noiseGain = nTarget; drive = dTarget; subGain = sTarget;
        harmGainStep = noiseGainStep = driveStep = subGainStep = 0.0f;
    }
    else
    {
        harmGainStep = (hTarget - harmGain) * invCtrl;
        noiseGainStep = (nTarget - noiseGain) * invCtrl;
        driveStep = (dTarget - drive) * invCtrl;
        subGainStep = (sTarget - subGain) * invCtrl;
    }
    growlDepth = p.growl * (0.5f + 0.5f * d);
    tongueGain = tongueEnv * env * 1.4f;

    // ---- breath-noise filters --------------------------------------------------
    const float shift = std::pow (2.0f, p.formantShift * 0.4f);
    nzF1.set (prof.noiseF1 * shift, 2.5f, (float) sr);
    nzF2.set (prof.noiseF2 * shift, 3.0f, (float) sr);
    airF.set (prof.airHz * std::pow (2.0f, (p.airTone - 0.5f) * 2.4f), 0.9f, (float) sr);

    instantStart = false;
}

//==============================================================================
void SaxVoice::render (float* out, int numSamples, const EngineParams& p, const ExpressionState& ex,
                       const SpectralShape& shape, const SaxProfile& prof)
{
    const float* tab = sinTable().t;
    int i = 0;
    while (i < numSamples && active)
    {
        if (ctrlCounter <= 0)
        {
            updateControl (p, ex, shape, prof);
            ctrlCounter = kCtrlInterval;
            if (! active) break;
        }

        const int todo = juce::jmin (numSamples - i, ctrlCounter);
        const int nh = numActiveHarm;

        for (int s = 0; s < todo; ++s)
        {
            // Harmonic oscillator bank (phase-locked to the fundamental)
            float h = 0.0f;
            for (int k = 0; k < nh; ++k)
            {
                float ph = phase * (float) (k + 1) + phaseOff[(size_t) k];
                ph -= (float) (int) ph;
                h += amp[(size_t) k] * sinLUT (tab, ph);
                amp[(size_t) k] += ampStep[(size_t) k];
            }

            // Growl: AM roughness + sub-octave component
            growlPhase += growlInc; if (growlPhase >= 1.0f) growlPhase -= 1.0f;
            const float gs = sinLUT (tab, growlPhase);
            const float am = 1.0f + growlDepth * 0.55f * gs;
            subPhase += inc * 0.5f; if (subPhase >= 1.0f) subPhase -= 1.0f;
            const float sub = subGain * sinLUT (tab, subPhase);

            // Reed nonlinearity (unity gain for small signals)
            const float x = h * harmGain;
            const float g = 2.0f * drive;
            const float y = fastTanh (g * x) / g;

            // Breath noise: pitch-synchronous component + free "air" hiss
            const float nz = white();
            float cph = phase + 0.25f; if (cph >= 1.0f) cph -= 1.0f;
            const float sync = 0.6f + 0.4f * sinLUT (tab, cph);
            const float nzp = nz * sync;
            const float b1 = nzF1.bandpass (nzp);
            const float b2 = nzF2.bandpass (nzp);
            const float ba = airF.bandpass (nz);
            float n = (b1 * 0.9f + b2 * 0.7f + ba * 0.8f) * noiseGain * (1.0f + growlDepth * 0.8f * (0.5f + 0.5f * gs));
            n += (b2 + ba) * tongueGain;

            out[i + s] += (y * am + sub) + n;

            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            inc += incStep;
            harmGain += harmGainStep; noiseGain += noiseGainStep;
            drive += driveStep; subGain += subGainStep;
        }

        i += todo;
        ctrlCounter -= todo;
    }
}

//==============================================================================
void SaxEngine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    for (int v = 0; v < kMaxVoices; ++v)
        voices[(size_t) v].prepare (sr, (uint32_t) (v + 1) * 7777u);
    scratch.assign ((size_t) juce::jmax (1, maxBlock), 0.0f);
    reset();
}

void SaxEngine::reset()
{
    for (auto& v : voices) v.reset();
    voiceNote.fill (-1);
    stackSize = 0;
    lastNote = -1.0f;
    sustain = false;
    sustained.fill (false);
    expr = {};
}

int SaxEngine::getActiveVoiceCount() const
{
    int n = 0;
    for (auto& v : voices) n += v.isActive() ? 1 : 0;
    return n;
}

float SaxEngine::mapNote (int note, const EngineParams& p) const
{
    int n = note + p.transpose;
    if (p.foldToRange)
    {
        const auto& prof = getProfile (p.instrument);
        while (n < prof.lowNote) n += 12;
        while (n > prof.highNote) n -= 12;
    }
    return (float) n;
}

void SaxEngine::releaseAll()
{
    for (auto& v : voices) v.noteOff();
    stackSize = 0;
    sustained.fill (false);
}

void SaxEngine::noteOn (int note, float vel, const EngineParams& p)
{
    const float mapped = mapNote (note, p);

    if (p.voiceMode == 2)   // ---- poly
    {
        int best = -1;
        for (int v = 0; v < kMaxVoices; ++v)
            if (! voices[(size_t) v].isActive()) { best = v; break; }
        if (best < 0)   // steal: released voice with lowest level, else oldest
        {
            float lowest = 1e9f;
            for (int v = 0; v < kMaxVoices; ++v)
                if (! voices[(size_t) v].isGateOn() && voices[(size_t) v].getLevel() < lowest)
                { lowest = voices[(size_t) v].getLevel(); best = v; }
            if (best < 0)
            {
                int oldest = std::numeric_limits<int>::max();
                for (int v = 0; v < kMaxVoices; ++v)
                    if (voices[(size_t) v].getAge() < oldest) { oldest = voices[(size_t) v].getAge(); best = v; }
            }
        }
        const float glideFrom = (p.glideMode == 1 && lastNote >= 0.0f && p.glideMs > 1.0f) ? lastNote : -1.0f;
        voices[(size_t) best].noteOn (mapped, vel, glideFrom);
        voiceNote[(size_t) best] = note;
        sustained[(size_t) note] = false;
        lastNote = mapped;
        return;
    }

    // ---- mono (last-note priority stack)
    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] == note)
        {
            for (int j = i; j < stackSize - 1; ++j) { stack[(size_t) j] = stack[(size_t) j + 1]; stackVel[(size_t) j] = stackVel[(size_t) j + 1]; }
            --stackSize;
            break;
        }
    if (stackSize < 128) { stack[(size_t) stackSize] = note; stackVel[(size_t) stackSize] = vel; ++stackSize; }

    auto& v = voices[0];
    const bool overlapping = v.isActive() && v.isGateOn();
    if (p.voiceMode == 0 && overlapping)
        v.legatoTo (mapped, vel);
    else
    {
        float glideFrom = -1.0f;
        if (p.glideMs > 1.0f && (overlapping || p.glideMode == 1))
            glideFrom = v.isActive() ? v.getCurrentNote() : lastNote;
        v.noteOn (mapped, vel, glideFrom);
    }
    voiceNote[0] = note;
    lastNote = mapped;
}

void SaxEngine::noteOff (int note, const EngineParams& p)
{
    if (p.voiceMode == 2)
    {
        for (int v = 0; v < kMaxVoices; ++v)
            if (voiceNote[(size_t) v] == note && voices[(size_t) v].isGateOn())
            {
                if (sustain) sustained[(size_t) note] = true;
                else { voices[(size_t) v].noteOff(); voiceNote[(size_t) v] = -1; }
            }
        return;
    }

    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] == note)
        {
            for (int j = i; j < stackSize - 1; ++j) { stack[(size_t) j] = stack[(size_t) j + 1]; stackVel[(size_t) j] = stackVel[(size_t) j + 1]; }
            --stackSize;
            break;
        }

    auto& v = voices[0];
    if (voiceNote[0] != note) return;

    if (stackSize > 0)
    {
        const int top = stack[(size_t) stackSize - 1];
        const float topVel = stackVel[(size_t) stackSize - 1];
        const float mapped = mapNote (top, p);
        if (p.voiceMode == 0) v.legatoTo (mapped, topVel);
        else v.noteOn (mapped, topVel, p.glideMs > 1.0f ? v.getCurrentNote() : -1.0f);
        voiceNote[0] = top;
        lastNote = mapped;
    }
    else if (sustain)
        sustained[(size_t) note] = true;
    else
        v.noteOff();
}

void SaxEngine::handleMidi (const juce::MidiMessage& m, const EngineParams& p)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber(), p);
    else if (m.isPitchWheel())
        expr.pitchBend = juce::jlimit (-1.0f, 1.0f, (float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isChannelPressure())
        expr.aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())
        expr.aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float val = (float) m.getControllerValue() / 127.0f;
        switch (cc)
        {
            case 1:  expr.modWheel = val; break;
            case 2:  expr.breathCC = val; break;
            case 11: expr.exprCC = val; break;
            case 64:
            {
                const bool down = m.getControllerValue() >= 64;
                if (sustain && ! down)
                {
                    if (p.voiceMode == 2)
                    {
                        for (int v = 0; v < kMaxVoices; ++v)
                        {
                            const int n = voiceNote[(size_t) v];
                            if (n >= 0 && sustained[(size_t) n]) { voices[(size_t) v].noteOff(); voiceNote[(size_t) v] = -1; }
                        }
                    }
                    else if (stackSize == 0)
                        voices[0].noteOff();
                    sustained.fill (false);
                }
                sustain = down;
                break;
            }
            case 120: for (auto& v : voices) v.kill(); stackSize = 0; break;   // all sound off
            case 123: releaseAll(); break;                                     // all notes off
            default: break;
        }
    }
    else if (m.isAllNotesOff())
        releaseAll();
}

void SaxEngine::renderVoices (float* out, int n, const EngineParams& p)
{
    const auto& prof = getProfile (p.instrument);
    const int count = (p.voiceMode == 2) ? kMaxVoices : 1;
    for (int v = 0; v < count; ++v)
        if (voices[(size_t) v].isActive())
            voices[(size_t) v].render (out, n, p, expr, shape, prof);
}

void SaxEngine::process (float* mono, int numSamples, const juce::MidiBuffer& midi, const EngineParams& p)
{
    if (p.voiceMode != lastVoiceMode)
    {
        if (lastVoiceMode >= 0)
        {
            for (auto& v : voices) v.noteOff();
            stackSize = 0;
            voiceNote.fill (-1);
        }
        lastVoiceMode = p.voiceMode;
    }

    if (p.instrument != shapeInstrument || std::abs (p.body - shapeBody) > 1e-4f
        || std::abs (p.formantShift - shapeShift) > 1e-4f || std::abs (p.warmth - shapeWarmth) > 1e-4f)
    {
        shape.rebuild (getProfile (p.instrument), p.body, p.formantShift, p.warmth);
        shapeInstrument = p.instrument; shapeBody = p.body; shapeShift = p.formantShift; shapeWarmth = p.warmth;
    }

    juce::FloatVectorOperations::clear (mono, numSamples);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos)
        {
            renderVoices (mono + pos, evPos - pos, p);
            pos = evPos;
        }
        handleMidi (meta.getMessage(), p);
    }
    if (pos < numSamples)
        renderVoices (mono + pos, numSamples - pos, p);

    float meter = 0.0f;
    for (auto& v : voices)
        if (v.isActive()) meter = juce::jmax (meter, v.getBreathDisplay());
    breathMeter.store (meter);
}

} // namespace silkreed
