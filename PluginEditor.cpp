#include "PluginEditor.h"

namespace
{
    const juce::Colour kBlue       { 0xff3e72ac };
    const juce::Colour kBlueTop    { 0xff4e86c0 };
    const juce::Colour kBlueLow    { 0xff1f5082 };
    const juce::Colour kBlueShadow { 0xff143c63 };
    const juce::Colour kGreen      { 0xff30d07c };
    const juce::Colour kGreenDark  { 0xff1b9553 };
    const juce::Colour kBlack      { 0xff0b0b0b };
    const juce::Colour kPurple     { 0xff4a3b6b };
    const juce::Colour kPurpleDark { 0xff2b2242 };
    const juce::Colour kScreenHi   { 0xff175539 };
    const juce::Colour kScreenLo   { 0xff071d12 };
    const juce::Colour kCrt        { 0xff7cf0a8 };
    const juce::Colour kWhite      { 0xffffffff };

    constexpr float kDesignW = 355.0f;
    constexpr float kDesignH = 415.0f;

    constexpr float kKnob1X = 216.0f, kKnob1Y = 247.0f;
    constexpr float kKnob2X = 276.0f, kKnob2Y = 247.0f;
    constexpr float kKnobR  = 21.0f;

    constexpr int kBarX = 182, kBarY = 178, kBarW = 16, kBarH = 94;
    constexpr int kSpdX = 310, kSpdY = 178, kSpdW = 16, kSpdH = 90;
    constexpr float kDelayX = 310.0f, kDelayY = 314.0f, kDelayR = 15.0f;
}

//==============================================================================
Lv999AudioProcessorEditor::Lv999AudioProcessorEditor (Lv999AudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setResizable (false, false);
    setSize (420, 490);
    startTimerHz (30);
}

//==============================================================================
juce::Rectangle<int> Lv999AudioProcessorEditor::designRect (int x, int y, int w, int h) const
{
    const float sx = (float) getWidth()  / kDesignW;
    const float sy = (float) getHeight() / kDesignH;

    return { juce::roundToInt ((float) x * sx), juce::roundToInt ((float) y * sy),
             juce::roundToInt ((float) w * sx), juce::roundToInt ((float) h * sy) };
}

float Lv999AudioProcessorEditor::smoothValue() const
{
    return processorRef.apvts.getRawParameterValue ("smooth")->load();
}

float Lv999AudioProcessorEditor::shiftValue() const
{
    return processorRef.apvts.getRawParameterValue ("shift")->load();
}

float Lv999AudioProcessorEditor::lowValue() const
{
    return processorRef.apvts.getRawParameterValue ("low")->load() * 0.01f;
}

float Lv999AudioProcessorEditor::highValue() const
{
    return processorRef.apvts.getRawParameterValue ("high")->load() * 0.01f;
}

float Lv999AudioProcessorEditor::rateValue() const
{
    return processorRef.apvts.getRawParameterValue ("rate")->load();
}

float Lv999AudioProcessorEditor::delayValue() const
{
    return processorRef.apvts.getRawParameterValue ("delay")->load();
}

void Lv999AudioProcessorEditor::setParam (const char* id, float value)
{
    if (auto* p = processorRef.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

//==============================================================================
juce::Rectangle<float> Lv999AudioProcessorEditor::rangeTrack() const
{
    return designRect (kBarX, kBarY, kBarW, kBarH).toFloat().reduced (5.0f, 6.0f);
}

float Lv999AudioProcessorEditor::rangeValToY (float v) const
{
    auto t = rangeTrack();
    return t.getBottom() - (v + 1.0f) * 0.5f * t.getHeight();
}

float Lv999AudioProcessorEditor::rangeYToVal (float y) const
{
    auto t = rangeTrack();
    return juce::jlimit (-1.0f, 1.0f, (t.getBottom() - y) / juce::jmax (1.0f, t.getHeight()) * 2.0f - 1.0f);
}

juce::Rectangle<float> Lv999AudioProcessorEditor::speedTrack() const
{
    return designRect (kSpdX, kSpdY, kSpdW, kSpdH).toFloat().reduced (5.0f, 6.0f);
}

float Lv999AudioProcessorEditor::speedToY (float hz) const
{
    auto t = speedTrack();
    const float u = std::log (juce::jlimit (1.0f, 30.0f, hz)) / std::log (30.0f);
    return t.getBottom() - u * t.getHeight();
}

float Lv999AudioProcessorEditor::speedYToHz (float y) const
{
    auto t = speedTrack();
    const float u = juce::jlimit (0.0f, 1.0f, (t.getBottom() - y) / juce::jmax (1.0f, t.getHeight()));
    return std::pow (30.0f, u);
}

//==============================================================================
void Lv999AudioProcessorEditor::timerCallback()
{
    repaint();
}

//==============================================================================
void Lv999AudioProcessorEditor::paint (juce::Graphics& g)
{
    const float sx = (float) getWidth()  / kDesignW;
    const float sy = (float) getHeight() / kDesignH;
    const float k  = juce::jmin (sx, sy);

    //--- case ---------------------------------------------------------------
    g.fillAll (kBlue);

    g.setGradientFill (juce::ColourGradient (kBlueTop, 0.0f, 0.0f, kBlue, 0.0f, 26.0f * k, false));
    g.fillRect (0.0f, 0.0f, (float) getWidth(), 26.0f * k);

    g.setColour (kBlueLow);
    g.fillRect (designRect (0, 345, 355, 70));

    g.setColour (kBlueShadow.withAlpha (0.55f));
    g.fillRect (0.0f, 345.0f * sy, (float) getWidth(), 2.0f * k);

    g.setColour (kBlueShadow);
    g.drawRect (getLocalBounds(), juce::roundToInt (2.0f * k));

    //--- cartridge shell ----------------------------------------------------
    juce::Path card;
    card.startNewSubPath (10.0f * sx, 10.0f * sy);
    card.lineTo (345.0f * sx, 10.0f * sy);
    card.lineTo (345.0f * sx, 345.0f * sy);
    card.lineTo (62.0f * sx, 345.0f * sy);
    card.lineTo (10.0f * sx, 293.0f * sy);
    card.closeSubPath();

    g.setColour (kBlueShadow.withAlpha (0.5f));
    g.fillPath (card, juce::AffineTransform::translation (4.0f * k, 5.0f * k));

    g.setColour (kGreen);
    g.fillPath (card);

    //--- the whole face is the screen ---------------------------------------
    juce::Path screen;
    screen.startNewSubPath (17.0f * sx, 17.0f * sy);
    screen.lineTo (338.0f * sx, 17.0f * sy);
    screen.lineTo (338.0f * sx, 338.0f * sy);
    screen.lineTo (65.0f * sx, 338.0f * sy);
    screen.lineTo (17.0f * sx, 290.0f * sy);
    screen.closeSubPath();

    g.setGradientFill (juce::ColourGradient (kScreenHi, 0.0f, 17.0f * sy,
                                             kScreenLo, 0.0f, 338.0f * sy, false));
    g.fillPath (screen);

    // glass reflection
    {
        const juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (screen);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f),
                                                 0.0f, 17.0f * sy,
                                                 juce::Colours::transparentBlack,
                                                 0.0f, 140.0f * sy, false));
        g.fillPath (screen);

        // scanlines
        g.setColour (juce::Colours::black.withAlpha (0.17f));

        for (int i = 0; i < 140; ++i)
        {
            const float y = 17.0f * sy + (float) i * 3.0f * k;

            if (y < 338.0f * sy)
                g.drawHorizontalLine (juce::roundToInt (y), 0.0f, (float) getWidth());
        }

        // vignette
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack,
                                                 (float) getWidth() * 0.5f,
                                                 (float) getHeight() * 0.33f,
                                                 juce::Colours::black.withAlpha (0.22f),
                                                 (float) getWidth() * 0.5f,
                                                 (float) getHeight() * 0.62f,
                                                 true));
        g.fillPath (screen);
    }

    g.setColour (kBlack);
    g.strokePath (card, juce::PathStrokeType (3.0f * k));

    g.setColour (kGreenDark);
    g.strokePath (screen, juce::PathStrokeType (2.0f * k));

    //--- panels -------------------------------------------------------------
    auto sampleArea = designRect (30, 38, 295, 120);
    auto lfoArea    = designRect (30, 178, 148, 94);

    drawPanel (g, sampleArea, k);
    drawPanel (g, lfoArea, k);

    if (dropHighlight)
    {
        g.setColour (kWhite);
        g.drawRect (sampleArea.expanded (juce::roundToInt (3.0f * k)), juce::roundToInt (2.0f * k));
    }

    drawSample (g, sampleArea.reduced (juce::roundToInt (5.0f * k)), k);
    drawLfo (g, lfoArea.reduced (juce::roundToInt (5.0f * k)), k);

    //--- range fader --------------------------------------------------------
    drawRange (g, k);
    drawSpeed (g, k);

    //--- random -------------------------------------------------------------
    {
        auto r = designRect (204, 180, 100, 42);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.setFont (juce::Font (26.0f * k, juce::Font::bold));
        g.drawText ("random", r.translated (juce::roundToInt (2.0f * k), juce::roundToInt (2.0f * k)),
                    juce::Justification::centredLeft, false);

        g.setColour (kCrt);
        g.drawText ("random", r, juce::Justification::centredLeft, false);
    }

    //--- knobs --------------------------------------------------------------
    const float modShift = processorRef.getShiftMod();
    const float rawShift = shiftValue() * 0.01f + modShift;
    const float clamped  = juce::jlimit (lowValue(), highValue(), juce::jlimit (-1.0f, 1.0f, rawShift));

    const float smoothAng = juce::jlimit (0.0f, 1.0f, smoothValue() * 0.01f);
    const float shiftAng  = juce::jlimit (0.0f, 1.0f, clamped * 0.5f + 0.5f);

    drawKnob (g, { kKnob1X * sx, kKnob1Y * sy }, kKnobR * k, smoothAng, dragKnob == 1, k);
    drawKnob (g, { kKnob2X * sx, kKnob2Y * sy }, kKnobR * k, shiftAng,  dragKnob == 2, k);

    g.setColour (kCrt.withAlpha (0.95f));
    g.setFont (juce::Font (13.0f * k, juce::Font::bold));
    g.drawText ("smooth",   designRect (176, 282, 80, 16), juce::Justification::centred, false);
    g.drawText ("shift+/-", designRect (232, 282, 88, 16), juce::Justification::centred, false);

    //--- delay --------------------------------------------------------------
    {
        const float dAng = juce::jlimit (0.0f, 1.0f, delayValue() * 0.01f);

        drawKnob (g, { kDelayX * sx, kDelayY * sy }, kDelayR * k, dAng, dragKnob == 6, k);

        g.setColour (kCrt.withAlpha (0.95f));
        g.setFont (juce::Font (12.0f * k, juce::Font::bold));
        g.drawText ("delay", designRect (244, 306, 46, 14), juce::Justification::right, false);
    }

    //--- the name -----------------------------------------------------------
    {
        auto r = designRect (14, 344, 336, 64);

        g.setColour (kPurpleDark.withAlpha (0.55f));
        g.setFont (juce::Font ("Impact", 56.0f * k, juce::Font::bold));
        g.drawText ("LV.999", r.translated (juce::roundToInt (2.0f * k), juce::roundToInt (2.0f * k)),
                    juce::Justification::centredLeft, false);

        g.setColour (kPurple);
        g.drawText ("LV.999", r, juce::Justification::centredLeft, false);
    }
}

//==============================================================================
void Lv999AudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> area, float k)
{
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRect (area);

    g.setColour (kCrt.withAlpha (0.55f));
    g.drawRect (area, juce::roundToInt (juce::jmax (1.0f, 2.0f * k)));

    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.45f), 0.0f, (float) area.getY(),
                                             juce::Colours::transparentBlack, 0.0f,
                                             (float) area.getY() + 10.0f * k, false));
    g.fillRect (area);
}

void Lv999AudioProcessorEditor::drawSample (juce::Graphics& g, juce::Rectangle<int> area, float k)
{
    juce::ignoreUnused (k);
    auto sample = processorRef.getSample();

    if (sample == nullptr || sample->len < 2)
    {
        g.setColour (kCrt.withAlpha (0.75f));
        g.setFont (juce::Font (15.0f * juce::jmin ((float) getWidth() / kDesignW,
                                                   (float) getHeight() / kDesignH), juce::Font::bold));
        g.drawText ("drop audio here", area, juce::Justification::centred, false);
        return;
    }

    const int w = area.getWidth();
    const float midY  = (float) area.getCentreY();
    const float halfH = (float) area.getHeight() * 0.46f;

    for (int x = 0; x < w; ++x)
    {
        const int b = juce::jlimit (0, SampleData::numPeaks - 1,
                                    (int) ((float) x / (float) juce::jmax (1, w) * SampleData::numPeaks));

        const float lo = sample->peakMin[(size_t) b];
        const float hi = sample->peakMax[(size_t) b];

        const float y1 = midY - hi * halfH;
        const float y2 = midY - lo * halfH;

        g.setColour (kCrt.withAlpha (0.22f));
        g.drawVerticalLine (area.getX() + x, y1 - 1.5f, y2 + 1.5f);

        g.setColour (kCrt);
        g.drawVerticalLine (area.getX() + x, y1, y2);
    }
}

void Lv999AudioProcessorEditor::drawLfo (juce::Graphics& g, juce::Rectangle<int> area, float k)
{
    processorRef.readHistory (lfoHistory.data());

    const int w = area.getWidth();
    const float midY  = (float) area.getCentreY();
    const float halfH = (float) area.getHeight() * 0.40f;

    juce::Path path;

    for (int x = 0; x < w; ++x)
    {
        const int idx = juce::jlimit (0, Lv999AudioProcessor::kHistory - 1,
                                      (int) ((float) x / (float) juce::jmax (1, w)
                                             * (float) Lv999AudioProcessor::kHistory));

        const float v  = lfoHistory[(size_t) idx];
        const float y  = midY - v * halfH;
        const float px = (float) area.getX() + (float) x;

        if (x == 0) path.startNewSubPath (px, y);
        else        path.lineTo (px, y);
    }

    g.setColour (kCrt.withAlpha (0.20f));
    g.strokePath (path, juce::PathStrokeType (5.0f * k, juce::PathStrokeType::curved));

    g.setColour (kCrt);
    g.strokePath (path, juce::PathStrokeType (2.0f * k, juce::PathStrokeType::curved));
}

void Lv999AudioProcessorEditor::drawRange (juce::Graphics& g, float k)
{
    auto bar = designRect (kBarX, kBarY, kBarW, kBarH);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (bar);

    g.setColour (kCrt.withAlpha (0.55f));
    g.drawRect (bar, juce::roundToInt (juce::jmax (1.0f, 2.0f * k)));

    auto t = rangeTrack();

    const float yHi = rangeValToY (highValue());
    const float yLo = rangeValToY (lowValue());

    g.setColour (kCrt.withAlpha (0.18f));
    g.fillRect (t.getX(), yHi, t.getWidth(), yLo - yHi);

    const float hh = 7.0f * k;
    const float hx = (float) bar.getX() + 1.0f * k;
    const float hw = (float) bar.getWidth() - 2.0f * k;

    for (int i = 0; i < 2; ++i)
    {
        const float y = (i == 0 ? yHi : yLo) - hh * 0.5f;

        g.setColour (kCrt.withAlpha (0.25f));
        g.fillRect (hx - 2.0f * k, y - 2.0f * k, hw + 4.0f * k, hh + 4.0f * k);

        g.setColour (kWhite);
        g.fillRect (hx, y, hw, hh);
    }
}

void Lv999AudioProcessorEditor::drawSpeed (juce::Graphics& g, float k)
{
    auto bar = designRect (kSpdX, kSpdY, kSpdW, kSpdH);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (bar);

    g.setColour (kCrt.withAlpha (0.55f));
    g.drawRect (bar, juce::roundToInt (juce::jmax (1.0f, 2.0f * k)));

    auto t = speedTrack();
    const float y = speedToY (rateValue());

    g.setColour (kCrt.withAlpha (0.18f));
    g.fillRect (t.getX(), y, t.getWidth(), t.getBottom() - y);

    const float hh = 7.0f * k;
    const float hx = (float) bar.getX() + 1.0f * k;
    const float hw = (float) bar.getWidth() - 2.0f * k;

    g.setColour (kCrt.withAlpha (0.25f));
    g.fillRect (hx - 2.0f * k, y - hh * 0.5f - 2.0f * k, hw + 4.0f * k, hh + 4.0f * k);

    g.setColour (kWhite);
    g.fillRect (hx, y - hh * 0.5f, hw, hh);

    g.setColour (kCrt.withAlpha (0.85f));
    g.setFont (juce::Font (9.0f * k, juce::Font::bold));
    g.drawText ("hz", designRect (kSpdX - 8, kSpdY + kSpdH + 1, kSpdW + 16, 12),
                juce::Justification::centred, false);
}

void Lv999AudioProcessorEditor::drawKnob (juce::Graphics& g, juce::Point<float> centre,
                                           float radius, float angle01, bool active, float k)
{
    // phosphor glow
    g.setColour (kCrt.withAlpha (active ? 0.35f : 0.18f));
    g.drawEllipse (centre.x - radius - 3.0f * k, centre.y - radius - 3.0f * k,
                   (radius + 3.0f * k) * 2.0f, (radius + 3.0f * k) * 2.0f, 3.0f * k);

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (centre.x - radius + 3.0f * k, centre.y - radius + 4.0f * k,
                   radius * 2.0f, radius * 2.0f);

    // body
    g.setGradientFill (juce::ColourGradient (kWhite, centre.x, centre.y - radius,
                                             juce::Colour (0xffc8d0d9), centre.x, centre.y + radius, false));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // rim
    g.setColour (kBlack);
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f,
                   active ? 4.0f * k : 2.5f * k);

    // pointer
    const float angle = (angle01 - 0.5f) * juce::MathConstants<float>::twoPi * 0.75f;
    const float len   = radius * 0.78f;

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (centre.x + 1.0f, centre.y + 2.0f,
                centre.x + 1.0f + std::sin (angle) * len,
                centre.y + 2.0f - std::cos (angle) * len,
                juce::jmax (2.0f, radius * 0.15f));

    g.setColour (kBlack);
    g.drawLine (centre.x, centre.y,
                centre.x + std::sin (angle) * len,
                centre.y - std::cos (angle) * len,
                juce::jmax (2.0f, radius * 0.15f));
}

//==============================================================================
void Lv999AudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    const float sx = (float) getWidth()  / kDesignW;
    const float sy = (float) getHeight() / kDesignH;
    const float k  = juce::jmin (sx, sy);

    dragKnob = 0;

    const juce::Point<float> k1 (kKnob1X * sx, kKnob1Y * sy);
    const juce::Point<float> k2 (kKnob2X * sx, kKnob2Y * sy);

    if (e.position.getDistanceFrom (k1) < kKnobR * k * 1.4f)
    {
        dragKnob = 1;
        dragStartValue = smoothValue();
    }
    else if (e.position.getDistanceFrom (k2) < kKnobR * k * 1.4f)
    {
        dragKnob = 2;
        dragStartValue = shiftValue();
    }
    else if (designRect (kBarX - 6, kBarY - 4, kBarW + 12, kBarH + 8).contains (e.getPosition()))
    {
        const float yHi = rangeValToY (highValue());
        const float yLo = rangeValToY (lowValue());

        dragKnob = std::abs (e.position.y - yHi) <= std::abs (e.position.y - yLo) ? 3 : 4;
    }
    else if (designRect (kSpdX - 6, kSpdY - 4, kSpdW + 12, kSpdH + 8).contains (e.getPosition()))
    {
        dragKnob = 5;
    }
    else if (e.position.getDistanceFrom ({ kDelayX * sx, kDelayY * sy }) < kDelayR * k * 1.5f)
    {
        dragKnob = 6;
        dragStartValue = delayValue();
    }

    repaint();
}

void Lv999AudioProcessorEditor::mouseDrag (const juce::MouseEvent& e)
{
    switch (dragKnob)
    {
        case 1:
            setParam ("smooth", juce::jlimit (0.0f, 100.0f,
                       dragStartValue - (float) e.getDistanceFromDragStartY() / 2.0f));
            break;

        case 2:
            setParam ("shift", juce::jlimit (-100.0f, 100.0f,
                       dragStartValue - (float) e.getDistanceFromDragStartY() / 2.0f));
            break;

        case 3:
        {
            const float v = juce::jmax (lowValue() + 0.02f, rangeYToVal (e.position.y));
            setParam ("high", v * 100.0f);
            break;
        }

        case 4:
        {
            const float v = juce::jmin (highValue() - 0.02f, rangeYToVal (e.position.y));
            setParam ("low", v * 100.0f);
            break;
        }

        case 5:
            setParam ("rate", speedYToHz (e.position.y));
            break;

        case 6:
            setParam ("delay", juce::jlimit (0.0f, 100.0f,
                       dragStartValue - (float) e.getDistanceFromDragStartY() / 2.0f));
            break;

        default: break;
    }

    repaint();
}

void Lv999AudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    dragKnob = 0;
    repaint();
}

void Lv999AudioProcessorEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const float sx = (float) getWidth()  / kDesignW;
    const float sy = (float) getHeight() / kDesignH;
    const float k  = juce::jmin (sx, sy);

    if (e.position.getDistanceFrom ({ kKnob1X * sx, kKnob1Y * sy }) < kKnobR * k * 1.4f)
        setParam ("smooth", 100.0f);
    else if (e.position.getDistanceFrom ({ kKnob2X * sx, kKnob2Y * sy }) < kKnobR * k * 1.4f)
        setParam ("shift", 0.0f);
    else if (designRect (kBarX - 6, kBarY - 4, kBarW + 12, kBarH + 8).contains (e.getPosition()))
    {
        setParam ("low", -100.0f);
        setParam ("high", 100.0f);
    }
    else if (designRect (kSpdX - 6, kSpdY - 4, kSpdW + 12, kSpdH + 8).contains (e.getPosition()))
    {
        setParam ("rate", 2.2f);
    }
    else if (e.position.getDistanceFrom ({ kDelayX * sx, kDelayY * sy }) < kDelayR * k * 1.5f)
    {
        setParam ("delay", 0.0f);
    }

    repaint();
}

//==============================================================================
bool Lv999AudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        const auto ext = juce::File (f).getFileExtension().toLowerCase();

        if (ext == ".wav" || ext == ".aif" || ext == ".aiff"
            || ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }

    return false;
}

void Lv999AudioProcessorEditor::fileDragEnter (const juce::StringArray&, int, int)
{
    dropHighlight = true;
    repaint();
}

void Lv999AudioProcessorEditor::fileDragExit (const juce::StringArray&)
{
    dropHighlight = false;
    repaint();
}

void Lv999AudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dropHighlight = false;

    for (const auto& f : files)
    {
        if (auto s = loadSampleFile (juce::File (f)))
        {
            processorRef.setSample (s);
            break;
        }
    }

    repaint();
}
