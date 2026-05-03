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
  float readCubic(int channel, float index) const;
  float readCubicWithAhead(int channel, float index, int ahead) const;
  float readDelaySample(int channel, float delay) const;
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
  int writeIndex = 0;
  int filledSamples = 0;

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
  float correctionFilter = 0.12f;
  float correctionGain = 1.0f;
  float correctionOvershoot = 1.0f;
  float noteBias = 0.038f;
  float detectorFollowFast = 0.45f;
  float detectorFollowSlow = 0.28f;
  int analysisIntervalFragments = 4;
  int lastNote = -1;
  int detectedNoteBits = 0;
  int unvoicedCounter = 5;
  bool voiced = false;

  std::vector<float> crossfade;

  static constexpr int detectorDecimation = 2;
  int detectorSize = 2048;
  std::vector<float> detectorBuffer;
  std::vector<float> detectorScratch;
  int detectorWriteIndex = 0;
  int detectorDecimationCounter = 0;
  int analysisSampleCounter = 0;
  float detectorLowpass = 0.0f;
  float detectedFrequency = 0.0f;
  float detectedClarity = 0.0f;
  float smoothedMidi = 0.0f;
  bool hasSmoothedMidi = false;

  Meters meters;
};
