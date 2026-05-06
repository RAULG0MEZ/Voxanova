#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <limits>

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
constexpr auto tuneEnabledId = "tuneEnabled";
constexpr auto tuneAmountId = "tuneAmount";
constexpr auto tuneKeyId = "tuneKey";
constexpr auto tuneScaleId = "tuneScale";
constexpr auto tuneCustomNotesId = "tuneCustomNotes";
constexpr auto tuneVoiceTypeId = "tuneVoiceType";
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
constexpr auto deEsserEnabledId = "deEsserEnabled";
constexpr auto deEsserAmountId = "deEsserAmount";
constexpr auto deEsserLowId = "deEsserLow";
constexpr auto deEsserHighId = "deEsserHigh";
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
constexpr auto delayAuxBusId = "delayAuxBus";
constexpr auto reverbAuxBusId = "reverbAuxBus";
constexpr auto compressorMinDb = -60.0f;
constexpr auto compressorMaxDb = 0.0f;
constexpr auto eqDynamicDetectorCalibrationDb = 12.0f;
constexpr auto spectrumMinFrequency = 10.0f;
constexpr auto fullSpectrumType = 9;
constexpr auto fullSpectrumMinRatio = 1.015f;
constexpr auto glueBandMinDb = compressorMinDb;
constexpr auto fullAmount = 100.0f;
constexpr std::array<float, 7> delayDivisionBeats { 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
constexpr std::array<float, 8> reverbPredelayDivisionBeats { 0.0f, 0.0625f, 0.125f, 0.25f,
                                                             0.5f, 1.0f, 2.0f, 4.0f };
constexpr std::array<const char*, 11> delayStyleLabels {
  "Clean", "Digital", "Tape", "Studio Tape", "Old Tape", "Cheap Tape",
  "Analog", "Radio", "Telephone", "Dirty", "Ambient"
};
[[maybe_unused]] constexpr std::array<const char*, 10> eqFilterTypeLabels {
  "Bell", "Surfer Bell", "Desser", "Low Cut", "High Cut", "Low Shelf", "High Shelf", "Notch", "Band Pass",
  "Full Spectrum"
};
std::array<float, 2> getSurferTrackingWindow(float anchorFrequency, float maxFrequency)
{
  const auto anchor = juce::jlimit(20.0f, maxFrequency, anchorFrequency);
  return {
    juce::jlimit(20.0f, maxFrequency, anchor * 0.5f),
    juce::jlimit(20.0f, maxFrequency, anchor * 1.5f)
  };
}
constexpr std::array<const char*, 22> reverbModeLabels {
  "Concert Hall", "Bright Hall", "Plate", "Room", "Chamber", "Random Space", "Chorus Space",
  "Ambience", "Sanctuary", "Dirty Hall", "Dirty Plate", "Smooth Plate", "Smooth Room",
  "Smooth Random", "Nonlin", "Chaotic Chamber", "Chaotic Hall", "Chaotic Neutral",
  "Cathedral", "Palace", "Chamber1979", "Hall1984"
};
constexpr std::array<const char*, 12> tuneKeyLabels {
  "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};
constexpr std::array<const char*, 3> tuneVoiceTypeLabels {
  "LOW MALE", "TENOR", "SOPRANO"
};
constexpr std::array<const char*, 39> tuneScaleLabels {
  "Custom", "Major", "Minor", "Chromatic", "Natural Minor", "Harmonic Minor", "Melodic Minor",
  "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian", "Major Pentatonic", "Minor Pentatonic",
  "Blues", "Major Blues", "Minor Blues", "Bebop Major", "Bebop Dominant", "Bebop Minor",
  "Altered", "Whole Tone", "Diminished W-H", "Diminished H-W", "Augmented", "Double Harmonic",
  "Phrygian Dominant", "Hungarian Minor", "Neapolitan Minor", "Neapolitan Major", "Spanish 8",
  "Arabic", "Persian", "Egyptian", "Hirajoshi", "In Sen", "Iwato", "Kumoi", "Pelog"
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
  return value >= 1000.0f ? juce::String(value / 1000.0f, 1) + " kHz" : juce::String(value, 0) + " Hz";
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

juce::String tuneKeyLabel(float value, int)
{
  return tuneKeyLabels[static_cast<size_t>(juce::jlimit(0, static_cast<int>(tuneKeyLabels.size()) - 1,
                                                        juce::roundToInt(value)))];
}

juce::String retunePitchLabel(float value, int)
{
  const auto amount = juce::jlimit(0.0f, 1.0f, value / 100.0f);
  const auto retuneMs = FatTuneEngine::retuneMillisecondsForAmount(amount);
  return juce::String(retuneMs, retuneMs < 10.0f ? 1 : 0) + " ms";
}

juce::String tuneScaleLabel(float value, int)
{
  return tuneScaleLabels[static_cast<size_t>(juce::jlimit(0, static_cast<int>(tuneScaleLabels.size()) - 1,
                                                          juce::roundToInt(value)))];
}

juce::String tuneCustomMaskLabel(float value, int)
{
  return juce::String(juce::jlimit(0, 4095, juce::roundToInt(value)));
}

juce::String tuneVoiceTypeLabel(float value, int)
{
  return tuneVoiceTypeLabels[static_cast<size_t>(juce::jlimit(
      0, static_cast<int>(tuneVoiceTypeLabels.size()) - 1, juce::roundToInt(value)))];
}

juce::String auxBusLabel(float value, int)
{
  const auto busIndex = juce::jlimit(0, 8, juce::roundToInt(value));
  return busIndex == 0 ? "Track" : "Bus " + juce::String(busIndex);
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
  tuneEnabledParam = parameters.getRawParameterValue(tuneEnabledId);
  tuneAmountParam = parameters.getRawParameterValue(tuneAmountId);
  tuneKeyParam = parameters.getRawParameterValue(tuneKeyId);
  tuneScaleParam = parameters.getRawParameterValue(tuneScaleId);
  tuneCustomNotesParam = parameters.getRawParameterValue(tuneCustomNotesId);
  tuneVoiceTypeParam = parameters.getRawParameterValue(tuneVoiceTypeId);
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
  deEsserEnabledParam = parameters.getRawParameterValue(deEsserEnabledId);
  deEsserAmountParam = parameters.getRawParameterValue(deEsserAmountId);
  deEsserLowParam = parameters.getRawParameterValue(deEsserLowId);
  deEsserHighParam = parameters.getRawParameterValue(deEsserHighId);
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
  delayAuxBusParam = parameters.getRawParameterValue(delayAuxBusId);
  reverbAuxBusParam = parameters.getRawParameterValue(reverbAuxBusId);
  eqSettings = std::make_shared<const EqSettings>();
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
  addFloat(stereoWidthId, "Stereo Width", 0.0f, 200.0f, 1.0f, 0.0f, percentLabel);
  addFloat(stereoLowBypassId, "Stereo Low Bypass", 0.0f, 20000.0f, 1.0f, 0.0f, hzLabel);
  addFloat(preSaturationModeId, "Pre Saturation Type", 0.0f, 3.0f, 1.0f, 0.0f, saturationModeLabel);
  addFloat(preSaturationAmountId, "Pre Saturation Amount", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(postSaturationModeId, "Post Saturation Type", 0.0f, 3.0f, 1.0f, 0.0f, saturationModeLabel);
  addFloat(postSaturationAmountId, "Post Saturation Amount", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addBool(tuneEnabledId, "Fairy Dust Tune", false);
  addFloat(tuneAmountId, "Retune Pitch", 0.0f, 100.0f, 1.0f, 82.0f, retunePitchLabel);
  addFloat(tuneKeyId, "Tune Key", 0.0f, static_cast<float>(tuneKeyLabels.size() - 1), 1.0f, 0.0f,
           tuneKeyLabel);
  addFloat(tuneScaleId, "Tune Scale", 0.0f, static_cast<float>(tuneScaleLabels.size() - 1), 1.0f, 1.0f,
           tuneScaleLabel);
  addFloat(tuneCustomNotesId, "Tune Custom Notes", 0.0f, 4095.0f, 1.0f, 4095.0f, tuneCustomMaskLabel);
  addFloat(tuneVoiceTypeId, "Tune Voice Type", 0.0f, static_cast<float>(tuneVoiceTypeLabels.size() - 1), 1.0f,
           1.0f, tuneVoiceTypeLabel);
  addBool(peakEnabledId, "Peak Tamer", true);
  addFloat(peakThresholdId, "Peak Tamer Threshold", compressorMinDb, compressorMaxDb, 0.1f, 0.0f, dbLabel);
  addBool(glueEnabledId, "Glue", true);
  addBool(glueMultibandId, "Glue Multiband", true);
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
  addBool(deEsserEnabledId, "De-Esser", false);
  addFloat(deEsserAmountId, "De-Esser Amount", 0.0f, 100.0f, 1.0f, 35.0f, percentLabel);
  addFloat(deEsserLowId, "De-Esser Low", 2500.0f, 11600.0f, 10.0f, 4700.0f, hzLabel);
  addFloat(deEsserHighId, "De-Esser High", 2900.0f, 12000.0f, 10.0f, 8500.0f, hzLabel);
  addBool(stereoEnabledId, "Stereo", false);
  addBool(reverbEnabledId, "Reverb", false);
  addFloat(reverbMixId, "Reverb Mix", 0.0f, 100.0f, 1.0f, 0.0f, percentLabel);
  addFloat(reverbDecayId, "Reverb Decay", 0.0f, 100.0f, 1.0f, 72.0f, reverbDecayLabel);
  addFloat(reverbSizeId, "Reverb Size", 0.0f, 100.0f, 1.0f, 100.0f, percentLabel);
  addFloat(reverbPredelayId, "Reverb Predelay", 0.0f, 100.0f, 1.0f, 40.0f, reverbPredelayLabel);
  addFloat(reverbLowCutId, "Reverb Low Cut", 0.0f, 100.0f, 1.0f, 0.0f, reverbLowCutLabel);
  addFloat(reverbHighCutId, "Reverb High Cut", 0.0f, 100.0f, 1.0f, 100.0f, reverbHighCutLabel);
  addFloat(reverbModeId, "Reverb Type", 0.0f, static_cast<float>(reverbModeLabels.size() - 1), 1.0f, 0.0f,
           reverbModeLabel);
  addBool(reverbSyncId, "Reverb BPM Sync", false);
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
  addFloat(delayAuxBusId, "Delay Output", 0.0f, 8.0f, 1.0f, 0.0f, auxBusLabel);
  addFloat(reverbAuxBusId, "Reverb Output", 0.0f, 8.0f, 1.0f, 0.0f, auxBusLabel);

  return { params.begin(), params.end() };
}

void VoxanovaAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  currentSampleRate = sampleRate;
  const auto delaySamples = static_cast<int>(sampleRate * 8.0);
  const auto reverbPredelaySamples = static_cast<int>(sampleRate * 2.0);
  const auto reverbEarlySamples = static_cast<int>(sampleRate * 1.5);
  const auto reverbTankSamples = static_cast<int>(sampleRate * 3.0);
  const auto reverbDiffuserSamples = static_cast<int>(sampleRate * 0.6);
  const auto reverbWidthSamples = static_cast<int>(sampleRate * 0.18);
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
  reverbWidthBuffer.setSize(1, reverbWidthSamples);
  reverbWidthBuffer.clear();
  reverbWidthWritePosition = 0;
  reverbWidthAllpassLeft = {};
  reverbWidthAllpassRight = {};
  reverbWidthModPhases = {};
  reverbWidthSideLowpass = 0.0f;
  reverbSizeSmoothed = 1.0f;
  reverbDecaySmoothed = 4.0f;
  lastReverbMode = -1;
  monoWidenBuffer.setSize(1, widenSamples);
  monoWidenBuffer.clear();
  prepareSpectra();
  tuneEngine.prepare(sampleRate, samplesPerBlock);
  setLatencySamples(tuneEngine.getLatencySamples());
  monoWidenWritePosition = 0;
  monoWidenSideLowpass = 0.0f;
  gateEnvelope = 0.0f;
  gateSmoothedGain = 1.0f;
  gateHoldSamples = 0;
  deEsserLowStates = {};
  deEsserHighStates = {};
  deEsserEnvelope = 0.0f;
  deEsserGain = 1.0f;
  preCompressorState = {};
  preEqStates.clear();
  postEqStates.clear();
  preEqStates.reserve(64);
  postEqStates.reserve(64);
  peakCompressorState = {};
  glueCompressorState = {};
  glueBandCompressorStates = {};
  for (auto& splitState : glueBandSplitStates)
    splitState.prepare(sampleRate, samplesPerBlock);
  faceCompressorState = {};
  postCompressorState = {};
  clearVisualState();
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
  for (auto band = 0u; band < snapshot.glueBandReductions.size(); ++band)
  {
    snapshot.glueBandReductions[band] = glueBandReductionMeters[band].load();
    snapshot.glueBandReductionDbs[band] = glueBandReductionDbMeters[band].load();
  }
  snapshot.peakLevel = peakLevelMeter.load();
  snapshot.glueLevel = glueLevelMeter.load();
  snapshot.faceLevel = faceLevelMeter.load();
  snapshot.gateLevel = gateLevelMeter.load();
  snapshot.hostBpm = hostBpm.load();
  snapshot.tuneFrequency = tuneFrequencyMeter.load();
  snapshot.tuneCents = tuneCentsMeter.load();
  snapshot.tuneConfidence = tuneConfidenceMeter.load();
  snapshot.tuneTargetMidi = tuneTargetMidiMeter.load();
  snapshot.visualSilence = visualSilenceActive.load();
  snapshot.processCounter = meterProcessCounter.load(std::memory_order_relaxed);
  const auto writeIndex = juce::jlimit(0, waveformSampleCount - 1, waveformWriteIndex.load());
  copyWaveform(inputWaveform, snapshot.inputWaveform, writeIndex);
  copyWaveform(peakWaveform, snapshot.peakWaveform, writeIndex);
  copyWaveform(peakOutputWaveform, snapshot.peakOutputWaveform, writeIndex);
  copyWaveform(glueWaveform, snapshot.glueWaveform, writeIndex);
  copyWaveform(glueOutputWaveform, snapshot.glueOutputWaveform, writeIndex);
  copyWaveform(faceWaveform, snapshot.faceWaveform, writeIndex);
  copyWaveform(faceOutputWaveform, snapshot.faceOutputWaveform, writeIndex);
  copyWaveform(gateWaveform, snapshot.gateWaveform, writeIndex);
  copyWaveform(gateOutputWaveform, snapshot.gateOutputWaveform, writeIndex);
  copySpectrum(preCompSpectrumAnalyzer.bins, snapshot.preCompSpectrum);
  copySpectrum(postCompSpectrumAnalyzer.bins, snapshot.postCompSpectrum);
  for (auto band = 0u; band < snapshot.preEqDetectorDbs.size(); ++band)
  {
    snapshot.preEqDetectorDbs[band] = preEqDetectorDbMeters[band].load(std::memory_order_relaxed);
    snapshot.postEqDetectorDbs[band] = postEqDetectorDbMeters[band].load(std::memory_order_relaxed);
  }

  return snapshot;
}

bool VoxanovaAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  const auto input = layouts.getMainInputChannelSet();
  const auto output = layouts.getMainOutputChannelSet();

  if (output != juce::AudioChannelSet::stereo() ||
      (input != juce::AudioChannelSet::mono() && input != juce::AudioChannelSet::stereo()))
    return false;

  for (auto busIndex = 1; busIndex < layouts.outputBuses.size(); ++busIndex)
  {
    const auto auxOutput = layouts.getChannelSet(false, busIndex);
    if (!auxOutput.isDisabled() && auxOutput != juce::AudioChannelSet::stereo())
      return false;
  }

  return true;
}

void VoxanovaAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;
  meterProcessCounter.fetch_add(1, std::memory_order_relaxed);

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
  const auto stereoWidthPercent = juce::jlimit(0.0f, 200.0f, stereoWidthParam->load());
  const auto stereoWidth = stereoWidthPercent / 100.0f;
  const auto stereoLowBypassHz = juce::jlimit(0.0f, 20000.0f, stereoLowBypassParam->load());
  const auto preSaturationMode = juce::jlimit(0, 3, juce::roundToInt(preSaturationModeParam->load()));
  const auto preSaturationAmount = juce::jlimit(0.0f, 100.0f, preSaturationAmountParam->load());
  const auto postSaturationMode = juce::jlimit(0, 3, juce::roundToInt(postSaturationModeParam->load()));
  const auto postSaturationAmount = juce::jlimit(0.0f, 100.0f, postSaturationAmountParam->load());
  const FatTuneEngine::Settings tuneSettings {
    tuneEnabledParam->load() >= 0.5f,
    juce::jlimit(0.0f, 100.0f, tuneAmountParam->load()),
    juce::jlimit(0, static_cast<int>(tuneKeyLabels.size()) - 1, juce::roundToInt(tuneKeyParam->load())),
    juce::jlimit(0, static_cast<int>(tuneScaleLabels.size()) - 1, juce::roundToInt(tuneScaleParam->load())),
    juce::jlimit(0, 4095, juce::roundToInt(tuneCustomNotesParam->load())),
    juce::jlimit(0, static_cast<int>(tuneVoiceTypeLabels.size()) - 1, juce::roundToInt(tuneVoiceTypeParam->load()))
  };
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
  const auto deEsserEnabled = deEsserEnabledParam->load() >= 0.5f;
  const auto deEsserAmount = juce::jlimit(0.0f, 100.0f, deEsserAmountParam->load());
  const auto deEsserLow = juce::jlimit(2500.0f, 11600.0f, deEsserLowParam->load());
  const auto deEsserHigh = juce::jlimit(deEsserLow + 400.0f, 12000.0f, deEsserHighParam->load());
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
  const auto maxAuxBus = juce::jmax(0, juce::jmin(8, getBusCount(false) - 1));
  const auto delayAuxBus = juce::jlimit(0, maxAuxBus, juce::roundToInt(delayAuxBusParam->load()));
  const auto reverbAuxBus = juce::jlimit(0, maxAuxBus, juce::roundToInt(reverbAuxBusParam->load()));
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
  std::array<float, 4> glueBandReductions {};
  std::array<float, 4> glueBandReductionDbs {};
  const auto waveformHopSamples = juce::jmax(1, juce::roundToInt(currentSampleRate / 40.0));

  if (!stereoEnabled)
  {
    monoWidenBuffer.clear();
    monoWidenWritePosition = 0;
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
  const auto shortDecayAmount = juce::jlimit(0.0f, 1.0f, (0.95f - reverbDecaySmoothed) / 0.75f);
  const auto shortDecayTightness = shortDecayAmount * shortDecayAmount * (3.0f - 2.0f * shortDecayAmount);
  const auto shortDecayWetTrim = 1.0f - shortDecayTightness * 0.16f;
  const auto compactPredelayAddMs = reverbSpec.predelayAddMs * (1.0f - shortDecayTightness * 0.78f);
  const auto reverbWetGain =
      reverbEnabled
          ? (1.25f + std::pow(reverbMix, 1.05f) * (2.90f + reverbSizeNorm * 0.75f + reverbDecayLift * 0.80f)) *
                shortDecayWetTrim
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
    reverbWidthBuffer.clear();
    reverbWidthWritePosition = 0;
    reverbWidthAllpassLeft = {};
    reverbWidthAllpassRight = {};
    reverbWidthModPhases = {};
    reverbWidthSideLowpass = 0.0f;
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

  auto processDiffuser = [this, &readFractionalSample, reverbSpec, reverbSizeNorm,
                          shortDecayTightness](float input, int channel) {
    constexpr std::array<float, 4> baseMs { 8.9f, 17.3f, 31.7f, 49.1f };
    constexpr std::array<float, 4> modOffsets { 0.0f, 1.7f, 3.1f, 5.3f };
    auto output = input;
    const auto compactSizeNorm = reverbSizeNorm * (1.0f - shortDecayTightness * 0.62f);
    const auto compactRoomScale = 1.0f - shortDecayTightness * 0.36f;
    const auto sizeScale = (0.42f + compactSizeNorm * 1.45f) * reverbSpec.roomScale * compactRoomScale;
    const auto baseDiffusion = juce::jlimit(
        0.20f, 0.58f, (reverbSpec.diffusion * 0.68f + compactSizeNorm * 0.055f) *
                          (1.0f - shortDecayTightness * 0.18f));
    const auto channelSkew = channel == 0 ? 0.963f : 1.041f;
    const auto channelPhase = channel == 0 ? 0.0f : juce::MathConstants<float>::pi * 0.43f;
    const auto channelOffsetMs = channel == 0 ? -0.37f : 0.53f;

    for (auto stage = 0; stage < 4; ++stage)
    {
      const auto index = static_cast<size_t>(channel * 4 + stage);
      auto& delayBuffer = reverbDiffuserBuffers[index];
      auto& writePosition = reverbDiffuserWritePositions[index];
      auto& phase = reverbDiffuserModPhases[index];
      const auto diffusion = juce::jlimit(0.22f, 0.62f, baseDiffusion * (channel == 0 ? 0.965f : 1.035f));

      const auto rateHz = 0.022f + reverbSpec.modulation * 0.052f + modOffsets[static_cast<size_t>(stage)] * 0.002f;
      phase += 2.0f * juce::MathConstants<float>::pi * rateHz / static_cast<float>(currentSampleRate);
      if (phase > 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

      const auto modSamples =
          std::sin(phase + channelPhase + static_cast<float>(stage) * 0.19f) * (0.30f + compactSizeNorm * 2.1f) *
          reverbSpec.modulation * (stage + 1) * 0.14f * (1.0f - shortDecayTightness * 0.62f);
      const auto delaySamples =
          juce::jmax(1.0f, baseMs[static_cast<size_t>(stage)] * sizeScale * channelSkew +
                               channelOffsetMs * static_cast<float>(stage + 1)) *
              static_cast<float>(currentSampleRate) / 1000.0f +
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
                         juce::roundToInt((reverbPredelayMs + compactPredelayAddMs) *
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

  auto processEarlyReflections = [this, &readEarlyReflection, reverbSpec, reverbSizeNorm, shortDecayTightness,
                                  compactPredelayAddMs](float inputLeft, float inputRight) {
    constexpr std::array<float, 8> tapMs { 5.3f, 8.1f, 13.7f, 21.9f, 34.1f, 55.7f, 89.3f, 144.7f };
    constexpr std::array<float, 8> tapGain { 0.36f, -0.31f, 0.27f, -0.23f, 0.19f, -0.15f, 0.11f, -0.08f };
    constexpr std::array<float, 8> pan { -0.68f, 0.42f, -0.18f, 0.74f, -0.52f, 0.24f, 0.58f, -0.36f };

    auto earlyLeft = 0.0f;
    auto earlyRight = 0.0f;
    const auto compactSizeNorm = reverbSizeNorm * (1.0f - shortDecayTightness * 0.68f);
    const auto compactRoomScale = 1.0f - shortDecayTightness * 0.52f;
    const auto tapScale = (0.24f + compactSizeNorm * 0.95f) * reverbSpec.roomScale * compactRoomScale;

    for (auto i = 0u; i < tapMs.size(); ++i)
    {
      const auto delaySamples = juce::jlimit(
          1, reverbEarlyBuffer.getNumSamples() - 2,
          juce::roundToInt((tapMs[i] * tapScale + compactPredelayAddMs * 0.15f) *
                           static_cast<float>(currentSampleRate) / 1000.0f));
      const auto reflectedLeft = readEarlyReflection(0, delaySamples);
      const auto reflectedRight = readEarlyReflection(1, delaySamples);
      const auto monoReflection = (reflectedLeft + reflectedRight) * 0.5f;
      const auto leftWeight = 0.5f - pan[i] * 0.28f;
      const auto rightWeight = 0.5f + pan[i] * 0.28f;
      const auto shortTapDamp = std::exp(-shortDecayTightness * tapMs[i] * 0.035f);
      earlyLeft += (reflectedLeft * 0.62f + monoReflection * 0.38f) * tapGain[i] * leftWeight * shortTapDamp;
      earlyRight += (reflectedRight * 0.62f + monoReflection * 0.38f) * tapGain[i] * rightWeight * shortTapDamp;
    }

    if (reverbEarlyBuffer.getNumSamples() > 0)
    {
      reverbEarlyBuffer.setSample(0, reverbEarlyWritePosition, inputLeft);
      reverbEarlyBuffer.setSample(1, reverbEarlyWritePosition, inputRight);
      reverbEarlyWritePosition = (reverbEarlyWritePosition + 1) % reverbEarlyBuffer.getNumSamples();
    }

    const auto earlyTrim = 0.58f - shortDecayTightness * 0.08f;
    return std::array<float, 2> { earlyLeft * reverbSpec.earlyLevel * earlyTrim,
                                  earlyRight * reverbSpec.earlyLevel * earlyTrim };
  };

  auto processReverbTank = [this, &readFractionalSample, reverbSpec, reverbSizeNorm,
                            shortDecayTightness](float inputLeft, float inputRight) {
    constexpr std::array<float, 8> baseDelayMs { 31.1f, 37.7f, 43.3f, 53.9f, 61.7f, 73.3f, 89.9f, 107.3f };
    constexpr std::array<float, 8> injectionPan { -0.92f, 0.84f, -0.42f, 0.38f, 0.12f, -0.18f, 0.64f, -0.58f };
    constexpr std::array<float, 8> outputMidTap { 0.92f, 0.84f, 0.78f, 0.70f, 0.64f, 0.58f, 0.52f, 0.46f };
    constexpr std::array<float, 8> outputSideTap { 0.34f, -0.31f, 0.25f, -0.22f, 0.18f, -0.16f, 0.14f, -0.12f };

    std::array<float, 8> tankRead {};
    std::array<float, 8> tankDelaySamples {};
    const auto compactSizeNorm = reverbSizeNorm * (1.0f - shortDecayTightness * 0.62f);
    const auto sizeCurve = std::pow(compactSizeNorm, 1.22f);
    const auto sizeScale = (0.30f + sizeCurve * 1.78f) * reverbSpec.roomScale * (1.0f - shortDecayTightness * 0.42f);
    const auto tankInputGain = (0.22f + reverbSpec.density * 0.17f) * (1.0f - shortDecayTightness * 0.44f);
    const auto modeDecaySeconds = juce::jlimit(0.08f, 24.0f, reverbDecaySmoothed * reverbSpec.decayScale);
    const auto modeDecayNorm = juce::jlimit(0.0f, 1.0f, (modeDecaySeconds - 0.08f) / 23.92f);
    const auto dampingCutoff =
        juce::jlimit(1100.0f, static_cast<float>(currentSampleRate) * 0.32f,
                         reverbSpec.highCutHz * (0.42f + (1.0f - reverbSpec.damping) * 0.28f +
                                                 (1.0f - modeDecayNorm) * 0.07f) +
                         compactSizeNorm * 520.0f);
    const auto lowBloomCutoff = juce::jlimit(95.0f, 520.0f, 155.0f + compactSizeNorm * 205.0f);

    for (auto i = 0; i < 8; ++i)
    {
      auto& phase = reverbTankModPhases[static_cast<size_t>(i)];
      const auto rateHz = 0.045f + reverbSpec.modulation * (0.025f + static_cast<float>(i) * 0.0065f);
      phase += 2.0f * juce::MathConstants<float>::pi * rateHz / static_cast<float>(currentSampleRate);
      if (phase > 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;

      const auto modSamples =
          std::sin(phase + static_cast<float>(i) * 0.73f) * (0.8f + compactSizeNorm * 10.5f) *
          reverbSpec.modulation * (1.0f - shortDecayTightness * 0.65f);
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
      damped = damped * (0.92f - reverbSpec.damping * 0.08f) + lowBloom * (0.08f + compactSizeNorm * 0.035f);
      tankRead[static_cast<size_t>(i)] = damped;
    }

    auto sum = 0.0f;
    for (const auto value : tankRead)
      sum += value;

    auto lateMid = 0.0f;
    auto lateSide = 0.0f;
    const auto monoInput = (inputLeft + inputRight) * 0.5f;
    const auto tankOutputWidth =
        juce::jlimit(0.62f, 1.38f, (0.74f + reverbSpec.width * 0.50f) * (1.0f - shortDecayTightness * 0.18f));

    for (auto i = 0; i < 8; ++i)
    {
      const auto matrixOut = sum * 0.25f - tankRead[static_cast<size_t>(i)];
      const auto delaySeconds = tankDelaySamples[static_cast<size_t>(i)] / static_cast<float>(currentSampleRate);
      const auto rt60Gain = std::pow(10.0f, -3.0f * delaySeconds / juce::jmax(0.08f, modeDecaySeconds));
      const auto feedbackFloor = juce::jlimit(0.015f, 0.12f, 0.015f + modeDecayNorm * 0.17f);
      const auto feedbackGain =
          juce::jlimit(feedbackFloor, 0.996f,
                       rt60Gain * (0.965f + modeDecayNorm * 0.025f) * (1.0f - shortDecayTightness * 0.16f));
      const auto panAmount = injectionPan[static_cast<size_t>(i)];
      const auto stereoWeighted = inputLeft * (0.5f - panAmount * 0.26f) + inputRight * (0.5f + panAmount * 0.26f);
      const auto monoSpread = monoInput * panAmount * (0.16f + reverbSpec.spread * 0.11f + compactSizeNorm * 0.08f) *
                              (1.0f - shortDecayTightness * 0.36f);
      const auto injected = stereoWeighted * 0.86f + monoInput * 0.18f + monoSpread;
      const auto writeValue = injected * tankInputGain + matrixOut * feedbackGain;
      auto& tankBuffer = reverbTankBuffers[static_cast<size_t>(i)];
      auto& writePosition = reverbTankWritePositions[static_cast<size_t>(i)];
      tankBuffer.setSample(0, writePosition, std::tanh(writeValue * 0.72f) / 0.72f);
      writePosition = (writePosition + 1) % tankBuffer.getNumSamples();

      const auto tap = tankRead[static_cast<size_t>(i)];
      lateMid += tap * outputMidTap[static_cast<size_t>(i)];
      lateSide += tap * outputSideTap[static_cast<size_t>(i)] * tankOutputWidth;
    }

    const auto outputScale =
        (0.185f + reverbSpec.density * 0.075f) * reverbSpec.lateLevel * (1.0f - shortDecayTightness * 0.50f);
    return std::array<float, 2> { (lateMid + lateSide) * outputScale, (lateMid - lateSide) * outputScale };
  };

  std::array<juce::AudioBuffer<float>, 9> auxOutputBuffers;
  for (auto busIndex = 1; busIndex < juce::jmin(static_cast<int>(auxOutputBuffers.size()), getBusCount(false)); ++busIndex)
    auxOutputBuffers[static_cast<size_t>(busIndex)] = getBusBuffer(buffer, false, busIndex);

  auto protectOutput = [](float sampleValue) {
    const auto absSample = std::abs(sampleValue);
    if (absSample <= 0.96f)
      return sampleValue;

    const auto sign = sampleValue < 0.0f ? -1.0f : 1.0f;
    const auto limited = 0.96f + std::tanh((absSample - 0.96f) * 2.8f) * 0.04f;
    return sign * juce::jlimit(0.0f, 0.999f, limited);
  };

  auto addAuxOutput = [&auxOutputBuffers, outputGain, &protectOutput](int busIndex, int sampleIndex, float left,
                                                                      float right) {
    if (busIndex <= 0 || busIndex >= static_cast<int>(auxOutputBuffers.size()))
      return;

    auto& auxBuffer = auxOutputBuffers[static_cast<size_t>(busIndex)];
    if (auxBuffer.getNumChannels() < 2)
      return;

    auxBuffer.addSample(0, sampleIndex, protectOutput(left * outputGain));
    auxBuffer.addSample(1, sampleIndex, protectOutput(right * outputGain));
  };

  const auto eqSnapshot = std::atomic_load(&eqSettings);
  const std::vector<EqBandSettings> emptyEqBands;
  const auto& preEqBands = eqSnapshot != nullptr ? eqSnapshot->preBands : emptyEqBands;
  const auto& postEqBands = eqSnapshot != nullptr ? eqSnapshot->postBands : emptyEqBands;
  const auto surferEqActive = eqBandsNeedPitchTracking(preEqBands) || eqBandsNeedPitchTracking(postEqBands);
  for (auto& meter : preEqDetectorDbMeters)
    meter.store(-120.0f, std::memory_order_relaxed);
  for (auto& meter : postEqDetectorDbMeters)
    meter.store(-120.0f, std::memory_order_relaxed);

  if (preEqBands.empty())
    resetEqStates(preEqStates);
  else
    prepareEq(preEqStates, preEqBands);

  if (postEqBands.empty())
    resetEqStates(postEqStates);
  else
    prepareEq(postEqStates, postEqBands);

  for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
  {
    const auto rawLeft = buffer.getSample(0, sample);
    const auto rawRight = totalInputChannels > 1 ? buffer.getSample(1, sample) : rawLeft;

    inputPeaks[0] = juce::jmax(inputPeaks[0], std::abs(rawLeft));
    inputPeaks[1] = juce::jmax(inputPeaks[1], std::abs(rawRight));
    auto left = rawLeft * inputGain;
    auto right = rawRight * inputGain;
    auto peakDetectorSample = [](float leftSample, float rightSample) {
      return juce::jlimit(0.0f, 1.0f, juce::jmax(std::abs(leftSample), std::abs(rightSample)));
    };

    auto blendedDetectorSample = [](float leftSample, float rightSample, float peakWeight, float rmsWeight) {
      const auto peakDetector = juce::jmax(std::abs(leftSample), std::abs(rightSample));
      const auto rmsDetector = std::sqrt((leftSample * leftSample + rightSample * rightSample) * 0.5f);
      return juce::jlimit(0.0f, 1.0f, peakDetector * peakWeight + rmsDetector * rmsWeight);
    };

    const auto inputDisplaySample = peakDetectorSample(left, right);

    const auto tuned = tuneEngine.processSample(left, right, tuneSettings, surferEqActive);
    left = tuned.left;
    right = tuned.right;

    if (!preEqBands.empty())
      applyEq(preEqStates, preEqBands, left, right, preEqDetectorDbMeters);

    pushSpectrumSample(preCompSpectrumAnalyzer, (left + right) * 0.5f);

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
    const auto peakDisplaySample = peakDetectorSample(left, right);

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
    const auto peakOutputDisplaySample = peakDetectorSample(left, right);
    const auto glueDisplaySample = glueMultiband ? peakDetectorSample(left, right)
                                                 : blendedDetectorSample(left, right, 0.48f, 0.52f);

    if (glueEnabled && glueMultiband)
    {
      std::array<float, 4> leftBands {};
      std::array<float, 4> rightBands {};

      leftBands = glueBandSplitStates[0].process(left);
      rightBands = glueBandSplitStates[1].process(right);

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
        glueBandReductions[static_cast<size_t>(band)] =
            juce::jmax(glueBandReductions[static_cast<size_t>(band)], glueResult.reduction);
        glueBandReductionDbs[static_cast<size_t>(band)] =
            juce::jmax(glueBandReductionDbs[static_cast<size_t>(band)], glueResult.reductionDb);
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
    const auto glueOutputDisplaySample = glueMultiband ? peakDetectorSample(left, right)
                                                       : blendedDetectorSample(left, right, 0.48f, 0.52f);
    const auto faceDisplaySample = blendedDetectorSample(left, right, 0.32f, 0.68f);

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
    const auto faceOutputDisplaySample = blendedDetectorSample(left, right, 0.32f, 0.68f);

    if (deEsserEnabled && deEsserAmount > 0.0f)
    {
      applyDeEsser(left, right, deEsserAmount, deEsserLow, deEsserHigh);
    }
    else
    {
      deEsserLowStates = {};
      deEsserHighStates = {};
      deEsserEnvelope = 0.0f;
      deEsserGain = 1.0f;
    }

    applyStereoCompressor(left, right, -14.0f, 2.2f, fullAmount, 12.0f, 120.0f, 5.0f, postCompressorState);
    left = applySaturationModel(left, postSaturationMode, postSaturationAmount, postSaturationState, 0);
    right = applySaturationModel(right, postSaturationMode, postSaturationAmount, postSaturationState, 1);

    if (!postEqBands.empty())
      applyEq(postEqStates, postEqBands, left, right, postEqDetectorDbMeters);

    pushSpectrumSample(postCompSpectrumAnalyzer, (left + right) * 0.5f);

    const auto gateDisplaySample = blendedDetectorSample(left, right, 0.46f, 0.54f);

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
    const auto gateOutputDisplaySample = blendedDetectorSample(left, right, 0.46f, 0.54f);

    inputWaveformPeak = juce::jmax(inputWaveformPeak, std::abs(inputDisplaySample));
    peakWaveformPeak = juce::jmax(peakWaveformPeak, std::abs(peakDisplaySample));
    peakOutputWaveformPeak = juce::jmax(peakOutputWaveformPeak, std::abs(peakOutputDisplaySample));
    glueWaveformPeak = juce::jmax(glueWaveformPeak, std::abs(glueDisplaySample));
    glueOutputWaveformPeak = juce::jmax(glueOutputWaveformPeak, std::abs(glueOutputDisplaySample));
    faceWaveformPeak = juce::jmax(faceWaveformPeak, std::abs(faceDisplaySample));
    faceOutputWaveformPeak = juce::jmax(faceOutputWaveformPeak, std::abs(faceOutputDisplaySample));
    gateWaveformPeak = juce::jmax(gateWaveformPeak, std::abs(gateDisplaySample));
    gateOutputWaveformPeak = juce::jmax(gateOutputWaveformPeak, std::abs(gateOutputDisplaySample));

    if (++waveformDownsampleCounter >= waveformHopSamples)
    {
      waveformDownsampleCounter = 0;
      const auto writeIndex = juce::jlimit(0, waveformSampleCount - 1, waveformWriteIndex.load());
      storeWaveformSample(inputWaveform, writeIndex, inputWaveformPeak);
      storeWaveformSample(peakWaveform, writeIndex, peakWaveformPeak);
      storeWaveformSample(peakOutputWaveform, writeIndex, peakOutputWaveformPeak);
      storeWaveformSample(glueWaveform, writeIndex, glueWaveformPeak);
      storeWaveformSample(glueOutputWaveform, writeIndex, glueOutputWaveformPeak);
      storeWaveformSample(faceWaveform, writeIndex, faceWaveformPeak);
      storeWaveformSample(faceOutputWaveform, writeIndex, faceOutputWaveformPeak);
      storeWaveformSample(gateWaveform, writeIndex, gateWaveformPeak);
      storeWaveformSample(gateOutputWaveform, writeIndex, gateOutputWaveformPeak);
      waveformWriteIndex.store((writeIndex + 1) % waveformSampleCount);
      inputWaveformPeak = 0.0f;
      peakWaveformPeak = 0.0f;
      peakOutputWaveformPeak = 0.0f;
      glueWaveformPeak = 0.0f;
      glueOutputWaveformPeak = 0.0f;
      faceWaveformPeak = 0.0f;
      faceOutputWaveformPeak = 0.0f;
      gateWaveformPeak = 0.0f;
      gateOutputWaveformPeak = 0.0f;
    }

    if (stereoEnabled && totalOutputChannels > 1)
    {
      const auto mid = (left + right) * 0.5f;
      const auto rawSide = (left - right) * 0.5f;
      const auto lowAnchor =
          stereoLowBypassHz > 0.5f ? processOnePoleLowpass(mid, stereoLowBypassHz, monoWidenSideLowpass) : 0.0f;
      const auto widenInput = mid - lowAnchor;
      auto generatedSide = 0.0f;
      const auto widenBufferSize = monoWidenBuffer.getNumSamples();
      if (widenBufferSize > 2)
      {
        auto readMonoWidenDelay = [this, widenBufferSize](float delaySamples) {
          auto readPosition = static_cast<float>(monoWidenWritePosition) - delaySamples;
          while (readPosition < 0.0f)
            readPosition += static_cast<float>(widenBufferSize);

          const auto indexA = juce::jlimit(0, widenBufferSize - 1, static_cast<int>(readPosition));
          const auto indexB = (indexA + 1) % widenBufferSize;
          const auto fraction = readPosition - static_cast<float>(indexA);
          return monoWidenBuffer.getSample(0, indexA) * (1.0f - fraction) +
                 monoWidenBuffer.getSample(0, indexB) * fraction;
        };

        const auto delayMs =
            stereoWidthPercent <= 100.0f
                ? 1.25f + stereoWidthPercent * 0.025f
                : 3.75f + (stereoWidthPercent - 100.0f) * (2.9166667f / 100.0f);
        const auto delaySamples = juce::jlimit(1.0f, static_cast<float>(widenBufferSize - 2),
                                               delayMs * static_cast<float>(currentSampleRate) / 1000.0f);
        generatedSide = readMonoWidenDelay(delaySamples) * juce::jmin(stereoWidth, 1.0f);

        monoWidenBuffer.setSample(0, monoWidenWritePosition, widenInput);
        monoWidenWritePosition = (monoWidenWritePosition + 1) % widenBufferSize;
      }

      const auto side = rawSide + generatedSide;
      left = mid + side;
      right = mid - side;
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
    auto wetOnlyStage = [](float wet, float mix, float wetGain) {
      const auto clampedMix = juce::jlimit(0.0f, 1.0f, mix);
      return wet * std::sin(clampedMix * juce::MathConstants<float>::halfPi) * wetGain;
    };

    auto processReverbWet = [&](float inputLeft, float inputRight) {
      if (!reverbEnabled)
        return std::array<float, 2> { 0.0f, 0.0f };

      const auto stableSend = stabilizeStereoReturn(inputLeft, inputRight, 0.92f);
      const auto sendMid = (stableSend[0] + stableSend[1]) * 0.5f;
      const auto sendSide = (stableSend[0] - stableSend[1]) * 0.5f;
      const auto sendSideScale = juce::jlimit(0.66f, 1.32f, 0.78f + reverbSpec.width * 0.34f);
      const auto crossfeed = juce::jlimit(0.04f, 0.24f, 0.08f + reverbSpec.spread * 0.10f);
      const auto spreadLeft = sendMid + sendSide * sendSideScale;
      const auto spreadRight = sendMid - sendSide * sendSideScale;
      auto reverbInputLeft = spreadLeft * (1.0f - crossfeed) + spreadRight * crossfeed;
      auto reverbInputRight = spreadRight * (1.0f - crossfeed) + spreadLeft * crossfeed;

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
      const auto naturalSide = (tonedLeft - tonedRight) * 0.5f;

      auto processWidthAllpass = [](float input, float coefficient, float& state) {
        const auto output = -coefficient * input + state;
        state = input + coefficient * output;
        return output;
      };

      auto decorrelateWidth = [&processWidthAllpass](float input, std::array<float, 4>& states,
                                                     const std::array<float, 4>& coefficients) {
        auto output = input;
        for (auto i = 0u; i < states.size(); ++i)
          output = processWidthAllpass(output, coefficients[i], states[i]);
        return output;
      };

      auto readReverbWidthDelay = [this, &readFractionalSample](float delayInSamples) {
        if (reverbWidthBuffer.getNumSamples() <= 2)
          return 0.0f;

        return readFractionalSample(reverbWidthBuffer, reverbWidthWritePosition, delayInSamples);
      };

      const auto twoPi = 2.0f * juce::MathConstants<float>::pi;
      reverbWidthModPhases[0] += twoPi * (0.061f + reverbSpec.modulation * 0.017f) / static_cast<float>(currentSampleRate);
      reverbWidthModPhases[1] += twoPi * (0.087f + reverbSpec.modulation * 0.023f) / static_cast<float>(currentSampleRate);
      if (reverbWidthModPhases[0] > twoPi)
        reverbWidthModPhases[0] -= twoPi;
      if (reverbWidthModPhases[1] > twoPi)
        reverbWidthModPhases[1] -= twoPi;

      const auto widthDelayScale =
          (0.82f + reverbSizeNorm * 0.78f + reverbSpec.roomScale * 0.16f) *
          (1.0f - shortDecayTightness * 0.55f);
      const auto widthModDepthMs =
          (0.34f + reverbSpec.modulation * (0.56f + reverbSizeNorm * 0.42f)) *
          (1.0f - shortDecayTightness * 0.62f);
      const auto widthDelayLeftMs =
          18.7f * widthDelayScale + std::sin(reverbWidthModPhases[0]) * widthModDepthMs;
      const auto widthDelayRightMs =
          31.3f * widthDelayScale +
          std::sin(reverbWidthModPhases[1] + juce::MathConstants<float>::pi * 0.31f) * widthModDepthMs;
      const auto widthTapLeft =
          readReverbWidthDelay(widthDelayLeftMs * static_cast<float>(currentSampleRate) / 1000.0f);
      const auto widthTapRight =
          readReverbWidthDelay(widthDelayRightMs * static_cast<float>(currentSampleRate) / 1000.0f);

      if (reverbWidthBuffer.getNumSamples() > 2)
      {
        reverbWidthBuffer.setSample(0, reverbWidthWritePosition, juce::jlimit(-1.4f, 1.4f, wetMid));
        reverbWidthWritePosition = (reverbWidthWritePosition + 1) % reverbWidthBuffer.getNumSamples();
      }

      constexpr std::array<float, 4> widthLeftCoefficients { 0.527f, 0.713f, 0.842f, 0.931f };
      constexpr std::array<float, 4> widthRightCoefficients { 0.431f, 0.659f, 0.809f, 0.913f };
      const auto decorrelatedLeft =
          decorrelateWidth(widthTapLeft * 0.82f + wetMid * 0.18f, reverbWidthAllpassLeft, widthLeftCoefficients);
      const auto decorrelatedRight =
          decorrelateWidth(widthTapRight * 0.82f + wetMid * 0.18f, reverbWidthAllpassRight, widthRightCoefficients);
      const auto decorrelatedSide = (decorrelatedLeft - decorrelatedRight) * 0.5f;
      const auto lowSide =
          processOnePoleLowpass(decorrelatedSide, 150.0f + reverbSizeNorm * 70.0f, reverbWidthSideLowpass);
      const auto airyDecorrelatedSide = decorrelatedSide - lowSide * 0.72f;
      const auto naturalSideGain =
          juce::jlimit(2.6f, 7.4f, 3.15f + reverbSpec.width * 2.2f + reverbSpec.spread * 1.08f +
                                       reverbSizeNorm * 0.92f) *
          (1.0f - shortDecayTightness * 0.10f);
      const auto decorrelatedSideGain =
          juce::jlimit(0.62f, 2.85f,
                       0.78f + reverbSpec.width * 0.70f + reverbSpec.spread * 0.55f + reverbSizeNorm * 0.42f +
                           reverbDecayLift * 0.22f) *
          (1.0f - shortDecayTightness * 0.42f);
      const auto wetSideRaw = naturalSide * naturalSideGain + airyDecorrelatedSide * decorrelatedSideGain;
      const auto wetSide = std::tanh(wetSideRaw * 0.82f) / 0.82f;
      const auto widthMakeup = 1.0f / std::sqrt(1.0f + decorrelatedSideGain * 0.055f + naturalSideGain * 0.018f);
      const auto centeredLeft = (wetMid + wetSide) * reverbMakeupGain * widthMakeup;
      const auto centeredRight = (wetMid - wetSide) * reverbMakeupGain * widthMakeup;
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
    const auto delayIsExternal = delayEnabled && delayAuxBus > 0;
    const auto reverbIsExternal = reverbEnabled && reverbAuxBus > 0;

    if (delayPostReverb)
    {
      const auto reverbReturn = processReverbWet(dryLeft, dryRight);
      const auto reverbWetLeft = reverbEnabled ? wetOnlyStage(reverbReturn[0], reverbMix, reverbWetGain) : 0.0f;
      const auto reverbWetRight = reverbEnabled ? wetOnlyStage(reverbReturn[1], reverbMix, reverbWetGain) : 0.0f;
      if (reverbIsExternal)
        addAuxOutput(reverbAuxBus, sample, reverbWetLeft, reverbWetRight);

      const auto reverbStageLeft =
          reverbEnabled ? (reverbIsExternal ? dryLeft : mixStage(dryLeft, reverbReturn[0], reverbMix, reverbWetGain))
                        : dryLeft;
      const auto reverbStageRight =
          reverbEnabled ? (reverbIsExternal ? dryRight : mixStage(dryRight, reverbReturn[1], reverbMix, reverbWetGain))
                        : dryRight;
      const auto stablePostReverbDelaySend =
          stabilizeStereoReturn(reverbStageLeft, reverbStageRight, delayMode == 1 ? 0.76f : 0.64f);
      delayInputLeft = stablePostReverbDelaySend[0];
      delayInputRight = stablePostReverbDelaySend[1];
      const auto delayWetLeft = delayEnabled ? wetOnlyStage(delayReturnLeft, delayMix, delayWetGain) : 0.0f;
      const auto delayWetRight = delayEnabled ? wetOnlyStage(delayReturnRight, delayMix, delayWetGain) : 0.0f;
      if (delayIsExternal)
        addAuxOutput(delayAuxBus, sample, delayWetLeft, delayWetRight);

      preOutputLeft =
          delayEnabled ? (delayIsExternal ? reverbStageLeft : mixStage(reverbStageLeft, delayReturnLeft, delayMix, delayWetGain))
                       : reverbStageLeft;
      preOutputRight =
          delayEnabled ? (delayIsExternal ? reverbStageRight : mixStage(reverbStageRight, delayReturnRight, delayMix, delayWetGain))
                       : reverbStageRight;
    }
    else
    {
      delayInputLeft = stableDelaySend[0];
      delayInputRight = stableDelaySend[1];
      const auto delayWetLeft = delayEnabled ? wetOnlyStage(delayReturnLeft, delayMix, delayWetGain) : 0.0f;
      const auto delayWetRight = delayEnabled ? wetOnlyStage(delayReturnRight, delayMix, delayWetGain) : 0.0f;
      if (delayIsExternal)
        addAuxOutput(delayAuxBus, sample, delayWetLeft, delayWetRight);

      const auto delayStageLeft =
          delayEnabled ? (delayIsExternal ? dryLeft : mixStage(dryLeft, delayReturnLeft, delayMix, delayWetGain)) : dryLeft;
      const auto delayStageRight =
          delayEnabled ? (delayIsExternal ? dryRight : mixStage(dryRight, delayReturnRight, delayMix, delayWetGain))
                       : dryRight;
      const auto reverbReturn = processReverbWet(delayStageLeft, delayStageRight);
      const auto reverbWetLeft = reverbEnabled ? wetOnlyStage(reverbReturn[0], reverbMix, reverbWetGain) : 0.0f;
      const auto reverbWetRight = reverbEnabled ? wetOnlyStage(reverbReturn[1], reverbMix, reverbWetGain) : 0.0f;
      if (reverbIsExternal)
        addAuxOutput(reverbAuxBus, sample, reverbWetLeft, reverbWetRight);

      preOutputLeft =
          reverbEnabled
              ? (reverbIsExternal ? delayStageLeft : mixStage(delayStageLeft, reverbReturn[0], reverbMix, reverbWetGain))
              : delayStageLeft;
      preOutputRight =
          reverbEnabled
              ? (reverbIsExternal ? delayStageRight : mixStage(delayStageRight, reverbReturn[1], reverbMix, reverbWetGain))
              : delayStageRight;
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

  const auto tuneMeters = tuneEngine.getMeters();
  tuneFrequencyMeter.store(tuneMeters.frequency);
  tuneCentsMeter.store(tuneMeters.cents);
  tuneConfidenceMeter.store(tuneMeters.confidence);
  tuneTargetMidiMeter.store(tuneMeters.targetMidi);

  const auto blockSamples = buffer.getNumSamples();
  visualSilenceActive.store(false);

  for (auto i = 0; i < 2; ++i)
  {
    updateAtomicPeak(inputMeterPeaks[static_cast<size_t>(i)], inputPeaks[static_cast<size_t>(i)]);
    updateAtomicPeak(outputMeterPeaks[static_cast<size_t>(i)], outputPeaks[static_cast<size_t>(i)]);
  }

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

  for (auto band = 0u; band < glueBandReductionMeters.size(); ++band)
  {
    if (!glueEnabled || !glueMultiband)
    {
      glueBandReductionMeters[band].store(0.0f);
      glueBandReductionDbMeters[band].store(0.0f);
      continue;
    }

    updateAtomicBallistic(glueBandReductionMeters[band], glueBandReductions[band], currentSampleRate, blockSamples,
                          8.0f, 280.0f);
    updateAtomicBallistic(glueBandReductionDbMeters[band], glueBandReductionDbs[band], currentSampleRate, blockSamples,
                          8.0f, 280.0f);
  }
}

void VoxanovaAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
  juce::ScopedNoDenormals noDenormals;
  meterProcessCounter.fetch_add(1, std::memory_order_relaxed);

  const auto totalInputChannels = getTotalNumInputChannels();
  const auto totalOutputChannels = getTotalNumOutputChannels();
  activeInputChannels.store(juce::jlimit(1, 2, totalInputChannels));
  activeOutputChannels.store(2);

  if (totalInputChannels == 1 && totalOutputChannels > 1)
    buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

  for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
    if (!(totalInputChannels == 1 && channel == 1))
      buffer.clear(channel, 0, buffer.getNumSamples());

  clearVisualState();
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
  auto state = parameters.copyState();

  if (auto snapshot = std::atomic_load(&eqSettings))
    state.setProperty("eqBands", serializeEqBands(*snapshot), nullptr);

  if (auto xml = state.createXml())
    copyXmlToBinary(*xml, destData);
}

void VoxanovaAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  if (auto xml = getXmlFromBinary(data, sizeInBytes))
  {
    if (xml->hasTagName(parameters.state.getType()))
    {
      auto state = juce::ValueTree::fromXml(*xml);
      const auto eqBandsJson = state.getProperty("eqBands", {}).toString();
      parameters.replaceState(state);

      if (eqBandsJson.isNotEmpty())
        setEqBandsFromVar(juce::JSON::parse(eqBandsJson));
      else
        setEqBandsFromVar(juce::var(new juce::DynamicObject()));
    }
  }
}

std::vector<VoxanovaAudioProcessor::EqBandSettings> VoxanovaAudioProcessor::parseEqBandArray(const juce::var& bands)
{
  std::vector<EqBandSettings> parsed;
  const auto* array = bands.getArray();

  if (array == nullptr)
    return parsed;

  parsed.reserve(static_cast<size_t>(array->size()));

  auto readFloat = [](const juce::var& object, const juce::Identifier& id, float defaultValue) {
    const auto value = object.getProperty(id, defaultValue);
    if (value.isVoid())
      return defaultValue;
    return static_cast<float>(value);
  };

  auto readBool = [](const juce::var& object, const juce::Identifier& id, bool defaultValue) {
    const auto value = object.getProperty(id, defaultValue);
    if (value.isBool())
      return static_cast<bool>(value);
    if (value.isString())
    {
      const auto text = value.toString();
      if (text.equalsIgnoreCase("false") || text == "0" || text.equalsIgnoreCase("off"))
        return false;
      if (text.equalsIgnoreCase("true") || text == "1" || text.equalsIgnoreCase("on"))
        return true;
    }
    return static_cast<float>(value) >= 0.5f;
  };

  auto readType = [](const juce::var& object) {
    const auto value = object.getProperty("type", "Bell");

    if (!value.isString())
      return juce::jlimit(0, static_cast<int>(eqFilterTypeLabels.size()) - 1,
                          juce::roundToInt(static_cast<float>(value)));

    const auto text = value.toString();
    for (auto index = 0; index < static_cast<int>(eqFilterTypeLabels.size()); ++index)
      if (text.equalsIgnoreCase(eqFilterTypeLabels[static_cast<size_t>(index)]))
        return index;

    return 0;
  };

  auto defaultQForType = [](int type) {
    if (type == 0)
      return 1.0f;

    if (type == fullSpectrumType)
      return 8.0f;

    return type == 5 || type == 6 ? 1.3f : 5.0f;
  };

  auto defaultSlopeForType = [](int type) {
    return type == 3 ? 30 : 12;
  };

  auto readSlope = [&defaultSlopeForType](const juce::var& object, int type) {
    const auto value = object.getProperty("slope", defaultSlopeForType(type));
    if (value.isString() && value.toString().equalsIgnoreCase("wall"))
      return VoxanovaAudioProcessor::eqWallSlopeDb;

    return juce::jlimit(6, VoxanovaAudioProcessor::eqWallSlopeDb, juce::roundToInt(static_cast<float>(value)));
  };

  for (const auto& band : *array)
  {
    EqBandSettings settings;
    settings.enabled = readBool(band, "on", true) && readBool(band, "enabled", true);
    settings.solo = readBool(band, "solo", false);
    settings.type = readType(band);
    if (settings.type == 2)
      continue;

    settings.frequency = juce::jlimit(20.0f, 20000.0f, readFloat(band, "freq", 1000.0f));
    settings.gainDb = juce::jlimit(settings.type == 3 || settings.type == 4 ? 0.0f : -30.0f, 30.0f,
                                   readFloat(band, "gain", 0.0f));
    settings.q = juce::jlimit(0.1f, 50.0f, readFloat(band, "q", defaultQForType(settings.type)));
    if (settings.type == fullSpectrumType)
    {
      const auto range = normalizeFullSpectrumRange(
          settings.frequency, settings.q, readFloat(band, "rangeLow", std::numeric_limits<float>::quiet_NaN()),
          readFloat(band, "rangeHigh", std::numeric_limits<float>::quiet_NaN()));
      settings.rangeLowHz = range[0];
      settings.rangeHighHz = range[1];
      settings.frequency = getFullSpectrumCenter(settings.rangeLowHz, settings.rangeHighHz);
      settings.rangeLowSlope = juce::jlimit(0.1f, 50.0f, readFloat(band, "rangeLowSlope", settings.q));
      settings.rangeHighSlope = juce::jlimit(0.1f, 50.0f, readFloat(band, "rangeHighSlope", settings.q));
    }
    settings.compDb = juce::jlimit(-30.0f, 30.0f, readFloat(band, "comp", 0.0f));
    const auto legacyCompEnabled = std::abs(settings.compDb) > 0.05f &&
                                   std::abs(settings.compDb - settings.gainDb) > 0.05f;
    settings.compEnabled = readBool(band, "compEnabled", legacyCompEnabled);
    settings.compThresholdDb = juce::jlimit(-60.0f, 0.0f, readFloat(band, "compThreshold", -18.0f));
    settings.compAttackMs = juce::jlimit(0.1f, 200.0f, readFloat(band, "compAttack", 12.0f));
    settings.compReleaseMs = juce::jlimit(5.0f, 1000.0f, readFloat(band, "compRelease", 140.0f));
    settings.compRatio = juce::jlimit(1.0f, 20.0f, readFloat(band, "compRatio", 4.0f));
    settings.saturationMode = juce::jlimit(0, 3, juce::roundToInt(readFloat(band, "saturationMode", readFloat(band, "satMode", 0.0f))));
    settings.saturationAmount = settings.saturationMode > 0
                                    ? juce::jlimit(0.0f, 100.0f,
                                                   readFloat(band, "saturationAmount", readFloat(band, "satAmount", 20.0f)))
                                    : 0.0f;
    if (!eqBandSupportsSaturation(settings))
    {
      settings.saturationMode = 0;
      settings.saturationAmount = 0.0f;
    }
    settings.slopeDb = readSlope(band, settings.type);
    settings.thresholdDb = juce::jlimit(-60.0f, 0.0f, readFloat(band, "threshold", -24.0f));
    settings.intensity = juce::jlimit(0.0f, 100.0f, readFloat(band, "intensity", 50.0f));
    const auto deessMode = band.getProperty("deessMode", "split").toString();
    settings.deessMode = deessMode.equalsIgnoreCase("wider") || deessMode == "1" ? 1 : 0;
    settings.surfRatio = juce::jlimit(0.0f, 128.0f, readFloat(band, "surfRatio", 0.0f));

    parsed.push_back(settings);
  }

  return parsed;
}

juce::var VoxanovaAudioProcessor::eqBandArrayToVar(const std::vector<EqBandSettings>& bands)
{
  juce::Array<juce::var> array;
  array.ensureStorageAllocated(static_cast<int>(bands.size()));

  for (const auto& band : bands)
  {
    auto object = juce::DynamicObject::Ptr(new juce::DynamicObject());
    const auto typeIndex = juce::jlimit(0, static_cast<int>(eqFilterTypeLabels.size()) - 1, band.type);
    object->setProperty("type", eqFilterTypeLabels[static_cast<size_t>(typeIndex)]);
    object->setProperty("freq", band.frequency);
    object->setProperty("gain", band.gainDb);
    object->setProperty("q", band.q);
    if (band.type == fullSpectrumType)
    {
      object->setProperty("rangeLow", band.rangeLowHz);
      object->setProperty("rangeHigh", band.rangeHighHz);
      object->setProperty("rangeLowSlope", band.rangeLowSlope);
      object->setProperty("rangeHighSlope", band.rangeHighSlope);
    }
    object->setProperty("solo", band.solo);
    object->setProperty("comp", band.compDb);
    object->setProperty("compEnabled", band.compEnabled);
    object->setProperty("compThreshold", band.compThresholdDb);
    object->setProperty("compAttack", band.compAttackMs);
    object->setProperty("compRelease", band.compReleaseMs);
    object->setProperty("compRatio", band.compRatio);
    object->setProperty("saturationMode", band.saturationMode);
    object->setProperty("saturationAmount", band.saturationAmount);
    object->setProperty("slope", band.slopeDb >= eqWallSlopeDb ? juce::var("wall") : juce::var(band.slopeDb));
    object->setProperty("threshold", band.thresholdDb);
    object->setProperty("intensity", band.intensity);
    object->setProperty("deessMode", band.deessMode == 1 ? "wider" : "split");
    if (band.surfRatio > 0.0f)
      object->setProperty("surfRatio", band.surfRatio);
    object->setProperty("on", band.enabled);
    object->setProperty("placement", "stereo");
    array.add(juce::var(object.get()));
  }

  return juce::var(array);
}

juce::String VoxanovaAudioProcessor::serializeEqBands(const EqSettings& settings)
{
  auto object = juce::DynamicObject::Ptr(new juce::DynamicObject());
  object->setProperty("pre", eqBandArrayToVar(settings.preBands));
  object->setProperty("post", eqBandArrayToVar(settings.postBands));
  return juce::JSON::toString(juce::var(object.get()), false);
}

juce::var VoxanovaAudioProcessor::getEqBandsState() const
{
  auto snapshot = std::atomic_load(&eqSettings);
  auto object = juce::DynamicObject::Ptr(new juce::DynamicObject());

  if (snapshot != nullptr)
  {
    object->setProperty("pre", eqBandArrayToVar(snapshot->preBands));
    object->setProperty("post", eqBandArrayToVar(snapshot->postBands));
  }
  else
  {
    object->setProperty("pre", juce::var(juce::Array<juce::var>()));
    object->setProperty("post", juce::var(juce::Array<juce::var>()));
  }

  return juce::var(object.get());
}

void VoxanovaAudioProcessor::setEqBandsFromVar(const juce::var& payload)
{
  auto source = payload;
  if (source.isString())
    source = juce::JSON::parse(source.toString());

  auto next = std::make_shared<EqSettings>();
  next->preBands = parseEqBandArray(source.getProperty("pre", {}));
  next->postBands = parseEqBandArray(source.getProperty("post", {}));
  std::shared_ptr<const EqSettings> immutableNext = std::move(next);
  std::atomic_store(&eqSettings, immutableNext);
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

void VoxanovaAudioProcessor::clearWaveform(std::array<std::atomic<float>, waveformSampleCount>& waveform)
{
  for (auto& sample : waveform)
    sample.store(0.0f);
}

void VoxanovaAudioProcessor::storeWaveformSample(std::array<std::atomic<float>, waveformSampleCount>& waveform,
                                                 int index, float value)
{
  const auto clampedIndex = juce::jlimit(0, waveformSampleCount - 1, index);
  waveform[static_cast<size_t>(clampedIndex)].store(juce::jlimit(-1.0f, 1.0f, value));
}

void VoxanovaAudioProcessor::copyWaveform(const std::array<std::atomic<float>, waveformSampleCount>& source,
                                          std::array<float, waveformSampleCount>& destination, int writeIndex)
{
  const auto clampedWriteIndex = juce::jlimit(0, waveformSampleCount - 1, writeIndex);
  for (auto i = 0; i < waveformSampleCount; ++i)
  {
    const auto index = (clampedWriteIndex + i) % waveformSampleCount;
    destination[static_cast<size_t>(i)] = source[static_cast<size_t>(index)].load();
  }
}

void VoxanovaAudioProcessor::clearSpectrum(std::array<std::atomic<float>, spectrumBinCount>& spectrum)
{
  for (auto& bin : spectrum)
    bin.store(0.0f);
}

void VoxanovaAudioProcessor::copySpectrum(const std::array<std::atomic<float>, spectrumBinCount>& source,
                                          std::array<float, spectrumBinCount>& destination)
{
  for (auto i = 0; i < spectrumBinCount; ++i)
    destination[static_cast<size_t>(i)] = source[static_cast<size_t>(i)].load();
}

void VoxanovaAudioProcessor::prepareSpectrum(SpectrumAnalyzerState& analyzer)
{
  std::fill(analyzer.ring.begin(), analyzer.ring.end(), 0.0f);
  std::fill(analyzer.smoothed.begin(), analyzer.smoothed.end(), 0.0f);
  analyzer.writePosition = 0;
  analyzer.hopCounter = 0;
  clearSpectrum(analyzer.bins);
}

void VoxanovaAudioProcessor::prepareSpectra()
{
  for (auto i = 0; i < spectrumFftSize; ++i)
  {
    spectrumWindow[static_cast<size_t>(i)] =
        0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) /
                               static_cast<float>(spectrumFftSize - 1));
  }

  prepareSpectrum(preCompSpectrumAnalyzer);
  prepareSpectrum(postCompSpectrumAnalyzer);
}

void VoxanovaAudioProcessor::clearSpectrumAnalyzer(SpectrumAnalyzerState& analyzer)
{
  clearSpectrum(analyzer.bins);
  std::fill(analyzer.smoothed.begin(), analyzer.smoothed.end(), 0.0f);
}

void VoxanovaAudioProcessor::pushSpectrumSample(SpectrumAnalyzerState& analyzer, float sample)
{
  analyzer.ring[static_cast<size_t>(analyzer.writePosition)] =
      juce::jlimit(-2.0f, 2.0f, std::isfinite(sample) ? sample : 0.0f);
  analyzer.writePosition = (analyzer.writePosition + 1) % spectrumFftSize;

  if (++analyzer.hopCounter >= spectrumHopSize)
  {
    analyzer.hopCounter = 0;
    analyseSpectrum(analyzer);
  }
}

void VoxanovaAudioProcessor::analyseSpectrum(SpectrumAnalyzerState& analyzer)
{
  if (currentSampleRate <= 1000.0)
    return;

  std::array<float, spectrumFftSize * 2> fftFrame {};
  auto windowSum = 0.0f;

  for (auto i = 0; i < spectrumFftSize; ++i)
  {
    const auto readIndex = (analyzer.writePosition + i) % spectrumFftSize;
    const auto window = spectrumWindow[static_cast<size_t>(i)];
    fftFrame[static_cast<size_t>(i)] = analyzer.ring[static_cast<size_t>(readIndex)] * window;
    windowSum += window;
  }

  spectrumFft.performFrequencyOnlyForwardTransform(fftFrame.data(), true);

  const auto maxFftBin = spectrumFftSize / 2 - 1;
  const auto maxSpectrumFrequency =
      juce::jlimit(spectrumMinFrequency, 20000.0f, static_cast<float>(currentSampleRate * 0.5 - 1.0));
  const auto spectrumFrequencyRatio = juce::jmax(1.0001f, maxSpectrumFrequency / spectrumMinFrequency);
  const auto analysisIntervalSeconds = static_cast<float>(spectrumHopSize) /
                                       juce::jmax(1.0f, static_cast<float>(currentSampleRate));
  const auto attackCoeff = std::exp(-analysisIntervalSeconds / 0.026f);
  const auto releaseCoeff = std::exp(-analysisIntervalSeconds / 0.62f);
  const auto frequencyToBin = [this](float frequency) {
    return frequency / static_cast<float>(currentSampleRate) * static_cast<float>(spectrumFftSize);
  };
  std::array<float, spectrumFftSize / 2> fftMagnitudes {};

  for (auto fftBin = 1; fftBin <= maxFftBin; ++fftBin)
  {
    const auto magnitude = fftFrame[static_cast<size_t>(fftBin)] * 2.0f / juce::jmax(1.0f, windowSum);
    fftMagnitudes[static_cast<size_t>(fftBin)] = juce::jmax(0.0f, std::isfinite(magnitude) ? magnitude : 0.0f);
  }

  const auto sampleMagnitudeAtBin = [&fftMagnitudes](float binPosition) {
    const auto clamped = juce::jlimit(1.0f, static_cast<float>(maxFftBin), binPosition);
    const auto lower = juce::jlimit(1, maxFftBin, static_cast<int>(std::floor(clamped)));
    const auto upper = juce::jlimit(lower, maxFftBin, lower + 1);
    const auto mix = clamped - static_cast<float>(lower);
    const auto lowerMagnitude = fftMagnitudes[static_cast<size_t>(lower)];
    const auto upperMagnitude = fftMagnitudes[static_cast<size_t>(upper)];
    return lowerMagnitude + (upperMagnitude - lowerMagnitude) * mix;
  };

  for (auto bin = 0; bin < spectrumBinCount; ++bin)
  {
    const auto centerT =
        spectrumBinCount <= 1 ? 0.0f : static_cast<float>(bin) / static_cast<float>(spectrumBinCount - 1);
    const auto halfBinT = spectrumBinCount <= 1 ? 0.5f : 0.5f / static_cast<float>(spectrumBinCount - 1);
    const auto lowT = juce::jlimit(0.0f, 1.0f, centerT - halfBinT);
    const auto highT = juce::jlimit(0.0f, 1.0f, centerT + halfBinT);
    const auto lowFrequency =
        juce::jlimit(spectrumMinFrequency, maxSpectrumFrequency,
                     spectrumMinFrequency * std::pow(spectrumFrequencyRatio, lowT));
    const auto highFrequency =
        juce::jlimit(lowFrequency, maxSpectrumFrequency,
                     spectrumMinFrequency * std::pow(spectrumFrequencyRatio, highT));
    const auto centerFrequency =
        juce::jlimit(spectrumMinFrequency, maxSpectrumFrequency,
                     spectrumMinFrequency * std::pow(spectrumFrequencyRatio, centerT));
    const auto firstFftBin = juce::jlimit(1, maxFftBin, static_cast<int>(std::floor(frequencyToBin(lowFrequency))));
    const auto lastFftBin = juce::jlimit(firstFftBin, maxFftBin, static_cast<int>(std::ceil(frequencyToBin(highFrequency))));

    auto magnitudeSumSquares = 0.0f;
    auto weightSum = 0.0f;
    auto peakMagnitude = 0.0f;
    const auto smoothingOctaves =
        juce::jmax(0.010f, std::log2(juce::jmax(1.0001f, highFrequency / lowFrequency)) * 0.72f);

    for (auto fftBin = firstFftBin; fftBin <= lastFftBin; ++fftBin)
    {
      const auto magnitude = fftMagnitudes[static_cast<size_t>(fftBin)];
      const auto fftFrequency = static_cast<float>(fftBin) * static_cast<float>(currentSampleRate) /
                                static_cast<float>(spectrumFftSize);
      const auto distance = std::abs(std::log2(juce::jmax(1.0f, fftFrequency) / centerFrequency));
      const auto normalizedDistance = distance / smoothingOctaves;
      const auto weight = normalizedDistance >= 1.0f
                              ? 0.0f
                              : 0.5f + 0.5f * std::cos(juce::MathConstants<float>::pi * normalizedDistance);
      magnitudeSumSquares += magnitude * magnitude * weight;
      weightSum += weight;
      peakMagnitude = juce::jmax(peakMagnitude, magnitude);
    }

    const auto rmsMagnitude = weightSum > 0.0f ? std::sqrt(magnitudeSumSquares / weightSum) : 0.0f;
    const auto centerMagnitude = sampleMagnitudeAtBin(frequencyToBin(centerFrequency));
    const auto magnitude = juce::jmax(rmsMagnitude, juce::jmax(centerMagnitude * 0.72f, peakMagnitude * 0.32f));
    const auto db = juce::Decibels::gainToDecibels(magnitude, -120.0f);
    const auto tiltDb = 4.5f * std::log2(juce::jmax(20.0f, centerFrequency) / 1000.0f);
    constexpr auto analyzerFloorDb = -78.0f;
    constexpr auto analyzerCeilingDb = -18.0f;
    const auto normalized = juce::jlimit(0.0f, 1.0f, (db + tiltDb - analyzerFloorDb) /
                                                            (analyzerCeilingDb - analyzerFloorDb));
    auto& smoothed = analyzer.smoothed[static_cast<size_t>(bin)];
    const auto coeff = normalized > smoothed ? attackCoeff : releaseCoeff;
    smoothed = normalized + coeff * (smoothed - normalized);
    analyzer.bins[static_cast<size_t>(bin)].store(juce::jlimit(0.0f, 1.0f, smoothed));
  }
}

void VoxanovaAudioProcessor::resetWaveformAccumulators()
{
  inputWaveformPeak = 0.0f;
  peakWaveformPeak = 0.0f;
  peakOutputWaveformPeak = 0.0f;
  glueWaveformPeak = 0.0f;
  glueOutputWaveformPeak = 0.0f;
  faceWaveformPeak = 0.0f;
  faceOutputWaveformPeak = 0.0f;
  gateWaveformPeak = 0.0f;
  gateOutputWaveformPeak = 0.0f;
}

void VoxanovaAudioProcessor::clearWaveformBuffers()
{
  waveformWriteIndex.store(0);
  waveformDownsampleCounter = 0;
  clearWaveform(inputWaveform);
  clearWaveform(peakWaveform);
  clearWaveform(peakOutputWaveform);
  clearWaveform(glueWaveform);
  clearWaveform(glueOutputWaveform);
  clearWaveform(faceWaveform);
  clearWaveform(faceOutputWaveform);
  clearWaveform(gateWaveform);
  clearWaveform(gateOutputWaveform);
  resetWaveformAccumulators();
}

void VoxanovaAudioProcessor::clearVisualState(bool resetSpectrum)
{
  clearMeters();
  clearWaveformBuffers();
  if (resetSpectrum)
  {
    clearSpectrumAnalyzer(preCompSpectrumAnalyzer);
    clearSpectrumAnalyzer(postCompSpectrumAnalyzer);
  }
  visualSilenceActive.store(true);
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

  for (auto& meter : glueBandReductionMeters)
    meter.store(0.0f);
  for (auto& meter : glueBandReductionDbMeters)
    meter.store(0.0f);
  for (auto& meter : preEqDetectorDbMeters)
    meter.store(-120.0f);
  for (auto& meter : postEqDetectorDbMeters)
    meter.store(-120.0f);

  tuneFrequencyMeter.store(0.0f);
  tuneCentsMeter.store(0.0f);
  tuneConfidenceMeter.store(0.0f);
  tuneTargetMidiMeter.store(0.0f);
}

float VoxanovaAudioProcessor::processOnePoleLowpass(float input, float cutoffHz, float& state) const
{
  const auto alpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(currentSampleRate));
  state += alpha * (input - state);
  return state;
}

bool VoxanovaAudioProcessor::eqBandHasEffect(const EqBandSettings& settings)
{
  if (!settings.enabled)
    return false;

  if (settings.solo)
    return true;

  switch (settings.type)
  {
    case 2: // Desser
      return false;
    case 3: // Low Cut
    case 4: // High Cut
    case 7: // Notch
    case 8: // Band Pass
      return true;
    case fullSpectrumType: // Full Spectrum
      return std::abs(settings.gainDb) > 0.01f || eqBandHasCompressionTarget(settings) || eqBandHasSaturation(settings);
    default:
      return std::abs(settings.gainDb) > 0.01f || eqBandHasCompressionTarget(settings) || eqBandHasSaturation(settings);
  }
}

bool VoxanovaAudioProcessor::eqBandSupportsCompression(const EqBandSettings& settings)
{
  switch (settings.type)
  {
    case 0: // Bell
    case 1: // Surfer Bell
    case 5: // Low Shelf
    case 6: // High Shelf
    case 8: // Band Pass
    case fullSpectrumType: // Full Spectrum
      return true;
    default:
      return false;
  }
}

bool VoxanovaAudioProcessor::eqBandSupportsSaturation(const EqBandSettings& settings)
{
  return eqBandSupportsCompression(settings);
}

bool VoxanovaAudioProcessor::eqBandHasCompression(const EqBandSettings& settings)
{
  return eqBandHasCompressionTarget(settings) && settings.compEnabled;
}

bool VoxanovaAudioProcessor::eqBandHasSaturation(const EqBandSettings& settings)
{
  return settings.enabled && eqBandSupportsSaturation(settings) && settings.saturationMode > 0 &&
         settings.saturationAmount > 0.05f;
}

bool VoxanovaAudioProcessor::eqBandHasCompressionTarget(const EqBandSettings& settings)
{
  return settings.enabled && eqBandSupportsCompression(settings) && std::abs(settings.compDb - settings.gainDb) > 0.05f;
}

bool VoxanovaAudioProcessor::eqBandsNeedPitchTracking(const std::vector<EqBandSettings>& settings)
{
  return std::any_of(settings.begin(), settings.end(), [](const auto& band) {
    return band.enabled && band.type == 1;
  });
}

int VoxanovaAudioProcessor::eqFilterStageCount(int slopeDb)
{
  if (slopeDb >= eqWallSlopeDb)
    return eqMaxCutFilterStages;

  return juce::jlimit(1, eqMaxCutFilterStages, eqCutBiquadStageCount(slopeDb) +
                                                   (eqCutHasFirstOrderStage(slopeDb) ? 1 : 0));
}

bool VoxanovaAudioProcessor::eqCutHasFirstOrderStage(int slopeDb)
{
  if (slopeDb >= eqWallSlopeDb)
    return false;

  return slopeDb % 12 == 6;
}

int VoxanovaAudioProcessor::eqCutBiquadStageCount(int slopeDb)
{
  if (slopeDb >= eqWallSlopeDb)
    return eqMaxCutFilterStages;

  return juce::jlimit(0, eqMaxCutFilterStages, slopeDb / 12);
}

int VoxanovaAudioProcessor::eqBandFilterStageCount(const EqBandSettings& settings)
{
  if (settings.type == 3 || settings.type == 4)
    return eqFilterStageCount(settings.slopeDb) + 1;

  return settings.type == fullSpectrumType ? 2 : 1;
}

float VoxanovaAudioProcessor::getCutResonanceFrequency(const EqBandSettings& settings)
{
  const auto stageCount = juce::jmax(1, eqFilterStageCount(settings.slopeDb));
  const auto offsetOctaves = juce::jlimit(0.13f, 0.42f, 0.46f / std::sqrt(static_cast<float>(stageCount)));
  const auto ratio = std::pow(2.0f, offsetOctaves);
  return settings.type == 3
             ? juce::jlimit(20.0f, 20000.0f, settings.frequency * ratio)
             : juce::jlimit(20.0f, 20000.0f, settings.frequency / ratio);
}

float VoxanovaAudioProcessor::getCutResonanceQ(const EqBandSettings& settings)
{
  const auto stageCount = juce::jmax(1, eqFilterStageCount(settings.slopeDb));
  const auto gain = juce::jlimit(0.0f, 30.0f, settings.gainDb);
  return juce::jlimit(0.7f, 10.0f, 0.9f + std::sqrt(static_cast<float>(stageCount)) * 0.82f + gain * 0.055f);
}

float VoxanovaAudioProcessor::getFullSpectrumCenter(float lowHz, float highHz)
{
  return juce::jlimit(20.0f, 20000.0f, std::sqrt(juce::jmax(20.0f, lowHz) * juce::jmax(20.0f, highHz)));
}

std::array<float, 2> VoxanovaAudioProcessor::getFullSpectrumFallbackRange(float frequency, float q)
{
  const auto center = juce::jlimit(20.0f, 20000.0f, frequency);
  juce::ignoreUnused(q);
  constexpr auto factor = 2.0f;
  return {
    juce::jlimit(20.0f, 20000.0f, center / factor),
    juce::jlimit(20.0f, 20000.0f, center * factor)
  };
}

std::array<float, 2> VoxanovaAudioProcessor::normalizeFullSpectrumRange(float frequency, float q, float lowHz,
                                                                        float highHz)
{
  auto fallback = getFullSpectrumFallbackRange(frequency, q);
  auto low = lowHz;
  auto high = highHz;

  if (!std::isfinite(low) || !std::isfinite(high) || low <= 0.0f || high <= 0.0f || high <= low * fullSpectrumMinRatio)
  {
    low = fallback[0];
    high = fallback[1];
  }

  low = juce::jlimit(20.0f, 20000.0f, low);
  high = juce::jlimit(20.0f, 20000.0f, high);

  if (high <= low * fullSpectrumMinRatio)
  {
    const auto center = juce::jlimit(20.0f, 20000.0f, frequency);
    const auto halfRatio = std::sqrt(fullSpectrumMinRatio);
    low = juce::jlimit(20.0f, 20000.0f / fullSpectrumMinRatio, center / halfRatio);
    high = juce::jlimit(low * fullSpectrumMinRatio, 20000.0f, center * halfRatio);
  }

  return { low, high };
}

void VoxanovaAudioProcessor::setBiquadCoefficients(EqFilterStage& stage, float b0, float b1, float b2, float a0,
                                                   float a1, float a2)
{
  if (std::abs(a0) <= 0.000001f || !std::isfinite(a0))
  {
    stage.setBypass();
    return;
  }

  stage.setCoefficients(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

void VoxanovaAudioProcessor::setPeakingFilter(EqFilterStage& stage, float frequency, float q, float gainDb) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto safeQ = juce::jlimit(0.1f, 50.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto alpha = sinOmega / (2.0f * safeQ);
  const auto a = std::pow(10.0f, gainDb / 40.0f);

  setBiquadCoefficients(stage,
                        1.0f + alpha * a,
                        -2.0f * cosOmega,
                        1.0f - alpha * a,
                        1.0f + alpha / a,
                        -2.0f * cosOmega,
                        1.0f - alpha / a);
}

void VoxanovaAudioProcessor::setLowShelfFilter(EqFilterStage& stage, float frequency, float q, float gainDb) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto slope = juce::jlimit(0.1f, 2.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto a = std::pow(10.0f, gainDb / 40.0f);
  const auto twoSqrtAAlpha =
      2.0f * std::sqrt(a) * sinOmega * 0.5f *
      std::sqrt(juce::jmax(0.0f, (a + 1.0f / a) * (1.0f / slope - 1.0f) + 2.0f));

  setBiquadCoefficients(stage,
                        a * ((a + 1.0f) - (a - 1.0f) * cosOmega + twoSqrtAAlpha),
                        2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosOmega),
                        a * ((a + 1.0f) - (a - 1.0f) * cosOmega - twoSqrtAAlpha),
                        (a + 1.0f) + (a - 1.0f) * cosOmega + twoSqrtAAlpha,
                        -2.0f * ((a - 1.0f) + (a + 1.0f) * cosOmega),
                        (a + 1.0f) + (a - 1.0f) * cosOmega - twoSqrtAAlpha);
}

void VoxanovaAudioProcessor::setHighShelfFilter(EqFilterStage& stage, float frequency, float q, float gainDb) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto slope = juce::jlimit(0.1f, 2.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto a = std::pow(10.0f, gainDb / 40.0f);
  const auto twoSqrtAAlpha =
      2.0f * std::sqrt(a) * sinOmega * 0.5f *
      std::sqrt(juce::jmax(0.0f, (a + 1.0f / a) * (1.0f / slope - 1.0f) + 2.0f));

  setBiquadCoefficients(stage,
                        a * ((a + 1.0f) + (a - 1.0f) * cosOmega + twoSqrtAAlpha),
                        -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosOmega),
                        a * ((a + 1.0f) + (a - 1.0f) * cosOmega - twoSqrtAAlpha),
                        (a + 1.0f) - (a - 1.0f) * cosOmega + twoSqrtAAlpha,
                        2.0f * ((a - 1.0f) - (a + 1.0f) * cosOmega),
                        (a + 1.0f) - (a - 1.0f) * cosOmega - twoSqrtAAlpha);
}

void VoxanovaAudioProcessor::setLowPassFilter(EqFilterStage& stage, float frequency, float q) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto safeQ = juce::jlimit(0.25f, 4.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto alpha = sinOmega / (2.0f * safeQ);

  setBiquadCoefficients(stage,
                        (1.0f - cosOmega) * 0.5f,
                        1.0f - cosOmega,
                        (1.0f - cosOmega) * 0.5f,
                        1.0f + alpha,
                        -2.0f * cosOmega,
                        1.0f - alpha);
}

void VoxanovaAudioProcessor::setLowPassFirstOrderFilter(EqFilterStage& stage, float frequency) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto k = std::tan(juce::MathConstants<float>::pi * freq / safeRate);

  setBiquadCoefficients(stage, k, k, 0.0f, 1.0f + k, k - 1.0f, 0.0f);
}

void VoxanovaAudioProcessor::setHighPassFilter(EqFilterStage& stage, float frequency, float q) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto safeQ = juce::jlimit(0.25f, 4.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto alpha = sinOmega / (2.0f * safeQ);

  setBiquadCoefficients(stage,
                        (1.0f + cosOmega) * 0.5f,
                        -(1.0f + cosOmega),
                        (1.0f + cosOmega) * 0.5f,
                        1.0f + alpha,
                        -2.0f * cosOmega,
                        1.0f - alpha);
}

void VoxanovaAudioProcessor::setHighPassFirstOrderFilter(EqFilterStage& stage, float frequency) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto k = std::tan(juce::MathConstants<float>::pi * freq / safeRate);

  setBiquadCoefficients(stage, 1.0f, -1.0f, 0.0f, 1.0f + k, k - 1.0f, 0.0f);
}

void VoxanovaAudioProcessor::setCutResonanceFilter(EqFilterStage& stage, const EqBandSettings& settings,
                                                   float gainDb) const
{
  const auto resonanceGainDb = juce::jlimit(0.0f, 30.0f, gainDb);
  if (resonanceGainDb <= 0.01f)
  {
    stage.setBypass();
    return;
  }

  setPeakingFilter(stage, getCutResonanceFrequency(settings), getCutResonanceQ(settings), resonanceGainDb);
}

void VoxanovaAudioProcessor::setNotchFilter(EqFilterStage& stage, float frequency, float q) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto safeQ = juce::jlimit(0.1f, 50.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto alpha = sinOmega / (2.0f * safeQ);

  setBiquadCoefficients(stage, 1.0f, -2.0f * cosOmega, 1.0f, 1.0f + alpha, -2.0f * cosOmega, 1.0f - alpha);
}

void VoxanovaAudioProcessor::setBandPassFilter(EqFilterStage& stage, float frequency, float q, float gainDb) const
{
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto freq = juce::jlimit(20.0f, safeRate * 0.45f, frequency);
  const auto safeQ = juce::jlimit(0.1f, 50.0f, q);
  const auto omega = 2.0f * juce::MathConstants<float>::pi * freq / safeRate;
  const auto sinOmega = std::sin(omega);
  const auto cosOmega = std::cos(omega);
  const auto alpha = sinOmega / (2.0f * safeQ);
  const auto gain = dbToGain(gainDb);

  setBiquadCoefficients(stage, alpha * gain, 0.0f, -alpha * gain, 1.0f + alpha, -2.0f * cosOmega, 1.0f - alpha);
}

float VoxanovaAudioProcessor::getSurferEqFrequency(EqBandState& state, const EqBandSettings& settings) const
{
  const auto maxEqFrequency = juce::jmax(20.0f, static_cast<float>(currentSampleRate) * 0.45f);
  auto targetFrequency = juce::jlimit(20.0f, maxEqFrequency, settings.frequency);
  const auto detectedFrequency = tuneEngine.getDetectedFrequency();
  const auto detectorReady = detectedFrequency >= 55.0f && tuneEngine.getDetectedClarity() >= 0.58f;
  const auto staticFrequencyChanged = std::abs(settings.frequency - state.previousStaticFrequency) > 0.5f;

  if (!detectorReady && staticFrequencyChanged)
    state.hasSurferRatio = false;

  if (detectorReady)
  {
    const auto explicitRatio = settings.surfRatio > 0.0001f;

    if (explicitRatio)
    {
      state.surferRatio = settings.surfRatio;
      state.hasSurferRatio = true;
    }
    else if (!state.hasSurferRatio || staticFrequencyChanged)
    {
      state.surferRatio = juce::jlimit(0.125f, 128.0f, settings.frequency / detectedFrequency);
      state.hasSurferRatio = true;
    }

    if (state.hasSurferRatio)
    {
      const auto trackedFrequency = juce::jlimit(20.0f, maxEqFrequency, detectedFrequency * state.surferRatio);
      const auto trackingWindow = getSurferTrackingWindow(settings.frequency, maxEqFrequency);
      targetFrequency = trackedFrequency >= trackingWindow[0] && trackedFrequency <= trackingWindow[1]
                            ? trackedFrequency
                            : juce::jlimit(20.0f, maxEqFrequency, settings.frequency);
    }
  }

  state.previousStaticFrequency = settings.frequency;

  if (!detectorReady)
  {
    state.surferFrequency = targetFrequency;
    return state.surferFrequency;
  }

  if (!state.hasSurferFrequency)
  {
    state.surferFrequency = targetFrequency;
    state.hasSurferFrequency = true;
  }
  else
  {
    state.surferFrequency = targetFrequency + 0.68f * (state.surferFrequency - targetFrequency);
  }

  return state.surferFrequency;
}

void VoxanovaAudioProcessor::configureEqBandForGain(EqBandState& state, const EqBandSettings& settings,
                                                    float gainDb) const
{
  const auto bandFrequency =
      settings.type == 1 && state.hasSurferFrequency ? state.surferFrequency : settings.frequency;
  const auto safeGainDb = juce::jlimit(-30.0f, 30.0f, gainDb);

  for (auto& channelStages : state.filters)
  {
    for (auto& stage : channelStages)
      stage.setBypass();

    switch (settings.type)
    {
      case 1: // Surfer Bell
        setPeakingFilter(channelStages[0], bandFrequency, juce::jlimit(0.1f, 50.0f, settings.q * 0.58f),
                         safeGainDb);
        break;
      case 3: // Low Cut
      {
        auto stage = 0;
        if (eqCutHasFirstOrderStage(settings.slopeDb))
          setHighPassFirstOrderFilter(channelStages[static_cast<size_t>(stage++)], bandFrequency);
        for (auto biquad = 0; biquad < eqCutBiquadStageCount(settings.slopeDb); ++biquad)
          setHighPassFilter(channelStages[static_cast<size_t>(stage++)], bandFrequency, 0.7071f);
        setCutResonanceFilter(channelStages[static_cast<size_t>(stage)], settings, safeGainDb);
        break;
      }
      case 4: // High Cut
      {
        auto stage = 0;
        if (eqCutHasFirstOrderStage(settings.slopeDb))
          setLowPassFirstOrderFilter(channelStages[static_cast<size_t>(stage++)], bandFrequency);
        for (auto biquad = 0; biquad < eqCutBiquadStageCount(settings.slopeDb); ++biquad)
          setLowPassFilter(channelStages[static_cast<size_t>(stage++)], bandFrequency, 0.7071f);
        setCutResonanceFilter(channelStages[static_cast<size_t>(stage)], settings, safeGainDb);
        break;
      }
      case 5: // Low Shelf
        setLowShelfFilter(channelStages[0], bandFrequency, settings.q, safeGainDb);
        break;
      case 6: // High Shelf
        setHighShelfFilter(channelStages[0], bandFrequency, settings.q, safeGainDb);
        break;
      case 7: // Notch
        setNotchFilter(channelStages[0], bandFrequency, settings.q);
        break;
      case 8: // Band Pass
        setBandPassFilter(channelStages[0], bandFrequency, settings.q, safeGainDb);
        break;
      case fullSpectrumType: // Full Spectrum
        juce::ignoreUnused(safeGainDb);
        setHighPassFilter(channelStages[0], settings.rangeLowHz, 0.7071f);
        setLowPassFilter(channelStages[1], settings.rangeHighHz, 0.7071f);
        break;
      case 2: // Desser
        break;
      case 0: // Bell
      default:
        setPeakingFilter(channelStages[0], bandFrequency, settings.q, safeGainDb);
        break;
    }
  }
}

void VoxanovaAudioProcessor::configureEqBandSolo(EqBandState& state, const EqBandSettings& settings) const
{
  const auto bandFrequency =
      settings.type == 1 && state.hasSurferFrequency ? state.surferFrequency : settings.frequency;

  for (auto& channelStages : state.soloFilters)
  {
    for (auto& stage : channelStages)
      stage.setBypass();

    if (!settings.enabled || !settings.solo)
      continue;

    switch (settings.type)
    {
      case 3: // Low Cut
      case 5: // Low Shelf
        setLowPassFilter(channelStages[0], bandFrequency, 0.7071f);
        break;
      case 4: // High Cut
      case 6: // High Shelf
        setHighPassFilter(channelStages[0], bandFrequency, 0.7071f);
        break;
      case 1: // Surfer Bell
        setBandPassFilter(channelStages[0], bandFrequency, juce::jlimit(0.1f, 50.0f, settings.q * 0.58f), 0.0f);
        break;
      case 0: // Bell
      case 7: // Notch
      case 8: // Band Pass
        setBandPassFilter(channelStages[0], bandFrequency, settings.q, 0.0f);
        break;
      case fullSpectrumType: // Full Spectrum
        setHighPassFilter(channelStages[0], settings.rangeLowHz, 0.7071f);
        setLowPassFilter(channelStages[1], settings.rangeHighHz, 0.7071f);
        break;
      default:
        setBandPassFilter(channelStages[0], bandFrequency, settings.q, 0.0f);
        break;
    }
  }
}

void VoxanovaAudioProcessor::configureEqBandCompressionDetector(EqBandState& state,
                                                                const EqBandSettings& settings) const
{
  for (auto& detector : state.compDetectorFilters)
    for (auto& stage : detector)
      stage.setBypass();

  if (!eqBandHasCompressionTarget(settings))
    return;

  const auto bandFrequency =
      settings.type == 1 && state.hasSurferFrequency ? state.surferFrequency : settings.frequency;
  const auto detectorQ = settings.type == 1
                             ? juce::jlimit(0.1f, 50.0f, settings.q * 0.58f)
                             : juce::jlimit(0.35f, 50.0f, settings.q * 0.48f);

  for (auto& detectorStages : state.compDetectorFilters)
  {
    switch (settings.type)
    {
      case 5: // Low Shelf
        setLowPassFilter(detectorStages[0], bandFrequency, 0.7071f);
        break;
      case 6: // High Shelf
        setHighPassFilter(detectorStages[0], bandFrequency, 0.7071f);
        break;
      case fullSpectrumType: // Full Spectrum
        setHighPassFilter(detectorStages[0], settings.rangeLowHz, 0.7071f);
        setLowPassFilter(detectorStages[1], settings.rangeHighHz, 0.7071f);
        break;
      case 0: // Bell
      case 1: // Surfer Bell
      case 8: // Band Pass
      default:
        setBandPassFilter(detectorStages[0], bandFrequency, detectorQ, 0.0f);
        break;
    }
  }
}

void VoxanovaAudioProcessor::configureEqBandSaturationFilter(EqBandState& state,
                                                             const EqBandSettings& settings) const
{
  for (auto& channelStages : state.saturationFilters)
    for (auto& stage : channelStages)
      stage.setBypass();

  if (!eqBandHasSaturation(settings))
  {
    state.saturationState = {};
    return;
  }

  const auto bandFrequency =
      settings.type == 1 && state.hasSurferFrequency ? state.surferFrequency : settings.frequency;
  const auto bandQ = settings.type == 1
                         ? juce::jlimit(0.1f, 50.0f, settings.q * 0.58f)
                         : juce::jlimit(0.35f, 50.0f, settings.q * 0.58f);

  for (auto& channelStages : state.saturationFilters)
  {
    switch (settings.type)
    {
      case 5: // Low Shelf
        setLowPassFilter(channelStages[0], bandFrequency, 0.7071f);
        break;
      case 6: // High Shelf
        setHighPassFilter(channelStages[0], bandFrequency, 0.7071f);
        break;
      case fullSpectrumType: // Full Spectrum
        setHighPassFilter(channelStages[0], settings.rangeLowHz, 0.7071f);
        setLowPassFilter(channelStages[1], settings.rangeHighHz, 0.7071f);
        break;
      case 0: // Bell
      case 1: // Surfer Bell
      case 8: // Band Pass
      default:
        setBandPassFilter(channelStages[0], bandFrequency, bandQ, 0.0f);
        break;
    }
  }
}

void VoxanovaAudioProcessor::configureEqBand(EqBandState& state, const EqBandSettings& settings) const
{
  const auto active = eqBandHasEffect(settings);
  const auto stageCount = eqBandFilterStageCount(settings);

  if (!active)
  {
    if (state.wasActive)
      state.reset();
    return;
  }

  const auto resetState = !state.wasActive || state.previousType != settings.type || state.previousStageCount != stageCount;
  if (resetState)
    state.reset();

  state.wasActive = true;
  state.previousType = settings.type;
  state.previousStageCount = stageCount;

  if (settings.type == 1)
    getSurferEqFrequency(state, settings);

  if (!state.compGainInitialized || !eqBandHasCompression(settings))
  {
    state.compGainDb = settings.gainDb;
    state.compGainInitialized = true;
  }

  configureEqBandCompressionDetector(state, settings);
  configureEqBandSaturationFilter(state, settings);
  configureEqBandForGain(state, settings, settings.gainDb);
  configureEqBandSolo(state, settings);
}

void VoxanovaAudioProcessor::prepareEq(std::vector<EqBandState>& states,
                                       const std::vector<EqBandSettings>& settings) const
{
  if (states.size() < settings.size())
  {
    const auto oldSize = states.size();
    states.resize(settings.size());
    for (auto index = oldSize; index < states.size(); ++index)
      states[index].reset();
  }

  for (auto index = 0u; index < settings.size(); ++index)
    configureEqBand(states[index], settings[index]);

  for (auto index = settings.size(); index < states.size(); ++index)
    if (states[index].wasActive)
      states[index].reset();
}

float VoxanovaAudioProcessor::updateEqBandDetectorLevel(EqBandState& state, const EqBandSettings& settings, float left,
                                                        float right) const
{
  if (!eqBandHasCompressionTarget(settings))
  {
    state.compDetectorDb = -120.0f;
    return state.compDetectorDb;
  }

  auto detectorLeft = left;
  auto detectorRight = right;
  for (auto& detector : state.compDetectorFilters[0])
    detectorLeft = detector.process(detectorLeft);
  for (auto& detector : state.compDetectorFilters[1])
    detectorRight = detector.process(detectorRight);
  const auto peakDetector = juce::jmax(std::abs(detectorLeft), std::abs(detectorRight));
  const auto rmsDetector = std::sqrt((detectorLeft * detectorLeft + detectorRight * detectorRight) * 0.5f);
  const auto detector = peakDetector * 0.58f + rmsDetector * 0.42f;
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto detectorAttackMs = juce::jlimit(0.1f, 200.0f, settings.compAttackMs * 0.72f);
  const auto detectorReleaseMs = juce::jlimit(5.0f, 1000.0f, settings.compReleaseMs);
  const auto detectorCoeff =
      detector > state.compEnvelope
          ? std::exp(-1.0f / (safeRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / (safeRate * (detectorReleaseMs / 1000.0f)));
  state.compEnvelope = detector + detectorCoeff * (state.compEnvelope - detector);

  const auto detectorDb = state.compEnvelope > 0.000001f
                              ? juce::jlimit(-120.0f, 24.0f,
                                             juce::Decibels::gainToDecibels(state.compEnvelope) +
                                                 eqDynamicDetectorCalibrationDb)
                              : -120.0f;
  state.compDetectorDb = detectorDb;

  return detectorDb;
}

float VoxanovaAudioProcessor::updateEqBandDynamicGain(EqBandState& state, const EqBandSettings& settings, float left,
                                                      float right) const
{
  if (!eqBandHasCompressionTarget(settings))
    return settings.gainDb;

  if (!state.compGainInitialized)
  {
    state.compGainDb = settings.gainDb;
    state.compGainInitialized = true;
  }

  const auto detectorDb = updateEqBandDetectorLevel(state, settings, left, right);

  if (!eqBandHasCompression(settings))
  {
    state.compGainDb = settings.gainDb;
    return settings.gainDb;
  }

  const auto overDb = juce::jmax(0.0f, detectorDb - settings.compThresholdDb);
  const auto kneeEngagement = thresholdEngagement(detectorDb, settings.compThresholdDb, 6.0f);
  const auto dynamicRangeDb = std::abs(settings.compDb - settings.gainDb);
  const auto ratio = juce::jlimit(1.0f, 20.0f, settings.compRatio);
  const auto ratioMoveDb = overDb * (1.0f - 1.0f / ratio);
  const auto ratioEngagement = dynamicRangeDb > 0.001f
                                   ? juce::jlimit(0.0f, 1.0f, ratioMoveDb / dynamicRangeDb)
                                   : 0.0f;
  const auto engagement = juce::jlimit(0.0f, 1.0f, kneeEngagement * ratioEngagement);
  const auto targetGainDb = juce::jlimit(-30.0f, 30.0f,
                                         settings.gainDb + (settings.compDb - settings.gainDb) * engagement);
  const auto currentDepth = std::abs(state.compGainDb - settings.gainDb);
  const auto targetDepth = std::abs(targetGainDb - settings.gainDb);
  const auto gainTimeMs = targetDepth > currentDepth ? settings.compAttackMs : settings.compReleaseMs;
  const auto safeRate = juce::jmax(1000.0f, static_cast<float>(currentSampleRate));
  const auto gainCoeff = std::exp(-1.0f / (safeRate * (juce::jmax(0.1f, gainTimeMs) / 1000.0f)));
  state.compGainDb = targetGainDb + gainCoeff * (state.compGainDb - targetGainDb);

  return juce::jlimit(-30.0f, 30.0f, state.compGainDb);
}

void VoxanovaAudioProcessor::applyEqBandSaturation(EqBandState& state, const EqBandSettings& settings, float& left,
                                                   float& right) const
{
  if (!eqBandHasSaturation(settings))
    return;

  auto bandLeft = left;
  auto bandRight = right;
  for (auto stage = 0; stage < 2; ++stage)
  {
    bandLeft = state.saturationFilters[0][static_cast<size_t>(stage)].process(bandLeft);
    bandRight = state.saturationFilters[1][static_cast<size_t>(stage)].process(bandRight);
  }

  const auto saturatedLeft =
      applySaturationModel(bandLeft, settings.saturationMode, settings.saturationAmount, state.saturationState, 0);
  const auto saturatedRight =
      applySaturationModel(bandRight, settings.saturationMode, settings.saturationAmount, state.saturationState, 1);

  left = juce::jlimit(-4.0f, 4.0f, left + saturatedLeft - bandLeft);
  right = juce::jlimit(-4.0f, 4.0f, right + saturatedRight - bandRight);
}

void VoxanovaAudioProcessor::applyEq(std::vector<EqBandState>& states, const std::vector<EqBandSettings>& settings,
                                     float& left, float& right,
                                     std::array<std::atomic<float>, eqMeterBandCount>& detectorDbMeters)
{
  const auto count = juce::jmin(states.size(), settings.size());
  const auto hasSolo = std::any_of(settings.begin(), settings.begin() + static_cast<std::ptrdiff_t>(count),
                                   [](const auto& band) { return band.enabled && band.solo; });

  if (hasSolo)
  {
    const auto sourceLeft = left;
    const auto sourceRight = right;
    auto soloLeft = 0.0f;
    auto soloRight = 0.0f;

    for (auto index = 0u; index < count; ++index)
    {
      const auto& band = settings[index];
      if (!band.enabled || !band.solo)
        continue;

      auto& state = states[index];
      auto bandLeft = sourceLeft;
      auto bandRight = sourceRight;
      auto effectiveGainDb = band.gainDb;

      if (eqBandHasCompressionTarget(band))
      {
        effectiveGainDb = updateEqBandDynamicGain(state, band, sourceLeft, sourceRight);
        configureEqBandForGain(state, band, effectiveGainDb);
        if (index < detectorDbMeters.size())
          detectorDbMeters[index].store(state.compDetectorDb, std::memory_order_relaxed);
      }

      const auto eqStageCount = eqBandFilterStageCount(band);
      for (auto stage = 0; stage < eqStageCount; ++stage)
      {
        bandLeft = state.filters[0][static_cast<size_t>(stage)].process(bandLeft);
        bandRight = state.filters[1][static_cast<size_t>(stage)].process(bandRight);
      }

      if (band.type == fullSpectrumType)
      {
        const auto bandGain = dbToGain(effectiveGainDb);
        auto processedLeft = bandLeft * bandGain;
        auto processedRight = bandRight * bandGain;
        if (eqBandHasSaturation(band))
        {
          processedLeft =
              applySaturationModel(processedLeft, band.saturationMode, band.saturationAmount, state.saturationState, 0);
          processedRight =
              applySaturationModel(processedRight, band.saturationMode, band.saturationAmount, state.saturationState, 1);
        }
        soloLeft += processedLeft;
        soloRight += processedRight;
        continue;
      }

      for (auto stage = 0; stage < 2; ++stage)
      {
        bandLeft = state.soloFilters[0][static_cast<size_t>(stage)].process(bandLeft);
        bandRight = state.soloFilters[1][static_cast<size_t>(stage)].process(bandRight);
      }

      if (eqBandHasSaturation(band))
      {
        bandLeft = applySaturationModel(bandLeft, band.saturationMode, band.saturationAmount, state.saturationState, 0);
        bandRight =
            applySaturationModel(bandRight, band.saturationMode, band.saturationAmount, state.saturationState, 1);
      }

      soloLeft += bandLeft;
      soloRight += bandRight;
    }

    left = juce::jlimit(-4.0f, 4.0f, soloLeft);
    right = juce::jlimit(-4.0f, 4.0f, soloRight);
    return;
  }

  for (auto index = 0u; index < count; ++index)
  {
    const auto& band = settings[index];
    if (!eqBandHasEffect(band))
      continue;

    auto& state = states[index];
    if (band.type == 2)
      continue;

    auto effectiveGainDb = band.gainDb;
    if (eqBandHasCompressionTarget(band))
    {
      effectiveGainDb = updateEqBandDynamicGain(state, band, left, right);
      configureEqBandForGain(state, band, effectiveGainDb);
      if (index < detectorDbMeters.size())
        detectorDbMeters[index].store(state.compDetectorDb, std::memory_order_relaxed);
    }

    if (band.type == fullSpectrumType)
    {
      auto bandLeft = left;
      auto bandRight = right;
      const auto stageCount = eqBandFilterStageCount(band);
      for (auto stage = 0; stage < stageCount; ++stage)
      {
        bandLeft = state.filters[0][static_cast<size_t>(stage)].process(bandLeft);
        bandRight = state.filters[1][static_cast<size_t>(stage)].process(bandRight);
      }

      const auto bandGain = dbToGain(effectiveGainDb);
      auto processedLeft = bandLeft * bandGain;
      auto processedRight = bandRight * bandGain;
      if (eqBandHasSaturation(band))
      {
        processedLeft =
            applySaturationModel(processedLeft, band.saturationMode, band.saturationAmount, state.saturationState, 0);
        processedRight =
            applySaturationModel(processedRight, band.saturationMode, band.saturationAmount, state.saturationState, 1);
      }

      left = juce::jlimit(-4.0f, 4.0f, left + processedLeft - bandLeft);
      right = juce::jlimit(-4.0f, 4.0f, right + processedRight - bandRight);
      continue;
    }

    const auto stageCount = eqBandFilterStageCount(band);
    for (auto stage = 0; stage < stageCount; ++stage)
    {
      left = state.filters[0][static_cast<size_t>(stage)].process(left);
      right = state.filters[1][static_cast<size_t>(stage)].process(right);
    }

    applyEqBandSaturation(state, band, left, right);
  }
}

void VoxanovaAudioProcessor::applyEqDeEsser(EqBandState& state, const EqBandSettings& settings, float& left,
                                            float& right)
{
  const auto amount = juce::jlimit(0.0f, 1.0f, settings.intensity / 100.0f);
  if (amount <= 0.0f)
    return;

  const auto splitHz =
      juce::jlimit(1800.0f, static_cast<float>(currentSampleRate) * 0.42f,
                   settings.frequency * (settings.deessMode == 1 ? 0.68f : 1.0f));
  const auto detectorAttackMs = settings.deessMode == 1 ? 0.75f : 0.35f;
  const auto detectorReleaseMs = settings.deessMode == 1 ? 72.0f : 42.0f;
  const auto gainAttackMs = settings.deessMode == 1 ? 1.4f : 0.65f;
  const auto gainReleaseMs = settings.deessMode == 1 ? 96.0f : 58.0f;

  const auto lowLeft = processOnePoleLowpass(left, splitHz, state.deEsserLowStates[0]);
  const auto lowRight = processOnePoleLowpass(right, splitHz, state.deEsserLowStates[1]);
  const auto highLeft = left - lowLeft;
  const auto highRight = right - lowRight;
  const auto peakDetector = juce::jmax(std::abs(highLeft), std::abs(highRight));
  const auto rmsDetector = std::sqrt((highLeft * highLeft + highRight * highRight) * 0.5f);
  const auto detector = peakDetector * 0.62f + rmsDetector * 0.38f;

  const auto detectorCoeff =
      detector > state.deEsserEnvelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  state.deEsserEnvelope = detector + detectorCoeff * (state.deEsserEnvelope - detector);

  const auto detectorDb = state.deEsserEnvelope > 0.000001f
                              ? juce::Decibels::gainToDecibels(state.deEsserEnvelope)
                              : -120.0f;
  const auto overDb = juce::jmax(0.0f, detectorDb - settings.thresholdDb);
  const auto maxReductionDb = 1.5f + amount * (settings.deessMode == 1 ? 12.0f : 18.0f);
  const auto targetReductionDb = juce::jlimit(0.0f, maxReductionDb, overDb * (0.32f + amount * 0.62f));
  const auto targetGain = dbToGain(-targetReductionDb);
  const auto gainCoeff =
      targetGain < state.deEsserGain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainReleaseMs / 1000.0f)));
  state.deEsserGain = targetGain + gainCoeff * (state.deEsserGain - targetGain);

  left = lowLeft + highLeft * state.deEsserGain;
  right = lowRight + highRight * state.deEsserGain;
}

void VoxanovaAudioProcessor::resetEqStates(std::vector<EqBandState>& states)
{
  for (auto& state : states)
    state.reset();
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

void VoxanovaAudioProcessor::applyDeEsser(float& left, float& right, float amountPercent, float lowHz, float highHz)
{
  const auto amount = juce::jlimit(0.0f, 1.0f, amountPercent / 100.0f);
  if (amount <= 0.0f)
    return;

  const auto safeLowHz = juce::jlimit(1800.0f, static_cast<float>(currentSampleRate) * 0.38f, lowHz);
  const auto safeHighHz = juce::jlimit(safeLowHz + 250.0f, static_cast<float>(currentSampleRate) * 0.46f, highHz);
  constexpr auto detectorAttackMs = 0.22f;
  constexpr auto detectorReleaseMs = 64.0f;
  constexpr auto gainAttackMs = 0.38f;
  constexpr auto gainReleaseMs = 86.0f;

  const auto lowLeft = processOnePoleLowpass(left, safeLowHz, deEsserLowStates[0]);
  const auto lowRight = processOnePoleLowpass(right, safeLowHz, deEsserLowStates[1]);
  const auto highSplitLeft = processOnePoleLowpass(left, safeHighHz, deEsserHighStates[0]);
  const auto highSplitRight = processOnePoleLowpass(right, safeHighHz, deEsserHighStates[1]);
  const auto bandLeft = highSplitLeft - lowLeft;
  const auto bandRight = highSplitRight - lowRight;
  const auto peakDetector = juce::jmax(std::abs(bandLeft), std::abs(bandRight));
  const auto rmsDetector = std::sqrt((bandLeft * bandLeft + bandRight * bandRight) * 0.5f);
  const auto detector = peakDetector * 0.58f + rmsDetector * 0.42f;

  const auto detectorCoeff =
      detector > deEsserEnvelope
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (detectorReleaseMs / 1000.0f)));
  deEsserEnvelope = detector + detectorCoeff * (deEsserEnvelope - detector);

  const auto amountCurve = std::pow(amount, 0.62f);
  const auto detectorBoost = 2.8f + amountCurve * 7.2f;
  const auto detectorDb =
      deEsserEnvelope > 0.000001f ? juce::Decibels::gainToDecibels(deEsserEnvelope * detectorBoost) : -120.0f;
  const auto thresholdDb = -30.0f - amountCurve * 38.0f;
  const auto maxReductionDb = 10.0f + amountCurve * 50.0f;
  const auto overDb = juce::jmax(0.0f, detectorDb - thresholdDb);
  const auto targetReductionDb = juce::jlimit(0.0f, maxReductionDb, overDb * (0.82f + amountCurve * 2.25f));
  const auto targetGain = dbToGain(-targetReductionDb);
  const auto gainCoeff =
      targetGain < deEsserGain
          ? std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainAttackMs / 1000.0f)))
          : std::exp(-1.0f / static_cast<float>(currentSampleRate * (gainReleaseMs / 1000.0f)));
  deEsserGain = targetGain + gainCoeff * (deEsserGain - targetGain);

  const auto forcedReduction = amountCurve * amountCurve * 0.72f;
  const auto dynamicReduction = (1.0f - deEsserGain) * (1.0f + amountCurve * 1.9f);
  const auto reductionMix = juce::jlimit(0.0f, 1.18f, forcedReduction + dynamicReduction);
  left -= bandLeft * reductionMix;
  right -= bandRight * reductionMix;
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
