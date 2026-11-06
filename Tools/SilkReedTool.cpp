// SilkReed Sax - saxophone modeling instrument
// Copyright (c) 2026 SilkReed Audio. All rights reserved.
// Third-party components and their licences: see THIRD_PARTY_NOTICES.txt and README.md (Licensing).

// SilkReedTool — developer utility
//   SilkReedTool icon <out-dir>            render Resources/Icon.svg to PNGs (+ .iconset)
//   SilkReedTool render <out-dir>          render demo phrases for presets to WAV + print stats
//   SilkReedTool snapshot <out.png>        render the plugin editor to a PNG
//   SilkReedTool bench                     CPU benchmark of the engine
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

static int renderIcons (const File& outDir)
{
    auto drawable = Drawable::createFromImageData (BinaryData::Icon_svg, BinaryData::Icon_svgSize);
    if (! drawable) { std::cerr << "SVG parse failed\n"; return 1; }
    outDir.createDirectory();
    auto iconset = outDir.getChildFile ("SilkReed.iconset");
    iconset.createDirectory();

    auto renderPng = [&] (int size, const File& f)
    {
        Image img (Image::ARGB, size, size, true);
        Graphics g (img);
        drawable->drawWithin (g, Rectangle<float> (0, 0, (float) size, (float) size), RectanglePlacement::centred, 1.0f);
        f.deleteFile();
        FileOutputStream os (f);
        PNGImageFormat png;
        png.writeImageToStream (img, os);
    };

    for (int s : { 16, 32, 128, 256, 512 })
    {
        renderPng (s, iconset.getChildFile ("icon_" + String (s) + "x" + String (s) + ".png"));
        renderPng (s * 2, iconset.getChildFile ("icon_" + String (s) + "x" + String (s) + "@2x.png"));
    }
    renderPng (1024, outDir.getChildFile ("Icon_1024.png"));
    renderPng (512, outDir.getChildFile ("Icon_512.png"));
    std::cout << "icons written to " << outDir.getFullPathName() << "\n";
    return 0;
}

//==============================================================================
struct NoteEv { double beat, len; int note; float vel; };

static std::vector<NoteEv> phrase (int transposeSemis)
{
    // A lyrical smooth-jazz style line (original), legato overlaps for slurs.
    // Concert pitch for soprano; shifted down for alto / tenor.
    std::vector<NoteEv> p = {
        { 0.0, 1.55, 72, 0.70f }, { 1.5, 0.55, 74, 0.72f }, { 2.0, 0.55, 76, 0.78f }, { 2.5, 1.55, 79, 0.85f },
        { 4.0, 0.55, 77, 0.75f }, { 4.5, 0.55, 76, 0.72f }, { 5.0, 0.55, 74, 0.70f }, { 5.5, 2.40, 76, 0.80f },
        { 8.5, 0.45, 72, 0.60f }, { 9.0, 0.45, 74, 0.68f }, { 9.5, 0.45, 76, 0.75f }, { 10.0, 0.45, 79, 0.82f },
        { 10.5, 0.45, 81, 0.88f }, { 11.0, 4.0, 84, 0.95f },
        { 15.5, 0.9, 81, 0.50f }, { 16.5, 3.5, 79, 0.42f },
    };
    for (auto& e : p) e.note += transposeSemis;
    return p;
}

static void stats (const AudioBuffer<float>& b, const String& name)
{
    float peak = 0; double sum = 0; bool bad = false;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (c, i);
            if (! std::isfinite (v)) bad = true;
            peak = jmax (peak, std::abs (v));
            sum += (double) v * v;
        }
    const double rms = std::sqrt (sum / (b.getNumChannels() * (double) b.getNumSamples()));
    std::cout << name.paddedRight (' ', 34) << " peak " << String (Decibels::gainToDecibels (peak), 1).paddedLeft (' ', 6)
              << " dB   rms " << String (Decibels::gainToDecibels ((float) rms), 1).paddedLeft (' ', 6) << " dB"
              << (bad ? "   ** NaN/Inf **" : "") << "\n";
}

static int renderDemos (const File& outDir)
{
    outDir.createDirectory();
    const double sr = 48000.0; const int block = 256;
    const double bpm = 84.0, spb = 60.0 / bpm;
    const auto& presets = silkreed::getFactoryPresets();

    for (int pi = 0; pi < (int) presets.size(); ++pi)
    {
        SilkReedProcessor proc;
        proc.setCurrentProgram (pi);
        // optional overrides: SR_OVERRIDE="comp=0,tongue=0"
        if (auto* ov = std::getenv ("SR_OVERRIDE"))
            for (auto& kv : StringArray::fromTokens (ov, ",", ""))
                if (auto* prm = proc.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)))
                    prm->setValueNotifyingHost (prm->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        if (auto* only = std::getenv ("SR_ONLY")) if (String (only).getIntValue() != pi + 1) continue;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.prepareToPlay (sr, block);

        const int inst = (int) proc.apvts.getRawParameterValue (silkreed::ID::instrument)->load();
        const bool ewi = proc.apvts.getRawParameterValue (silkreed::ID::breathSource)->load() > 0.5f;
        const auto notes = phrase (std::getenv ("SR_SAME_NOTES") ? -5 : (inst == 0 ? 0 : (inst == 1 ? -7 : -12 - 7 + 2)));

        const double lengthSec = 22.0 * spb + 3.0;
        const int total = (int) (lengthSec * sr);
        AudioBuffer<float> out (2, total);
        AudioBuffer<float> buf (2, block);

        for (int pos = 0; pos < total; pos += block)
        {
            const int n = jmin (block, total - pos);
            MidiBuffer midi;
            const double t0 = pos / sr, t1 = (pos + n) / sr;
            for (const auto& e : notes)
            {
                const double on = e.beat * spb, off = (e.beat + e.len) * spb;
                if (on >= t0 && on < t1) midi.addEvent (MidiMessage::noteOn (1, e.note, e.vel), (int) ((on - t0) * sr));
                if (off >= t0 && off < t1) midi.addEvent (MidiMessage::noteOff (1, e.note), (int) ((off - t0) * sr));
            }
            if (ewi && (pos / block) % 4 == 0)
            {
                // synthetic breath: swells per phrase
                const double beat = t0 / spb;
                double br = 0.0;
                for (const auto& e : notes)
                    if (beat >= e.beat && beat < e.beat + e.len + 0.05)
                        br = jmax (br, (double) e.vel * (0.75 + 0.25 * std::sin ((beat - e.beat) * 1.3)));
                midi.addEvent (MidiMessage::controllerEvent (1, 2, jlimit (0, 127, (int) (br * 127))), 0);
            }
            buf.setSize (2, n, false, false, true);
            buf.clear();
            proc.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        }

        auto name = presets[(size_t) pi].name.replace (" - ", "_").replace (" ", "_").replace ("(", "").replace (")", "");
        auto file = outDir.getChildFile (String (pi + 1).paddedLeft ('0', 2) + "_" + name + ".wav");
        file.deleteFile();
        WavAudioFormat wav;
        std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (file), sr, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (out, 0, total);
        stats (out, presets[(size_t) pi].name);
    }
    return 0;
}

static int snapshot (const File& outFile)
{
    SilkReedProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 256);
    proc.prepareToPlay (48000.0, 256);
    std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
    ed->setSize (SilkReedEditor::kWidth, SilkReedEditor::kHeight);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    outFile.deleteFile();
    FileOutputStream os (outFile);
    PNGImageFormat().writeImageToStream (img, os);
    std::cout << "snapshot: " << outFile.getFullPathName() << "\n";
    return 0;
}

// Simulates the user: real mouse clicks at the instrument buttons / preset arrows, preset menu selection
static int uitest()
{
    SilkReedProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 256);
    proc.prepareToPlay (48000.0, 256);
    std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
    const float uiScale = std::getenv ("SR_SCALE") ? String (std::getenv ("SR_SCALE")).getFloatValue() : 1.0f;
    ed->setSize (roundToInt (SilkReedEditor::kWidth * uiScale), roundToInt (SilkReedEditor::kHeight * uiScale));
    std::cout << "editor size " << ed->getWidth() << "x" << ed->getHeight() << "\n";
    ed->addToDesktop (ComponentPeer::windowHasTitleBar);
    ed->setVisible (true);
    auto pump = [] (int ms) { MessageManager::getInstance()->runDispatchLoopUntil (ms); };
    pump (300);

    auto inst = [&] { return (int) proc.apvts.getRawParameterValue (silkreed::ID::instrument)->load(); };
    auto findButton = [&] (const String& text) -> Button*
    {
        std::function<Button* (Component*)> rec = [&] (Component* c) -> Button*
        {
            if (auto* b = dynamic_cast<TextButton*> (c)) if (b->getButtonText() == text) return b;
            for (auto* ch : c->getChildren()) if (auto* r = rec (ch)) return r;
            return nullptr;
        };
        return rec (ed.get());
    };
    auto clickAt = [&] (Component* target)
    {
        const auto centre = ed->getLocalArea (target->getParentComponent(), target->getBounds()).getCentre();
        auto* hit = ed->getComponentAt (centre);
        std::cout << "  hit-test at " << centre.toString() << " -> "
                  << (hit ? String (typeid (*hit).name()) + " '" + hit->getName() + "'" : String ("nothing"))
                  << (hit == target ? "  (OK)" : "  (** NOT the button **)") << "\n";
        // real mouse event through the peer
        auto src = Desktop::getInstance().getMainMouseSource();
        const auto screen = ed->localPointToGlobal (centre).toFloat();
        src.setScreenPosition (screen);
        if (auto* peer = ed->getPeer())
        {
            const auto local = peer->globalToLocal (screen);
            if (std::getenv ("SR_HOVER"))   // hover like a person, long enough for the tooltip to appear
            {
                peer->handleMouseEvent (MouseInputSource::InputSourceType::mouse, local, ModifierKeys(),
                                        MouseInputSource::defaultPressure, 0.0f, Time::currentTimeMillis());
                pump (1200);
                std::function<Component* (Component*)> findTip = [&] (Component* c) -> Component*
                {
                    if (dynamic_cast<TooltipWindow*> (c) != nullptr) return c;
                    for (auto* ch : c->getChildren()) if (auto* r = findTip (ch)) return r;
                    return nullptr;
                };
                if (auto* tip = findTip (ed.get()))
                {
                    auto* hit2 = ed->getComponentAt (centre);
                    std::cout << "  tooltip visible=" << (int) tip->isVisible() << " bounds=" << tip->getBounds().toString()
                              << "  hit-test now -> " << (hit2 ? String (typeid (*hit2).name()) : String ("nothing")) << "\n";
                }
            }
            const auto t = Time::currentTimeMillis();
            peer->handleMouseEvent (MouseInputSource::InputSourceType::mouse, local, ModifierKeys (ModifierKeys::leftButtonModifier),
                                    MouseInputSource::defaultPressure, 0.0f, t);
            pump (30);
            peer->handleMouseEvent (MouseInputSource::InputSourceType::mouse, local, ModifierKeys(),
                                    MouseInputSource::defaultPressure, 0.0f, t + 40);
        }
        pump (200);
    };

    std::cout << "initial instrument = " << inst() << ", preset = " << proc.currentPresetName << "\n";
    for (auto name : { "ALTO", "TENOR", "SOPRANO" })
    {
        auto* b = findButton (name);
        std::cout << "click " << name << ":\n";
        clickAt (b);
        std::cout << "  instrument now = " << inst() << ", toggle = " << (int) b->getToggleState()
                  << ", preset = " << proc.currentPresetName
                  << ", brightness = " << proc.apvts.getRawParameterValue (silkreed::ID::brightness)->load() << "\n";
    }
    // per-instrument memory: choose "Alto - Funk Edge", go to TENOR, come back to ALTO
    proc.setCurrentProgram (7);
    pump (200);
    std::cout << "memory test: loaded " << proc.currentPresetName << "\n";
    clickAt (findButton ("TENOR"));
    std::cout << "  after TENOR: " << proc.currentPresetName << "\n";
    clickAt (findButton ("ALTO"));
    std::cout << "  after ALTO : " << proc.currentPresetName << "   (expected Alto - Funk Edge)\n";
    clickAt (findButton ("ALTO"));
    std::cout << "  ALTO again : " << proc.currentPresetName << "   (expected unchanged)\n";
    clickAt (findButton ("SOPRANO"));
    for (auto name : { ">", ">", "<" })
    {
        std::cout << "click preset '" << name << "':\n";
        clickAt (findButton (name));
        std::cout << "  preset now = " << proc.currentPresetName << ", program = " << proc.getCurrentProgram()
                  << ", instrument = " << inst() << "\n";
    }
    {
        auto* nb = findButton (">");
        std::cout << "triggerClick '>' (program before " << proc.getCurrentProgram() << ")\n";
        nb->triggerClick();
        pump (10);
        std::cout << "  after 10ms: program " << proc.getCurrentProgram() << " " << proc.currentPresetName << "\n";
        pump (300);
        std::cout << "  after 300ms: program " << proc.getCurrentProgram() << " " << proc.currentPresetName << "\n";
    }
    // preset combo: select item 11 (Tenor - Smooth Lead) as the user would from the menu
    std::function<ComboBox* (Component*)> findCombo = [&] (Component* c) -> ComboBox*
    {
        if (auto* cb = dynamic_cast<ComboBox*> (c)) if (cb->getTooltip().contains ("presets")) return cb;
        for (auto* ch : c->getChildren()) if (auto* r = findCombo (ch)) return r;
        return nullptr;
    };
    if (auto* cb = findCombo (ed.get()))
    {
        cb->setSelectedId (11, sendNotificationAsync);
        pump (300);
        std::cout << "menu select 11: preset now = " << proc.currentPresetName << ", instrument = " << inst()
                  << ", combo text = " << cb->getText() << "\n";
    }
    ed->removeFromDesktop();
    return 0;
}

// Decodes the plug-in state stored in the Standalone app's settings file
static int dumpState (const File& f)
{
    auto xml = XmlDocument::parse (f);
    if (! xml) { std::cerr << "cannot parse " << f.getFullPathName() << "\n"; return 1; }
    for (auto* v : xml->getChildIterator())
        if (v->getStringAttribute ("name") == "filterState")
        {
            MemoryBlock mb;
            mb.fromBase64Encoding (v->getStringAttribute ("val"));
            if (auto state = AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize()))
            {
                std::cout << "presetName=" << state->getStringAttribute ("presetName") << " program=" << state->getStringAttribute ("program") << "\n";
                for (auto* p : state->getChildIterator())
                    std::cout << "  " << p->getStringAttribute ("id") << " = " << p->getStringAttribute ("value") << "\n";
            }
        }
    return 0;
}

static int bench()
{
    SilkReedProcessor proc;
    const double sr = 48000.0; const int block = 256;
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);
    AudioBuffer<float> buf (2, block);
    for (int mode : { 0, 2 })
    {
        proc.apvts.getParameter (silkreed::ID::voiceMode)->setValueNotifyingHost (mode == 2 ? 1.0f : 0.0f);
        MidiBuffer midi;
        const int voices = mode == 2 ? 8 : 1;
        for (int v = 0; v < voices; ++v) midi.addEvent (MidiMessage::noteOn (1, 48 + v * 3, 0.9f), 0);
        const int blocks = (int) (10.0 * sr / block);
        const auto t0 = Time::getMillisecondCounterHiRes();
        for (int i = 0; i < blocks; ++i) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); }
        const auto ms = Time::getMillisecondCounterHiRes() - t0;
        std::cout << (mode == 2 ? "poly x8 " : "mono    ") << ": " << String (ms / 100.0, 2) << "% of one core (10 s audio in "
                  << String (ms, 0) << " ms)\n";
        midi.addEvent (MidiMessage::allNotesOff (1), 0);
        for (int i = 0; i < 200; ++i) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); }
    }
    return 0;
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::cout << "usage: SilkReedTool icon|render|snapshot|bench [path]\n"; return 1; }
    const String cmd (argv[1]);
    const File arg = argc > 2 ? File::getCurrentWorkingDirectory().getChildFile (argv[2]) : File();
    if (cmd == "icon") return renderIcons (arg);
    if (cmd == "render") return renderDemos (arg);
    if (cmd == "snapshot") return snapshot (arg);
    if (cmd == "bench") return bench();
    if (cmd == "uitest") return uitest();
    if (cmd == "state") return dumpState (File (argv[2]));
    return 1;
}
