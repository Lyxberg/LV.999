#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <atomic>

//==============================================================================
void computePeaks (SampleData& s)
{
    if (s.len < 1)
        return;

    const int per = juce::jmax (1, s.len / SampleData::numPeaks);

    for (int b = 0; b < SampleData::numPeaks; ++b)
    {
        const int start = b * per;
        const int end   = juce::jmin (s.len, start + per);

        float lo = 0.0f, hi = 0.0f;

        for (int i = start; i < end; ++i)
        {
            lo = juce::jmin (lo, s.l[(size_t) i]);
            hi = juce::jmax (hi, s.l[(size_t) i]);
        }

        s.peakMin[(size_t) b] = lo;
        s.peakMax[(size_t) b] = hi;
    }
}

std::shared_ptr<SampleData> loadSampleFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return nullptr;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples < 2)
        return nullptr;

    // cap at 8 seconds so the cartridge stays a cartridge
    const int maxLen = (int) juce::jmin ((juce::int64) (reader->sampleRate * 8.0),
                                         reader->lengthInSamples);

    auto s = std::make_shared<SampleData>();
    s->len    = maxLen;
    s->sr     = reader->sampleRate;
    s->stereo = reader->numChannels > 1;

    s->l.resize ((size_t) maxLen);
    s->r.resize ((size_t) maxLen);

    juce::AudioBuffer<float> buf ((int) reader->numChannels, maxLen);
    reader->read (&buf, 0, maxLen, 0, true, true);

    std::copy (buf.getReadPointer (0), buf.getReadPointer (0) + maxLen, s->l.begin());

    if (reader->numChannels > 1)
        std::copy (buf.getReadPointer (1), buf.getReadPointer (1) + maxLen, s->r.begin());
    else
        s->r = s->l;

    computePeaks (*s);
    return s;
}

//==============================================================================
Lv999AudioProcessor::Lv999AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Lv999AudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto fmt = [] (int dp, const juce::String& suffix)
    {
        return juce::AudioParameterFloatAttributes()
                   .withStringFromValueFunction ([dp, suffix] (float v, int)
                                                 { return juce::String (v, dp) + suffix; });
    };

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "smooth", 1 },
                   "Smooth", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, fmt (0, " %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "shift", 1 },
                   "Shift", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, fmt (0, " %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "low", 1 },
                   "Range Low", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), -100.0f, fmt (0, " %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "high", 1 },
                   "Range High", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 100.0f, fmt (0, " %")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "rate", 1 },
                   "LFO Rate", juce::NormalisableRange<float> (1.0f, 30.0f, 0.0f, 0.4f), 2.2f, fmt (1, " Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "delay", 1 },
                   "Delay", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, fmt (0, " %")));

    return layout;
}

void Lv999AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    lfoBlock.assign ((size_t) juce::jmax (64, samplesPerBlock), 0.0f);
    delayL.prepare ((int) (sampleRate * 1.2));
    delayR.prepare ((int) (sampleRate * 1.2));
    delaySamples = (float) (sampleRate * 0.02);
    wobblePhase = 0.0;

    for (auto& k : wobbleKnots)
        k = rng.nextFloat() * 2.0f - 1.0f;

    pos  = 0.0;
    fade = 0.0f;
    lfoCurrent = 0.0f;
    lfoPhase   = 0.0;

    for (auto& k : knots)
        k = rng.nextFloat() * 2.0f - 1.0f;

    gate = false;
}

bool Lv999AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    return out == juce::AudioChannelSet::stereo()
        || out == juce::AudioChannelSet::mono();
}

//==============================================================================
void Lv999AudioProcessor::updateLfo (int numSamples)
{
    const float shape  = apvts.getRawParameterValue ("smooth")->load() * 0.01f;   // 0 = bars, 1 = smooth
    const float rateHz = juce::jmax (0.05f, apvts.getRawParameterValue ("rate")->load());
    const double period = 1.0 / (double) rateHz;                                  // seconds per step
    const double step   = 1.0 / (period * currentSampleRate);

    if ((int) lfoBlock.size() < numSamples)
        lfoBlock.assign ((size_t) numSamples, 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        lfoPhase += step;

        while (lfoPhase >= 1.0)
        {
            lfoPhase -= 1.0;
            knots[0] = knots[1];
            knots[1] = knots[2];
            knots[2] = knots[3];
            knots[3] = rng.nextFloat() * 2.0f - 1.0f;
        }

        const float t = (float) lfoPhase;

        // 0 = hard bars (sample & hold), 1 = fully rounded curve
        const float w  = juce::jmax (0.0001f, shape);
        float tw = juce::jlimit (0.0f, 1.0f, (t - (0.5f - 0.5f * w)) / w);

        // smootherstep: no corners, no overshoot
        tw = tw * tw * tw * (tw * (tw * 6.0f - 15.0f) + 10.0f);

        lfoCurrent = juce::jlimit (-1.0f, 1.0f, knots[1] + (knots[2] - knots[1]) * tw);
        lfoBlock[(size_t) i] = lfoCurrent;

        if (++histCount >= 96)
        {
            histCount = 0;

            const juce::SpinLock::ScopedLockType sl (historyLock);
            history[(size_t) historyPos] = lfoCurrent;
            historyPos = (historyPos + 1) % kHistory;
        }
    }

    lfoValue = lfoCurrent;
}

void Lv999AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    buffer.clear();

    //--- MIDI ---------------------------------------------------------------
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();

        if (m.isNoteOn())
        {
            pos  = 0.0;
            gate = true;
            lastNote = m.getNoteNumber();
        }
        else if (m.isNoteOff() || m.isAllNotesOff() || m.isAllSoundOff())
        {
            gate = false;
        }
    }

    updateLfo (n);

    auto s = std::atomic_load (&sampleData);

    if (s == nullptr || s->len < 2)
        return;

    const float shiftBase = apvts.getRawParameterValue ("shift")->load() * 0.01f;
    const float lowLim    = apvts.getRawParameterValue ("low")->load()  * 0.01f;
    const float highLim   = apvts.getRawParameterValue ("high")->load() * 0.01f;
    const double srRatio  = s->sr / currentSampleRate;

    // classic sampler: the note sets the base speed (C3 = original)
    const double noteRatio = std::pow (2.0, ((double) lastNote.load() - 60.0) / 12.0);

    const float fadeStep = (float) (1.0 / (0.006 * currentSampleRate));
    const bool  stereo   = s->stereo;

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    const int len = s->len;
    float f = fade;

    //--- ping pong delay -----------------------------------------------------
    const float delayAmt = apvts.getRawParameterValue ("delay")->load() * 0.01f;
    const bool  delayOn  = delayAmt > 0.005f;
    const float delayWet = 0.45f * juce::jlimit (0.0f, 1.0f, delayAmt * 5.0f);
    const float delayTarget = juce::jmax (2.0f, (20.0f + delayAmt * 780.0f) * 0.001f * (float) currentSampleRate);
    const float dCoef = (float) (1.0 - std::exp (-1.0 / (0.05 * currentSampleRate)));

    // the more delay, the more the repeats drift out of tune
    const float wobbleDepth = delayAmt * 0.025f * (float) currentSampleRate;

    for (int i = 0; i < n; ++i)
    {
        const float target = gate.load() ? 1.0f : 0.0f;

        if (f < target)      f = juce::jmin (target, f + fadeStep);
        else if (f > target) f = juce::jmax (target, f - fadeStep);

        float outl = 0.0f, outr = 0.0f;

        if (f > 0.0f)
        {
            // LFO rides on top of the knob; the fader clamps the result
            const float lfo   = lfoBlock[(size_t) i];
            const float raw   = shiftBase + lfo * 0.35f;
            const float total = juce::jlimit (lowLim, highLim, juce::jlimit (-1.0f, 1.0f, raw));

            const double dir  = total >= 0.0f ? 1.0 : -1.0;
            const double inc  = noteRatio * dir * (1.0 + std::abs ((double) total) * 3.0) * srRatio;

            pos += inc;

            if (pos >= (double) len) pos -= (double) len;
            else if (pos < 0.0)      pos += (double) len;

            const int   i0   = (int) pos;
            const int   i1   = (i0 + 1) % len;
            const float frac = (float) (pos - (double) i0);

            const float l = s->l[(size_t) i0] + (s->l[(size_t) i1] - s->l[(size_t) i0]) * frac;
            const float r = stereo
                ? s->r[(size_t) i0] + (s->r[(size_t) i1] - s->r[(size_t) i0]) * frac
                : l;

            outl = l * f * 0.9f;
            outr = r * f * 0.9f;
        }

        if (delayOn)
        {
            delaySamples += (delayTarget - delaySamples) * dCoef;

            // random pitch drift on the repeats
            wobblePhase += 1.0 / (wobblePeriod * currentSampleRate);

            if (wobblePhase >= 1.0)
            {
                wobblePhase -= 1.0;
                wobbleKnots[0] = wobbleKnots[1];
                wobbleKnots[1] = wobbleKnots[2];
                wobbleKnots[2] = wobbleKnots[3];
                wobbleKnots[3] = rng.nextFloat() * 2.0f - 1.0f;
                wobblePeriod = 0.5 + rng.nextFloat() * 0.9;
            }

            float tw = juce::jlimit (0.0f, 1.0f, (float) wobblePhase);
            tw = tw * tw * tw * (tw * (tw * 6.0f - 15.0f) + 10.0f);

            const float wob = (wobbleKnots[1] + (wobbleKnots[2] - wobbleKnots[1]) * tw) * wobbleDepth;
            const float dRead = juce::jmax (2.0f, delaySamples + wob);

            const float dl = delayL.read (dRead);
            const float dr = delayR.read (dRead);

            const float in = (outl + outr) * 0.5f;
            delayL.write (in + dr * 0.38f);
            delayR.write (dl);

            outl += dl * delayWet;
            outr += dr * delayWet;
        }
        else
        {
            delayL.write (0.0f);
            delayR.write (0.0f);
        }

        outL[i] = outl;
        if (outR != nullptr) outR[i] = outr;
    }

    fade = f;
}

//==============================================================================
juce::AudioProcessorEditor* Lv999AudioProcessor::createEditor()
{
    return new Lv999AudioProcessorEditor (*this);
}

//==============================================================================
void Lv999AudioProcessor::setSample (std::shared_ptr<const SampleData> newSample)
{
    std::atomic_store (&sampleData, std::move (newSample));
}

std::shared_ptr<const SampleData> Lv999AudioProcessor::getSample() const
{
    return std::atomic_load (&sampleData);
}

void Lv999AudioProcessor::readHistory (float* dest) const
{
    const juce::SpinLock::ScopedLockType sl (historyLock);

    for (int i = 0; i < kHistory; ++i)
        dest[i] = history[(size_t) ((historyPos + i) % kHistory)];
}

//==============================================================================
void Lv999AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // the cartridge carries its own sound: embed the sample as FLAC
    if (auto s = getSample())
    {
        if (s->len > 1)
        {
            juce::AudioBuffer<float> buf (s->stereo ? 2 : 1, s->len);
            std::copy (s->l.begin(), s->l.end(), buf.getWritePointer (0));

            if (s->stereo)
                std::copy (s->r.begin(), s->r.end(), buf.getWritePointer (1));

            auto* mos = new juce::MemoryOutputStream();
            juce::FlacAudioFormat flac;

            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (flac.createWriterFor (mos, s->sr,
                                                        (unsigned int) buf.getNumChannels(), 16, {}, 0)))
            {
                w->writeFromAudioSampleBuffer (buf, 0, s->len);
                w->flush();

                state.setProperty ("sample", juce::Base64::toBase64 (mos->getData(), mos->getDataSize()), nullptr);

                state.setProperty ("sampleSR",     s->sr,     nullptr);
                state.setProperty ("sampleStereo", s->stereo, nullptr);
            }
            else
            {
                delete mos;
            }
        }
    }

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void Lv999AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);

    if (state.hasProperty ("sample"))
    {
        juce::MemoryOutputStream mos;

        if (juce::Base64::convertFromBase64 (mos, state["sample"].toString()))
        {
            auto* mis = new juce::MemoryInputStream (mos.getData(), mos.getDataSize(), false);
            juce::FlacAudioFormat flac;

            std::unique_ptr<juce::AudioFormatReader> reader (flac.createReaderFor (mis, true));

            if (reader != nullptr && reader->lengthInSamples > 1)
            {
                const int len = (int) juce::jmin ((juce::int64) (reader->sampleRate * 8.0),
                                                   reader->lengthInSamples);

                auto s = std::make_shared<SampleData>();
                s->len    = len;
                s->sr     = reader->sampleRate;
                s->stereo = reader->numChannels > 1;
                s->l.resize ((size_t) len);
                s->r.resize ((size_t) len);

                juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
                reader->read (&buf, 0, len, 0, true, true);

                std::copy (buf.getReadPointer (0), buf.getReadPointer (0) + len, s->l.begin());

                if (reader->numChannels > 1)
                    std::copy (buf.getReadPointer (1), buf.getReadPointer (1) + len, s->r.begin());
                else
                    s->r = s->l;

                computePeaks (*s);
                setSample (s);
            }
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Lv999AudioProcessor();
}
