// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SilkLookAndFeel.h"

namespace silkreed
{

//==============================================================================
class KnobCell : public juce::Component
{
public:
    KnobCell (juce::AudioProcessorValueTreeState& s, const char* paramId, const juce::String& title,
              const juce::String& tooltip, bool bipolar = false);
    void resized() override;

    juce::Slider slider;
    juce::Label label;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ChoiceCell : public juce::Component
{
public:
    ChoiceCell (juce::AudioProcessorValueTreeState& s, const char* paramId, const juce::String& title,
                const juce::String& tooltip);
    void resized() override;

    juce::ComboBox box;
    juce::Label label;

private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class Panel : public juce::Component
{
public:
    Panel (const juce::String& panelTitle, int columns) : title (panelTitle), cols (columns) {}
    void add (std::unique_ptr<juce::Component> c) { addAndMakeVisible (*c); cells.push_back (std::move (c)); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String title;
    int cols;
    std::vector<std::unique_ptr<juce::Component>> cells;
};

class Meter : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Meter (const juce::String& name) : caption (name) {}
    void setLevel (float v) { if (std::abs (v - level) > 0.002f) { level = v; repaint(); } }
    void paint (juce::Graphics&) override;

private:
    juce::String caption;
    float level = 0.0f;
};

} // namespace silkreed

//==============================================================================
class SilkReedEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SilkReedEditor (SilkReedProcessor&);
    ~SilkReedEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kWidth = 1170, kHeight = 700;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (SilkReedEditor& e) : owner (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        SilkReedEditor& owner;
    };

    void timerCallback() override;
    void buildPresetMenu();
    void selectInstrument (int index);
    void stepPreset (int delta);
    void savePreset();
    void loadPreset();
    void showAbout();

    SilkReedProcessor& proc;
    silkreed::SilkLookAndFeel lnf;
    Content content { *this };

    std::unique_ptr<juce::Drawable> icon;
    juce::TextButton instButtons[3];
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "SAVE" }, loadButton { "LOAD" }, aboutButton { "ABOUT" };
    std::unique_ptr<silkreed::KnobCell> masterKnob;
    silkreed::Meter breathMeter { "BREATH" }, outMeter { "OUTPUT" };

    std::unique_ptr<silkreed::Panel> tonePanel, dynPanel, vibPanel, artPanel, charPanel, fxPanel;
    juce::MidiKeyboardComponent keyboard;
    juce::TooltipWindow tooltipWindow { this, 600 };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Array<juce::File> userPresetFiles;
    juce::String shownPresetName;
    int shownInstrument = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkReedEditor)
};
