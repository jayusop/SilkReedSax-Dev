// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace silkreed
{

namespace Colours
{
    const juce::Colour bgTop      { 0xff0c1222 };
    const juce::Colour bgBottom   { 0xff070a14 };
    const juce::Colour panel      { 0xff141b2d };
    const juce::Colour panelEdge  { 0xff2a3452 };
    const juce::Colour gold       { 0xffe6b450 };
    const juce::Colour goldLight  { 0xfffbe2a0 };
    const juce::Colour goldDark   { 0xff9a6a1e };
    const juce::Colour text       { 0xffece6d6 };
    const juce::Colour textDim    { 0xff8d96ad };
    const juce::Colour knobTop    { 0xff2e3750 };
    const juce::Colour knobBottom { 0xff0f1320 };
    const juce::Colour track      { 0xff080b13 };
}

class SilkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SilkLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

} // namespace silkreed
