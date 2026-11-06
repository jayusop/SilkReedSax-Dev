// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

// Acoustic profiles for the three saxophones.
// All note numbers are CONCERT pitch (sounding), MIDI numbering (60 = C4).
// Formant data is a stylised fit of published saxophone radiation spectra:
// a main bore/bell resonance, the tone-hole lattice peak, and upper "edge" peaks.

namespace silkreed
{

struct Formant
{
    float freq;      // Hz
    float gainDb;    // peak gain
    float widthOct;  // gaussian width in octaves (sigma)
};

struct SaxProfile
{
    const char* name;
    int lowNote;          // lowest sounding note (normal range)
    int highNote;         // highest sounding note incl. common altissimo
    Formant formants[5];
    float cutoffScale;    // scales the dynamic spectral cutoff (smaller horn -> higher)
    float fundamentalDb;  // tweak for the 1st harmonic
    float secondDb;       // tweak for the 2nd harmonic
    float noiseF1, noiseF2;   // resonances that colour the breath noise
    float airHz;          // centre of the "air" hiss band at airTone = 0.5
    float levelTrimDb;
    float breathiness;    // inherent noise amount
};

enum class SaxType { Soprano = 0, Alto = 1, Tenor = 2 };

inline const SaxProfile& getProfile (int type)
{
    static const SaxProfile profiles[3] =
    {
        // Soprano (Bb) — written Bb3..F6 -> sounding Ab3 (56) .. Eb6 (87), altissimo to ~A6
        // Small conical bore: bright, focused, slightly nasal upper resonance.
        { "Soprano", 56, 93,
          { { 900.0f, 6.0f, 0.40f }, { 1800.0f, 3.0f, 0.30f }, { 3100.0f, 8.5f, 0.28f },
            { 5200.0f, 5.0f, 0.30f }, { 8200.0f, -2.0f, 0.60f } },
          1.75f, -2.5f, 0.0f, 1300.0f, 3400.0f, 6800.0f, -1.5f, 0.90f },

        // Alto (Eb) — written Bb3..F6 -> sounding Db3 (49) .. Ab5 (80), altissimo to ~D6
        // Medium bore: singing mid-range "honk", rounder top end than the soprano.
        { "Alto", 49, 86,
          { { 620.0f, 7.0f, 0.45f }, { 1350.0f, 7.0f, 0.30f }, { 2600.0f, 2.5f, 0.30f },
            { 4000.0f, -1.0f, 0.40f }, { 6500.0f, -7.0f, 0.60f } },
          1.0f, 0.0f, 2.5f, 820.0f, 2300.0f, 4800.0f, 0.0f, 1.00f },

        // Tenor (Bb) — written Bb3..F6 -> sounding Ab2 (44) .. Eb5 (75), altissimo to ~A5
        // Large bore: deep, full body, darker top and more breath.
        { "Tenor", 44, 81,
          { { 420.0f, 9.5f, 0.55f }, { 950.0f, 4.5f, 0.40f }, { 1900.0f, 1.5f, 0.35f },
            { 3200.0f, -2.5f, 0.40f }, { 5500.0f, -9.0f, 0.60f } },
          0.60f, 3.5f, 2.0f, 520.0f, 1500.0f, 3600.0f, 1.0f, 1.30f },
    };

    if (type < 0) type = 0;
    if (type > 2) type = 2;
    return profiles[type];
}

} // namespace silkreed
