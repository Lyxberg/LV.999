#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class Lv999AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer,
                                        public juce::FileDragAndDropTarget
{
public:
    explicit Lv999AudioProcessorEditor (Lv999AudioProcessor&);
    ~Lv999AudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;

private:
    void timerCallback() override;

    void   drawPanel (juce::Graphics&, juce::Rectangle<int> area, float k);
    void drawSample (juce::Graphics&, juce::Rectangle<int> area, float k);
    void drawLfo (juce::Graphics&, juce::Rectangle<int> area, float k);
    void drawRange (juce::Graphics&, float k);
    void drawSpeed (juce::Graphics&, float k);
    void drawKnob (juce::Graphics&, juce::Point<float> centre, float radius,
                   float angle01, bool active, float k);

    juce::Rectangle<int> designRect (int x, int y, int w, int h) const;

    juce::Rectangle<float> rangeTrack() const;
    float rangeValToY (float v) const;
    float rangeYToVal (float y) const;

    juce::Rectangle<float> speedTrack() const;
    float speedToY (float hz) const;
    float speedYToHz (float y) const;

    float smoothValue() const;
    float shiftValue() const;
    float lowValue() const;
    float highValue() const;
    float rateValue() const;
    float delayValue() const;
    void  setParam (const char* id, float value);

    Lv999AudioProcessor& processorRef;

    int   dragKnob = 0;          // 1 smooth, 2 shift, 3 range high, 4 range low
    float dragStartValue = 0.0f;
    bool  dropHighlight = false;

    std::array<float, Lv999AudioProcessor::kHistory> lfoHistory {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Lv999AudioProcessorEditor)
};
