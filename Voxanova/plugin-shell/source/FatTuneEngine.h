#pragma once

#include <array>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>

class FatTuneEngine final
{
public:
  struct Settings
  {
    bool enabled = false;
    float amount = 0.0f;
    int key = 0;
    int scale = 0;
    int customMask = 0;
  };

  struct Result
  {
    float left = 0.0f;
    float right = 0.0f;
  };

  struct Meters
  {
    float frequency = 0.0f;
    float cents = 0.0f;
    float confidence = 0.0f;
    float targetMidi = 0.0f;
  };

  static float retuneMillisecondsForAmount(float amount);

  void prepare(double sampleRate, int samplesPerBlock);
  void reset();
  Result processSample(float left, float right, const Settings& settings, bool forcePitchTracking);

  int getLatencySamples() const { return latencySamples; }
  float getDetectedFrequency() const { return detectedFrequency; }
  float getDetectedClarity() const { return detectedClarity; }
  Meters getMeters() const { return meters; }

private:
  static constexpr int numChannels = 2;
  static constexpr int maxScaleCount = 39;

  void configureCorrection(const Settings& settings);
  void pushDetectorSample(float sample);
  void analysePitch(int noteMask);
  void updateCorrection(int noteMask);
  void updateJumpState(int noteMask);
  Result readShiftedSample(bool correctionActive, int noteMask);
  Result readDelayOnly() const;
  float findBestSpliceIndex(float referenceIndex, float targetIndex, float searchRadius) const;
  float readCubic(int channel, float index) const;
  float readCubicWithAhead(int channel, float index, int ahead) const;
  float readCubicResidual(int channel, float index) const;
  float readCubicResidualWithAhead(int channel, float index, int ahead) const;
  float readDelaySample(int channel, float delay) const;
  void writeResidualSample();
  void maybeUpdateLpc();
  void updateLpcCoefficients();
  static bool levinsonDurbin(const float* r, int order, float* a, float& gain);
  float processOnePoleLowpass(float input, float cutoffHz, float& state) const;
  int makeScaleMask(int key, int scale, int customMask) const;
  float findNearestScaleMidi(float midi, int key, int scale, int customMask) const;
  static float wrapIndex(float index, int size);
  static float wrapDistance(float writeIndex, float readIndex, int size);
  static float cubic(const float* samples, float amount);

  double sampleRate = 48000.0;
  int bufferSize = 4096;
  int fragmentSize = 128;
  int latencySamples = 1024;

  std::array<std::vector<float>, numChannels> inputBuffer;
  std::array<std::vector<float>, numChannels> residualBuffer;
  int writeIndex = 0;
  int filledSamples = 0;

  // LPC source-filter formant preservation. The shifter resamples the residual
  // (whitened input) and then re-imposes the current spectral envelope via the
  // synthesis filter, so formants do not move when pitch shifts.
  static constexpr int lpcOrder = 14;
  std::array<float, lpcOrder + 1> lpcCoeffs {};
  std::array<float, lpcOrder + 1> lpcCoeffsPrev {};
  int lpcCrossfadeRemaining = 0;
  int lpcCrossfadeLength = 64;
  int lpcUpdateInterval = 96;
  int lpcSampleCounter = 0;
  int lpcAnalysisLength = 768;
  int lpcAnalysisCapacity = 0;
  std::array<std::array<float, lpcOrder>, numChannels> lpcSynthHistory {};
  std::vector<float> lpcAnalysisWindow;
  std::vector<float> lpcLagWindow;
  std::vector<float> lpcAnalysisScratch;
  bool lpcReady = false;

  float readIndex1 = 0.0f;
  float readIndex2 = 0.0f;
  int fragmentIndex = 0;
  int fragmentCounter = 0;
  bool crossfading = false;
  int readAhead = 0;
  int previousReadAhead = 0;
  bool fastMode = false;

  float cycleSamples = 128.0f;
  float pitchErrorOctaves = 0.0f;
  float pitchRatio = 1.0f;
  float pitchRatioTarget = 1.0f;
  float pitchRatioRampStep = 0.0f;
  int pitchRatioRampRemaining = 0;
  float pitchRatioOctaves = 0.0f;
  float pitchRatioFollow = 0.42f;
  float correctionFilter = 0.12f;
  float correctionGain = 1.0f;
  float correctionOvershoot = 1.0f;
  float noteSwitchMarginOctaves = 0.018f;
  float noteReleaseOctaves = 0.060f;
  float noteTransitionFilter = 0.38f;
  float noteSelectMidi = 0.0f;
  float noteSelectFollow = 0.08f;
  bool hasNoteSelectMidi = false;
  float correctionMidi = 0.0f;
  float correctionMidiFollowFast = 0.26f;
  float correctionMidiFollowSlow = 0.16f;
  bool hasCorrectionMidi = false;
  float tuneLockStrength = 0.0f;
  float centerLockGain = 1.0f;
  float tonalCorrectionFloor = 0.35f;
  float pitchTrendSemitones = 0.0f;
  float detectorFollowFast = 0.45f;
  float detectorFollowSlow = 0.28f;
  bool correctionConfigValid = false;
  bool configuredEnabled = false;
  float configuredAmount = -1.0f;
  int configuredKey = -1;
  int configuredScale = -1;
  int configuredCustomMask = -1;
  int configuredFragmentSize = 0;
  double configuredSampleRate = 0.0;
  int configuredNoteMask = 0;
  int analysisIntervalFragments = 4;
  int noteHoldAnalyses = 1;
  int noteReleaseAnalyses = 2;
  int noteTransitionFramesRemaining = 0;
  int stableNoteAnalyses = 0;
  int targetLatchAnalyses = 0;
  int onsetGuardAnalyses = 0;
  int pitchDirection = 0;
  int directionalRun = 0;
  int lastNote = -1;
  int pendingNote = -1;
  int pendingNoteCount = 0;
  int detectedNoteBits = 0;
  int unvoicedCounter = 5;
  bool voiced = false;
  float shiftedBlend = 0.0f;
  float shiftedBlendTarget = 0.0f;
  float shiftedBlendAttackAlpha = 1.0f;
  float shiftedBlendReleaseAlpha = 1.0f;
  float highPreserveMix = 0.35f;
  float highPreserveTarget = 0.35f;
  float highPreserveAlpha = 1.0f;
  float highGuardLowpassAlpha = 1.0f;
  std::array<float, numChannels> shiftedHighGuardLowpass {};
  std::array<float, numChannels> delayHighGuardLowpass {};

  std::vector<float> crossfade;

  static constexpr int detectorDecimation = 4;
  int detectorSize = 2048;
  std::vector<float> detectorBuffer;
  std::vector<float> detectorScratch;
  std::vector<float> pitchCmndf;
  int detectorWriteIndex = 0;
  int detectorDecimationCounter = 0;
  int analysisSampleCounter = 0;
  float detectorLowpass = 0.0f;
  float detectorLowpassAlpha = 1.0f;
  float detectedFrequency = 0.0f;
  float detectedClarity = 0.0f;
  float smoothedMidi = 0.0f;
  bool hasSmoothedMidi = false;

  Meters meters;
};
