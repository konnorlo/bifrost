#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/AdsrEnvelopeView.h"
#include "ui/BifrostLookAndFeel.h"
#include "ui/SampleDropTarget.h"
#include "ui/StateMapView.h"
#include "ui/WaveformView.h"

class BifrostAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit BifrostAudioProcessorEditor(BifrostAudioProcessor&);
    ~BifrostAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BifrostAudioProcessor& audioProcessor;
    BifrostLookAndFeel lookAndFeel;
    SampleDropTarget sampleDropTarget;
    WaveformView waveformView;
    StateMapView stateMapView;
    AdsrEnvelopeView adsrEnvelopeView;
    juce::Image backgroundImage;

    juce::Label title;
    juce::Label subtitle;
    juce::Label rootOverrideLabel;
    juce::Label modeLabel;
    juce::Label qualityLabel;
    juce::Label macrosLabel;
    juce::Label envelopeLabel;
    juce::Label performanceLabel;
    juce::Label bodyLabel;
    juce::Label airLabel;
    juce::Label metalLabel;
    juce::Label brightnessLabel;
    juce::Label motionLabel;
    juce::Label inertiaLabel;
    juce::Label mutationLabel;
    juce::Label transientLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label adsrCurveLabel;
    juce::Label velocityLabel;
    juce::Label panLabel;
    juce::Label stereoWidthLabel;
    juce::Label stereoReconstructLabel;
    juce::Label phaseOffsetLabel;
    juce::Label phaseRandomLabel;
    juce::Label timeSyncLabel;
    juce::Label rootRandomLabel;
    juce::Label randomDirectionLabel;
    juce::Label loopStartLabel;
    juce::Label loopEndLabel;
    juce::Label unisonVoicesLabel;
    juce::Label unisonDetuneLabel;
    juce::TextButton analyzeButton { "Analyze" };
    juce::TextButton cancelButton { "Cancel" };
    juce::ComboBox modeBox;
    juce::ComboBox qualityBox;
    juce::Slider rootOverrideSlider;
    juce::Slider bodySlider;
    juce::Slider airSlider;
    juce::Slider metalSlider;
    juce::Slider brightnessSlider;
    juce::Slider motionSlider;
    juce::Slider inertiaSlider;
    juce::Slider mutationSlider;
    juce::Slider transientSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider adsrCurveSlider;
    juce::Slider velocitySlider;
    juce::Slider panSlider;
    juce::Slider stereoWidthSlider;
    juce::Slider stereoReconstructSlider;
    juce::Slider phaseOffsetSlider;
    juce::Slider phaseRandomSlider;
    juce::Slider timeSyncSlider;
    juce::Slider rootRandomSlider;
    juce::Slider randomDirectionSlider;
    juce::Slider loopStartSlider;
    juce::Slider loopEndSlider;
    juce::Slider unisonVoicesSlider;
    juce::Slider unisonDetuneSlider;
    juce::Rectangle<int> rootSectionBounds;
    juce::Rectangle<int> envelopeSectionBounds;
    juce::Rectangle<int> performanceSectionBounds;
    juce::Rectangle<int> timbreSectionBounds;
    bool adjustingLoopRange = false;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ComboBoxAttachment> qualityAttachment;
    std::unique_ptr<SliderAttachment> rootOverrideAttachment;
    std::unique_ptr<SliderAttachment> bodyAttachment;
    std::unique_ptr<SliderAttachment> airAttachment;
    std::unique_ptr<SliderAttachment> metalAttachment;
    std::unique_ptr<SliderAttachment> brightnessAttachment;
    std::unique_ptr<SliderAttachment> motionAttachment;
    std::unique_ptr<SliderAttachment> inertiaAttachment;
    std::unique_ptr<SliderAttachment> mutationAttachment;
    std::unique_ptr<SliderAttachment> transientAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> adsrCurveAttachment;
    std::unique_ptr<SliderAttachment> velocityAttachment;
    std::unique_ptr<SliderAttachment> panAttachment;
    std::unique_ptr<SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<SliderAttachment> stereoReconstructAttachment;
    std::unique_ptr<SliderAttachment> phaseOffsetAttachment;
    std::unique_ptr<SliderAttachment> phaseRandomAttachment;
    std::unique_ptr<SliderAttachment> timeSyncAttachment;
    std::unique_ptr<SliderAttachment> rootRandomAttachment;
    std::unique_ptr<SliderAttachment> randomDirectionAttachment;
    std::unique_ptr<SliderAttachment> loopStartAttachment;
    std::unique_ptr<SliderAttachment> loopEndAttachment;
    std::unique_ptr<SliderAttachment> unisonVoicesAttachment;
    std::unique_ptr<SliderAttachment> unisonDetuneAttachment;

    void configureSlider(juce::Slider& slider, const juce::String& name);
    void configureAdsrSlider(juce::Slider& slider, const juce::String& name, bool secondsValue);
    void configureRootOverrideSlider();
    void configureLabel(juce::Label& label, const juce::String& text, float alpha = 0.68f);
    void placeLabeledKnob(juce::Rectangle<int> bounds, juce::Slider& slider, juce::Label& label);
    void enforceLoopRange(juce::Slider& movedSlider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BifrostAudioProcessorEditor)
};
