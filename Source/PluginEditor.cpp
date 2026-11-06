// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;
namespace C = silkreed::Colours;

namespace silkreed
{

//==============================================================================
KnobCell::KnobCell (AudioProcessorValueTreeState& s, const char* paramId, const String& title,
                    const String& tooltip, bool bipolar)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 76, 16);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (220);
    slider.getProperties().set ("bipolar", bipolar);
    slider.setColour (Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (Slider::textBoxTextColourId, C::textDim);
    slider.setTooltip (tooltip);
    addAndMakeVisible (slider);

    label.setText (title, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (12.5f, Font::bold)).withExtraKerningFactor (0.03f));
    label.setColour (Label::textColourId, C::text);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, paramId, slider);
    // double-click resets to the parameter default
    if (auto* p = s.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void KnobCell::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r.reduced (2, 0));
}

//==============================================================================
ChoiceCell::ChoiceCell (AudioProcessorValueTreeState& s, const char* paramId, const String& title,
                        const String& tooltip)
{
    if (auto* p = dynamic_cast<AudioParameterChoice*> (s.getParameter (paramId)))
        box.addItemList (p->choices, 1);
    box.setTooltip (tooltip);
    addAndMakeVisible (box);

    label.setText (title, dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (12.5f, Font::bold)).withExtraKerningFactor (0.03f));
    label.setColour (Label::textColourId, C::text);
    addAndMakeVisible (label);

    attachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, paramId, box);
}

void ChoiceCell::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth() - 4, 26).translated (0, -8));
}

//==============================================================================
void Panel::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    ColourGradient grad (C::panel.brighter (0.06f), 0, 0, C::panel.darker (0.25f), 0, r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (C::panelEdge);
    g.drawRoundedRectangle (r, 9.0f, 1.0f);

    // title with gold accent bar
    g.setColour (C::gold);
    g.fillRoundedRectangle (14.0f, 12.0f, 3.0f, 13.0f, 1.5f);
    g.setFont (Font (FontOptions (13.0f, Font::bold)).withExtraKerningFactor (0.22f));
    g.setColour (C::goldLight);
    g.drawText (title.toUpperCase(), 24, 8, getWidth() - 30, 20, Justification::centredLeft);
    g.setColour (C::panelEdge.withAlpha (0.6f));
    g.drawHorizontalLine (32, 12.0f, (float) getWidth() - 12.0f);
}

void Panel::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    r.removeFromTop (30);
    const int rows = ((int) cells.size() + cols - 1) / cols;
    const float cw = (float) r.getWidth() / (float) cols;
    const float ch = (float) r.getHeight() / (float) juce::jmax (1, rows);
    for (size_t i = 0; i < cells.size(); ++i)
    {
        const int col = (int) i % cols, row = (int) i / cols;
        cells[i]->setBounds (Rectangle<float> ((float) r.getX() + (float) col * cw, (float) r.getY() + (float) row * ch, cw, ch)
                                 .toNearestInt().reduced (2, 1));
    }
}

//==============================================================================
void Meter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto cap = r.removeFromBottom (14);
    g.setColour (C::textDim);
    g.setFont (Font (FontOptions (9.0f, Font::bold)).withExtraKerningFactor (0.02f));
    g.drawText (caption, cap, Justification::centred);

    auto bar = r.withSizeKeepingCentre (12.0f, r.getHeight() - 4.0f);
    g.setColour (C::track);
    g.fillRoundedRectangle (bar, 3.0f);
    const float v = jlimit (0.0f, 1.0f, level);
    auto fill = bar.withTop (bar.getBottom() - bar.getHeight() * v);
    ColourGradient grad (Colour (0xffffe9a8), bar.getX(), bar.getY(), C::goldDark, bar.getX(), bar.getBottom(), false);
    if (level > 0.99f) grad = ColourGradient (Colour (0xffff7b5c), bar.getX(), bar.getY(), Colour (0xffb83a24), bar.getX(), bar.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (fill, 3.0f);
    g.setColour (C::panelEdge);
    g.drawRoundedRectangle (bar, 3.0f, 1.0f);
}

} // namespace silkreed

//==============================================================================
using silkreed::KnobCell;
using silkreed::ChoiceCell;
using silkreed::Panel;

SilkReedEditor::SilkReedEditor (SilkReedProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    auto& s = proc.apvts;

    icon = Drawable::createFromImageData (BinaryData::Icon_svg, BinaryData::Icon_svgSize);
    if (icon) content.addAndMakeVisible (*icon);

    // ---- instrument selector
    const char* instNames[3] = { "SOPRANO", "ALTO", "TENOR" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = instButtons[i];
        b.setButtonText (instNames[i]);
        b.setClickingTogglesState (false);
        b.setTooltip (String ("Load a ") + String (instNames[i]).toLowerCase() + " saxophone preset (returns to the last one you used)");
        b.onClick = [this, i] { selectInstrument (i); };
        content.addAndMakeVisible (b);
    }

    // ---- presets
    presetBox.setTextWhenNothingSelected ("Preset");
    presetBox.setTooltip ("Factory and user presets");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) proc.setCurrentProgram (id - 1);
        else if (isPositiveAndBelow (id - 1000, userPresetFiles.size()))
            proc.loadUserPreset (userPresetFiles[id - 1000]);
        shownPresetName = {};
    };
    content.addAndMakeVisible (presetBox);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };
    aboutButton.onClick = [this] { showAbout(); };
    aboutButton.setTooltip ("About SilkReed Sax: copyright, licences and third-party notices");
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    saveButton.setTooltip ("Save the current sound as a user preset");
    loadButton.setTooltip ("Load a user preset file");
    for (auto* b : { &prevButton, &nextButton, &saveButton, &loadButton, &aboutButton })
        content.addAndMakeVisible (*b);
    buildPresetMenu();

    masterKnob = std::make_unique<KnobCell> (s, silkreed::ID::master, "MASTER", "Output level");
    content.addAndMakeVisible (*masterKnob);
    breathMeter.setTooltip ("Live breath / dynamics value driving the model");
    outMeter.setTooltip ("Output peak level");
    content.addAndMakeVisible (breathMeter);
    content.addAndMakeVisible (outMeter);

    // ---- panels
    auto knob = [&s] (const char* id, const char* name, const char* tip, bool bip = false)
    { return std::make_unique<KnobCell> (s, id, name, tip, bip); };
    auto choice = [&s] (const char* id, const char* name, const char* tip)
    { return std::make_unique<ChoiceCell> (s, id, name, tip); };
    using namespace silkreed;

    tonePanel = std::make_unique<Panel> ("Tone", 4);
    tonePanel->add (knob (ID::brightness, "Brightness", "Overall spectral brightness of the horn"));
    tonePanel->add (knob (ID::reed, "Reed Edge", "Reed hardness: steeper, buzzier upper partials and saturation at high dynamics"));
    tonePanel->add (knob (ID::warmth, "Warmth", "Low-register body and softened upper-mid edge"));
    tonePanel->add (knob (ID::body, "Body", "Strength of the bore/bell formant resonances"));
    tonePanel->add (knob (ID::formant, "Formant", "Shifts the body resonances (smaller/larger horn character)", true));
    tonePanel->add (knob (ID::air, "Air", "Breath noise amount (pitch-synchronous + free air)"));
    tonePanel->add (knob (ID::airTone, "Air Tone", "Colour of the breath noise, dark to bright"));
    tonePanel->add (knob (ID::subtone, "Subtone", "Soft, dark, breathy subtone playing"));

    dynPanel = std::make_unique<Panel> ("Breath & Dynamics", 4);
    dynPanel->add (choice (ID::breathSource, "Source", "What drives the breath: velocity, EWI/breath controller (CC2), expression pedal (CC11), aftertouch, or velocity x CC11"));
    dynPanel->add (knob (ID::breathSens, "Sensitivity", "Breath input gain"));
    dynPanel->add (knob (ID::breathCurve, "Curve", "Response curve: negative = harder to play loud, positive = easier", true));
    dynPanel->add (knob (ID::dynRange, "Dyn Timbre", "How much the timbre brightens as you play louder"));
    dynPanel->add (knob (ID::attack, "Attack", "Note onset time"));
    dynPanel->add (knob (ID::release, "Release", "Note release time"));
    dynPanel->add (knob (ID::swell, "Auto Swell", "Automatic crescendo on sustained notes (velocity/aftertouch modes)"));
    dynPanel->add (knob (ID::swellTime, "Swell Time", "Time for the auto swell to reach full level"));

    vibPanel = std::make_unique<Panel> ("Vibrato", 4);
    vibPanel->add (knob (ID::vibRate, "Rate", "Vibrato speed"));
    vibPanel->add (knob (ID::vibDepth, "Depth", "Automatic vibrato depth in cents"));
    vibPanel->add (knob (ID::vibDelay, "Delay", "Time before the vibrato starts on each note"));
    vibPanel->add (knob (ID::vibFade, "Fade In", "Time for the vibrato to reach full depth"));
    vibPanel->add (knob (ID::vibMod, "Mod Wheel", "Additional vibrato depth from the mod wheel (CC1)"));
    vibPanel->add (knob (ID::vibAmp, "Amp/Tone", "Amount of loudness & brightness movement in the vibrato (lip vibrato)"));
    vibPanel->add (knob (ID::vibRandom, "Random", "Natural rate/depth variation"));

    artPanel = std::make_unique<Panel> ("Articulation", 4);
    artPanel->add (choice (ID::voiceMode, "Voice Mode", "Mono legato (slurs overlapping notes), mono retrigger, or polyphonic"));
    artPanel->add (knob (ID::glide, "Glide", "Portamento time between notes"));
    artPanel->add (choice (ID::glideMode, "Glide Mode", "Glide only on overlapping (legato) notes, or always"));
    artPanel->add (knob (ID::pbRange, "Bend Range", "Pitch-bend range in semitones"));
    artPanel->add (knob (ID::scoop, "Scoop", "Pitch scoop from below at the start of tongued notes"));
    artPanel->add (knob (ID::scoopTime, "Scoop Time", "Length of the scoop"));
    artPanel->add (knob (ID::tongue, "Tongue", "Strength of the tongued attack transient and slur key noise"));
    artPanel->add (knob (ID::fall, "Fall", "Pitch fall-off on note release"));

    charPanel = std::make_unique<Panel> ("Character", 3);
    charPanel->add (knob (ID::growl, "Growl", "Growl / flutter roughness"));
    charPanel->add (knob (ID::growlRate, "Growl Rate", "Speed of the growl modulation"));
    charPanel->add (knob (ID::humanize, "Humanize", "Subtle pitch drift, breath and partial fluctuations"));
    charPanel->add (knob (ID::transpose, "Transpose", "Transpose in semitones", true));
    charPanel->add (knob (ID::tune, "Fine Tune", "Fine tuning in cents", true));
    charPanel->add (choice (ID::rangeMode, "Range", "Free, or fold notes into the real instrument's range by octaves"));

    fxPanel = std::make_unique<Panel> ("Studio Effects", 6);
    fxPanel->add (knob (ID::eqLow, "Low", "Low shelf EQ (220 Hz)", true));
    fxPanel->add (knob (ID::eqMid, "Mid", "Peaking mid EQ", true));
    fxPanel->add (knob (ID::eqMidFreq, "Mid Freq", "Mid EQ frequency"));
    fxPanel->add (knob (ID::eqHigh, "High", "High shelf EQ (5.5 kHz)", true));
    fxPanel->add (knob (ID::comp, "Compressor", "Smooth vocal-style compression"));
    fxPanel->add (knob (ID::width, "Width", "Stereo early reflections / room width"));
    fxPanel->add (knob (ID::delayMix, "Delay", "Ping-pong delay level"));
    fxPanel->add (knob (ID::delayTime, "Delay Time", "Delay time"));
    fxPanel->add (knob (ID::delayFb, "Feedback", "Delay feedback"));
    fxPanel->add (knob (ID::revMix, "Reverb", "Reverb level"));
    fxPanel->add (knob (ID::revSize, "Size", "Reverb size / decay"));
    fxPanel->add (knob (ID::revDamp, "Damping", "Reverb high-frequency damping"));

    for (auto* panel : { tonePanel.get(), dynPanel.get(), vibPanel.get(), artPanel.get(), charPanel.get(), fxPanel.get() })
        content.addAndMakeVisible (*panel);

    // ---- keyboard
    keyboard.setAvailableRange (36, 96);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xffe9e4d8));
    keyboard.setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff141824));
    keyboard.setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xff8d8678));
    keyboard.setColour (MidiKeyboardComponent::keyDownOverlayColourId, C::gold.withAlpha (0.75f));
    keyboard.setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, C::gold.withAlpha (0.25f));
    keyboard.setColour (MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keyboard.setColour (MidiKeyboardComponent::upDownButtonBackgroundColourId, C::panel);
    keyboard.setColour (MidiKeyboardComponent::upDownButtonArrowColourId, C::gold);
    keyboard.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (kWidth * 6 / 10, kHeight * 6 / 10, kWidth * 2, kHeight * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kWidth / (double) kHeight);
    setSize (kWidth, kHeight);
    startTimerHz (30);
    timerCallback();
}

SilkReedEditor::~SilkReedEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SilkReedEditor::paint (Graphics& g)
{
    g.fillAll (C::bgBottom);
}

void SilkReedEditor::resized()
{
    const float scale = (float) getWidth() / (float) kWidth;
    content.setBounds (0, 0, kWidth, kHeight);
    content.setTransform (AffineTransform::scale (scale));
}

void SilkReedEditor::Content::paint (Graphics& g)
{
    ColourGradient bg (C::bgTop, 0, 0, C::bgBottom, 0, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // header
    ColourGradient head (Colour (0xff18223d), 0, 0, Colour (0xff0d1324), 0, 78, false);
    g.setGradientFill (head);
    g.fillRect (0, 0, getWidth(), 78);
    ColourGradient line (C::gold.withAlpha (0.0f), 0, 0, C::gold.withAlpha (0.0f), (float) getWidth(), 0, false);
    line.addColour (0.5, C::gold.withAlpha (0.8f));
    g.setGradientFill (line);
    g.fillRect (0, 77, getWidth(), 1);

    g.setColour (C::goldLight);
    g.setFont (Font (FontOptions (28.0f, Font::bold)).withExtraKerningFactor (0.04f));
    g.drawText ("SilkReed", 86, 12, 220, 32, Justification::centredLeft);
    g.setColour (C::textDim);
    g.setFont (Font (FontOptions (10.5f, Font::bold)).withExtraKerningFactor (0.28f));
    g.drawText ("SAXOPHONE MODELING INSTRUMENT", 87, 44, 260, 16, Justification::centredLeft);

    g.setColour (C::textDim.withAlpha (0.6f));
    g.setFont (Font (FontOptions (10.0f)));
    g.drawText ("v" JUCE_STRINGIFY (SILKREED_VERSION_STRING), getWidth() - 60, getHeight() - 16, 54, 14, Justification::centredRight);
}

void SilkReedEditor::Content::resized()
{
    auto& e = owner;
    if (e.icon) e.icon->setTransformToFit (Rectangle<float> (16, 9, 60, 60), RectanglePlacement::centred);

    for (int i = 0; i < 3; ++i)
        e.instButtons[i].setBounds (352 + i * 100, 22, 94, 34);

    e.prevButton.setBounds (676, 14, 28, 28);
    e.presetBox.setBounds (708, 14, 260, 28);
    e.nextButton.setBounds (972, 14, 28, 28);
    e.saveButton.setBounds (708, 47, 62, 20);
    e.loadButton.setBounds (774, 47, 62, 20);
    e.aboutButton.setBounds (840, 47, 62, 20);

    e.breathMeter.setBounds (1008, 6, 46, 66);
    e.outMeter.setBounds (1052, 6, 46, 66);
    e.masterKnob->setBounds (1096, 2, 70, 74);

    const int m = 14, gap = 12;
    const int rowH = 250;
    int y = 78 + 10;
    const int w3 = (kWidth - 2 * m - 2 * gap) / 3;
    e.tonePanel->setBounds (m, y, w3, rowH);
    e.dynPanel->setBounds (m + w3 + gap, y, w3, rowH);
    e.vibPanel->setBounds (m + 2 * (w3 + gap), y, kWidth - m - (m + 2 * (w3 + gap)), rowH);

    y += rowH + gap;
    const int avail = kWidth - 2 * m - 2 * gap;
    const int colW = avail / 13;
    const int wA = colW * 4, wC = colW * 3;
    e.artPanel->setBounds (m, y, wA, rowH);
    e.charPanel->setBounds (m + wA + gap, y, wC, rowH);
    e.fxPanel->setBounds (m + wA + gap + wC + gap, y, kWidth - m - (m + wA + gap + wC + gap), rowH);

    y += rowH + gap;
    e.keyboard.setBounds (m, y, kWidth - 2 * m, kHeight - y - 18);
    e.keyboard.setKeyWidth ((float) e.keyboard.getWidth() / 36.0f);   // 36 white keys in C2..C7
}

void SilkReedEditor::timerCallback()
{
    const int inst = (int) proc.apvts.getRawParameterValue (silkreed::ID::instrument)->load();
    if (inst != shownInstrument)
    {
        shownInstrument = inst;
        for (int i = 0; i < 3; ++i)
            instButtons[i].setToggleState (i == inst, dontSendNotification);
    }
    if (proc.currentPresetName != shownPresetName)
    {
        shownPresetName = proc.currentPresetName;
        presetBox.setText (shownPresetName, dontSendNotification);
    }
    breathMeter.setLevel (proc.getBreathMeter());
    outMeter.setLevel (proc.getOutputPeak());
}

void SilkReedEditor::selectInstrument (int index)
{
    if (index == shownInstrument) return;   // already on this instrument: keep the current sound
    proc.selectInstrument (index);
    shownPresetName = {};
}

void SilkReedEditor::buildPresetMenu()
{
    presetBox.clear (dontSendNotification);
    const auto& presets = silkreed::getFactoryPresets();
    presetBox.addSectionHeading ("Factory");
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox.addItem (presets[i].name, (int) i + 1);

    userPresetFiles = SilkReedProcessor::getUserPresetFolder().findChildFiles (File::findFiles, false, "*.srpreset");
    userPresetFiles.sort();
    if (! userPresetFiles.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem (userPresetFiles[i].getFileNameWithoutExtension(), 1000 + i);
    }
    shownPresetName = {};
}

void SilkReedEditor::stepPreset (int delta)
{
    const int n = proc.getNumPrograms();
    int idx = proc.getCurrentProgram() + delta;
    idx = (idx % n + n) % n;
    proc.setCurrentProgram (idx);
    shownPresetName = {};
}

void SilkReedEditor::savePreset()
{
    chooser = std::make_unique<FileChooser> ("Save SilkReed preset",
                                             SilkReedProcessor::getUserPresetFolder().getChildFile (proc.currentPresetName + ".srpreset"),
                                             "*.srpreset");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles
                              | FileBrowserComponent::warnAboutOverwriting,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == File()) return;
                              proc.saveUserPreset (f.withFileExtension ("srpreset"));
                              buildPresetMenu();
                          });
}

void SilkReedEditor::loadPreset()
{
    chooser = std::make_unique<FileChooser> ("Load SilkReed preset", SilkReedProcessor::getUserPresetFolder(), "*.srpreset");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile()) proc.loadUserPreset (f);
                              shownPresetName = {};
                          });
}

void SilkReedEditor::showAbout()
{
    juce::String text;
    text << "SilkReed Sax  v" << JUCE_STRINGIFY (SILKREED_VERSION_STRING) << "\n"
         << "Saxophone modeling instrument (soprano / alto / tenor)\n"
         << "Copyright (c) 2026 SilkReed Audio. All rights reserved.\n"
         << "Built with JUCE " << juce::SystemStats::getJUCEVersion().fromFirstOccurrenceOf ("v", false, false) << "\n\n"
         << juce::String::createStringFromData (BinaryData::THIRD_PARTY_NOTICES_txt, BinaryData::THIRD_PARTY_NOTICES_txtSize);

    auto* editor = new juce::TextEditor();
    editor->setMultiLine (true, true);
    editor->setReadOnly (true);
    editor->setScrollbarsShown (true);
    editor->setCaretVisible (false);
    editor->setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain)));
    editor->setColour (juce::TextEditor::backgroundColourId, C::panel);
    editor->setColour (juce::TextEditor::textColourId, C::text);
    editor->setColour (juce::TextEditor::outlineColourId, C::panelEdge);
    editor->setText (text, false);
    editor->setSize (680, 520);

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (editor);
    o.dialogTitle = "About SilkReed Sax";
    o.dialogBackgroundColour = C::bgTop;
    o.componentToCentreAround = this;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = true;
    o.launchAsync();
}
