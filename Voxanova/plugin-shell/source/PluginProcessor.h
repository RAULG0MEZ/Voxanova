#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

class VoxanovaAudioProcessor final : public juce::AudioProcessor
{
public:
  using APVTS = juce::AudioProcessorValueTreeState;

  struct MeterSnapshot
  {
    std::array<float, 2> input {};
    std::array<float, 2> output {};
    float peakLevel = 0.0f;
    float glueLevel = 0.0f;
    float faceLevel = 0.0f;
    float gateLevel = 0.0f;
    float gateReduction = 0.0f;
    float peakReduction = 0.0f;
    float glueReduction = 0.0f;
    float faceReduction = 0.0f;
    float gateReductionDb = 0.0f;
    float peakReductionDb = 0.0f;
    float glueReductionDb = 0.0f;
    float faceReductionDb = 0.0f;
    float hostBpm = 120.0f;
    int inputChannels = 2;
    int outputChannels = 2;
  };

  VoxanovaAudioProcessor();
  ~VoxanovaAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;
  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  APVTS parameters;

  static APVTS::ParameterLayout createParameterLayout();
  MeterSnapshot getMeterSnapshot() const;

private:
  struct CompressorResult
  {
    float left = 0.0f;
    float right = 0.0f;
    float reduction = 0.0f;
    float reductionDb = 0.0f;
    float detectorLevel = 0.0f;
  };

  struct CompressorState
  {
    float envelope = 0.0f;
    float gain = 1.0f;
    float limiterGain = 1.0f;
    float engagement = 0.0f;
    int holdSamples = 0;
  };

  struct BandSplitState
  {
    float low100 = 0.0f;
    float low1000 = 0.0f;
    float low10000 = 0.0f;
  };

  struct SaturationState
  {
    std::array<float, 2> lowTone {};
    std::array<float, 2> highTone {};
    std::array<float, 2> dcBlock {};
  };

  struct GateResult
  {
    float left = 0.0f;
    float right = 0.0f;
    float reduction = 0.0f;
    float reductionDb = 0.0f;
    float detectorLevel = 0.0f;
  };

  static float dbToGain(float db);
  static float peakToMeter(float peak);
  static float peakToFader(float peak, float minDb, float maxDb);
  static void updateAtomicPeak(std::atomic<float>& target, float value);
  static void updateAtomicBallistic(std::atomic<float>& target, float value, double sampleRate, int numSamples,
                                    float attackMs, float releaseMs);
  void clearMeters();
  float processOnePoleLowpass(float input, float cutoffHz, float& state) const;
  GateResult applyVocalGate(float left, float right, float detectorLeft, float detectorRight, float thresholdDb);
  CompressorResult applyPeakTamer(float left, float right, float thresholdDb, CompressorState& state) const;
  CompressorResult applyGlueCompressor(float left, float right, float thresholdDb, CompressorState& state,
                                       bool multiband, int bandIndex) const;
  CompressorResult applyInYourFaceCompressor(float left, float right, float mixPercent, CompressorState& state) const;
  CompressorResult applyCompressor(float left, float right, float thresholdDb, float ratio, float amountPercent,
                                   float attackMs, float releaseMs, float kneeDb, CompressorState& state) const;
  float applySaturationModel(float sample, int mode, float amountPercent, SaturationState& state, int channel) const;
  static float applySoftClip(float sample, float drive);
  static float applyUnitySaturation(float sample, float drive, float mix);

  std::atomic<float>* inputGainParam = nullptr;
  std::atomic<float>* outputGainParam = nullptr;
  std::atomic<float>* gateParam = nullptr;
  std::atomic<float>* stereoWidthParam = nullptr;
  std::atomic<float>* stereoLowBypassParam = nullptr;
  std::atomic<float>* preSaturationModeParam = nullptr;
  std::atomic<float>* preSaturationAmountParam = nullptr;
  std::atomic<float>* postSaturationModeParam = nullptr;
  std::atomic<float>* postSaturationAmountParam = nullptr;
  std::atomic<float>* peakEnabledParam = nullptr;
  std::atomic<float>* peakThresholdParam = nullptr;
  std::atomic<float>* glueEnabledParam = nullptr;
  std::atomic<float>* glueMultibandParam = nullptr;
  std::atomic<float>* glueThresholdParam = nullptr;
  std::atomic<float>* glueLowThresholdParam = nullptr;
  std::atomic<float>* glueLowMidThresholdParam = nullptr;
  std::atomic<float>* glueHighMidThresholdParam = nullptr;
  std::atomic<float>* glueAirThresholdParam = nullptr;
  std::atomic<float>* faceEnabledParam = nullptr;
  std::atomic<float>* faceThresholdParam = nullptr;
  std::atomic<float>* gateEnabledParam = nullptr;
  std::atomic<float>* stereoEnabledParam = nullptr;
  std::atomic<float>* reverbEnabledParam = nullptr;
  std::atomic<float>* reverbMixParam = nullptr;
  std::atomic<float>* reverbDecayParam = nullptr;
  std::atomic<float>* reverbSizeParam = nullptr;
  std::atomic<float>* reverbPredelayParam = nullptr;
  std::atomic<float>* reverbLowCutParam = nullptr;
  std::atomic<float>* reverbHighCutParam = nullptr;
  std::atomic<float>* reverbModeParam = nullptr;
  std::atomic<float>* reverbSyncParam = nullptr;
  std::atomic<float>* reverbNoteModeParam = nullptr;
  std::atomic<float>* reverbDecaySyncParam = nullptr;
  std::atomic<float>* reverbPredelaySyncParam = nullptr;
  std::atomic<float>* reverbDecayDivisionParam = nullptr;
  std::atomic<float>* reverbPredelayDivisionParam = nullptr;
  std::atomic<float>* delayEnabledParam = nullptr;
  std::atomic<float>* delayMixParam = nullptr;
  std::atomic<float>* delayFeedbackParam = nullptr;
  std::atomic<float>* delayLowCutParam = nullptr;
  std::atomic<float>* delayHighCutParam = nullptr;
  std::atomic<float>* delaySyncParam = nullptr;
  std::atomic<float>* delayDivisionParam = nullptr;
  std::atomic<float>* delayNoteModeParam = nullptr;
  std::atomic<float>* delayTimeMsParam = nullptr;
  std::atomic<float>* delayModeParam = nullptr;
  std::atomic<float>* delayPostReverbParam = nullptr;
  std::atomic<float>* delayStyleParam = nullptr;

  std::array<juce::AudioBuffer<float>, 2> delayBuffers;
  std::array<int, 2> delayWritePositions {};
  std::array<std::array<float, 4>, 2> delayLowCutStates {};
  std::array<std::array<float, 4>, 2> delayHighCutStates {};
  std::array<std::array<float, 2>, 2> delayStyleLowCutStates {};
  std::array<std::array<float, 2>, 2> delayStyleHighCutStates {};
  juce::AudioBuffer<float> reverbPredelayBuffer;
  int reverbPredelayWritePosition = 0;
  juce::AudioBuffer<float> reverbEarlyBuffer;
  int reverbEarlyWritePosition = 0;
  std::array<juce::AudioBuffer<float>, 8> reverbTankBuffers;
  std::array<int, 8> reverbTankWritePositions {};
  std::array<float, 8> reverbTankHighDampStates {};
  std::array<float, 8> reverbTankLowDampStates {};
  std::array<float, 8> reverbTankModPhases {};
  std::array<juce::AudioBuffer<float>, 8> reverbDiffuserBuffers;
  std::array<int, 8> reverbDiffuserWritePositions {};
  std::array<float, 8> reverbDiffuserModPhases {};
  std::array<std::array<float, 4>, 2> reverbLowCutStates {};
  std::array<std::array<float, 4>, 2> reverbHighCutStates {};
  std::array<std::array<float, 2>, 2> reverbModeLowCutStates {};
  std::array<std::array<float, 2>, 2> reverbModeHighCutStates {};
  std::array<float, 2> reverbWarmLowStates {};
  std::array<float, 2> reverbWarmHighStates {};
  std::array<float, 2> reverbSilkStates {};
  float reverbSizeSmoothed = 0.68f;
  float reverbDecaySmoothed = 4.0f;
  int lastReverbMode = -1;
  juce::AudioBuffer<float> monoWidenBuffer;
  int monoWidenWritePosition = 0;
  std::array<float, 4> monoWidenAllpassLeft {};
  std::array<float, 4> monoWidenAllpassRight {};
  float monoWidenSideLowpass = 0.0f;
  float gateEnvelope = 0.0f;
  float gateSmoothedGain = 1.0f;
  int gateHoldSamples = 0;
  CompressorState preCompressorState;
  SaturationState preSaturationState;
  CompressorState peakCompressorState;
  CompressorState glueCompressorState;
  std::array<CompressorState, 4> glueBandCompressorStates {};
  std::array<BandSplitState, 2> glueBandSplitStates {};
  CompressorState faceCompressorState;
  CompressorState postCompressorState;
  SaturationState postSaturationState;
  std::array<std::atomic<float>, 2> inputMeterPeaks {};
  std::array<std::atomic<float>, 2> outputMeterPeaks {};
  std::atomic<float> peakLevelMeter { 0.0f };
  std::atomic<float> glueLevelMeter { 0.0f };
  std::atomic<float> faceLevelMeter { 0.0f };
  std::atomic<float> gateLevelMeter { 0.0f };
  std::atomic<float> gateReductionMeter { 0.0f };
  std::atomic<float> peakReductionMeter { 0.0f };
  std::atomic<float> glueReductionMeter { 0.0f };
  std::atomic<float> faceReductionMeter { 0.0f };
  std::atomic<float> gateReductionDbMeter { 0.0f };
  std::atomic<float> peakReductionDbMeter { 0.0f };
  std::atomic<float> glueReductionDbMeter { 0.0f };
  std::atomic<float> faceReductionDbMeter { 0.0f };
  std::atomic<int> activeInputChannels { 2 };
  std::atomic<int> activeOutputChannels { 2 };
  std::atomic<float> hostBpm { 120.0f };

  double currentSampleRate = 44100.0;
};
