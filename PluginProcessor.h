#pragma once

#include <JuceHeader.h>
#include <memory>

//==============================================================================
struct SampleData
{
    static constexpr int numPeaks = 512;

    std::vector<float> l, r;
    int    len    = 0;
    double sr     = 44100.0;
    bool   stereo = false;

    std::array<float, numPeaks> peakMin {}, peakMax {};
};

void computePeaks (SampleData& s);
std::shared_ptr<SampleData> loadSampleFile (const juce::File& file);

//==============================================================================
class Lv999AudioProcessor final : public juce::AudioProcessor
{
public:
    Lv999AudioProcessor();
    ~Lv999AudioProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LV.999"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    void setSample (std::shared_ptr<const SampleData> newSample);
    std::shared_ptr<const SampleData> getSample() const;

    float getLfo()      const noexcept { return lfoValue.load(); }
    float getShiftMod() const noexcept { return lfoValue.load() * 0.35f; }
    bool  isGateOpen()  const noexcept { return gate.load(); }

    static constexpr int kHistory = 512;
    void readHistory (float* dest) const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateLfo (int numSamples);

    //--- ping pong delay ----------------------------------------------------
    struct DelayLine
    {
        std::vector<float> buf;
        int size = 0, writePos = 0;

        void prepare (int maxSamples)
        {
            size = juce::jmax (4, maxSamples);
            buf.assign ((size_t) size, 0.0f);
            writePos = 0;
        }

        float read (float delaySamples) const noexcept
        {
            float rp = (float) writePos - delaySamples;

            while (rp < 0.0f) rp += (float) size;

            const int   i0   = (int) rp % size;
            const int   i1   = (i0 + 1) % size;
            const float frac = rp - std::floor (rp);

            return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
        }

        void write (float v) noexcept
        {
            buf[(size_t) writePos] = v;
            writePos = (writePos + 1) % size;
        }
    };

    DelayLine delayL, delayR;
    float delaySamples = 480.0f;

    // random pitch drift for the repeats (tape style wobble)
    float  wobbleKnots[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    double wobblePhase = 0.0;
    double wobblePeriod = 0.8;

    std::vector<float> lfoBlock;
    int histCount = 0;

    std::shared_ptr<const SampleData> sampleData;

    double currentSampleRate = 48000.0;
    double pos  = 0.0;
    float  fade = 0.0f;

    // random LFO
    float  lfoCurrent = 0.0f;
    float  knots[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    double lfoPhase = 0.0;
    juce::Random rng;

    mutable juce::SpinLock historyLock;
    std::array<float, kHistory> history {};
    int historyPos = 0;

    std::atomic<float> lfoValue { 0.0f };
    std::atomic<bool>  gate { false };
    std::atomic<int>   lastNote { 60 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Lv999AudioProcessor)
};
