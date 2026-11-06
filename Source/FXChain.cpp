// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "FXChain.h"
#include <cmath>

namespace silkreed
{

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;
    inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
}

//==============================================================================
// RBJ cookbook biquads
static void normalise (Biquad& f, float b0, float b1, float b2, float a0, float a1, float a2)
{
    f.b0 = b0 / a0; f.b1 = b1 / a0; f.b2 = b2 / a0; f.a1 = a1 / a0; f.a2 = a2 / a0;
}

void Biquad::setLowShelf (float sr, float f, float db, float q)
{
    const float A = std::pow (10.0f, db / 40.0f), w = 2 * pi * f / sr, c = std::cos (w), s = std::sin (w);
    const float alpha = s / (2 * q), sq = 2 * std::sqrt (A) * alpha;
    normalise (*this, A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
               (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
}

void Biquad::setHighShelf (float sr, float f, float db, float q)
{
    const float A = std::pow (10.0f, db / 40.0f), w = 2 * pi * f / sr, c = std::cos (w), s = std::sin (w);
    const float alpha = s / (2 * q), sq = 2 * std::sqrt (A) * alpha;
    normalise (*this, A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
               (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
}

void Biquad::setPeak (float sr, float f, float db, float q)
{
    const float A = std::pow (10.0f, db / 40.0f), w = 2 * pi * f / sr, c = std::cos (w), s = std::sin (w);
    const float alpha = s / (2 * q);
    normalise (*this, 1 + alpha * A, -2 * c, 1 - alpha * A, 1 + alpha / A, -2 * c, 1 - alpha / A);
}

void Biquad::setHighpass (float sr, float f, float q)
{
    const float w = 2 * pi * f / sr, c = std::cos (w), s = std::sin (w), alpha = s / (2 * q);
    normalise (*this, (1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
}

void Biquad::setLowpass (float sr, float f, float q)
{
    f = juce::jmin (f, sr * 0.45f);
    const float w = 2 * pi * f / sr, c = std::cos (w), s = std::sin (w), alpha = s / (2 * q);
    normalise (*this, (1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alpha, -2 * c, 1 - alpha);
}

//==============================================================================
void FDNReverb::prepare (double sampleRate)
{
    sr = sampleRate;
    const float scale = (float) (sr / 48000.0);
    const float ms[N] = { 31.7f, 37.9f, 41.3f, 46.1f, 53.9f, 59.3f, 67.1f, 73.7f };
    for (int i = 0; i < N; ++i)
    {
        baseLen[(size_t) i] = ms[i] * 0.001f * (float) sr;
        lines[(size_t) i].buf.assign ((size_t) (baseLen[(size_t) i] * 1.7f + 64), 0.0f);
    }
    const int ap[4] = { 142, 107, 379, 277 };
    for (int i = 0; i < 4; ++i)
    {
        diff[(size_t) i].len = juce::jmax (8, (int) ((float) ap[i] * scale * 1.09f));
        diff[(size_t) i].buf.assign ((size_t) diff[(size_t) i].len, 0.0f);
    }
    pre.assign ((size_t) (0.08 * sr) + 8, 0.0f);
    curSize = curDamp = -1;
    reset();
}

void FDNReverb::reset()
{
    for (auto& l : lines) { std::fill (l.buf.begin(), l.buf.end(), 0.0f); l.w = 0; l.lp = 0; }
    for (auto& a : diff) { std::fill (a.buf.begin(), a.buf.end(), 0.0f); a.w = 0; }
    std::fill (pre.begin(), pre.end(), 0.0f); preW = 0;
}

void FDNReverb::setParams (float size, float damp)
{
    if (std::abs (size - curSize) < 1e-4f && std::abs (damp - curDamp) < 1e-4f) return;
    curSize = size; curDamp = damp;
    const float lenScale = 0.55f + 1.05f * size;
    const float rt60 = 0.35f + 5.5f * size * size + 0.6f * size;
    for (int i = 0; i < N; ++i)
    {
        lines[(size_t) i].len = baseLen[(size_t) i] * lenScale;
        gains[(size_t) i] = std::pow (10.0f, -3.0f * lines[(size_t) i].len / (rt60 * (float) sr));
    }
    const float cutoff = 15000.0f * std::pow (0.12f, damp);   // 15k .. 1.8k
    dampCoef = 1.0f - std::exp (-2.0f * pi * cutoff / (float) sr);
    preLen = juce::jlimit (1, (int) pre.size() - 1, (int) ((0.008f + 0.03f * size) * (float) sr));
}

void FDNReverb::process (const float* inL, const float* inR, float* outL, float* outR, int n)
{
    const float lfoInc = 2.0f * pi * 0.37f / (float) sr;
    for (int s = 0; s < n; ++s)
    {
        // predelay
        pre[(size_t) preW] = 0.5f * (inL[s] + inR[s]);
        int r = preW - preLen; if (r < 0) r += (int) pre.size();
        float x = pre[(size_t) r];
        if (++preW >= (int) pre.size()) preW = 0;

        // input diffusion
        x = diff[0].process (x, 0.72f);
        x = diff[1].process (x, 0.72f);
        x = diff[2].process (x, 0.62f);
        x = diff[3].process (x, 0.62f);

        lfo += lfoInc; if (lfo > 2 * pi) lfo -= 2 * pi;

        float o[N];
        float sum = 0;
        for (int i = 0; i < N; ++i)
        {
            auto& L = lines[(size_t) i];
            const int size = (int) L.buf.size();
            const float mod = (i & 1 ? 1.0f : -1.0f) * 6.0f * std::sin (lfo + (float) i * 0.7f);
            float rp = (float) L.w - L.len - mod;
            while (rp < 0) rp += (float) size;
            const int i0 = (int) rp; const float fr = rp - (float) i0;
            const int i1 = (i0 + 1) % size;
            float v = L.buf[(size_t) i0] + (L.buf[(size_t) i1] - L.buf[(size_t) i0]) * fr;
            L.lp += (v - L.lp) * dampCoef;
            o[i] = L.lp * gains[(size_t) i];
            sum += o[i];
        }
        const float h = sum * (2.0f / N);   // Householder reflection
        float l = 0, rr = 0;
        for (int i = 0; i < N; ++i)
        {
            auto& L = lines[(size_t) i];
            const float in = x * ((i & 2) ? -0.35f : 0.35f);
            L.buf[(size_t) L.w] = o[i] - h + in;
            if (++L.w >= (int) L.buf.size()) L.w = 0;
            if (i & 1) rr += o[i] * ((i & 2) ? -1.0f : 1.0f);
            else       l  += o[i] * ((i & 4) ? -1.0f : 1.0f);
        }
        outL[s] = l * 0.5f;
        outR[s] = rr * 0.5f;
    }
}

//==============================================================================
void FXChain::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    const auto mb = (size_t) juce::jmax (1, maxBlock);
    tmpL.assign (mb, 0); tmpR.assign (mb, 0); revL.assign (mb, 0); revR.assign (mb, 0); monoProc.assign (mb, 0);
    erBuf.assign ((size_t) (0.05 * sr) + 8, 0.0f);
    dlL.assign ((size_t) (1.1 * sr) + 8, 0.0f);
    dlR.assign ((size_t) (1.1 * sr) + 8, 0.0f);
    reverb.prepare (sr);
    firstUpdate = true;
    reset();
}

void FXChain::reset()
{
    hp.reset(); lowShelf.reset(); mid.reset(); highShelf.reset(); dcBlock.reset();
    erLpL.reset(); erLpR.reset(); dlDampL.reset(); dlDampR.reset(); dlHpL.reset(); dlHpR.reset();
    std::fill (erBuf.begin(), erBuf.end(), 0.0f); erW = 0;
    std::fill (dlL.begin(), dlL.end(), 0.0f); std::fill (dlR.begin(), dlR.end(), 0.0f); dlW = 0;
    compEnv = 0; compGainSmoothed = 0;
    reverb.reset();
    dlTimeSmoothed = 0;
}

void FXChain::updateFilters (const FXParams& p)
{
    const float fs = (float) sr;
    if (firstUpdate)
    {
        hp.setHighpass (fs, 45.0f);
        erLpL.setLowpass (fs, 6500.0f); erLpR.setLowpass (fs, 6000.0f);
        dlDampL.setLowpass (fs, 4200.0f); dlDampR.setLowpass (fs, 3800.0f);
        dlHpL.setHighpass (fs, 180.0f); dlHpR.setHighpass (fs, 180.0f);
    }
    // exact comparison is intended: only recompute coefficients when a value actually changed
    auto changed = [] (float a, float b) { return ! juce::exactlyEqual (a, b); };
    if (firstUpdate || changed (p.eqLowDb, last.eqLowDb))   lowShelf.setLowShelf (fs, 220.0f, p.eqLowDb);
    if (firstUpdate || changed (p.eqMidDb, last.eqMidDb) || changed (p.eqMidHz, last.eqMidHz)) mid.setPeak (fs, p.eqMidHz, p.eqMidDb, 0.9f);
    if (firstUpdate || changed (p.eqHighDb, last.eqHighDb)) highShelf.setHighShelf (fs, 5500.0f, p.eqHighDb);
    firstUpdate = false;
    last = p;
}

void FXChain::process (const float* mono, float* outL, float* outR, int n, const FXParams& p)
{
    updateFilters (p);
    reverb.setParams (p.revSize, p.revDamp);
    const float fs = (float) sr;

    // ---- EQ + compressor (mono) ---------------------------------------------------
    const float thrDb = -10.0f - 14.0f * p.comp;
    const float ratio = 1.5f + 3.0f * p.comp;
    const float slopeK = 1.0f - 1.0f / ratio;
    // auto makeup: compensate ~80% of the static gain reduction at a nominal -12 dBFS RMS level
    const float makeup = dbToGain (0.8f * juce::jmax (0.0f, -12.0f - thrDb) * slopeK);
    const float rmsCoef = 1.0f - std::exp (-1.0f / (0.012f * fs));
    const float atk = 1.0f - std::exp (-1.0f / (0.003f * fs));
    const float rel = 1.0f - std::exp (-1.0f / (0.160f * fs));

    for (int i = 0; i < n; ++i)
    {
        float x = hp.process (mono[i]);
        x = lowShelf.process (x);
        x = mid.process (x);
        x = highShelf.process (x);

        if (p.comp > 0.001f)
        {
            compEnv += (x * x - compEnv) * rmsCoef;                 // mean-square detector
            const float envDb = 10.0f * std::log10 (compEnv * 2.0f + 1e-12f);   // sine-RMS referenced
            const float knee = 8.0f;
            const float over = envDb - thrDb;
            float gr = 0.0f;
            if (over > knee * 0.5f) gr = over * slopeK;
            else if (over > -knee * 0.5f) { const float t = over + knee * 0.5f; gr = slopeK * t * t / (2.0f * knee); }
            // smooth gain reduction (dB domain): fast attack, slow release
            compGainSmoothed += (gr - compGainSmoothed) * (gr > compGainSmoothed ? atk : rel);
            x *= dbToGain (-compGainSmoothed) * makeup;
        }
        monoProc[(size_t) i] = x;
    }

    // ---- early reflections -> stereo image -----------------------------------------
    static constexpr float erMsL[5] = { 7.3f, 12.9f, 19.1f, 26.7f, 33.1f };
    static constexpr float erMsR[5] = { 8.9f, 15.7f, 21.3f, 29.3f, 37.7f };
    static constexpr float erG[5]   = { 0.42f, 0.33f, 0.26f, 0.19f, 0.14f };
    const int erSize = (int) erBuf.size();
    const float erAmt = p.width * 0.75f;
    for (int i = 0; i < n; ++i)
    {
        const float x = monoProc[(size_t) i];
        erBuf[(size_t) erW] = x;
        float l = 0, r = 0;
        for (int k = 0; k < 5; ++k)
        {
            int il = erW - (int) (erMsL[k] * 0.001f * fs); if (il < 0) il += erSize;
            int ir = erW - (int) (erMsR[k] * 0.001f * fs); if (ir < 0) ir += erSize;
            l += erBuf[(size_t) il] * erG[k] * ((k & 1) ? -1.0f : 1.0f);
            r += erBuf[(size_t) ir] * erG[k] * ((k & 1) ? 1.0f : -1.0f);
        }
        if (++erW >= erSize) erW = 0;
        l = erLpL.process (l); r = erLpR.process (r);
        tmpL[(size_t) i] = x + erAmt * l;
        tmpR[(size_t) i] = x + erAmt * r;
    }

    // ---- stereo delay (cross-fed / ping-pong) ---------------------------------------
    const int dSize = (int) dlL.size();
    const float targetSamples = juce::jlimit (1.0f, (float) dSize - 4.0f, p.delayMs * 0.001f * fs);
    if (dlTimeSmoothed <= 0) dlTimeSmoothed = targetSamples;
    const float tSm = 1.0f - std::exp (-1.0f / (0.08f * fs));
    for (int i = 0; i < n; ++i)
    {
        dlTimeSmoothed += (targetSamples - dlTimeSmoothed) * tSm;
        float rp = (float) dlW - dlTimeSmoothed; while (rp < 0) rp += (float) dSize;
        const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % dSize;
        const float dL = dlL[(size_t) i0] + (dlL[(size_t) i1] - dlL[(size_t) i0]) * fr;
        const float dR = dlR[(size_t) i0] + (dlR[(size_t) i1] - dlR[(size_t) i0]) * fr;
        const float in = 0.5f * (tmpL[(size_t) i] + tmpR[(size_t) i]);
        // ping-pong: input enters left, the repeats alternate sides
        dlL[(size_t) dlW] = dlHpL.process (dlDampL.process (in + dR * p.delayFb));
        dlR[(size_t) dlW] = dlHpR.process (dlDampR.process (dL * p.delayFb));
        if (++dlW >= dSize) dlW = 0;
        tmpL[(size_t) i] += p.delayMix * dL;
        tmpR[(size_t) i] += p.delayMix * dR;
    }

    // ---- reverb (parallel send) -------------------------------------------------------
    reverb.process (tmpL.data(), tmpR.data(), revL.data(), revR.data(), n);

    // ---- mix + master + safety limiter -------------------------------------------------
    const float masterT = dbToGain (p.masterDb);
    const float mSm = 1.0f - std::exp (-1.0f / (0.02f * fs));
    const float dryG = 1.0f - 0.35f * p.revMix;
    const float wetG = 1.1f * p.revMix;
    float peak = outPeak * 0.92f;
    for (int i = 0; i < n; ++i)
    {
        masterSmoothed += (masterT - masterSmoothed) * mSm;
        float l = (tmpL[(size_t) i] * dryG + revL[(size_t) i] * wetG) * masterSmoothed;
        float r = (tmpR[(size_t) i] * dryG + revR[(size_t) i] * wetG) * masterSmoothed;
        auto soft = [] (float v)
        {
            const float a = std::abs (v);
            if (a <= 0.85f) return v;
            const float over = a - 0.85f;
            const float lim = 0.85f + 0.15f * (over / (over + 0.15f));
            return v < 0 ? -lim : lim;
        };
        l = soft (l); r = soft (r);
        outL[i] = l; outR[i] = r;
        peak = juce::jmax (peak, std::abs (l), std::abs (r));
    }
    outPeak = peak;
}

} // namespace silkreed
