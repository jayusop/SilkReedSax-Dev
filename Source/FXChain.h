// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace silkreed
{

struct FXParams
{
    float eqLowDb = 0, eqMidDb = 0, eqMidHz = 1500, eqHighDb = 0;
    float comp = 0.3f;
    float width = 0.5f;
    float delayMix = 0.1f, delayMs = 380.0f, delayFb = 0.3f;
    float revMix = 0.25f, revSize = 0.6f, revDamp = 0.4f;
    float masterDb = 0.0f;
};

//==============================================================================
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
    void setLowShelf (float sr, float f, float db, float q = 0.707f);
    void setHighShelf (float sr, float f, float db, float q = 0.707f);
    void setPeak (float sr, float f, float db, float q);
    void setHighpass (float sr, float f, float q = 0.707f);
    void setLowpass (float sr, float f, float q = 0.707f);
};

//==============================================================================
// 8-line feedback delay network reverb with input diffusion and modulation
class FDNReverb
{
public:
    void prepare (double sr);
    void reset();
    void setParams (float size, float damp);
    void process (const float* inL, const float* inR, float* outL, float* outR, int n);

private:
    static constexpr int N = 8;
    struct Line
    {
        std::vector<float> buf; int w = 0; float len = 100;
        float lp = 0;
    };
    struct Allpass
    {
        std::vector<float> buf; int w = 0; int len = 100;
        inline float process (float x, float g)
        {
            const float d = buf[(size_t) w];
            const float y = -g * x + d;
            buf[(size_t) w] = x + g * y;
            if (++w >= len) w = 0;
            return y;
        }
    };
    double sr = 48000;
    std::array<Line, N> lines;
    std::array<Allpass, 4> diff;
    std::vector<float> pre; int preW = 0; int preLen = 1000;
    std::array<float, N> gains {}, baseLen {};
    float dampCoef = 0.3f;
    float lfo = 0;
    float curSize = -1, curDamp = -1;
};

//==============================================================================
class FXChain
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    // Input: mono dry signal. Output: stereo buffer (L,R) of numSamples.
    void process (const float* mono, float* outL, float* outR, int numSamples, const FXParams& p);

    float getOutputPeak() const { return outPeak; }

private:
    void updateFilters (const FXParams& p);

    double sr = 48000;
    Biquad hp, lowShelf, mid, highShelf, dcBlock;
    FXParams last; bool firstUpdate = true;

    // compressor
    float compEnv = 0, compGainSmoothed = 0;   // mean-square env, gain reduction in dB

    // early reflections (width)
    std::vector<float> erBuf; int erW = 0;
    Biquad erLpL, erLpR;

    // delay
    std::vector<float> dlL, dlR; int dlW = 0;
    float dlTimeSmoothed = 0;
    Biquad dlDampL, dlDampR, dlHpL, dlHpR;

    FDNReverb reverb;
    std::vector<float> tmpL, tmpR, revL, revR, monoProc;
    float masterSmoothed = 1;
    float outPeak = 0;
};

} // namespace silkreed
