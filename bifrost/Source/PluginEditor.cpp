#include "PluginEditor.h"
#include "BinaryData.h"

BifrostAudioProcessorEditor::BifrostAudioProcessorEditor(
    BifrostAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), sampleDropTarget(p),
      waveformView(p), stateMapView(p), adsrEnvelopeView(p) {
  setLookAndFeel(&lookAndFeel);
  setSize(1160, 760);
  backgroundImage = juce::ImageFileFormat::loadFrom(
      BinaryData::background_png, BinaryData::background_pngSize);

  title.setText("B I F R O S T", juce::dontSendNotification);
  title.setJustificationType(juce::Justification::centredLeft);
  title.setFont(
      BifrostLookAndFeel::logoFont(21.0f).withExtraKerningFactor(0.12f));
  addAndMakeVisible(title);

  subtitle.setText("by klosabre", juce::dontSendNotification);
  subtitle.setJustificationType(juce::Justification::centredLeft);
  subtitle.setFont(
      BifrostLookAndFeel::uiFont(8.5f).withExtraKerningFactor(0.12f));
  subtitle.setColour(juce::Label::textColourId,
                     BifrostLookAndFeel::text().withAlpha(0.45f));
  addAndMakeVisible(subtitle);

  configureLabel(rootOverrideLabel, "ROOT HZ");
  configureLabel(modeLabel, "Mode");
  configureLabel(qualityLabel, "Quality");
  configureLabel(macrosLabel, "TIMBRE");
  configureLabel(envelopeLabel, "AMP ADSR");
  configureLabel(performanceLabel, "PERFORMANCE");
  configureLabel(bodyLabel, "Body");
  configureLabel(airLabel, "Air");
  configureLabel(metalLabel, "Metal");
  configureLabel(brightnessLabel, "Brightness");
  configureLabel(motionLabel, "Motion");
  configureLabel(inertiaLabel, "Inertia");
  configureLabel(mutationLabel, "Mutation");
  configureLabel(transientLabel, "Transient");
  configureLabel(attackLabel, "Attack");
  configureLabel(decayLabel, "Decay");
  configureLabel(sustainLabel, "Sustain");
  configureLabel(releaseLabel, "Release");
  configureLabel(adsrCurveLabel, "Curve");
  configureLabel(velocityLabel, "Velocity");
  configureLabel(panLabel, "Pan");
  configureLabel(stereoWidthLabel, "Width");
  configureLabel(stereoReconstructLabel, "Stereo");
  configureLabel(phaseOffsetLabel, "Phase");
  configureLabel(phaseRandomLabel, "Phase Rand");
  configureLabel(timeSyncLabel, "Time Sync");
  configureLabel(rootRandomLabel, "Start Rand");
  configureLabel(randomDirectionLabel, "Direction");
  configureLabel(loopStartLabel, "Loop Start");
  configureLabel(loopEndLabel, "Loop End");
  configureLabel(unisonVoicesLabel, "Unison");
  configureLabel(unisonDetuneLabel, "Detune");
  for (auto *label : {&macrosLabel, &envelopeLabel, &performanceLabel,
                      &modeLabel, &qualityLabel})
    label->setJustificationType(juce::Justification::centred);

  addAndMakeVisible(sampleDropTarget);
  addAndMakeVisible(waveformView);
  addAndMakeVisible(stateMapView);
  addAndMakeVisible(adsrEnvelopeView);
  addAndMakeVisible(analyzeButton);
  addAndMakeVisible(cancelButton);
  addAndMakeVisible(modeBox);
  addAndMakeVisible(qualityBox);
  addAndMakeVisible(rootOverrideSlider);

  for (auto *slider : {&bodySlider,
                       &airSlider,
                       &metalSlider,
                       &brightnessSlider,
                       &motionSlider,
                       &inertiaSlider,
                       &mutationSlider,
                       &transientSlider,
                       &attackSlider,
                       &decaySlider,
                       &sustainSlider,
                       &releaseSlider,
                       &adsrCurveSlider,
                       &velocitySlider,
                       &panSlider,
                       &stereoWidthSlider,
                       &stereoReconstructSlider,
                       &phaseOffsetSlider,
                       &phaseRandomSlider,
                       &timeSyncSlider,
                       &rootRandomSlider,
                       &randomDirectionSlider,
                       &loopStartSlider,
                       &loopEndSlider,
                       &unisonVoicesSlider,
                       &unisonDetuneSlider})
    addAndMakeVisible(*slider);

  configureRootOverrideSlider();
  modeBox.addItemList(juce::StringArray{"One-Shot", "Sustain", "Harmonizer"},
                      1);
  qualityBox.addItemList(juce::StringArray{"Eco", "Balanced", "High"}, 1);
  modeBox.setJustificationType(juce::Justification::centredLeft);
  qualityBox.setJustificationType(juce::Justification::centredLeft);

  configureSlider(bodySlider, "Body");
  configureSlider(airSlider, "Air");
  configureSlider(metalSlider, "Metal");
  configureSlider(brightnessSlider, "Brightness");
  configureSlider(motionSlider, "Motion");
  configureSlider(inertiaSlider, "Inertia");
  configureSlider(mutationSlider, "Mutation");
  configureSlider(transientSlider, "Transient");
  configureAdsrSlider(attackSlider, "Attack", true);
  configureAdsrSlider(decaySlider, "Decay", true);
  configureAdsrSlider(sustainSlider, "Sustain", false);
  configureAdsrSlider(releaseSlider, "Release", true);
  configureSlider(adsrCurveSlider, "ADSR Curve");
  configureSlider(velocitySlider, "Velocity Sensitivity");
  configureSlider(panSlider, "Pan");
  configureSlider(stereoWidthSlider, "Stereo Width");
  configureSlider(stereoReconstructSlider, "Stereo Reconstruction");
  configureSlider(phaseOffsetSlider, "Phase");
  configureSlider(phaseRandomSlider, "Phase Random");
  configureSlider(timeSyncSlider, "Time Sync");
  configureSlider(rootRandomSlider, "Start Random");
  configureSlider(randomDirectionSlider, "Random Direction");
  configureSlider(loopStartSlider, "Loop Start");
  configureSlider(loopEndSlider, "Loop End");
  configureSlider(unisonVoicesSlider, "Unison Voices");
  configureSlider(unisonDetuneSlider, "Unison Detune");
  auto normalizedText = [](double value) {
    return juce::String(static_cast<int>(std::round(value * 100.0))) + "%";
  };
  for (auto *slider :
       {&bodySlider, &airSlider, &metalSlider, &brightnessSlider, &motionSlider,
        &inertiaSlider, &mutationSlider, &transientSlider, &velocitySlider,
        &stereoWidthSlider, &stereoReconstructSlider, &phaseOffsetSlider,
        &phaseRandomSlider, &timeSyncSlider, &adsrCurveSlider,
        &rootRandomSlider, &randomDirectionSlider, &loopStartSlider,
        &loopEndSlider})
    slider->textFromValueFunction = normalizedText;
  panSlider.textFromValueFunction = [](double value) {
    if (std::abs(value) < 0.005)
      return juce::String("C");
    return juce::String(value < 0.0 ? "L " : "R ") +
           juce::String(static_cast<int>(std::round(std::abs(value) * 100.0))) +
           "%";
  };
  unisonVoicesSlider.textFromValueFunction = [](double value) {
    return juce::String(static_cast<int>(std::round(value))) + "x";
  };
  unisonDetuneSlider.textFromValueFunction = [](double value) {
    return juce::String(value, 1) + " c";
  };

  modeAttachment = std::make_unique<ComboBoxAttachment>(
      audioProcessor.parameters(), "mode", modeBox);
  qualityAttachment = std::make_unique<ComboBoxAttachment>(
      audioProcessor.parameters(), "quality", qualityBox);
  rootOverrideAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "root_override_hz", rootOverrideSlider);
  bodyAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "body", bodySlider);
  airAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "air", airSlider);
  metalAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "metal", metalSlider);
  brightnessAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "brightness", brightnessSlider);
  motionAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "motion", motionSlider);
  inertiaAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "inertia", inertiaSlider);
  mutationAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "mutation", mutationSlider);
  transientAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "transient", transientSlider);
  attackAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "attack", attackSlider);
  decayAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "decay", decaySlider);
  sustainAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "sustain", sustainSlider);
  releaseAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "release", releaseSlider);
  adsrCurveAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "adsr_curve", adsrCurveSlider);
  velocityAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "velocity_sensitivity", velocitySlider);
  panAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "pan", panSlider);
  stereoWidthAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "stereo_width", stereoWidthSlider);
  stereoReconstructAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "stereo_reconstruction",
      stereoReconstructSlider);
  phaseOffsetAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "phase_offset", phaseOffsetSlider);
  phaseRandomAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "phase_random", phaseRandomSlider);
  timeSyncAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "time_sync", timeSyncSlider);
  rootRandomAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "root_random", rootRandomSlider);
  randomDirectionAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "random_direction", randomDirectionSlider);
  loopStartAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "loop_start", loopStartSlider);
  loopEndAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "loop_end", loopEndSlider);
  unisonVoicesAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "unison_voices", unisonVoicesSlider);
  unisonDetuneAttachment = std::make_unique<SliderAttachment>(
      audioProcessor.parameters(), "unison_detune", unisonDetuneSlider);

  for (auto *slider :
       {&bodySlider, &airSlider, &metalSlider, &brightnessSlider, &motionSlider,
        &inertiaSlider, &mutationSlider, &transientSlider, &velocitySlider,
        &stereoWidthSlider, &stereoReconstructSlider, &phaseOffsetSlider,
        &phaseRandomSlider, &timeSyncSlider, &adsrCurveSlider,
        &rootRandomSlider, &randomDirectionSlider, &loopStartSlider,
        &loopEndSlider})
    slider->textFromValueFunction = normalizedText;
  panSlider.textFromValueFunction = [](double value) {
    if (std::abs(value) < 0.005)
      return juce::String("C");
    return juce::String(value < 0.0 ? "L " : "R ") +
           juce::String(static_cast<int>(std::round(std::abs(value) * 100.0))) +
           "%";
  };
  unisonVoicesSlider.textFromValueFunction = [](double value) {
    return juce::String(static_cast<int>(std::round(value))) + "x";
  };
  unisonDetuneSlider.textFromValueFunction = [](double value) {
    return juce::String(value, 1) + " c";
  };
  auto secondsText = [](double value) {
    if (value < 1.0)
      return juce::String(value * 1000.0, 0) + " ms";
    return juce::String(value, 2) + " s";
  };
  attackSlider.textFromValueFunction = secondsText;
  decaySlider.textFromValueFunction = secondsText;
  releaseSlider.textFromValueFunction = secondsText;
  sustainSlider.textFromValueFunction = normalizedText;
  rootOverrideSlider.textFromValueFunction = [](double value) {
    if (value < 20.0)
      return juce::String("Auto");
    return juce::String(value, value < 1000.0 ? 1 : 0) + " Hz";
  };

  loopStartSlider.onValueChange = [this] { enforceLoopRange(loopStartSlider); };
  loopEndSlider.onValueChange = [this] { enforceLoopRange(loopEndSlider); };

  analyzeButton.onClick = [this] {
    audioProcessor.analysis().reanalyzeLastFile();
  };
  cancelButton.onClick = [this] { audioProcessor.analysis().cancel(); };

  startTimerHz(30);
}

BifrostAudioProcessorEditor::~BifrostAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

void BifrostAudioProcessorEditor::configureSlider(juce::Slider &slider,
                                                  const juce::String &name) {
  slider.setName(name);
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 15);
  slider.setNumDecimalPlacesToDisplay(2);
}

void BifrostAudioProcessorEditor::configureAdsrSlider(juce::Slider &slider,
                                                      const juce::String &name,
                                                      bool secondsValue) {
  slider.setName(name);
  slider.setSliderStyle(juce::Slider::LinearVertical);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 15);
  slider.setNumDecimalPlacesToDisplay(secondsValue ? 2 : 2);

  if (secondsValue) {
    slider.textFromValueFunction = [](double value) {
      if (value < 1.0)
        return juce::String(value * 1000.0, value < 0.1 ? 0 : 0) + " ms";
      return juce::String(value, 2) + " s";
    };
  } else {
    slider.textFromValueFunction = [](double value) {
      return juce::String(value, 2);
    };
  }
}

void BifrostAudioProcessorEditor::configureRootOverrideSlider() {
  rootOverrideSlider.setName("Root Override Hz");
  rootOverrideSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  rootOverrideSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 18);
  rootOverrideSlider.setNumDecimalPlacesToDisplay(0);
  rootOverrideSlider.textFromValueFunction = [](double value) {
    if (value < 20.0)
      return juce::String("Auto");
    return juce::String(static_cast<int>(std::round(value))) + " Hz";
  };
}

void BifrostAudioProcessorEditor::configureLabel(juce::Label &label,
                                                 const juce::String &text,
                                                 float alpha) {
  label.setText(text.toUpperCase(), juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centredLeft);
  label.setColour(juce::Label::textColourId,
                  BifrostLookAndFeel::text().withAlpha(alpha));
  label.setFont(
      BifrostLookAndFeel::uiFont(11.5f, true).withExtraKerningFactor(0.10f));
  addAndMakeVisible(label);
}

void BifrostAudioProcessorEditor::placeLabeledKnob(juce::Rectangle<int> bounds,
                                                   juce::Slider &slider,
                                                   juce::Label &label) {
  label.setJustificationType(juce::Justification::centred);
  label.setBounds(bounds.removeFromTop(15));
  slider.setBounds(bounds.reduced(8, 1));
}

void BifrostAudioProcessorEditor::enforceLoopRange(juce::Slider &movedSlider) {
  if (adjustingLoopRange)
    return;

  juce::ScopedValueSetter<bool> lock(adjustingLoopRange, true);
  constexpr double minLoopGap = 0.01;
  const double start = loopStartSlider.getValue();
  const double end = loopEndSlider.getValue();

  if (&movedSlider == &loopStartSlider && start > end - minLoopGap) {
    loopStartSlider.setValue(
        std::max(loopStartSlider.getMinimum(), end - minLoopGap),
        juce::sendNotificationSync);
  } else if (&movedSlider == &loopEndSlider && end < start + minLoopGap) {
    loopEndSlider.setValue(
        std::min(loopEndSlider.getMaximum(), start + minLoopGap),
        juce::sendNotificationSync);
  }
}

void BifrostAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(BifrostLookAndFeel::background());
  if (backgroundImage.isValid()) {
    g.setOpacity(BifrostLookAndFeel::backgroundImageOpacity);
    g.drawImage(
        backgroundImage, getLocalBounds().toFloat(),
        juce::RectanglePlacement(juce::RectanglePlacement::fillDestination));
    g.setOpacity(1.0f);
  }

  auto outer = getLocalBounds().toFloat().reduced(5.0f);
  g.setColour(BifrostLookAndFeel::panel().withAlpha(
      BifrostLookAndFeel::outerBoxFillOpacity));
  g.fillRoundedRectangle(outer, 5.0f);
  g.setColour(BifrostLookAndFeel::panelEdge().withAlpha(
      BifrostLookAndFeel::outerBoxBorderOpacity));
  g.drawRoundedRectangle(outer, 5.0f, 1.0f);

  for (auto section : {rootSectionBounds, envelopeSectionBounds,
                       performanceSectionBounds, timbreSectionBounds}) {
    if (!section.isEmpty())
      BifrostLookAndFeel::fillPanel(g, section.toFloat(), 5.0f);
  }

  BifrostLookAndFeel::drawBifrostGlyph(
      g, juce::Rectangle<float>(24.0f, 24.0f, 34.0f, 34.0f), 0.88f);
}

void BifrostAudioProcessorEditor::resized() {
  rootSectionBounds = {};
  envelopeSectionBounds = {};
  performanceSectionBounds = {};
  timbreSectionBounds = {};

  auto area = getLocalBounds().reduced(18);
  auto titleRow = area.removeFromTop(50);
  auto mark = titleRow.removeFromLeft(58);
  auto titleStrip = titleRow.removeFromLeft(202);
  title.setBounds(titleStrip.removeFromTop(24).reduced(0, 7));
  subtitle.setBounds(titleStrip.removeFromTop(14).withTrimmedLeft(0));
  juce::ignoreUnused(mark);
  titleRow.removeFromLeft(10);
  auto modeArea = titleRow.removeFromLeft(142).reduced(4, 6);
  modeLabel.setBounds(modeArea.removeFromTop(12));
  modeBox.setBounds(modeArea);
  auto qualityArea = titleRow.removeFromLeft(132).reduced(4, 6);
  qualityLabel.setBounds(qualityArea.removeFromTop(12));
  qualityBox.setBounds(qualityArea);
  analyzeButton.setBounds(titleRow.removeFromLeft(96).reduced(4, 6));
  cancelButton.setBounds(titleRow.removeFromLeft(86).reduced(4, 6));

  area.removeFromTop(8);
  auto top = area.removeFromTop(250);
  auto left = top.removeFromLeft(352);
  auto sampleBounds = left.removeFromTop(150).reduced(4);
  sampleDropTarget.setBounds(sampleBounds);
  auto rootRow = sampleBounds.reduced(18, 0).removeFromBottom(30);
  rootOverrideLabel.setBounds(rootRow.removeFromLeft(44).withHeight(24));
  rootOverrideSlider.setBounds(rootRow.withHeight(24));
  rootOverrideLabel.toFront(false);
  rootOverrideSlider.toFront(false);
  waveformView.setBounds(left.reduced(4, 6));
  stateMapView.setBounds(top.reduced(4));

  area.removeFromTop(8);

  auto controls = area;
  envelopeSectionBounds = controls.removeFromLeft(404).reduced(0, 1);
  auto envelope = envelopeSectionBounds.reduced(10, 8);
  envelopeLabel.setBounds(envelope.removeFromTop(18));
  adsrEnvelopeView.setBounds(envelope.removeFromTop(104).reduced(0, 2));
  envelope.removeFromTop(4);
  auto adsrRow = envelope.removeFromTop(138).reduced(2, 0);
  auto adsrWidth = adsrRow.getWidth() / 4;
  auto placeAdsr = [&adsrRow, adsrWidth](juce::Slider &slider,
                                         juce::Label &label) {
    auto cell = adsrRow.removeFromLeft(adsrWidth).reduced(5, 1);
    label.setJustificationType(juce::Justification::centred);
    label.setBounds(cell.removeFromTop(16));
    slider.setBounds(cell);
  };
  placeAdsr(attackSlider, attackLabel);
  placeAdsr(decaySlider, decayLabel);
  placeAdsr(sustainSlider, sustainLabel);
  placeAdsr(releaseSlider, releaseLabel);
  auto loopAndCurve = envelope;
  if (!loopAndCurve.isEmpty()) {
    auto quarter = loopAndCurve.getWidth() / 4;
    placeLabeledKnob(loopAndCurve.removeFromLeft(quarter).reduced(7),
                     adsrCurveSlider, adsrCurveLabel);
    placeLabeledKnob(loopAndCurve.removeFromLeft(quarter).reduced(7),
                     loopStartSlider, loopStartLabel);
    placeLabeledKnob(loopAndCurve.removeFromLeft(quarter).reduced(7),
                     loopEndSlider, loopEndLabel);
    placeLabeledKnob(loopAndCurve.reduced(7), timeSyncSlider, timeSyncLabel);
  }

  controls.removeFromLeft(14);
  performanceSectionBounds = controls.removeFromTop(184).reduced(0, 1);
  auto performanceArea = performanceSectionBounds.reduced(18, 8);
  performanceLabel.setBounds(performanceArea.removeFromTop(18));
  auto performance = performanceArea;
  const int performanceColW = performance.getWidth() / 5;
  const int performanceRowH = performance.getHeight() / 2;
  auto placePerformance = [this, &performance, performanceColW](
                              juce::Slider &slider, juce::Label &label) {
    placeLabeledKnob(performance.removeFromLeft(performanceColW).reduced(12, 2),
                     slider, label);
  };
  auto performanceTop = performance.removeFromTop(performanceRowH);
  auto performanceBottom = performance;
  performance = performanceTop;
  placePerformance(velocitySlider, velocityLabel);
  placePerformance(panSlider, panLabel);
  placePerformance(stereoWidthSlider, stereoWidthLabel);
  placePerformance(unisonVoicesSlider, unisonVoicesLabel);
  placePerformance(phaseOffsetSlider, phaseOffsetLabel);
  performance = performanceBottom;
  placePerformance(unisonDetuneSlider, unisonDetuneLabel);
  placePerformance(stereoReconstructSlider, stereoReconstructLabel);
  placePerformance(phaseRandomSlider, phaseRandomLabel);
  placePerformance(rootRandomSlider, rootRandomLabel);
  placePerformance(randomDirectionSlider, randomDirectionLabel);

  controls.removeFromTop(8);
  timbreSectionBounds = controls.reduced(0, 1);
  auto macrosArea = timbreSectionBounds.reduced(18, 8);
  macrosLabel.setBounds(macrosArea.removeFromTop(18));
  auto macros = macrosArea;
  const int colW = macros.getWidth() / 4;
  const int rowH = macros.getHeight() / 2;
  auto place = [this, &macros, colW](juce::Slider &slider, juce::Label &label) {
    placeLabeledKnob(macros.removeFromLeft(colW).reduced(12, 2), slider, label);
  };

  auto topRow = macros.removeFromTop(rowH);
  auto bottomRow = macros;
  macros = topRow;
  place(bodySlider, bodyLabel);
  place(airSlider, airLabel);
  place(metalSlider, metalLabel);
  place(brightnessSlider, brightnessLabel);
  macros = bottomRow;
  place(motionSlider, motionLabel);
  place(inertiaSlider, inertiaLabel);
  place(mutationSlider, mutationLabel);
  place(transientSlider, transientLabel);
}

void BifrostAudioProcessorEditor::timerCallback() {
  sampleDropTarget.repaint();
  waveformView.repaint();
  stateMapView.repaint();
  adsrEnvelopeView.repaint();
}
