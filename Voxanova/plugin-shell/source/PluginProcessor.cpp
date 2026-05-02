#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr auto inputGainId = "inputGain";
constexpr auto outputGainId = "outputGain";
constexpr auto gateThresholdId = "gateThreshold";
constexpr auto stereoWidthId = "stereoWidth";
constexpr auto stereoLowBypassId = "stereoLowBypass";
constexpr auto preSaturationModeId = "preSaturationMode";
constexpr auto preSaturationAmountId = "preSaturationAmount";
constexpr auto postSaturationModeId = "postSaturationMode";
constexpr auto postSaturationAmountId = "postSaturationAmount";
constexpr auto peakEnabledId = "peakEnabled";
constexpr auto peakThresholdId = "peakThreshold";
constexpr auto glueEnabledId = "glueEnabled";
constexpr auto glueMultibandId = "glueMultiband";
constexpr auto glueThresholdId = "glueThreshold";
constexpr auto glueLowThresholdId = "glueLowThreshold";
constexpr auto glueLowMidThresholdId = "glueLowMidThreshold";
constexpr auto glueHighMidThresholdId = "glueHighMidThreshold";
constexpr auto glueAirThresholdId = "glueAirThreshold";
constexpr auto faceEnabledId = "faceEnabled";
constexpr auto faceThresholdId = "faceThreshold";
constexpr auto gateEnabledId = "gateEnabled";
constexpr auto stereoEnabledId = "stereoEnabled";
constexpr auto reverbEnabledId = "reverbEnabled";
constexpr auto reverbMixId = "reverbMix";
constexpr auto reverbDecayId = "reverbDecay";
constexpr auto reverbSizeId = "reverbSize";
constexpr auto reverbPredelayId = "reverbPredelay";
constexpr auto reverbLowCutId = "reverbLowCut";
constexpr auto reverbHighCutId = "reverbHighCut";
constexpr auto reverbModeId = "reverbMode";
constexpr auto reverbSyncId = "reverbSync";
constexpr auto reverbNoteModeId = "reverbNoteMode";
constexpr auto reverbDecaySyncId = "reverbDecaySync";
constexpr auto reverbPredelaySyncId = "reverbPredelaySync";
constexpr auto reverbDecayDivisionId = "reverbDecayDivision";
constexpr auto reverbPredelayDivisionId = "reverbPredelayDivision";
constexpr auto delayEnabledId = "delayEnabled";
constexpr auto delayMixId = "delayMix";
constexpr auto delayFeedbackId = "delayFeedback";
constexpr auto delayLowCutId = "delayLowCut";
constexpr auto delayHighCutId = "delayHighCut";
constexpr auto delaySyncId = "delaySync";
constexpr auto delayDivisionId = "delayDivision";
constexpr auto delayNoteModeId = "delayNoteMode";
constexpr auto delayTimeMsId = "delayTimeMs";
constexpr auto delayModeId = "delayMode";
constexpr auto delayPostReverbId = "delayPostReverb";
constexpr auto delayStyleId = "delayStyle";
constexpr auto compressorMinDb = -60.0f;
constexpr auto compressorMaxDb = 0.0f;
constexpr auto glueBandMinDb = -48.0f;
constexpr auto fullAmount = 100.0f;
constexpr std::array<float, 7> delayDivisionBeats { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
constexpr std::array<float, 8> reverbPredelayDivisionBeats { 0.0f, 0.0625f, 0.125f, 0.25f,
                                                             0.5f, 1.0f, 2.0f, 4.0f };
constexpr std::array<const char*, 11> delayStyleLabels {
  "Clean", "Digital", "Tape", "Studio Tape", "Old Tape", "Cheap Tape",
  "Analog", "Radio", "Telephone", "Dirty", "Ambient"
};
constexpr std::array<const char*, 22> reverbModeLabels {
  "Concert Hall", "Bright Hall", "Plate", "Room", "Chamber", "Random Space", "Chorus Space",
  "Ambience", "Sanctuary", "Dirty Hall", "Dirty Plate", "Smooth Plate", "Smooth Room",
  "Smooth Random", "Nonlin", "Chaotic Chamber", "Chaotic Hall", "Chaotic Neutral",
  "Cathedral", "Palace", "Chamber1979", "Hall1984"
};

juce::String dbLabel(float value, int)
{
  return juce::String(value, 1) + " dB";
}

juce::String percentLabel(float value, int)
{
  return juce::String(value, 0) + "%";
}

juce::String hzLabel(float value, int)
{
  return juce::String(value, 0) + " Hz";
}

juce::String msLabel(float value, int)
{
  return juce::String(value, 0) + " ms";
}

juce::String reverbDecayLabel(float value, int)
{
  const auto norm = juce::jlimit(0.0f, 1.0f, value / 100.0f);
  const auto seconds = 0.2f + ((std::pow(90.0f, norm) - 1.0f) / 89.0f) * 17.8f;
  return juce::String(seconds, seconds >= 10.0f ? 1 : 2) + " s";
}

juce::String reverbPredelayLabel(float value, int)
{
  return juce::String(value * 0.5f, 1) + " ms";
}

juce::String reverbLowCutLabel(float value, int)
{
  const auto hz = 20.0f * std::pow(50.0f, juce::jlimit(0.0f, 1.0f, value / 100.0f));
  return hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + " kHz" : juce::String(hz, 0) + " Hz";
}

juce::String reverbHighCutLabel(float value, int)
{
  const auto hz = 1500.0f * std::pow(13.333333f, juce::jlimit(0.0f, 1.0f, value / 100.0f));
  return hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + " kHz" : juce::String(hz, 0) + " Hz";
}

juce::String delayLowCutLabel(float value, int)
{
  const auto hz = 20.0f * std::pow(900.0f, juce::jlimit(0.0f, 1.0f, value / 100.0f));
  return hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + " kHz" : juce::String(hz, 0) + " Hz";
}

juce::String delayHighCutLabel(float value, int)
{
  const auto norm = juce::jlimit(0.0f, 1.0f, value / 100.0f);
  const auto hz = 160.0f * std::pow(125.0f, norm);
  return hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + " kHz" : juce::String(hz, 0) + " Hz";
}

juce::String delayDivisionLabel(float value, int)
{
  constexpr std::array<const char*, 7> labels { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
  return labels[static_cast<size_t>(juce::jlimit(0, 6, juce::roundToInt(value)))];
}

juce::String reverbPredelayDivisionLabel(float value, int)
{
  constexpr std::array<const char*, 8> labels { "None", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" };
  return labels[static_cast<size_t>(juce::jlimit(0, 7, juce::roundToInt(value)))];
}

juce::String saturationModeLabel(float value, int)
{
  constexpr std::array<const char*, 4> labels { "Off", "1073", "Tape", "Tube" };
  return labels[static_cast<size_t>(juce::jlimit(0, 3, juce::roundToInt(value)))];
}

juce::String delayNoteModeLabel(float value, int)
{
  constexpr std::array<const char*, 3> labels { "Note", "Dot", "Triplet" };
  return labels[static_cast<size_t>(juce::jlimit(0, 2, juce::roundToInt(value)))];
}

juce::String delayModeLabel(float value, int)
{
  constexpr std::array<const char*, 3> labels { "Normal", "Wide", "Ping-Pong" };
  return labels[static_cast<size_t>(juce::jlimit(0, 2, juce::roundToInt(value)))];
}

juce::String delayStyleLabel(float value, int)
{
  return delayStyleLabels[static_cast<size_t>(juce::jlimit(0, static_cast<int>(delayStyleLabels.size()) - 1,
                                                           juce::roundToInt(value)))];
}

juce::String reverbModeLabel(float value, int)
{
  return reverbModeLabels[static_cast<size_t>(juce::jlimit(0, static_cast<int>(reverbModeLabels.size()) - 1,
                                                           juce::roundToInt(value)))];
}

juce::String onOffLabel(bool value, int)
{
  return value ? "On" : "Off";
}

float thresholdEngagement(float levelDb, float thresholdDb, float kneeDb)
{
  if (kneeDb <= 0.0f)
    return levelDb > thresholdDb ? 1.0f : 0.0f;

  const auto transition = juce::jlimit(0.0f, 1.0f, (levelDb - thresholdDb + kneeDb * 0.5f) / kneeDb);
  return transition * transition * (3.0f - 2.0f * transition);
}
} // namespace

VoxanovaAudioProcessor::VoxanovaAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
  inputGainParam = parameters.getRawParameterValue(inputGainId);
  outputGainParam = parameters.getRawParameterValue(outputGainId);
  gateParam = parameters.getRawParameterValue(gateThresholdId);
  stereoWidthParam = parameters.getRawParameterValue(stereoWidthId);
  stereoLowBypassParam = parameters.getRawParameterValue(stereoLowBypassId);
  preSaturationModeParam = parameters.getRawParameterValue(preSaturationModeId);
  preSaturationAmountParam = parameters.getRawParameterValue(preSaturationAmountId);
  postSaturationModeParam = parameters.getRawParameterValue(postSaturationModeId);
  postSaturationAmountParam = parameters.getRawParameterValue(postSaturationAmountId);
  peakEnabledParam = parameters.getRawParameterValue(peakEnabledId);
  peakThresholdParam = parameters.getRawParameterValue(peakThresholdId);
  glueEnabledParam = parameters.getRawParameterValue(glueEnabledId);
  glueMultibandParam = parameters.getRawParameterValue(glueMultibandId);
  glueThresholdParam = parameters.getRawParameterValue(glueThresholdId);
  glueLowThresholdParam = parameters.getRawParameterValue(glueLowThresholdId);
  glueLowMidThresholdParam = parameters.getRawParameterValue(glueLowMidThresholdId);
  glueHighMidThresholdParam = parameters.getRawParameterValue(glueHighMidThresholdId);
  glueAirThresholdParam = parameters.getRawParameterValue(glueAirThresholdId);
  faceEnabledParam = parameters.getRawParameterValue(faceEnabledId);
  faceThresholdParam = parameters.getRawParameterValue(faceThresholdId);
  gateEnabledParam = parameters.getRawParameterValue(gateEnabledId);
  stereoEnabledParam = parameters.getRawParameterValue(stereoEnabledId);
  reverbEnabledParam = parameters.getRawParameterValue(reverbEnabledId);
  reverbMixParam = parameters.getRawParameterValue(reverbMixId);
  reverbDecayParam = parameters.getRawParameterValue(reverbDecayId);
  reverbSizeParam = parameters.getRawParameterValue(reverbSizeId);
  reverbPredelayParam = parameters.getRawParameterValue(reverbPredelayId);
  reverbLowCutParam = parameters.getRawParameterValue(reverbLowCutId);
  reverbHighCutParam = parameters.getRawParameterValue(reverbHighCutId);
  reverbModeParam = parameters.getRawParameterValue(reverbModeId);
  reverbSyncParam = parameters.getRawParameterValue(reverbSyncId);
  reverbNoteModeParam = parameters.getRawParameterValue(reverbNoteModeId);
  reverbDecaySyncParam = parameters.getRawParameterValue(reverbDecaySyncId);
  reverbPredelaySyncParam = parameters.getRawParameterValue(reverbPredelaySyncId);
  reverbDecayDivisionParam = parameters.getRawParameterValue(reverbDecayDivisionId);
  reverbPredelayDivisionParam = parameters.getRawParameterValue(reverbPredelayDivisionId);
  delayEnabledParam = parameters.getRawParameterValue(delayEnabledId);
  delayMixParam = parameters.getRawParameterValue(delayMixId);
  delayFeedbackParam = parameters.getRawParameterValue(delayFeedbackId);
  delayLowCutParam = parameters.getRawParameterValue(delayLowCutId);
  delayHighCutParam = parameters.getRawParameterValue(delayHighCutId);
  delaySyncParam = parameters.getRawParameterValue(delaySyncId);
  delayDivisionParam = parameters.getRawParameterValue(delayDivisionId);
  delayNoteModeParam = parameters.getRawParameterValue(delayNoteModeId);
  delayTimeMsParam = parameters.getRawParameterValue(delayTimeMsId);
  delayModeParam = parameters.getRawParameterValue(delayModeId);
  delayPostReverbParam = parameters.getRawParameterValue(delayPostReverbId);
  delayStyleParam = parameters.getRawParameterValue(delayStyleId);
}

VoxanovaAudioProcessor::APVTS::ParameterLayout VoxanovaAudioProcessor::createParameterLayout()
{
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  auto addFloat = [&params](const juce::String& id, const juce::String& name, float min, float max, float step,
                            float defaultValue, juce::String (*labelFn)(float, int)) {
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name, juce::NormalisableRange<float>(min, max, step), defaultValue,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(labelFn)));
  };

  auto addBool = [&params](const juce::String& id, const juce::String& name, bool defaultValue) {
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(id, 1), name, defaultValue,
        juce::AudioParameterBoolAttributes().withStringFromValueFunction(onOffLabel)));
  };

  addFloat(inputGainId, "Input Gain", -24.0f, 24.0f, 0.1f, 0.0f, dbLabel);
  addFloat(outputGainId, "Output Gain", -24.0f, 24.0f, 0.1f, 0.0f, dbLabel);
  addFloat(gateThresholdId, "Gate Threshold", -80.0f, 0.0f, 0.1f, -80.0f, dbLabel);
  addFloat(stereoWidthId, "Stereo Width", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(stereoLowBypassId, "Stereo Low Bypass", 0.0f, 500.0f, 1.0f, 0.0f, hzLabel);
  addFloat(preSaturationModeId, "Pre Saturation Type", 0.0f, 3.0f, 1.0f, 0.0f, saturationModeLabel);
  addFloat(preSaturationAmountId, "Pre Saturation Amount", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(postSaturationModeId, "Post Saturation Type", 0.0f, 3.0f, 1.0f, 0.0f, saturationModeLabel);
  addFloat(postSaturationAmountId, "Post Saturation Amount", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addBool(peakEnabledId, "Peak Tamer", true);
  addFloat(peakThresholdId, "Peak Tamer Threshold", compressorMinDb, compressorMaxDb, 0.1f, 0.0f, dbLabel);
  addBool(glueEnabledId, "Glue", true);
  addBool(glueMultibandId, "Glue Multiband", false);
  addFloat(glueThresholdId, "Glue Threshold", compressorMinDb, compressorMaxDb, 0.1f, 0.0f, dbLabel);
  addFloat(glueLowThresholdId, "Glue 0-100 Hz Threshold", glueBandMinDb, compressorMaxDb, 0.1f, 0.0f, dbLabel);
  addFloat(glueLowMidThresholdId, "Glue 100 Hz-1 kHz Threshold", glueBandMinDb, compressorMaxDb, 0.1f, 0.0f,
           dbLabel);
  addFloat(glueHighMidThresholdId, "Glue 1-10 kHz Threshold", glueBandMinDb, compressorMaxDb, 0.1f, 0.0f,
           dbLabel);
  addFloat(glueAirThresholdId, "Glue 10 kHz+ Threshold", glueBandMinDb, compressorMaxDb, 0.1f, 0.0f, dbLabel);
  addBool(faceEnabledId, "In Your Face", true);
  addFloat(faceThresholdId, "In Your Face Amount", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addBool(gateEnabledId, "Gate", true);
  addBool(stereoEnabledId, "Stereo", false);
  addBool(reverbEnabledId, "Reverb", false);
  addFloat(reverbMixId, "Reverb Mix", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(reverbDecayId, "Reverb Decay", 0.0f, 100.0f, 1.0f, 72.0f, reverbDecayLabel);
  addFloat(reverbSizeId, "Reverb Size", 0.0f, 100.0f, 1.0f, 68.0f, percentLabel);
  addFloat(reverbPredelayId, "Reverb Predelay", 0.0f, 100.0f, 1.0f, 40.0f, reverbPredelayLabel);
  addFloat(reverbLowCutId, "Reverb Low Cut", 0.0f, 100.0f, 1.0f, 0.0f, reverbLowCutLabel);
  addFloat(reverbHighCutId, "Reverb High Cut", 0.0f, 100.0f, 1.0f, 100.0f, reverbHighCutLabel);
  addFloat(reverbModeId, "Reverb Type", 0.0f, static_cast<float>(reverbModeLabels.size() - 1), 1.0f, 0.0f,
           reverbModeLabel);
  addBool(reverbSyncId, "Reverb BPM Sync", true);
  addFloat(reverbNoteModeId, "Reverb Note Mode", 0.0f, 2.0f, 1.0f, 0.0f, delayNoteModeLabel);
  addBool(reverbDecaySyncId, "Reverb Decay Sync", true);
  addBool(reverbPredelaySyncId, "Reverb Predelay Sync", true);
  addFloat(reverbDecayDivisionId, "Reverb Decay Division", 0.0f, 6.0f, 1.0f, 2.0f, delayDivisionLabel);
  addFloat(reverbPredelayDivisionId, "Reverb Predelay Division", 0.0f, 7.0f, 1.0f, 3.0f,
           reverbPredelayDivisionLabel);
  addBool(delayEnabledId, "Delay", false);
  addFloat(delayMixId, "Delay Mix", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(delayFeedbackId, "Delay Feedback", 0.0f, 100.0f, 1.0f, 35.0f, percentLabel);
  addFloat(delayLowCutId, "Delay Low Cut", 0.0f, 100.0f, 1.0f, 0.0f, delayLowCutLabel);
  addFloat(delayHighCutId, "Delay High Cut", 0.0f, 100.0f, 1.0f, 100.0f, delayHighCutLabel);
  addBool(delaySyncId, "Delay BPM Sync", true);
  addFloat(delayDivisionId, "Delay Division", 0.0f, 6.0f, 1.0f, 2.0f, delayDivisionLabel);
  addFloat(delayNoteModeId, "Delay Note Mode", 0.0f, 2.0f, 1.0f, 0.0f, delayNoteModeLabel);
  addFloat(delayTimeMsId, "Delay Time", 1.0f, 2000.0f, 1.0f, 500.0f, msLabel);
  addFloat(delayModeId, "Delay Mode", 0.0f, 2.0f, 1.0f, 0.0f, delayModeLabel);
  addBool(delayPostReverbId, "Delay Post Reverb", false);
  addFloat(delayStyleId, "Delay Type", 0.0f, static_cast<float>(delayStyleLabels.size() - 1), 1.0f, 0.0f,
           delayStyleLabel);

  return { params.begin(), params.end() };
}

void VoxanovaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  juce::ignoreUnused(samplesPerBlock);

  currentSampleRate = sampleRate;
  const auto delaySamples = static_cast<int>(sampleRate * 8.0);
  const auto reverbPredelaySamples = static_cast<int>(sampleRate * 2.0);
  const auto reverbEarlySamples = static_cast<int>(sampleRate * 1.5);
  const auto reverbTankSamples = static_cast<int>(sampleRate * 3.0);
  const auto reverbDiffuserSamples = static_cast<int>(sampleRate * 0.6);
  const auto widenSamples = static_cast<int>(sampleRate * 0.04);

  for (auto& delayBuffer : delayBuffers)
  {
    delayBuffer.setSize(1, delaySamples);
    delayBuffer.clear();
  }

  delayWritePositions = {};
  delayLowCutStates = {};
  delayHighCutStates = {};
  delayStyleLowCutStates = {};
  delayStyleHighCutStates = {};
  reverbPredelayBuffer.setSize(2, reverbPredelaySamples);
  reverbPredelayBuffer.clear();
  reverbPredelayWritePosition = 0;
  reverbEarlyBuffer.setSize(2, reverbEarlySamples);
  reverbEarlyBuffer.clear();
  reverbEarlyWritePosition = 0;
  for (auto& tankBuffer : reverbTankBuffers)
  {
    tankBuffer.setSize(1, reverbTankSamples);
    tankBuffer.clear();
  }
  reverbTankWritePositions = {};
  reverbTankHighDampStates = {};
  reverbTankLowDampStates = {};
  reverbTankModPhases = {};
  for (auto& diffuserBuffer : reverbDiffuserBuffers)
  {
    diffuserBuffer.setSize(1, reverbDiffuserSamples);
    diffuserBuffer.clear();
  }
  reverbDiffuserWritePositions = {};
  reverbDiffuserModPhases = {};
  reverbLowCutStates = {};
  reverbHighCutStates = {};
  reverbModeLowCutStates = {};
  reverbModeHighCutStates = {};
  reverbWarmLowStates = {};
  reverbWarmHighStates = {};
  reverbSilkStates = {};
  reverbSizeSmoothed = 0.68f;
  reverbDecaySmoothed = 4.0f;
  lastReverbMode = -1;
  monoWidenBuffer.setSize(1, widenSamples);
  monoWidenBuffer.clear();
  monoWidenWritePosition = 0;
  monoWidenAllpassLeft = {};
  monoWidenAllpassRight = {};
  monoWidenSideLowpass = 0.0f;
  gateEnvelope = 0.0f;
  gateSmoothedGain = 1.0f;
  gateHoldSamples = 0;
  preCompressorState = {};
  peakCompressorState = {};
  glueCompressorState = {};
  glueBandCompressorStates = {};
  glueBandSplitStates = {};
  faceCompressorState = {};
  postCompressorState = {};
  gateReductionMeter.store(0.0f);
  peakReductionMeter.store(0.0f);
  glueReductionMeter.store(0.0f);
  faceReductionMeter.store(0.0f);
  gateReductionDbMeter.store(0.0f);
  peakReductionDbMeter.store(0.0f);
  glueReductionDbMeter.store(0.0f);
  faceReductionDbMeter.store(0.0f);
}

void VoxanovaAudioProcessor::releaseResources()
{
}

VoxanovaAudioProcessor::MeterSnapshot VoxanovaAudioProcessor::getMeterSnapshot() const
{
  MeterSnapshot snapshot;
  snapshot.inputChannels = activeInputChannels.load();
  snapshot.outputChannels = activeOutputChannels.load();

  for (auto i = 0; i < 2; ++i)
  {
    snapshot.input[static_cast<size_t>(i)] = peakToMeter(inputMeterPeaks[static_cast<size_t>(i)].load());
    snapshot.output[static_cast<size_t>(i)] = peakToMeter(outputMeterPeaks[static_cast<size_t>(i)].load());
  }

  snapshot.gateReduction = gateReductionMeter.load();
  snapshot.peakReduction = peakReductionMeter.load();
  snapshot.glueReduction = glueReductionMeter.load();
  snapshot.faceReduction = faceReductionMeter.load();
  snapshot.gateReductionDb = gateReductionDbMeter.load();
  snapshot.peakReductionDb = peakReductionDbMeter.load();
  snapshot.glueReductionDb = glueReductionDbMeter.load();
  snapshot.faceReductionDb = faceReductionDbMeter.load();
  snapshot.peakLevel = peakLevelMeter.load();
  snapshot.glueLevel = glueLevelMeter.load();
  snapshot.faceLevel = faceLevelMeter.load();
  snapshot.gateLevel = gateLevelMeter.load();
  snapshot.hostBpm = hostBpm.load();

  return snapshot;
}

bool VoxanovaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  const auto input = layouts.getMainInputChannelSet();
  const auto output = layouts.getMainOutputChannelSet();

  return output == juce::AudioChannelSet::stereo() &&
         (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void VoxanovaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  const auto totalInputChannels = getTotalNumInputChannels();
  const auto totalOutputChannels = getTotalNumOutputChannels();
  const auto meteredInputChannels = juce::jlimit(1, 2, totalInputChannels);
  const auto meteredOutputChannels = 2;
  auto blockHostBpm = hostBpm.load();

  if (auto* playHead = getPlayHead())
  {
    if (auto position = playHead->getPosition())
    {
      if (auto bpm = position->getBpm())
        blockHostBpm = juce::jlimit(20.0f, 300.0f, static_cast<float>(*bpm));
    }
  }

  hostBpm.store(blockHostBpm);

  activeInputChannels.store(meteredInputChannels);
  activeOutputChannels.store(meteredOutputChannels);

  for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
    buffer.clear(channel, 0, buffer.getNumSamples());

  const auto inputGain = dbToGain(inputGainParam->load());
  const auto outputGain = dbToGain(outputGainParam->load());
  const auto gateThresholdDb = gateParam->load();
  const auto stereoWidth = juce::jlimit(0.0f, 1.0f, stereoWidthParam->load() / 100.0f);
  const auto stereoLowBypassHz = juce::jlimit(0.0f, 500.0f, stereoLowBypassParam->load());
  const auto preSaturationMode = juce::jlimit(0, 3, juce::roundToInt(preSaturationModeParam->load()));
  const auto preSaturationAmount = juce::jlimit(0.0f, 100.0f, preSaturationAmountParam->load());
  const auto postSaturationMode = juce::jlimit(0, 3, juce::roundToInt(postSaturationModeParam->load()));
  const auto postSaturationAmount = juce::jlimit(0.0f, 100.0f, postSaturationAmountParam->load());
  const auto peakEnabled = peakEnabledParam->load() >= 0.5f;
  const auto peakThreshold = peakThresholdParam->load();
  const auto glueEnabled = glueEnabledParam->load() >= 0.5f;
  const auto glueMultiband = glueMultibandParam->load() >= 0.5f;
  const auto glueThreshold = glueThresholdParam->load();
  const std::array<float, 4> glueBandThresholds {
    glueLowThresholdParam->load(),
    glueLowMidThresholdParam->load(),
    glueHighMidThresholdParam->load(),
    glueAirThresholdParam->load()
  };
  const auto faceEnabled = faceEnabledParam->load() >= 0.5f;
  const auto faceMix = faceThresholdParam->load();
  const auto gateEnabled = gateEnabledParam->load() >= 0.5f;
  const auto stereoEnabled = stereoEnabledParam->load() >= 0.5f;
  const auto reverbEnabled = reverbEnabledParam->load() >= 0.5f;
  const auto delayEnabled = delayEnabledParam->load() >= 0.5f;
  const auto reverbMix = reverbEnabled ? reverbMixParam->load() / 100.0f : 0.0f;
  const auto reverbDecayControl = juce::jlimit(0.0f, 100.0f, reverbDecayParam->load());
  const auto reverbSizeControl = juce::jlimit(0.0f, 100.0f, reverbSizeParam->load());
  const auto reverbPredelayControl = juce::jlimit(0.0f, 100.0f, reverbPredelayParam->load());
  const auto reverbLowCut = juce::jlimit(0.0f, 100.0f, reverbLowCutParam->load());
  const auto reverbHighCut = juce::jlimit(0.0f, 100.0f, reverbHighCutParam->load());
  const auto reverbMode = juce::jlimit(0, static_cast<int>(reverbModeLabels.size()) - 1,
                                       juce::roundToInt(reverbModeParam->load()));
  const auto reverbSync = reverbSyncParam->load() >= 0.5f;
  const auto reverbNoteMode = juce::jlimit(0, 2, juce::roundToInt(reverbNoteModeParam->load()));
  const auto reverbDecaySync = reverbDecaySyncParam->load() >= 0.5f;
  const auto reverbPredelaySync = reverbPredelaySyncParam->load() >= 0.5f;
  const auto reverbDecayDivision = juce::jlimit(0, 6, juce::roundToInt(reverbDecayDivisionParam->load()));
  const auto reverbPredelayDivision = juce::jlimit(0, 7, juce::roundToInt(reverbPredelayDivisionParam->load()));
  const auto delayMix = delayEnabled ? delayMixParam->load() / 100.0f : 0.0f;
  const auto delayFeedback = delayEnabled ? juce::jlimit(0.0f, 0.985f, delayFeedbackParam->load() / 100.0f * 0.985f) : 0.0f;
  const auto delayLowCut = juce::jlimit(0.0f, 100.0f, delayLowCutParam->load());
  const auto delayHighCut = juce::jlimit(0.0f, 100.0f, delayHighCutParam->load());
  const auto delaySync = delaySyncParam->load() >= 0.5f;
  const auto delayDivision = juce::jlimit(0, 6, juce::roundToInt(delayDivisionParam->load()));
  const auto delayNoteMode = juce::jlimit(0, 2, juce::roundToInt(delayNoteModeParam->load()));
  const auto delayMode = juce::jlimit(0, 2, juce::roundToInt(delayModeParam->load()));
  const auto delayPostReverb = delayPostReverbParam->load() >= 0.5f;
  const auto delayStyle = juce::jlimit(0, static_cast<int>(delayStyleLabels.size()) - 1,
                                      juce::roundToInt(delayStyleParam->load()));
  const auto noteMultiplier = delayNoteMode == 1 ? 1.5f : delayNoteMode == 2 ? (2.0f / 3.0f) : 1.0f;
  const auto reverbNoteMultiplier = reverbNoteMode == 1 ? 1.5f : reverbNoteMode == 2 ? (2.0f / 3.0f) : 1.0f;
  const auto delayTimeSeconds =
      delaySync ? (60.0f / blockHostBpm) * delayDivisionBeats[static_cast<size_t>(delayDivision)] * noteMultiplier
                : delayTimeMsParam->load() / 1000.0f;
  const auto reverbDecaySeconds =
      reverbSync && reverbDecaySync
          ? juce::jlimit(0.16f, 18.0f,
                         (60.0f / blockHostBpm) * delayDivisionBeats[static_cast<size_t>(reverbDecayDivision)] *
                             reverbNoteMultiplier)
          : 0.2f + ((std::pow(90.0f, reverbDecayControl / 100.0f) - 1.0f) / 89.0f) * 17.8f;
  const auto reverbPredelayMs =
      reverbSync && reverbPredelaySync
          ? juce::jlimit(0.0f, 1000.0f,
                         (60000.0f / blockHostBpm) *
                             reverbPredelayDivisionBeats[static_cast<size_t>(reverbPredelayDivision)] *
                             reverbNoteMultiplier)
          : reverbPredelayControl * 0.5f;
  const auto baseDelaySamples = juce::jlimit(1.0f, static_cast<float>(delayBuffers[0].getNumSamples() - 2),
                                             delayTimeSeconds * static_cast<float>(currentSampleRate));
  const auto leftDelaySamples = delayMode == 1 ? baseDelaySamples * 0.92f : baseDelaySamples;
  const auto rightDelaySamples = delayMode == 1 ? baseDelaySamples * 1.08f : baseDelaySamples;
  std::array<float, 2> inputPeaks {};
  std::array<float, 2> outputPeaks {};
  auto peakLevel = 0.0f;
  auto glueLevel = 0.0f;
  auto faceLevel = 0.0f;
  auto gateLevel = 0.0f;
  auto gateReduction = 0.0f;
  auto peakReduction = 0.0f;
  auto glueReduction = 0.0f;
  auto faceReduction = 0.0f;
  auto gateReductionDb = 0.0f;
  auto peakReductionDb = 0.0f;
  auto glueReductionDb = 0.0f;
  auto faceReductionDb = 0.0f;

  if (!stereoEnabled)
  {
    monoWidenBuffer.clear();
    monoWidenWritePosition = 0;
    monoWidenAllpassLeft = {};
    monoWidenAllpassRight = {};
    monoWidenSideLowpass = 0.0f;
  }

  if (!delayEnabled)
  {
    for (auto& delayBuffer : delayBuffers)
      delayBuffer.clear();

    delayWritePositions = {};
    delayLowCutStates = {};
    delayHighCutStates = {};
    delayStyleLowCutStates = {};
    delayStyleHighCutStates = {};
  }

  if (preSaturationMode == 0 || preSaturationAmount <= 0.0f)
    preSaturationState = {};

  if (postSaturationMode == 0 || postSaturationAmount <= 0.0f)
    postSaturationState = {};

  struct ReverbModeSpec
  {
    float roomScale;
    float damping;
    float width;
    float lowCutHz;
    float highCutHz;
    float predelayAddMs;
    float wetTrim;
    float spread;
    float density;
    float modulation;
    float earlyLevel;
    float lateLevel;
    float diffusion;
    float decayScale;
  };

  constexpr std::array<ReverbModeSpec, 22> reverbModes {
    ReverbModeSpec { 1.10f, 0.30f, 1.00f, 70.0f, 12800.0f, 12.0f, 0.86f, 0.34f, 0.78f, 0.48f, 0.36f, 1.08f, 0.70f, 1.25f },
    ReverbModeSpec { 1.28f, 0.18f, 1.00f, 55.0f, 16800.0f, 16.0f, 0.88f, 0.38f, 0.70f, 0.52f, 0.28f, 1.18f, 0.62f, 1.36f },
    ReverbModeSpec { 0.54f, 0.10f, 1.00f, 170.0f, 18000.0f, 3.0f, 1.08f, 0.28f, 1.28f, 0.74f, 0.10f, 1.14f, 0.84f, 0.95f },
    ReverbModeSpec { 0.24f, 0.48f, 0.58f, 150.0f, 7600.0f, 0.0f, 0.92f, 0.04f, 0.92f, 0.07f, 0.86f, 0.25f, 0.38f, 0.34f },
    ReverbModeSpec { 0.66f, 0.35f, 0.82f, 105.0f, 9400.0f, 6.0f, 0.90f, 0.22f, 0.92f, 0.30f, 0.52f, 0.82f, 0.74f, 0.82f },
    ReverbModeSpec { 0.86f, 0.16f, 1.00f, 95.0f, 17200.0f, 9.0f, 0.84f, 0.44f, 1.18f, 0.82f, 0.28f, 1.05f, 0.82f, 1.18f },
    ReverbModeSpec { 0.92f, 0.12f, 1.00f, 120.0f, 18400.0f, 11.0f, 0.82f, 0.52f, 1.08f, 0.96f, 0.22f, 1.16f, 0.88f, 1.28f },
    ReverbModeSpec { 0.18f, 0.18f, 0.92f, 120.0f, 16000.0f, 0.0f, 0.84f, 0.16f, 1.22f, 0.22f, 1.18f, 0.20f, 0.72f, 0.30f },
    ReverbModeSpec { 1.36f, 0.58f, 0.90f, 155.0f, 6400.0f, 26.0f, 0.70f, 0.40f, 0.58f, 0.24f, 0.22f, 1.16f, 0.72f, 1.46f },
    ReverbModeSpec { 1.02f, 0.74f, 0.76f, 230.0f, 5200.0f, 18.0f, 0.62f, 0.24f, 0.54f, 0.18f, 0.22f, 1.00f, 0.56f, 1.18f },
    ReverbModeSpec { 0.46f, 0.62f, 0.88f, 210.0f, 7800.0f, 4.0f, 0.90f, 0.20f, 1.08f, 0.42f, 0.14f, 0.96f, 0.70f, 0.82f },
    ReverbModeSpec { 0.62f, 0.28f, 0.96f, 150.0f, 14200.0f, 2.0f, 0.96f, 0.24f, 1.18f, 0.30f, 0.16f, 1.06f, 0.82f, 0.90f },
    ReverbModeSpec { 0.34f, 0.34f, 0.78f, 115.0f, 10400.0f, 1.0f, 0.92f, 0.08f, 1.04f, 0.12f, 0.78f, 0.38f, 0.54f, 0.44f },
    ReverbModeSpec { 0.78f, 0.26f, 0.96f, 135.0f, 13600.0f, 5.0f, 0.82f, 0.34f, 1.16f, 0.38f, 0.52f, 0.74f, 0.78f, 0.86f },
    ReverbModeSpec { 0.38f, 0.58f, 0.50f, 190.0f, 8800.0f, 0.0f, 0.72f, 0.02f, 0.82f, 0.04f, 1.08f, 0.16f, 0.30f, 0.24f },
    ReverbModeSpec { 0.70f, 0.48f, 0.78f, 130.0f, 9200.0f, 7.0f, 0.76f, 0.30f, 0.82f, 0.70f, 0.42f, 0.66f, 0.58f, 0.78f },
    ReverbModeSpec { 1.24f, 0.36f, 0.92f, 95.0f, 14800.0f, 20.0f, 0.72f, 0.48f, 0.66f, 0.86f, 0.18f, 1.22f, 0.56f, 1.34f },
    ReverbModeSpec { 0.96f, 0.46f, 0.84f, 140.0f, 11200.0f, 12.0f, 0.78f, 0.36f, 0.88f, 0.68f, 0.36f, 0.86f, 0.62f, 0.92f },
    ReverbModeSpec { 1.72f, 0.72f, 0.92f, 180.0f, 5200.0f, 34.0f, 0.64f, 0.48f, 0.46f, 0.18f, 0.18f, 1.22f, 0.76f, 1.80f },
    ReverbModeSpec { 1.58f, 0.24f, 1.00f, 70.0f, 15600.0f, 24.0f, 0.80f, 0.46f, 0.62f, 0.54f, 0.18f, 1.30f, 0.66f, 1.58f },
    ReverbModeSpec { 0.58f, 0.42f, 0.72f, 135.0f, 8600.0f, 8.0f, 0.86f, 0.18f, 0.88f, 0.20f, 0.48f, 0.74f, 0.60f, 0.74f },
    ReverbModeSpec { 1.34f, 0.40f, 0.96f, 100.0f, 11600.0f, 28.0f, 0.76f, 0.36f, 0.60f, 0.36f, 0.24f, 1.12f, 0.64f, 1.42f }
  };

  const auto& reverbSpec = reverbModes[static_cast<size_t>(reverbMode)];
  constexpr std::array<float, 22> reverbModeMakeup {
    1.08f, 1.08f, 0.98f, 1.22f, 1.10f, 1.04f, 1.05f, 1.18f, 1.22f, 1.34f, 1.08f,
    1.08f, 1.18f, 1.10f, 1.38f, 1.24f, 1.20f, 1.18f, 1.42f, 1.16f, 1.12f, 1.18f
  };
  const auto reverbMakeupGain = reverbModeMakeup[static_cast<size_t>(reverbMode)];
  const auto targetReverbSize = juce::jlimit(0.0f, 1.0f, reverbSizeControl / 100.0f);
  const auto blockSmoothingCoeff =
      std::exp(-static_cast<float>(buffer.getNumSamples()) / static_cast<float>(currentSampleRate * 0.045));
  reverbSizeSmoothed = targetReverbSize + blockSmoothingCoeff * (reverbSizeSmoothed - targetReverbSize);
  reverbDecaySmoothed = reverbDecaySeconds + blockSmoothingCoeff * (reverbDecaySmoothed - reverbDecaySeconds);
  const auto reverbSizeNorm = juce::jlimit(0.0f, 1.0f, reverbSizeSmoothed);
  const auto reverbDecayLift =
      juce::jlimit(0.0f, 1.0f, (reverbDecaySmoothed * reverbSpec.decayScale - 0.35f) / 10.0f);
  const auto reverbWetGain =
      reverbEnabled ? 1.25f + std::pow(reverbMix, 1.05f) * (2.90f + reverbSizeNorm * 0.75f + reverbDecayLift * 0.80f)
                    : 1.0f;
  const auto delayWetGain =
      delayEnabled ? 1.05f + std::pow(delayMix, 1.08f) * (1.85f + delayFeedback * 1.15f) : 1.0f;

  auto clearReverbState = [this] {
    reverbPredelayBuffer.clear();
    reverbPredelayWritePosition = 0;
    reverbEarlyBuffer.clear();
    reverbEarlyWritePosition = 0;
    for (auto& tankBuffer : reverbTankBuffers)
      tankBuffer.clear();
    reverbTankWritePositions = {};
    reverbTankHighDampStates = {};
    reverbTankLowDampStates = {};
    reverbTankModPhases = {};
    for (auto& diffuserBuffer : reverbDiffuserBuffers)
      diffuserBuffer.clear();
    reverbDiffuserWritePositions = {};
    reverbDiffuserModPhases = {};
    reverbLowCutStates = {};
    reverbHighCutStates = {};
    reverbModeLowCutStates = {};
    reverbModeHighCutStates = {};
    reverbWarmLowStates = {};
    reverbWarmHighStates = {};
    reverbSilkStates = {};
  };

  if (lastReverbMode != reverbMode)
  {
    clearReverbState();
    lastReverbMode = reverbMode;
  }

  if (!reverbEnabled)
  {
    clearReverbState();
  }

  auto readDelaySample = [this](int channel, float delayInSamples) {
    auto& delayBuffer = delayBuffers[static_cast<size_t>(channel)];
    const auto bufferSize = delayBuffer.getNumSamples();
    const auto writePosition = delayWritePositions[static_cast<size_t>(channel)];
    const auto clampedDelay = juce::jlimit(1.0f, static_cast<float>(bufferSize - 2), delayInSamples);
    const auto delayFloor = static_cast<int>(std::floor(clampedDelay));
    const auto fraction = clampedDelay - static_cast<float>(delayFloor);
    const auto readPositionA = (writePosition + bufferSize - delayFloor) % bufferSize;
    const auto readPositionB = (readPositionA + bufferSize - 1) % bufferSize;
    const auto sampleA = delayBuffer.getSample(0, readPositionA);
    const auto sampleB = delayBuffer.getSample(0, readPositionB);
    return sampleA + (sampleB - sampleA) * fraction;
  };

  auto processDelayTone = [this, delayLowCut, delayHighCut, delayStyle](float value, int channel) {
    const auto index = static_cast<size_t>(channel);
    const auto nyquistSafe = static_cast<float>(currentSampleRate) * 0.45f;
    const auto lowCutNorm = juce::jlimit(0.0f, 1.0f, delayLowCut / 100.0f);
    const auto highCutNorm = juce::jlimit(0.0f, 1.0f, delayHighCut / 100.0f);
    const auto lowCutHz = juce::jlimit(20.0f, nyquistSafe, 20.0f * std::pow(900.0f, lowCutNorm));
    const auto highCutHz = juce::jlimit(160.0f, nyquistSafe, 160.0f * std::pow(125.0f, highCutNorm));

    auto highPassed = value;
    for (auto& state : delayLowCutStates[index])
    {
      const auto low = processOnePoleLowpass(highPassed, lowCutHz, state);
      highPassed -= low;
    }

    auto filtered = highPassed;
    for (auto& state : delayHighCutStates[index])
      filtered = processOnePoleLowpass(filtered, highCutHz, state);

    auto styleLowCutHz = 20.0f;
    auto styleHighCutHz = nyquistSafe;
    auto drive = 1.0f;
    auto wetTrim = 1.0f;
    auto midBoost = 0.0f;

    switch (delayStyle)
    {
      case 1: // Digital
        styleLowCutHz = 35.0f;
        styleHighCutHz = nyquistSafe;
        wetTrim = 1.0f;
        break;
      case 2: // Tape
        styleLowCutHz = 55.0f;
        styleHighCutHz = 10500.0f;
        drive = 1.35f;
        wetTrim = 1.10f;
        break;
      case 3: // Studio Tape
        styleLowCutHz = 45.0f;
        styleHighCutHz = 13500.0f;
        drive = 1.18f;
        wetTrim = 1.06f;
        break;
      case 4: // Old Tape
        styleLowCutHz = 95.0f;
        styleHighCutHz = 6200.0f;
        drive = 1.55f;
        wetTrim = 1.20f;
        break;
      case 5: // Cheap Tape
        styleLowCutHz = 140.0f;
        styleHighCutHz = 7600.0f;
        drive = 1.85f;
        midBoost = 0.18f;
        wetTrim = 1.22f;
        break;
      case 6: // Analog
        styleLowCutHz = 70.0f;
        styleHighCutHz = 8200.0f;
        drive = 1.28f;
        wetTrim = 1.12f;
        break;
      case 7: // Radio
        styleLowCutHz = 520.0f;
        styleHighCutHz = 4700.0f;
        drive = 2.0f;
        midBoost = 0.22f;
        wetTrim = 1.42f;
        break;
      case 8: // Telephone
        styleLowCutHz = 360.0f;
        styleHighCutHz = 3200.0f;
        drive = 1.65f;
        midBoost = 0.12f;
        wetTrim = 1.48f;
        break;
      case 9: // Dirty
        styleLowCutHz = 95.0f;
        styleHighCutHz = 5900.0f;
        drive = 2.35f;
        midBoost = 0.16f;
        wetTrim = 1.34f;
        break;
      case 10: // Ambient
        styleLowCutHz = 120.0f;
        styleHighCutHz = 9200.0f;
        drive = 1.08f;
        wetTrim = 1.16f;
        break;
      default:
        break;
    }

    auto styled = filtered;
    for (auto& state : delayStyleLowCutStates[index])
    {
      const auto low = processOnePoleLowpass(styled, juce::jlimit(20.0f, nyquistSafe, styleLowCutHz), state);
      styled -= low;
    }

    for (auto& state : delayStyleHighCutStates[index])
      styled = processOnePoleLowpass(styled, juce::jlimit(160.0f, nyquistSafe, styleHighCutHz), state);

    if (midBoost > 0.0f)
      styled += std::tanh(styled * 2.2f) * midBoost;

    if (drive > 1.0f)
      styled = std::tanh(styled * drive) / drive;

    return styled * wetTrim;
  };

  auto readFractionalSample = [](const juce::AudioBuffer<float>& delayBuffer, int writePosition, float delayInSamples) {
    const auto bufferSize = delayBuffer.getNumSamples();
    if (bufferSize <= 2)
      return 0.0f;

    const auto clampedDelay = juce::jlimit(1.0f, static_cast<float>(bufferSize - 2), delayInSamples);
    const auto delayFloor = static_cast<int>(std::floor(clampedDelay));
    const auto fraction = clampedDelay - static_cast<float>(delayFloor);
    const auto readPositionA = (writePosition + bufferSize - delayFloor) % bufferSize;
    const auto readPositionB = (readPositionA + bufferSize - 1) % bufferSize;
    const auto sampleA = delayBuffer.getSample(0, readPositionA);
    const auto sampleB = delayBuffer.getSample(0, readPositionB);
    return sampleA + (sampleB - sampleA) * fraction;
  };

  auto processDiffuser = [this, &readFractionalSample, reverbSpec, reverbSizeNorm](float input, int channel) {
    constexpr std::array<float, 4> baseMs { 8.9f, 17.3f, 31.7f, 49.1f };
    constexpr std::array<float, 4> modOffsets { 0.0f, 1.7f, 3.1f, 5.3f };
    auto output = input;
    const auto sizeScale = 0.55f + reverbSizeNorm * 1.85f * reverbSpec.roomScale;
    const auto diffusion = juce::jlimit(0.24f, 0.58f, reverbSpec.diffusion * 0.68f + reverbSizeNorm * 0.055f);

    for (auto stage = 0; stage < 4; ++stage)
    {
      const auto index = static_cast<size_t>(channel * 4 + stage);
      auto& delayBuffer = reverbDiffuserBuffers[index];
      auto& writePosition = reverbDiffuserWritePositions[index];
      auto& phase = reverbDiffuserModPhases[index];

      const auto rateHz = 0.022f + reverbSpec.modulation * 0.052f + modOffsets[static_cast<size_t>(stage)] * 0.002f;
      phase += 2.0f * juce::MathConstants<float>::pi * rateHz / static_cast<float>(currentSampleRate);
      if (phase > 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

      const auto modSamples =
          std::sin(phase) * (0.35f + reverbSizeNorm * 2.8f) * reverbSpec.modulation * (stage + 1) * 0.14f;
      const auto delaySamples =
          baseMs[static_cast<size_t>(stage)] * sizeScale * static_cast<float>(currentSampleRate) / 1000.0f +
          modSamples;
      const auto delayed = readFractionalSample(delayBuffer, writePosition, delaySamples);
      const auto allpassOut = -diffusion * output + delayed;
      const auto writeValue = output + diffusion * allpassOut;
      delayBuffer.setSample(0, writePosition, juce::jlimit(-1.6f, 1.6f, writeValue));
      writePosition = (writePosition + 1) % delayBuffer.getNumSamples();
      output = allpassOut;
    }

    return output;
  };

  const auto reverbPredelaySamples =
      reverbPredelayBuffer.getNumSamples() > 2
          ? juce::jlimit(0, reverbPredelayBuffer.getNumSamples() - 2,
                         juce::roundToInt((reverbPredelayMs + reverbSpec.predelayAddMs) *
                                          static_cast<float>(currentSampleRate) / 1000.0f))
          : 0;

  auto readReverbPredelay = [this](int channel, int delayInSamples) {
    if (delayInSamples <= 0 || reverbPredelayBuffer.getNumSamples() <= 0)
      return 0.0f;

    const auto bufferSize = reverbPredelayBuffer.getNumSamples();
    const auto readPosition = (reverbPredelayWritePosition + bufferSize - delayInSamples) % bufferSize;
    return reverbPredelayBuffer.getSample(channel, readPosition);
  };

  auto processReverbTone = [this, reverbLowCut, reverbHighCut, reverbSpec, reverbMode](float value, int channel) {
    const auto index = static_cast<size_t>(channel);
    const auto nyquistSafe = static_cast<float>(currentSampleRate) * 0.45f;
    const auto lowCutNorm = juce::jlimit(0.0f, 1.0f, reverbLowCut / 100.0f);
    const auto highCutNorm = juce::jlimit(0.0f, 1.0f, reverbHighCut / 100.0f);
    const auto userLowCutHz = juce::jlimit(20.0f, nyquistSafe, 20.0f * std::pow(50.0f, lowCutNorm));
    const auto userHighCutHz = juce::jlimit(1500.0f, nyquistSafe, 1500.0f * std::pow(13.333333f, highCutNorm));
    const auto modeLowCutHz = juce::jlimit(20.0f, nyquistSafe, reverbSpec.lowCutHz);
    const auto modeHighCutHz = juce::jlimit(400.0f, nyquistSafe, reverbSpec.highCutHz);
    const auto lowCutHz = juce::jlimit(20.0f, nyquistSafe, juce::jmax(userLowCutHz, modeLowCutHz));
    const auto highCutHz = juce::jlimit(lowCutHz + 80.0f, nyquistSafe, juce::jmin(userHighCutHz, modeHighCutHz));
    const auto musicalHighCutHz =
        juce::jlimit(lowCutHz + 120.0f, nyquistSafe,
                     juce::jmin(highCutHz, 5600.0f + (1.0f - reverbSpec.damping) * 3200.0f +
                                               reverbSpec.width * 650.0f));

    auto filtered = value;
    for (auto& state : reverbModeLowCutStates[index])
    {
      const auto low = processOnePoleLowpass(filtered, modeLowCutHz, state);
      filtered -= low;
    }

    for (auto& state : reverbLowCutStates[index])
    {
      const auto low = processOnePoleLowpass(filtered, lowCutHz, state);
      filtered -= low;
    }

    for (auto& state : reverbHighCutStates[index])
      filtered = processOnePoleLowpass(filtered, musicalHighCutHz, state);

    for (auto& state : reverbModeHighCutStates[index])
      filtered = processOnePoleLowpass(filtered, modeHighCutHz, state);

    const auto warmLow = processOnePoleLowpass(filtered, 1650.0f, reverbWarmLowStates[index]);
    const auto warmHigh = processOnePoleLowpass(filtered, 5600.0f, reverbWarmHighStates[index]);
    const auto metallicBand = warmHigh - warmLow;
    const auto deMetalAmount = 0.16f + reverbSpec.damping * 0.14f;
    filtered = filtered - metallicBand * deMetalAmount + warmLow * 0.055f;

    const auto silk = processOnePoleLowpass(filtered, juce::jlimit(3600.0f, nyquistSafe, musicalHighCutHz * 0.82f),
                                            reverbSilkStates[index]);
    filtered = filtered * 0.68f + silk * 0.32f;

    switch (reverbMode)
    {
      case 2: // Plate
      case 11: // Smooth Plate
        filtered = filtered * 1.02f + std::tanh(filtered * 1.18f) * 0.035f;
        break;
      case 3: // Room
      case 12: // Smooth Room
        filtered *= 0.96f;
        break;
      case 8: // Sanctuary
      case 18: // Cathedral
      case 19: // Palace
        filtered *= 0.92f;
        break;
      case 9: // Dirty Hall
      case 10: // Dirty Plate
        filtered = filtered * 0.93f + std::tanh(filtered * 2.0f) * 0.045f;
        break;
      case 14: // Nonlin
        filtered *= 0.86f;
        break;
      case 15: // Chaotic Chamber
      case 16: // Chaotic Hall
      case 17: // Chaotic Neutral
        filtered = filtered * 0.95f + std::tanh(filtered * 1.45f) * 0.028f;
        break;
      case 20: // Chamber1979
      case 21: // Hall1984
        filtered = filtered * 0.96f + warmLow * 0.025f;
        break;
      default:
        break;
    }

    return filtered * reverbSpec.wetTrim;
  };

  auto readEarlyReflection = [this](int channel, int delayInSamples) {
    if (delayInSamples <= 0 || reverbEarlyBuffer.getNumSamples() <= 0)
      return 0.0f;

    const auto bufferSize = reverbEarlyBuffer.getNumSamples();
    const auto readPosition = (reverbEarlyWritePosition + bufferSize - delayInSamples) % bufferSize;
    return reverbEarlyBuffer.getSample(channel, readPosition);
  };

  auto processEarlyReflections = [this, &readEarlyReflection, reverbSpec, reverbSizeNorm](float inputLeft,
                                                                                          float inputRight) {
    constexpr std::array<float, 8> tapMs { 5.3f, 8.1f, 13.7f, 21.9f, 34.1f, 55.7f, 89.3f, 144.7f };
    constexpr std::array<float, 8> tapGain { 0.36f, -0.31f, 0.27f, -0.23f, 0.19f, -0.15f, 0.11f, -0.08f };
    constexpr std::array<float, 8> pan { -0.68f, 0.42f, -0.18f, 0.74f, -0.52f, 0.24f, 0.58f, -0.36f };

    auto earlyLeft = 0.0f;
    auto earlyRight = 0.0f;
    const auto tapScale = (0.34f + reverbSizeNorm * 1.55f) * reverbSpec.roomScale;

    for (auto i = 0u; i < tapMs.size(); ++i)
    {
      const auto delaySamples = juce::jlimit(
          1, reverbEarlyBuffer.getNumSamples() - 2,
          juce::roundToInt((tapMs[i] * tapScale + reverbSpec.predelayAddMs * 0.15f) *
                           static_cast<float>(currentSampleRate) / 1000.0f));
      const auto reflectedLeft = readEarlyReflection(0, delaySamples);
      const auto reflectedRight = readEarlyReflection(1, delaySamples);
      const auto monoReflection = (reflectedLeft + reflectedRight) * 0.5f;
      const auto leftWeight = 0.5f - pan[i] * 0.28f;
      const auto rightWeight = 0.5f + pan[i] * 0.28f;
      earlyLeft += (reflectedLeft * 0.62f + monoReflection * 0.38f) * tapGain[i] * leftWeight;
      earlyRight += (reflectedRight * 0.62f + monoReflection * 0.38f) * tapGain[i] * rightWeight;
    }

    if (reverbEarlyBuffer.getNumSamples() > 0)
    {
      reverbEarlyBuffer.setSample(0, reverbEarlyWritePosition, inputLeft);
      reverbEarlyBuffer.setSample(1, reverbEarlyWritePosition, inputRight);
      reverbEarlyWritePosition = (reverbEarlyWritePosition + 1) % reverbEarlyBuffer.getNumSamples();
    }

    return std::array<float, 2> { earlyLeft * reverbSpec.earlyLevel * 0.58f,
                                  earlyRight * reverbSpec.earlyLevel * 0.58f };
  };

  auto processReverbTank = [this, &readFractionalSample, reverbSpec, reverbSizeNorm](float inputLeft, float inputRight) {
    constexpr std::array<float, 8> baseDelayMs { 31.1f, 37.7f, 43.3f, 53.9f, 61.7f, 73.3f, 89.9f, 107.3f };
    constexpr std::array<float, 8> injectionPan { -0.92f, 0.84f, -0.42f, 0.38f, 0.12f, -0.18f, 0.64f, -0.58f };
    constexpr std::array<float, 8> outputMidTap { 0.92f, 0.84f, 0.78f, 0.70f, 0.64f, 0.58f, 0.52f, 0.46f };
    constexpr std::array<float, 8> outputSideTap { 0.34f, -0.31f, 0.25f, -0.22f, 0.18f, -0.16f, 0.14f, -0.12f };

    std::array<float, 8> tankRead {};
    std::array<float, 8> tankDelaySamples {};
    const auto sizeCurve = std::pow(reverbSizeNorm, 1.22f);
    const auto sizeScale = (0.42f + sizeCurve * 2.75f) * reverbSpec.roomScale;
    const auto tankInputGain = 0.22f + reverbSpec.density * 0.17f;
    const auto modeDecaySeconds = juce::jlimit(0.08f, 24.0f, reverbDecaySmoothed * reverbSpec.decayScale);
    const auto modeDecayNorm = juce::jlimit(0.0f, 1.0f, (modeDecaySeconds - 0.08f) / 23.92f);
    const auto dampingCutoff =
        juce::jlimit(1100.0f, static_cast<float>(currentSampleRate) * 0.32f,
                     reverbSpec.highCutHz * (0.42f + (1.0f - reverbSpec.damping) * 0.28f +
                                             (1.0f - modeDecayNorm) * 0.07f) +
                         reverbSizeNorm * 520.0f);
    const auto lowBloomCutoff = juce::jlimit(95.0f, 520.0f, 155.0f + reverbSizeNorm * 205.0f);

    for (auto i = 0; i < 8; ++i)
    {
      auto& phase = reverbTankModPhases[static_cast<size_t>(i)];
      const auto rateHz = 0.045f + reverbSpec.modulation * (0.025f + static_cast<float>(i) * 0.0065f);
      phase += 2.0f * juce::MathConstants<float>::pi * rateHz / static_cast<float>(currentSampleRate);
      if (phase > 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

      const auto modSamples =
          std::sin(phase + static_cast<float>(i) * 0.73f) * (1.0f + reverbSizeNorm * 13.0f) * reverbSpec.modulation;
      const auto delaySamples =
          baseDelayMs[static_cast<size_t>(i)] * sizeScale * static_cast<float>(currentSampleRate) / 1000.0f +
          modSamples;
      tankDelaySamples[static_cast<size_t>(i)] = juce::jlimit(
          4.0f, static_cast<float>(reverbTankBuffers[static_cast<size_t>(i)].getNumSamples() - 2), delaySamples);

      const auto rawDelayed = readFractionalSample(reverbTankBuffers[static_cast<size_t>(i)],
                                                   reverbTankWritePositions[static_cast<size_t>(i)],
                                                   tankDelaySamples[static_cast<size_t>(i)]);
      auto damped = processOnePoleLowpass(rawDelayed, dampingCutoff * (0.88f + static_cast<float>(i) * 0.035f),
                                          reverbTankHighDampStates[static_cast<size_t>(i)]);
      const auto lowBloom = processOnePoleLowpass(damped, lowBloomCutoff, reverbTankLowDampStates[static_cast<size_t>(i)]);
      damped = damped * (0.92f - reverbSpec.damping * 0.08f) + lowBloom * (0.08f + reverbSizeNorm * 0.035f);
      tankRead[static_cast<size_t>(i)] = damped;
    }

    auto sum = 0.0f;
    for (const auto value : tankRead)
      sum += value;

    auto lateMid = 0.0f;
    auto lateSide = 0.0f;
    const auto monoInput = (inputLeft + inputRight) * 0.5f;
    const auto tankOutputWidth = 0.52f + reverbSpec.width * 0.34f;

    for (auto i = 0; i < 8; ++i)
    {
      const auto matrixOut = sum * 0.25f - tankRead[static_cast<size_t>(i)];
      const auto delaySeconds = tankDelaySamples[static_cast<size_t>(i)] / static_cast<float>(currentSampleRate);
      const auto rt60Gain = std::pow(10.0f, -3.0f * delaySeconds / juce::jmax(0.08f, modeDecaySeconds));
      const auto feedbackGain = juce::jlimit(0.12f, 0.996f, rt60Gain * (0.965f + modeDecayNorm * 0.025f));
      const auto panAmount = injectionPan[static_cast<size_t>(i)];
      const auto injected = monoInput * 0.32f + inputLeft * (0.5f - panAmount * 0.24f) +
                            inputRight * (0.5f + panAmount * 0.24f);
      const auto writeValue = injected * tankInputGain + matrixOut * feedbackGain;
      auto& tankBuffer = reverbTankBuffers[static_cast<size_t>(i)];
      auto& writePosition = reverbTankWritePositions[static_cast<size_t>(i)];
      tankBuffer.setSample(0, writePosition, std::tanh(writeValue * 0.72f) / 0.72f);
      writePosition = (writePosition + 1) % tankBuffer.getNumSamples();

      const auto tap = tankRead[static_cast<size_t>(i)];
      lateMid += tap * outputMidTap[static_cast<size_t>(i)];
      lateSide += tap * outputSideTap[static_cast<size_t>(i)] * tankOutputWidth;
    }

    const auto outputScale = (0.185f + reverbSpec.density * 0.075f) * reverbSpec.lateLevel;
    return std::array<float, 2> { (lateMid + lateSide) * outputScale, (lateMid - lateSide) * outputScale };
  };

  for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
  {
    const auto rawLeft = buffer.getSample(0, sample);
    const auto rawRight = totalInputChannels > 1 ? buffer.getSample(1, sample) : rawLeft;

    inputPeaks[0] = juce::jmax(inputPeaks[0], std::abs(rawLeft));
    inputPeaks[1] = juce::jmax(inputPeaks[1], std::abs(rawRight));

    auto left = rawLeft * inputGain;
    auto right = rawRight * inputGain;

    auto applyStereoCompressor = [this](float& leftSample, float& rightSample, float thresholdDb, float ratio,
                                        float amountPercent, float attackMs, float releaseMs, float kneeDb,
                                        CompressorState& state) {
      const auto result =
          applyCompressor(leftSample, rightSample, thresholdDb, ratio, amountPercent, attackMs, releaseMs, kneeDb, state);
      leftSample = result.left;
      rightSample = result.right;
      return result.reduction;
    };

    // Input -> Pre-comp -> Saturation -> Peak Tamer -> Glue -> In Your Face -> Post-comp -> Saturation.
    applyStereoCompressor(left, right, -18.0f, 1.8f, fullAmount, 8.0f, 85.0f, 5.0f, preCompressorState);
    left = applySaturationModel(left, preSaturationMode, preSaturationAmount, preSaturationState, 0);
    right = applySaturationModel(right, preSaturationMode, preSaturationAmount, preSaturationState, 1);

    if (peakEnabled)
    {
      const auto peakResult = applyPeakTamer(left, right, peakThreshold, peakCompressorState);
      left = peakResult.left;
      right = peakResult.right;
      peakLevel = juce::jmax(peakLevel, peakResult.detectorLevel);
      peakReduction = juce::jmax(peakReduction, peakResult.reduction);
      peakReductionDb = juce::jmax(peakReductionDb, peakResult.reductionDb);
    }
    else
    {
      peakCompressorState = {};
    }
    if (glueEnabled && glueMultiband)
    {
      std::array<float, 4> leftBands {};
      std::array<float, 4> rightBands {};

      auto splitToBands = [this](float sampleValue, BandSplitState& splitState) {
        const auto low100 = processOnePoleLowpass(sampleValue, 100.0f, splitState.low100);
        const auto low1000 = processOnePoleLowpass(sampleValue, 1000.0f, splitState.low1000);
        const auto low10000 = processOnePoleLowpass(sampleValue, 10000.0f, splitState.low10000);
        return std::array<float, 4> { low100, low1000 - low100, low10000 - low1000, sampleValue - low10000 };
      };

      leftBands = splitToBands(left, glueBandSplitStates[0]);
      rightBands = splitToBands(right, glueBandSplitStates[1]);

      left = 0.0f;
      right = 0.0f;

      for (auto band = 0; band < 4; ++band)
      {
        const auto glueResult =
            applyGlueCompressor(leftBands[static_cast<size_t>(band)], rightBands[static_cast<size_t>(band)],
                                glueBandThresholds[static_cast<size_t>(band)],
                                glueBandCompressorStates[static_cast<size_t>(band)], true, band);
        left += glueResult.left;
        right += glueResult.right;
        glueLevel = juce::jmax(glueLevel, glueResult.detectorLevel);
        glueReduction = juce::jmax(glueReduction, glueResult.reduction);
        glueReductionDb = juce::jmax(glueReductionDb, glueResult.reductionDb);
      }
    }
    else if (glueEnabled)
    {
      const auto glueResult = applyGlueCompressor(left, right, glueThreshold, glueCompressorState, false, -1);
      left = glueResult.left;
      right = glueResult.right;
      glueLevel = juce::jmax(glueLevel, glueResult.detectorLevel);
      glueReduction = juce::jmax(glueReduction, glueResult.reduction);
      glueReductionDb = juce::jmax(glueReductionDb, glueResult.reductionDb);
    }
    else
    {
      glueCompressorState = {};
      glueBandCompressorStates = {};
    }

    if (faceEnabled)
    {
      const auto faceResult = applyInYourFaceCompressor(left, right, faceMix, faceCompressorState);
      left = faceResult.left;
      right = faceResult.right;
      faceLevel = juce::jmax(faceLevel, faceResult.detectorLevel);
      faceReduction = juce::jmax(faceReduction, faceResult.reduction);
      faceReductionDb = juce::jmax(faceReductionDb, faceResult.reductionDb);
    }
    else
    {
      faceCompressorState = {};
    }

    applyStereoCompressor(left, right, -14.0f, 2.2f, fullAmount, 12.0f, 120.0f, 5.0f, postCompressorState);
    left = applySaturationModel(left, postSaturationMode, postSaturationAmount, postSaturationState, 0);
    right = applySaturationModel(right, postSaturationMode, postSaturationAmount, postSaturationState, 1);
    // Gate sits after dynamics/saturation so it cleans tails without chopping the performance.
    if (gateEnabled)
    {
      const auto gateResult = applyVocalGate(left, right, left, right, gateThresholdDb);
      left = gateResult.left;
      right = gateResult.right;
      gateLevel = juce::jmax(gateLevel, gateResult.detectorLevel);
      gateReduction = juce::jmax(gateReduction, gateResult.reduction);
      gateReductionDb = juce::jmax(gateReductionDb, gateResult.reductionDb);
    }
    else
    {
      gateEnvelope = 0.0f;
      gateSmoothedGain = 1.0f;
      gateHoldSamples = 0;
    }

    if (stereoEnabled && totalInputChannels == 1 && totalOutputChannels > 1)
    {
      const auto mono = (left + right) * 0.5f;
      const auto lowAnchor = stereoLowBypassHz > 0.5f ? processOnePoleLowpass(mono, stereoLowBypassHz, monoWidenSideLowpass) : 0.0f;
      const auto widenInput = mono - lowAnchor;

      auto processAllpass = [](float input, float coefficient, float& state) {
        const auto output = -coefficient * input + state;
        state = input + coefficient * output;
        return output;
      };

      auto decorrelate = [&processAllpass](float input, std::array<float, 4>& states,
                                           const std::array<float, 4>& coefficients) {
        auto output = input;
        for (auto i = 0u; i < states.size(); ++i)
          output = processAllpass(output, coefficients[i], states[i]);
        return output;
      };

      constexpr std::array<float, 4> leftCoefficients { 0.6923878f, 0.9360654f, 0.9882295f, 0.9987488f };
      constexpr std::array<float, 4> rightCoefficients { 0.4021921f, 0.8561711f, 0.9722910f, 0.9952885f };
      const auto decorrelatedLeft = decorrelate(widenInput, monoWidenAllpassLeft, leftCoefficients);
      const auto decorrelatedRight = decorrelate(widenInput, monoWidenAllpassRight, rightCoefficients);

      const auto widthCurve = std::pow(stereoWidth, 0.58f);
      const auto dryGain = std::cos(widthCurve * juce::MathConstants<float>::halfPi);
      const auto wetGain = std::sin(widthCurve * juce::MathConstants<float>::halfPi);
      const auto centerAnchor = wetGain * 0.08f;
      const auto crossfeed = wetGain * 0.045f;
      const auto mainWet = wetGain * (1.0f - 0.045f);
      const auto centerGain = dryGain + centerAnchor;
      const auto makeup = 1.0f / std::sqrt(centerGain * centerGain + mainWet * mainWet + crossfeed * crossfeed);
      left = lowAnchor + (widenInput * centerGain + decorrelatedLeft * mainWet + decorrelatedRight * crossfeed) * makeup;
      right = lowAnchor + (widenInput * centerGain + decorrelatedRight * mainWet + decorrelatedLeft * crossfeed) * makeup;
    }
    else if (stereoEnabled && totalOutputChannels > 1)
    {
      const auto mid = (left + right) * 0.5f;
      const auto widthCurve = std::pow(stereoWidth, 0.72f);
      const auto sideMultiplier = widthCurve * 2.35f;
      const auto rawSide = (left - right) * 0.5f;
      const auto lowSide = stereoLowBypassHz > 0.5f ? processOnePoleLowpass(rawSide, stereoLowBypassHz, monoWidenSideLowpass) : 0.0f;
      const auto side = lowSide * widthCurve + (rawSide - lowSide) * sideMultiplier;
      const auto makeup = 1.0f / std::sqrt(1.0f + sideMultiplier * sideMultiplier * 0.08f);
      left = (mid + side) * makeup;
      right = (mid - side) * makeup;
    }

    const auto dryLeft = left;
    const auto dryRight = right;
    auto stabilizeStereoReturn = [](float stereoLeft, float stereoRight, float sideScale) {
      const auto mid = (stereoLeft + stereoRight) * 0.5f;
      const auto side = (stereoLeft - stereoRight) * 0.5f * sideScale;
      return std::array<float, 2> { mid + side, mid - side };
    };

    const auto delayedRawLeft = delayEnabled ? readDelaySample(0, leftDelaySamples) : 0.0f;
    const auto delayedRawRight = delayEnabled ? readDelaySample(1, rightDelaySamples) : 0.0f;
    const auto delayedLeft = delayEnabled ? processDelayTone(delayedRawLeft, 0) : 0.0f;
    const auto delayedRight = delayEnabled ? processDelayTone(delayedRawRight, 1) : 0.0f;

    auto mixStage = [](float dry, float wet, float mix, float wetGain) {
      const auto clampedMix = juce::jlimit(0.0f, 1.0f, mix);
      const auto dryGain = std::cos(clampedMix * juce::MathConstants<float>::halfPi);
      const auto wetLevel = std::sin(clampedMix * juce::MathConstants<float>::halfPi) * wetGain;
      return dry * dryGain + wet * wetLevel;
    };

    auto processReverbWet = [&](float inputLeft, float inputRight) {
      if (!reverbEnabled)
        return std::array<float, 2> { 0.0f, 0.0f };

      const auto stableSend = stabilizeStereoReturn(inputLeft, inputRight, 0.42f);
      auto reverbInputLeft = stableSend[0];
      auto reverbInputRight = stableSend[1];
      const auto reverbMonoSend = (reverbInputLeft + reverbInputRight) * 0.5f;
      reverbInputLeft = reverbInputLeft * (1.0f - reverbSpec.spread) + reverbMonoSend * reverbSpec.spread;
      reverbInputRight = reverbInputRight * (1.0f - reverbSpec.spread) + reverbMonoSend * reverbSpec.spread;

      const auto early = processEarlyReflections(reverbInputLeft, reverbInputRight);

      auto reverbFeedLeft = reverbPredelaySamples > 0 ? readReverbPredelay(0, reverbPredelaySamples) : reverbInputLeft;
      auto reverbFeedRight = reverbPredelaySamples > 0 ? readReverbPredelay(1, reverbPredelaySamples) : reverbInputRight;

      if (reverbPredelayBuffer.getNumSamples() > 0)
      {
        reverbPredelayBuffer.setSample(0, reverbPredelayWritePosition, reverbInputLeft);
        reverbPredelayBuffer.setSample(1, reverbPredelayWritePosition, reverbInputRight);
        reverbPredelayWritePosition = (reverbPredelayWritePosition + 1) % reverbPredelayBuffer.getNumSamples();
      }

      reverbFeedLeft = processDiffuser(reverbFeedLeft, 0);
      reverbFeedRight = processDiffuser(reverbFeedRight, 1);
      const auto late = processReverbTank(reverbFeedLeft, reverbFeedRight);

      const auto wetRawLeft = early[0] + late[0];
      const auto wetRawRight = early[1] + late[1];
      const auto tonedLeft = processReverbTone(wetRawLeft, 0);
      const auto tonedRight = processReverbTone(wetRawRight, 1);
      const auto wetMid = (tonedLeft + tonedRight) * 0.5f;
      const auto wetSide =
          (tonedLeft - tonedRight) * 0.5f * juce::jlimit(0.12f, 0.64f, 0.24f + reverbSpec.width * 0.32f);
      const auto centeredLeft = (wetMid + wetSide) * reverbMakeupGain;
      const auto centeredRight = (wetMid - wetSide) * reverbMakeupGain;
      return std::array<float, 2> { centeredLeft, centeredRight };
    };

    const auto pingPongCenter = delayMode == 2 ? 0.28f : 0.0f;
    const auto rawDelayReturnLeft = delayedLeft + delayedRight * pingPongCenter;
    const auto rawDelayReturnRight = delayedRight + delayedLeft * pingPongCenter;
    const auto stableDelayReturn =
        stabilizeStereoReturn(rawDelayReturnLeft, rawDelayReturnRight, delayMode == 1 ? 0.76f : delayMode == 2 ? 0.52f : 0.68f);
    const auto delayReturnLeft = stableDelayReturn[0];
    const auto delayReturnRight = stableDelayReturn[1];
    const auto stableDelaySend = stabilizeStereoReturn(dryLeft, dryRight, delayMode == 1 ? 0.76f : 0.64f);

    auto delayInputLeft = stableDelaySend[0];
    auto delayInputRight = stableDelaySend[1];
    auto preOutputLeft = dryLeft;
    auto preOutputRight = dryRight;

    if (delayPostReverb)
    {
      const auto reverbReturn = processReverbWet(dryLeft, dryRight);
      const auto reverbStageLeft = reverbEnabled ? mixStage(dryLeft, reverbReturn[0], reverbMix, reverbWetGain) : dryLeft;
      const auto reverbStageRight =
          reverbEnabled ? mixStage(dryRight, reverbReturn[1], reverbMix, reverbWetGain) : dryRight;
      const auto stablePostReverbDelaySend =
          stabilizeStereoReturn(reverbStageLeft, reverbStageRight, delayMode == 1 ? 0.76f : 0.64f);
      delayInputLeft = stablePostReverbDelaySend[0];
      delayInputRight = stablePostReverbDelaySend[1];
      preOutputLeft = delayEnabled ? mixStage(reverbStageLeft, delayReturnLeft, delayMix, delayWetGain) : reverbStageLeft;
      preOutputRight =
          delayEnabled ? mixStage(reverbStageRight, delayReturnRight, delayMix, delayWetGain) : reverbStageRight;
    }
    else
    {
      delayInputLeft = stableDelaySend[0];
      delayInputRight = stableDelaySend[1];
      const auto delayStageLeft = delayEnabled ? mixStage(dryLeft, delayReturnLeft, delayMix, delayWetGain) : dryLeft;
      const auto delayStageRight = delayEnabled ? mixStage(dryRight, delayReturnRight, delayMix, delayWetGain) : dryRight;
      const auto reverbReturn = processReverbWet(delayStageLeft, delayStageRight);
      preOutputLeft = reverbEnabled ? mixStage(delayStageLeft, reverbReturn[0], reverbMix, reverbWetGain) : delayStageLeft;
      preOutputRight =
          reverbEnabled ? mixStage(delayStageRight, reverbReturn[1], reverbMix, reverbWetGain) : delayStageRight;
    }

    if (delayEnabled)
    {
      auto writeLeft = 0.0f;
      auto writeRight = 0.0f;

      if (delayMode == 2)
      {
        const auto monoDelayInput = (delayInputLeft + delayInputRight) * 0.5f;
        writeLeft = (totalInputChannels == 1 ? monoDelayInput * 0.64f : delayInputLeft) + delayedRight * delayFeedback;
        writeRight = (totalInputChannels == 1 ? monoDelayInput * 0.64f : delayInputRight) + delayedLeft * delayFeedback;
      }
      else
      {
        writeLeft = delayInputLeft + delayedLeft * delayFeedback;
        writeRight = delayInputRight + delayedRight * delayFeedback;
      }

      writeLeft = juce::jlimit(-1.25f, 1.25f, writeLeft);
      writeRight = juce::jlimit(-1.25f, 1.25f, writeRight);

      for (auto channel = 0; channel < 2; ++channel)
      {
        auto& delayBuffer = delayBuffers[static_cast<size_t>(channel)];
        auto& writePosition = delayWritePositions[static_cast<size_t>(channel)];
        delayBuffer.setSample(0, writePosition, channel == 0 ? writeLeft : writeRight);
        writePosition = (writePosition + 1) % delayBuffer.getNumSamples();
      }
    }

    const auto outputLeft = preOutputLeft * outputGain;
    const auto outputRight = preOutputRight * outputGain;
    auto protectOutput = [](float sampleValue) {
      const auto absSample = std::abs(sampleValue);
      if (absSample <= 0.96f)
        return sampleValue;

      const auto sign = sampleValue < 0.0f ? -1.0f : 1.0f;
      const auto limited = 0.96f + std::tanh((absSample - 0.96f) * 2.8f) * 0.04f;
      return sign * juce::jlimit(0.0f, 0.999f, limited);
    };

    const std::array<float, 2> outputs {
      protectOutput(outputLeft),
      protectOutput(outputRight)
    };

    for (auto channel = 0; channel < juce::jmin(2, totalOutputChannels); ++channel)
    {
      const auto limitedOutput = outputs[static_cast<size_t>(channel)];
      outputPeaks[static_cast<size_t>(channel)] = juce::jmax(outputPeaks[static_cast<size_t>(channel)],
                                                             std::abs(limitedOutput));
      buffer.setSample(channel, sample, limitedOutput);
    }
  }

  if (meteredInputChannels == 1)
    inputPeaks[1] = inputPeaks[0];

  if (meteredOutputChannels == 1)
    outputPeaks[1] = outputPeaks[0];

  for (auto i = 0; i < 2; ++i)
  {
    updateAtomicPeak(inputMeterPeaks[static_cast<size_t>(i)], inputPeaks[static_cast<size_t>(i)]);
    updateAtomicPeak(outputMeterPeaks[static_cast<size_t>(i)], outputPeaks[static_cast<size_t>(i)]);
  }

  const auto blockSamples = buffer.getNumSamples();
  auto updateStageMeter = [this, blockSamples](bool enabled, std::atomic<float>& reductionMeter,
                                               std::atomic<float>& reductionDbMeter, std::atomic<float>& levelMeter,
                                               float reduction, float reductionDb, float level, float attackMs,
                                               float releaseMs) {
    if (!enabled)
    {
      reductionMeter.store(0.0f);
      reductionDbMeter.store(0.0f);
      levelMeter.store(0.0f);
      return;
    }

    updateAtomicBallistic(reductionMeter, reduction, currentSampleRate, blockSamples, attackMs, releaseMs);
    updateAtomicBallistic(reductionDbMeter, reductionDb, currentSampleRate, blockSamples, attackMs, releaseMs);
    updateAtomicBallistic(levelMeter, level, currentSampleRate, blockSamples, 2.0f, 70.0f);
  };

  updateStageMeter(gateEnabled, gateReductionMeter, gateReductionDbMeter, gateLevelMeter, gateReduction,
                   gateReductionDb, gateLevel, 4.0f, 320.0f);
  updateStageMeter(peakEnabled, peakReductionMeter, peakReductionDbMeter, peakLevelMeter, peakReduction,
                   peakReductionDb, peakLevel, 4.0f, 175.0f);
  updateStageMeter(glueEnabled, glueReductionMeter, glueReductionDbMeter, glueLevelMeter, glueReduction,
                   glueReductionDb, glueLevel, 8.0f, 280.0f);
  updateStageMeter(faceEnabled, faceReductionMeter, faceReductionDbMeter, faceLevelMeter, faceReduction,
                   faceReductionDb, faceLevel, 6.0f, 240.0f);
}

void VoxanovaAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;

  const auto totalInputChannels = getTotalNumInputChannels();
  const auto totalOutputChannels = getTotalNumOutputChannels();
  activeInputChannels.store(juce::jlimit(1, 2, totalInputChannels));
  activeOutputChannels.store(2);

  if (totalInputChannels == 1 && totalOutputChannels > 1)
    buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

  for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
    if (!(totalInputChannels == 1 && channel == 1))
      buffer.clear(channel, 0, buffer.getNumSamples());

  clearMeters();
}

juce::AudioProcessorEditor* VoxanovaAudioProcessor::createEditor()
{
  return new VoxanovaAudioProcessorEditor(*this);
}

bool VoxanovaAudioProcessor::hasEditor() const
{
  return true;
}

const juce::String VoxanovaAudioProcessor::getName() const
{
  return "Voxanova";
}

bool VoxanovaAudioProcessor::acceptsMidi() const
{
  return false;
}

bool VoxanovaAudioProcessor::producesMidi() const
{
  return false;
}

bool VoxanovaAudioProcessor::isMidiEffect() const
{
  return false;
}

double VoxanovaAudioProcessor::getTailLengthSeconds() const
{
  return 20.0;
}

int VoxanovaAudioProcessor::getNumPrograms()
{
  return 1;
}

int VoxanovaAudioProcessor::getCurrentProgram()
{
  return 0;
}

void VoxanovaAudioProcessor::setCurrentProgram(int)
{
}

const juce::String VoxanovaAudioProcessor::getProgramName(int)
{
  return {};
}

void VoxanovaAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void VoxanovaAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  if (auto state = parameters.copyState(); auto xml = state.createXml())
    copyXmlToBinary(*xml, destData);
}

void VoxanovaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  if (auto xml = getXmlFromBinary(data, sizeInBytes))
    if (xml->hasTagName(parameters.state.getType()))
      parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

float VoxanovaAudioProcessor::dbToGain(float db)
{
  return juce::Decibels::decibelsToGain(db);
}

float VoxanovaAudioProcessor::peakToMeter(float peak)
{
  if (peak <= 0.000001f)
    return 0.0f;

  const auto db = juce::Decibels::gainToDecibels(peak);
  return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 72.0f);
}

float VoxanovaAudioProcessor::peakToFader(float peak, float minDb, float maxDb)
{
  if (peak <= 0.000001f)
    return 0.0f;

  const auto db = juce::Decibels::gainToDecibels(peak);
  return juce::jlimit(0.0f, 100.0f, (db - minDb) / (maxDb - minDb) * 100.0f);
}

void VoxanovaAudioProcessor::updateAtomicPeak(std::atomic<float>& target, float value)
{
  auto current = target.load();
  const auto decayed = current * 0.86f;
  target.store(juce::jmax(value, decayed));
}

void VoxanovaAudioProcessor::updateAtomicBallistic(std::atomic<float>& target, float value, double sampleRate,
                                                   int numSamples, float attackMs, float releaseMs)
{
  const auto clampedTarget = juce::jlimit(0.0f, 100.0f, value);

  if (sampleRate <= 0.0 || numSamples <= 0)
  {
    target.store(clampedTarget);
    return;
  }

  const auto current = target.load();
  const auto timeMs = clampedTarget > current ? attackMs : releaseMs;

  if (timeMs <= 0.0f)
  {
    target.store(clampedTarget);
    return;
  }

  const auto coeff = std::exp(-static_cast<float>(numSamples) / static_cast<float>(sampleRate * (timeMs / 1000.0f)));
  const auto smoothed = clampedTarget + coeff * (current - clampedTarget);
  target.store(juce::jlimit(0.0f, 100.0f, smoothed));
}

void VoxanovaAudioProcessor::clearMeters()
{
  for (auto i = 0; i < 2; ++i)
  {
    inputMeterPeaks[static_cast<size_t>(i)].store(0.0f);
    outputMeterPeaks[static_cast<size_t>(i)].store(0.0f);
  }

  peakLevelMeter.store(0.0f);
  glueLevelMeter.store(0.0f);
  faceLevelMeter.store(0.0f);
  gateLevelMeter.store(0.0f);
  gateReductionMeter.store(0.0f);
  peakReductionMeter.store(0.0f);
  glueReductionMeter.store(0.0f);
  faceReductionMeter.store(0.0f);
  gateReductionDbMeter.store(0.0f);
  peakReductionDbMeter.store(0.0f);
  glueReductionDbMeter.store(0.0f);
  faceReductionDbMeter.store(0.0f);
}

float VoxanovaAudioProcessor::processOnePoleLowpass(float input, float cutoffHz, float& state) const
{
  const auto alpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(currentSampleRate));
  state += alpha * (input - state);
  return state;
}

VoxanovaAudioProcessor::GateResult VoxanovaAudioProcessor::applyVocalGate(float left, float right, float detectorLeft,
                                                                          float detectorRight, float thresholdDb)
{
  GateResult result { left, right, 0.0f };

  if (thresholdDb <= -79.9f)
  {
    gateEnvelope = 0.0f;
    gateSmoothedGain = 1.0f;
    gateHoldSamples = 0;
    result.detectorLevel = peakToFader(juce::jmax(std::abs(detectorLeft), std::abs(detectorRight)), -80.0f, 0.0f);
    return result;
  }

  constexpr auto detectorAttackMs = 0.75f;
  constexpr auto detectorReleaseMs = 48.0f;
  constexpr auto openAttackMs = 2.2f;
  constexpr auto closeReleaseMs = 125.0f;
  constexpr auto closeDeepReleaseMs = 185.0f;
  constexpr auto holdMs = 58.0f;
  constexpr auto closeKneeDb = 12.0f;
  constexpr auto maxReductionDb = 120.0f;

  const auto peakDetector = juce::jmax(std::abs(detectorLeft), std::abs(detectorRight));
  const auto rmsDetector = std::sqrt((detectorLeft * detectorLeft + detectorRight * detectorRight) * 0.5f);
  const auto detector = peakDetector * 0.46f + rmsDetector * 0.54f;

  const auto detectorCoeff =
      detector > gateEnvelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  gateEnvelope = detector + detectorCoeff * (gateEnvelope - detector);
  result.detectorLevel = peakToFader(gateEnvelope, -80.0f, 0.0f);

  const auto envelopeDb = gateEnvelope > 0.000001f ? juce::Decibels::gainToDecibels(gateEnvelope) : -120.0f;
  auto targetReductionDb = 0.0f;

  if (envelopeDb >= thresholdDb)
  {
    gateHoldSamples = juce::roundToInt(currentSampleRate * (holdMs / 1000.0));
  }
  else if (gateHoldSamples > 0)
  {
    --gateHoldSamples;
  }
  else
  {
    const auto underThresholdDb = thresholdDb - envelopeDb;
    const auto closeProgress = juce::jlimit(0.0f, 1.0f, underThresholdDb / closeKneeDb);
    const auto softClose = closeProgress * closeProgress * (3.0f - 2.0f * closeProgress);
    targetReductionDb = maxReductionDb * softClose;
  }

  const auto targetGateGain = dbToGain(-juce::jlimit(0.0f, maxReductionDb, targetReductionDb));
  const auto closeDepth = juce::jlimit(0.0f, 1.0f, targetReductionDb / maxReductionDb);
  const auto effectiveCloseReleaseMs = closeReleaseMs + (closeDeepReleaseMs - closeReleaseMs) * closeDepth;
  const auto gainCoeff =
      targetGateGain > gateSmoothedGain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (openAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (effectiveCloseReleaseMs / 1000.0f)));
  gateSmoothedGain = targetGateGain + gainCoeff * (gateSmoothedGain - targetGateGain);

  result.left = left * gateSmoothedGain;
  result.right = right * gateSmoothedGain;

  const auto reductionDb =
      gateSmoothedGain > 0.000001f && gateSmoothedGain < 0.999f ? -juce::Decibels::gainToDecibels(gateSmoothedGain)
                                                                 : targetReductionDb;
  result.reductionDb = reductionDb > 0.02f ? reductionDb : 0.0f;
  result.reduction = reductionDb > 0.02f ? juce::jlimit(0.0f, 100.0f, reductionDb / maxReductionDb * 100.0f) : 0.0f;
  return result;
}

VoxanovaAudioProcessor::CompressorResult VoxanovaAudioProcessor::applyPeakTamer(float left, float right,
                                                                                float thresholdDb,
                                                                                CompressorState& state) const
{
  CompressorResult result { left, right, 0.0f };

  const auto rawDetector = juce::jmax(std::abs(left), std::abs(right));

  constexpr auto detectorAttackMs = 0.18f;
  constexpr auto detectorReleaseMs = 54.0f;
  constexpr auto gainAttackMs = 0.75f;
  constexpr auto minReleaseMs = 62.0f;
  constexpr auto maxReleaseMs = 260.0f;
  constexpr auto ratio = 12.0f;
  constexpr auto kneeDb = 10.0f;
  constexpr auto maxReductionDb = 28.0f;
  constexpr auto recoveryRatio = 0.28f;
  constexpr auto outputLiftRatio = 0.06f;
  constexpr auto thresholdLiftRatio = 0.025f;
  constexpr auto maxOutputLiftDb = 3.2f;
  constexpr auto inputKneeDb = 18.0f;
  constexpr auto engagementAttackMs = 5.0f;
  constexpr auto engagementHoldMs = 48.0f;
  constexpr auto engagementReleaseMs = 150.0f;

  const auto rawLevelDb = rawDetector > 0.000001f ? juce::Decibels::gainToDecibels(rawDetector) : -120.0f;
  auto targetEngagement = thresholdEngagement(rawLevelDb, thresholdDb, inputKneeDb);

  if (targetEngagement > state.engagement + 0.001f)
  {
    state.holdSamples = juce::roundToInt(currentSampleRate * (engagementHoldMs / 1000.0));
  }
  else if (state.holdSamples > 0)
  {
    targetEngagement = juce::jmax(targetEngagement, state.engagement);
    --state.holdSamples;
  }

  const auto engagementCoeff =
      targetEngagement > state.engagement
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (engagementAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (engagementReleaseMs / 1000.0f)));
  state.engagement = targetEngagement + engagementCoeff * (state.engagement - targetEngagement);

  const auto pushDepth = juce::jlimit(0.0f, 1.0f, -thresholdDb / 60.0f);
  const auto inputPushDb = 42.0f * std::pow(pushDepth, 1.18f) * state.engagement;
  const auto inputPushGain = dbToGain(inputPushDb);
  const auto drivenLeft = left * inputPushGain;
  const auto drivenRight = right * inputPushGain;
  const auto detector = juce::jmax(std::abs(drivenLeft), std::abs(drivenRight));

  const auto detectorCoeff =
      detector > state.envelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  state.envelope = detector + detectorCoeff * (state.envelope - detector);
  result.detectorLevel = peakToFader(state.envelope, compressorMinDb, compressorMaxDb);

  auto targetGainDb = 0.0f;
  const auto levelDb = state.envelope > 0.000001f ? juce::Decibels::gainToDecibels(state.envelope) : -120.0f;
  const auto overDb = levelDb - thresholdDb;
  const auto halfKnee = kneeDb * 0.5f;

  if (overDb > 0.0f && overDb < kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * overDb * overDb / (2.0f * kneeDb);
  }
  else if (overDb >= kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * (overDb - halfKnee);
  }

  const auto unclampedReductionDb = juce::jmax(0.0f, -targetGainDb);
  const auto dynamicMaxReductionDb = maxReductionDb + pushDepth * 8.0f;
  const auto shapedReductionDb =
      dynamicMaxReductionDb * (1.0f - std::exp(-unclampedReductionDb / dynamicMaxReductionDb));
  targetGainDb = -juce::jlimit(0.0f, dynamicMaxReductionDb, shapedReductionDb);

  const auto targetGain = dbToGain(targetGainDb);
  const auto reductionDepth = juce::jlimit(0.0f, 1.0f, -targetGainDb / dynamicMaxReductionDb);
  const auto releaseCurve = reductionDepth * reductionDepth * (3.0f - 2.0f * reductionDepth);
  const auto releaseMs = minReleaseMs + (maxReleaseMs - minReleaseMs) * releaseCurve;
  const auto gainCoeff =
      targetGain < state.gain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (releaseMs / 1000.0f)));
  state.gain = targetGain + gainCoeff * (state.gain - targetGain);

  const auto reductionDb = state.gain < 0.999f ? -juce::Decibels::gainToDecibels(state.gain) : 0.0f;
  const auto requestedReductionDb = juce::jmax(0.0f, -targetGainDb);
  const auto recoveryDb = reductionDb * recoveryRatio;
  const auto outputLiftDb = reductionDb > 0.02f ? requestedReductionDb * outputLiftRatio : 0.0f;
  const auto thresholdLiftDb = reductionDb > 0.02f ? juce::jmax(0.0f, -thresholdDb) * thresholdLiftRatio : 0.0f;
  const auto inputLiftDb = reductionDb > 0.02f ? pushDepth * state.engagement * 1.6f : 0.0f;
  const auto dynamicMaxOutputLiftDb = maxOutputLiftDb + pushDepth * 2.6f;
  const auto outputDb = reductionDb > 0.02f
                            ? juce::jlimit(0.0f, dynamicMaxOutputLiftDb,
                                           recoveryDb + outputLiftDb + thresholdLiftDb + inputLiftDb)
                            : 0.0f;
  const auto outputGain = dbToGain(outputDb);

  result.left = left * state.gain * outputGain;
  result.right = right * state.gain * outputGain;
  result.reductionDb = reductionDb > 0.02f ? reductionDb : 0.0f;
  result.reduction =
      reductionDb > 0.02f ? juce::jlimit(0.0f, 100.0f, reductionDb / dynamicMaxReductionDb * 100.0f) : 0.0f;
  return result;
}

VoxanovaAudioProcessor::CompressorResult VoxanovaAudioProcessor::applyGlueCompressor(float left, float right,
                                                                                     float thresholdDb,
                                                                                     CompressorState& state,
                                                                                     bool multiband,
                                                                                     int bandIndex) const
{
  juce::ignoreUnused(bandIndex);

  CompressorResult result { left, right, 0.0f };

  const auto rawPeakDetector = juce::jmax(std::abs(left), std::abs(right));
  const auto rawRmsDetector = std::sqrt((left * left + right * right) * 0.5f);
  const auto rawDetector = multiband ? rawPeakDetector : rawPeakDetector * 0.48f + rawRmsDetector * 0.52f;

  auto detectorAttackMs = 7.5f;
  auto detectorReleaseMs = 115.0f;
  auto gainAttackMs = 6.0f;
  auto minReleaseMs = 125.0f;
  auto maxReleaseMs = 620.0f;
  auto ratio = 4.25f;
  auto kneeDb = 9.0f;
  auto maxReductionDb = 14.0f;
  auto makeupRatio = 0.74f;
  auto outputLiftRatio = 0.30f;
  auto thresholdLiftRatio = 0.09f;
  auto maxMakeupDb = 9.5f;
  auto maxInputPushDb = 36.0f;
  auto inputKneeDb = 8.0f;
  auto engagementAttackMs = 10.0f;
  auto engagementHoldMs = 70.0f;
  auto engagementReleaseMs = 320.0f;

  if (multiband)
  {
    detectorAttackMs = 2.5f;
    detectorReleaseMs = 250.0f;
    gainAttackMs = 2.5f;
    minReleaseMs = 250.0f;
    maxReleaseMs = 250.0f;
    ratio = 2.0f;
    kneeDb = 0.0f;
    maxReductionDb = 48.0f;
    makeupRatio = 0.0f;
    outputLiftRatio = 0.0f;
    thresholdLiftRatio = 0.0f;
    maxMakeupDb = 0.0f;
    maxInputPushDb = 0.0f;
    inputKneeDb = 0.0f;
    engagementAttackMs = 2.5f;
    engagementHoldMs = 0.0f;
    engagementReleaseMs = 250.0f;
  }

  const auto rawLevelDb = rawDetector > 0.000001f ? juce::Decibels::gainToDecibels(rawDetector) : -120.0f;
  auto targetEngagement = thresholdEngagement(rawLevelDb, thresholdDb, inputKneeDb);

  if (targetEngagement > state.engagement + 0.001f)
  {
    state.holdSamples = juce::roundToInt(currentSampleRate * (engagementHoldMs / 1000.0));
  }
  else if (state.holdSamples > 0)
  {
    targetEngagement = juce::jmax(targetEngagement, state.engagement);
    --state.holdSamples;
  }

  const auto engagementCoeff =
      targetEngagement > state.engagement
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (engagementAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (engagementReleaseMs / 1000.0f)));
  state.engagement = targetEngagement + engagementCoeff * (state.engagement - targetEngagement);

  const auto pushDepth = juce::jlimit(0.0f, 1.0f, -thresholdDb / 60.0f);
  const auto inputPushDb = maxInputPushDb * std::pow(pushDepth, 1.12f) * state.engagement;
  const auto inputPushGain = dbToGain(inputPushDb);
  const auto drivenLeft = left * inputPushGain;
  const auto drivenRight = right * inputPushGain;
  const auto peakDetector = juce::jmax(std::abs(drivenLeft), std::abs(drivenRight));
  const auto rmsDetector = std::sqrt((drivenLeft * drivenLeft + drivenRight * drivenRight) * 0.5f);
  const auto detector = multiband ? peakDetector : peakDetector * 0.48f + rmsDetector * 0.52f;

  const auto detectorCoeff =
      detector > state.envelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  state.envelope = detector + detectorCoeff * (state.envelope - detector);
  result.detectorLevel = peakToFader(state.envelope, compressorMinDb, compressorMaxDb);

  const auto levelDb = state.envelope > 0.000001f ? juce::Decibels::gainToDecibels(state.envelope) : -120.0f;
  const auto overDb = levelDb - thresholdDb;
  const auto halfKnee = kneeDb * 0.5f;
  auto targetGainDb = 0.0f;

  if (overDb > 0.0f && overDb < kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * overDb * overDb / (2.0f * kneeDb);
  }
  else if (overDb >= kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * (overDb - halfKnee);
  }

  const auto unclampedReductionDb = juce::jmax(0.0f, -targetGainDb);
  const auto dynamicMaxReductionDb = maxReductionDb + pushDepth * (multiband ? 5.0f : 16.0f);
  if (multiband)
  {
    targetGainDb = -juce::jlimit(0.0f, dynamicMaxReductionDb, unclampedReductionDb);
  }
  else
  {
    const auto shapedReductionDb =
        dynamicMaxReductionDb * (1.0f - std::exp(-unclampedReductionDb / dynamicMaxReductionDb));
    targetGainDb = -juce::jlimit(0.0f, dynamicMaxReductionDb, shapedReductionDb);
  }

  const auto targetGain = dbToGain(targetGainDb);
  const auto reductionDepth = juce::jlimit(0.0f, 1.0f, -targetGainDb / dynamicMaxReductionDb);
  const auto releaseMs = minReleaseMs + (maxReleaseMs - minReleaseMs) * reductionDepth;
  const auto gainCoeff =
      targetGain < state.gain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (releaseMs / 1000.0f)));
  state.gain = targetGain + gainCoeff * (state.gain - targetGain);

  const auto reductionDb = state.gain < 0.999f ? -juce::Decibels::gainToDecibels(state.gain) : 0.0f;
  const auto requestedReductionDb = juce::jmax(0.0f, -targetGainDb);
  const auto recoveryDb = reductionDb * makeupRatio;
  const auto outputLiftDb = reductionDb > 0.02f ? requestedReductionDb * outputLiftRatio : 0.0f;
  const auto thresholdLiftDb = reductionDb > 0.02f ? juce::jmax(0.0f, -thresholdDb) * thresholdLiftRatio : 0.0f;
  const auto inputLiftDb = reductionDb > 0.02f ? pushDepth * state.engagement * (multiband ? 1.8f : 5.2f) : 0.0f;
  const auto dynamicMaxMakeupDb = maxMakeupDb + pushDepth * (multiband ? 3.2f : 7.0f);
  const auto makeupGain =
      multiband ? 1.0f
                : dbToGain(juce::jlimit(0.0f, dynamicMaxMakeupDb,
                                        recoveryDb + outputLiftDb + thresholdLiftDb + inputLiftDb));

  result.left = left * state.gain * makeupGain;
  result.right = right * state.gain * makeupGain;
  result.reductionDb = reductionDb > 0.02f ? reductionDb : 0.0f;
  result.reduction =
      reductionDb > 0.02f ? juce::jlimit(0.0f, 100.0f, reductionDb / dynamicMaxReductionDb * 100.0f) : 0.0f;
  return result;
}

VoxanovaAudioProcessor::CompressorResult VoxanovaAudioProcessor::applyInYourFaceCompressor(float left, float right,
                                                                                           float mixPercent,
                                                                                           CompressorState& state) const
{
  CompressorResult result { left, right, 0.0f };

  const auto amount = juce::jlimit(0.0f, 1.0f, mixPercent / 100.0f);
  if (amount <= 0.0001f)
  {
    state = {};
    return result;
  }

  constexpr auto detectorAttackMs = 0.55f;
  constexpr auto detectorReleaseMs = 92.0f;
  constexpr auto gainAttackMs = 0.75f;
  constexpr auto minReleaseMs = 95.0f;
  constexpr auto maxReleaseMs = 310.0f;
  constexpr auto kneeDb = 11.0f;
  constexpr auto maxReductionDb = 36.0f;
  constexpr auto maxMakeupDb = 28.0f;
  auto smoothstep = [](float edge0, float edge1, float value) {
    const auto t = juce::jlimit(0.0f, 1.0f, (value - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
  };

  const auto topSquash = smoothstep(0.68f, 1.0f, amount);
  const auto ratio = 2.4f + amount * 10.5f + topSquash * 8.0f;
  const auto thresholdDb = -36.0f * amount;
  const auto peakDetector = juce::jmax(std::abs(left), std::abs(right));
  const auto rmsDetector = std::sqrt((left * left + right * right) * 0.5f);
  const auto detector = peakDetector * 0.32f + rmsDetector * 0.68f;

  const auto detectorCoeff =
      detector > state.envelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  state.envelope = detector + detectorCoeff * (state.envelope - detector);
  result.detectorLevel = peakToFader(state.envelope, compressorMinDb, compressorMaxDb);

  const auto levelDb = state.envelope > 0.000001f ? juce::Decibels::gainToDecibels(state.envelope) : -120.0f;
  const auto overDb = levelDb - thresholdDb;
  const auto halfKnee = kneeDb * 0.5f;
  auto targetGainDb = 0.0f;

  if (overDb > 0.0f && overDb < kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * overDb * overDb / (2.0f * kneeDb);
  }
  else if (overDb >= kneeDb)
  {
    targetGainDb = (1.0f / ratio - 1.0f) * (overDb - halfKnee);
  }

  const auto unclampedReductionDb = juce::jmax(0.0f, -targetGainDb);
  constexpr auto dynamicMaxReductionDb = maxReductionDb;
  const auto shapedReductionDb =
      dynamicMaxReductionDb * (1.0f - std::exp(-unclampedReductionDb / dynamicMaxReductionDb));
  targetGainDb = -juce::jlimit(0.0f, dynamicMaxReductionDb, shapedReductionDb);

  const auto targetGain = dbToGain(targetGainDb);
  const auto reductionDepth = juce::jlimit(0.0f, 1.0f, -targetGainDb / dynamicMaxReductionDb);
  const auto releaseMs = minReleaseMs + (maxReleaseMs - minReleaseMs) * reductionDepth;
  const auto gainCoeff =
      targetGain < state.gain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (releaseMs / 1000.0f)));
  state.gain = targetGain + gainCoeff * (state.gain - targetGain);

  const auto reductionDb = state.gain < 0.999f ? -juce::Decibels::gainToDecibels(state.gain) : 0.0f;
  const auto activeSignal = juce::jlimit(0.0f, 1.0f, (levelDb + 70.0f) / 40.0f);
  const auto compressedMakeupDb =
      juce::jlimit(0.0f, maxMakeupDb, reductionDb * 0.86f + amount * 7.0f + topSquash * 5.0f) * activeSignal;
  const auto compressedMakeupGain = dbToGain(compressedMakeupDb);

  auto applyCleanLimiter = [this, &state](float& leftSample, float& rightSample) {
    constexpr auto ceiling = 0.965f;
    const auto peak = juce::jmax(std::abs(leftSample), std::abs(rightSample));
    const auto limiterTargetGain = peak > ceiling ? ceiling / peak : 1.0f;

    if (limiterTargetGain < state.limiterGain)
    {
      state.limiterGain = limiterTargetGain;
    }
    else
    {
      const auto releaseCoeff = std::exp(-1.0f / static_cast<float>(currentSampleRate * 0.115f));
      state.limiterGain = limiterTargetGain + releaseCoeff * (state.limiterGain - limiterTargetGain);
    }

    leftSample *= state.limiterGain;
    rightSample *= state.limiterGain;

    const auto limitedPeak = juce::jmax(std::abs(leftSample), std::abs(rightSample));
    if (limitedPeak > ceiling)
    {
      const auto safetyGain = ceiling / limitedPeak;
      leftSample *= safetyGain;
      rightSample *= safetyGain;
      state.limiterGain *= safetyGain;
    }
  };

  auto mixedLeft = left * state.gain * compressedMakeupGain;
  auto mixedRight = right * state.gain * compressedMakeupGain;

  const auto requestedLoudnessLiftDb =
      juce::jlimit(0.0f, 14.0f, (amount * 5.5f + topSquash * 4.0f + reductionDb * 0.18f) * activeSignal);
  const auto loudnessLiftGain = dbToGain(requestedLoudnessLiftDb);
  mixedLeft *= loudnessLiftGain;
  mixedRight *= loudnessLiftGain;
  applyCleanLimiter(mixedLeft, mixedRight);

  const auto peak = juce::jmax(std::abs(mixedLeft), std::abs(mixedRight));
  if (peak > 0.965f)
  {
    const auto safetyGain = 0.965f / peak;
    mixedLeft *= safetyGain;
    mixedRight *= safetyGain;
  }

  result.left = mixedLeft;
  result.right = mixedRight;
  result.reductionDb = reductionDb > 0.02f ? reductionDb : 0.0f;
  result.reduction =
      reductionDb > 0.02f ? juce::jlimit(0.0f, 100.0f, reductionDb / dynamicMaxReductionDb * 100.0f) : 0.0f;
  return result;
}

VoxanovaAudioProcessor::CompressorResult VoxanovaAudioProcessor::applyCompressor(float left, float right,
                                                                                 float thresholdDb, float ratio,
                                                                                 float amountPercent, float attackMs,
                                                                                 float releaseMs, float kneeDb,
                                                                                 CompressorState& state) const
{
  CompressorResult result { left, right, 0.0f };
  const auto amount = amountPercent / 100.0f;
  if (amount <= 0.0f)
    return result;

  const auto detector = juce::jmax(std::abs(left), std::abs(right));

  const auto detectorCoeff =
      detector > state.envelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (attackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (releaseMs / 1000.0f)));
  state.envelope = detector + detectorCoeff * (state.envelope - detector);

  auto targetGainDb = 0.0f;
  if (state.envelope > 0.000001f)
  {
    const auto levelDb = juce::Decibels::gainToDecibels(state.envelope);
    const auto halfKnee = kneeDb * 0.5f;
    const auto overDb = levelDb - thresholdDb;

    if (kneeDb > 0.0f && overDb > 0.0f && overDb < kneeDb)
    {
      targetGainDb = (1.0f / ratio - 1.0f) * overDb * overDb / (2.0f * kneeDb);
    }
    else if (overDb >= kneeDb || (kneeDb <= 0.0f && overDb > 0.0f))
    {
      targetGainDb =
          kneeDb > 0.0f ? (1.0f / ratio - 1.0f) * (overDb - halfKnee) : (1.0f / ratio - 1.0f) * overDb;
    }
  }

  targetGainDb *= amount;
  const auto targetGain = dbToGain(targetGainDb);
  const auto gainCoeff =
      targetGain < state.gain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (attackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (releaseMs / 1000.0f)));
  state.gain = targetGain + gainCoeff * (state.gain - targetGain);

  result.left = left * state.gain;
  result.right = right * state.gain;
  result.reduction =
      state.gain < 0.999f
          ? juce::jlimit(0.0f, 100.0f, -juce::Decibels::gainToDecibels(state.gain) / 24.0f * 100.0f)
          : 0.0f;
  return result;
}

float VoxanovaAudioProcessor::applySoftClip(float sample, float drive)
{
  return std::tanh(sample * drive) / std::tanh(drive);
}

float VoxanovaAudioProcessor::applySaturationModel(float sample, int mode, float amountPercent, SaturationState& state,
                                                   int channel) const
{
  const auto amount = juce::jlimit(0.0f, 1.0f, amountPercent / 100.0f);
  if (mode <= 0 || amount <= 0.0f)
    return sample;

  const auto index = static_cast<size_t>(juce::jlimit(0, 1, channel));
  const auto highCut = [this, &state, index](float value, float cutoffHz) {
    return processOnePoleLowpass(value, cutoffHz, state.highTone[index]);
  };
  const auto lowBand = [this, &state, index](float value, float cutoffHz) {
    return processOnePoleLowpass(value, cutoffHz, state.lowTone[index]);
  };
  const auto dcBlock = [this, &state, index](float value) {
    const auto dc = processOnePoleLowpass(value, 18.0f, state.dcBlock[index]);
    return value - dc;
  };

  auto wet = sample;

  switch (mode)
  {
    case 1: // 1073-inspired Class-A transformer color.
    {
      const auto low = lowBand(sample, 260.0f);
      const auto shapedInput = sample + low * (0.035f + amount * 0.10f);
      const auto drive = 1.12f + amount * 3.15f;
      const auto bias = amount * 0.035f;
      const auto biased = std::tanh(shapedInput * drive + bias) - std::tanh(bias);
      const auto transformer = biased / juce::jmax(0.2f, std::tanh(drive));
      const auto softened = highCut(transformer, 18000.0f - amount * 5200.0f);
      wet = dcBlock(softened * (1.0f + amount * 0.045f));
      break;
    }
    case 2: // Studer-style tape: rounded transients, head bump, soft top.
    {
      const auto low = lowBand(sample, 95.0f);
      const auto tapeInput = sample + low * (0.06f + amount * 0.16f);
      const auto drive = 1.05f + amount * 4.25f;
      const auto compressed = std::atan(tapeInput * drive) / std::atan(drive);
      const auto tapeTop = highCut(compressed, 15000.0f - amount * 7800.0f);
      wet = dcBlock(tapeTop * (1.0f - amount * 0.035f));
      break;
    }
    case 3: // Triode-style tube attitude for vocals without hard clipping.
    {
      const auto low = lowBand(sample, 180.0f);
      const auto tubeInput = sample - low * amount * 0.035f;
      const auto drive = 1.18f + amount * 5.4f;
      const auto bias = amount * 0.075f;
      const auto triode = (std::tanh(tubeInput * drive + bias) - std::tanh(bias)) / std::tanh(drive + bias);
      const auto oddControl = triode - triode * triode * triode * (0.06f + amount * 0.10f);
      const auto rounded = highCut(oddControl, 14200.0f - amount * 5600.0f);
      wet = dcBlock(rounded * (1.0f + amount * 0.025f));
      break;
    }
    default:
      return sample;
  }

  const auto blend = std::pow(amount, 0.78f);
  const auto output = sample + (wet - sample) * blend;
  return juce::jlimit(-1.15f, 1.15f, output);
}

float VoxanovaAudioProcessor::applyUnitySaturation(float sample, float drive, float mix)
{
  const auto clampedMix = juce::jlimit(0.0f, 1.0f, mix);
  if (clampedMix <= 0.0f || drive <= 1.0f)
    return sample;

  const auto wet = std::tanh(sample * drive) / drive;
  return sample + (wet - sample) * clampedMix;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new VoxanovaAudioProcessor();
}
