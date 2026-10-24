#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static std::shared_ptr<SampleData> makeTestSample()
{
    auto s = std::make_shared<SampleData>();

    const double sr = 44100.0;
    const int    len = (int) (sr * 2.0);

    s->len    = len;
    s->sr     = sr;
    s->stereo = false;

    s->l.resize ((size_t) len);
    s->r.resize ((size_t) len);

    juce::Random rng (7);

    for (int i = 0; i < len; ++i)
    {
        const float t = (float) i / (float) sr;

        const float wobble = 0.55f * std::sin (juce::MathConstants<float>::twoPi * 110.0f * t
                                               + 2.6f * std::sin (juce::MathConstants<float>::twoPi * 3.1f * t));
        const float am     = 0.55f + 0.45f * std::sin (juce::MathConstants<float>::twoPi * 2.3f * t);
        const float grit   = (rng.nextFloat() * 2.0f - 1.0f) * 0.18f;

        s->l[(size_t) i] = juce::jlimit (-1.0f, 1.0f, wobble * am + grit);
    }

    s->r = s->l;
    computePeaks (*s);
    return s;
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "ui" && argc > 2)
    {
        Lv999AudioProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.setSample (makeTestSample());

        if (argc > 3)
        {
            const float sm = (float) juce::String (argv[3]).getFloatValue();

            if (auto* p = proc.apvts.getParameter ("smooth"))
                p->setValueNotifyingHost (p->convertTo0to1 (sm));
        }

        if (argc > 4)
        {
            const float rt = (float) juce::String (argv[4]).getFloatValue();

            if (auto* p = proc.apvts.getParameter ("rate"))
                p->setValueNotifyingHost (p->convertTo0to1 (rt));
        }

        // let the engine run so the LFO trace has something to show
        {
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 48, 1.0f), 0);
            proc.processBlock (buf, midi);

            juce::MidiBuffer empty;

            for (int b = 0; b < 620; ++b)
            {
                buf.clear();
                proc.processBlock (buf, empty);
            }
        }

        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (420, 490);

        const auto img = ed->createComponentSnapshot (ed->getLocalBounds());

        juce::File out { juce::String (argv[2]) };
        out.deleteFile();

        juce::FileOutputStream os (out);
        juce::PNGImageFormat png;

        if (png.writeImageToStream (img, os))
            std::printf ("wrote %s\n", out.getFullPathName().toRawUTF8());

        ed.reset();
        return 0;
    }

    if (mode == "audio")
    {
        // quick sanity render: does the cartridge make sound and reverse?
        Lv999AudioProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.setSample (makeTestSample());

        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 48, 1.0f), 0);
        proc.processBlock (buf, midi);

        float peak = 0.0f;

        for (int b = 0; b < 200; ++b)
        {
            buf.clear();
            juce::MidiBuffer empty;
            proc.processBlock (buf, empty);

            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < buf.getNumSamples(); ++i)
                    peak = juce::jmax (peak, std::abs (buf.getSample (c, i)));
        }

        std::printf ("peak after 2.1 s: %.4f  -> %s\n", peak, peak > 0.01f ? "OK" : "SILENT");
        return peak > 0.01f ? 0 : 1;
    }

    if (mode == "state")
    {
        Lv999AudioProcessor a;
        a.prepareToPlay (48000.0, 512);
        a.setSample (makeTestSample());

        juce::MemoryBlock mb;
        a.getStateInformation (mb);

        Lv999AudioProcessor b;
        b.setStateInformation (mb.getData(), (int) mb.getSize());

        auto s = b.getSample();
        const bool ok = s != nullptr && s->len > 0;

        std::printf ("state %d bytes, restored sample len %d -> %s\n",
                     (int) mb.getSize(), s ? s->len : -1, ok ? "OK" : "LOST");
        return ok ? 0 : 1;
    }

    if (mode == "delay")
    {
        const auto tailRms = [&] (float delayPct)
        {
            Lv999AudioProcessor p;
            p.prepareToPlay (48000.0, 512);
            p.setSample (makeTestSample());

            if (auto* d = p.apvts.getParameter ("delay"))
                d->setValueNotifyingHost (d->convertTo0to1 (delayPct));

            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer on;
            on.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
            p.processBlock (buf, on);

            for (int b = 0; b < 94; ++b)      // 1 s of sound
            {
                buf.clear();
                juce::MidiBuffer e;
                p.processBlock (buf, e);
            }

            juce::MidiBuffer off;
            off.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            buf.clear();
            p.processBlock (buf, off);

            double sum = 0.0;
            int    cnt = 0;

            for (int b = 0; b < 94; ++b)      // 1 s of tail
            {
                buf.clear();
                juce::MidiBuffer e;
                p.processBlock (buf, e);

                for (int i = 0; i < 512; ++i)
                {
                    const double v = buf.getSample (0, i);
                    sum += v * v;
                    ++cnt;
                }
            }

            return std::sqrt (sum / juce::jmax (1, cnt));
        };

        const double dry  = tailRms (0.0f);
        const double wet  = tailRms (70.0f);
        const bool   ok   = wet > dry * 10.0 + 1.0e-5;

        std::printf ("tail rms  off %.6f  delay70 %.6f -> %s\n", dry, wet, ok ? "OK" : "CHECK");
        return ok ? 0 : 1;
    }

    std::printf ("usage: Lv999Test ui <png> | audio | state | delay\n");
    return 0;
}
