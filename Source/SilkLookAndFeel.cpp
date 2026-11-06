// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

#include "SilkLookAndFeel.h"

namespace silkreed
{

using namespace juce;

SilkLookAndFeel::SilkLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, Colours::bgTop);
    setColour (Label::textColourId, Colours::text);
    setColour (Slider::textBoxTextColourId, Colours::textDim);
    setColour (Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, Colours::gold.withAlpha (0.4f));
    setColour (ComboBox::backgroundColourId, Colours::knobBottom);
    setColour (ComboBox::textColourId, Colours::text);
    setColour (ComboBox::outlineColourId, Colours::panelEdge);
    setColour (ComboBox::arrowColourId, Colours::gold);
    setColour (PopupMenu::backgroundColourId, Colour (0xff111726));
    setColour (PopupMenu::textColourId, Colours::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Colours::gold.withAlpha (0.25f));
    setColour (PopupMenu::highlightedTextColourId, Colours::goldLight);
    setColour (PopupMenu::headerTextColourId, Colours::gold);
    setColour (TextButton::buttonColourId, Colours::knobBottom);
    setColour (TextButton::buttonOnColourId, Colours::gold);
    setColour (TextButton::textColourOffId, Colours::textDim);
    setColour (TextButton::textColourOnId, Colour (0xff1a1206));
    setColour (TooltipWindow::backgroundColourId, Colour (0xff1b2338));
    setColour (TooltipWindow::textColourId, Colours::text);
    setColour (TooltipWindow::outlineColourId, Colours::gold.withAlpha (0.5f));
    setColour (TextEditor::backgroundColourId, Colours::knobBottom);
    setColour (TextEditor::textColourId, Colours::text);
    setColour (TextEditor::highlightColourId, Colours::gold.withAlpha (0.35f));
    setColour (CaretComponent::caretColourId, Colours::gold);
    setColour (AlertWindow::backgroundColourId, Colours::panel);
    setColour (AlertWindow::textColourId, Colours::text);
}

void SilkLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const bool hover = s.isMouseOverOrDragging();

    // Track
    const float trackR = radius - 2.5f;
    Path track;
    track.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::track);
    g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Value arc
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    Path arc;
    arc.addCentredArc (c.x, c.y, trackR, trackR, 0.0f, jmin (from, angle), jmax (from, angle), true);
    ColourGradient gold (Colours::goldLight, c.x - radius, c.y - radius, Colours::goldDark, c.x + radius, c.y + radius, false);
    g.setGradientFill (gold);
    g.strokePath (arc, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Knob body
    const float kr = radius - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2, kr * 2);
    ColourGradient body (hover ? Colours::knobTop.brighter (0.15f) : Colours::knobTop, c.x, c.y - kr,
                         Colours::knobBottom, c.x, c.y + kr, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (Colours::panelEdge.brighter (0.2f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);
    // brushed highlight
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillEllipse (c.x - kr * 0.7f, c.y - kr * 0.92f, kr * 1.4f, kr * 0.8f);

    // Pointer
    const float pIn = kr * 0.30f, pOut = kr * 0.86f;
    const Point<float> a (c.x + pIn * std::sin (angle), c.y - pIn * std::cos (angle));
    const Point<float> b (c.x + pOut * std::sin (angle), c.y - pOut * std::cos (angle));
    g.setColour (Colours::goldLight);
    g.drawLine ({ a, b }, 2.2f);
}

void SilkLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver (true) ? Colours::gold.withAlpha (0.7f) : Colours::panelEdge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (Colours::gold);
    g.fillPath (arrow);
}

Font SilkLookAndFeel::getComboBoxFont (ComboBox& box)
{
    return Font (FontOptions (jmin (14.0f, (float) box.getHeight() * 0.55f)));
}

void SilkLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void SilkLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    if (on)
    {
        ColourGradient grad (Colours::goldLight, r.getX(), r.getY(), Colours::goldDark, r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 6.0f);
    }
    else
    {
        g.setColour (Colours::knobBottom.brighter (over ? 0.25f : 0.0f).darker (down ? 0.2f : 0.0f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? Colours::gold.withAlpha (0.7f) : Colours::panelEdge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
}

void SilkLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool over, bool)
{
    const bool on = b.getToggleState();
    g.setColour (on ? b.findColour (TextButton::textColourOnId)
                    : (over ? Colours::goldLight : b.findColour (TextButton::textColourOffId)));
    const float fh = jmin (15.0f, (float) b.getHeight() * 0.48f);
    g.setFont (Font (FontOptions (fh, on ? Font::bold : Font::plain)).withExtraKerningFactor (0.08f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 0), Justification::centred, 1);
}

Font SilkLookAndFeel::getLabelFont (Label& l)
{
    return l.getFont();
}

Font SilkLookAndFeel::getPopupMenuFont()
{
    return Font (FontOptions (14.5f));
}

void SilkLookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    g.setColour (Colours::gold.withAlpha (0.35f));
    g.drawRect (0, 0, w, h);
}

void SilkLookAndFeel::drawTooltip (Graphics& g, const String& text, int w, int h)
{
    const auto r = Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (findColour (TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (findColour (TooltipWindow::outlineColourId));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (findColour (TooltipWindow::textColourId));
    g.setFont (Font (FontOptions (13.5f)));
    g.drawFittedText (text, Rectangle<int> (0, 0, w, h).reduced (8, 4), Justification::centredLeft, 4);
}

Label* SilkLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Font (FontOptions (12.0f)));
    l->setColour (Label::textColourId, Colours::textDim);
    l->setColour (Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (Label::outlineWhenEditingColourId, Colours::gold);
    return l;
}

} // namespace silkreed
