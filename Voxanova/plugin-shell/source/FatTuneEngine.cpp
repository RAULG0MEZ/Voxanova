#include "FatTuneEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int pitchClassBit(int pitchClass)
{
  return 1 << pitchClass;
}

float foldMidiToReference(float midi, float reference)
{
  auto foldedMidi = midi;
  auto foldedDistance = std::abs(foldedMidi - reference);
  for (auto octave = -2; octave <= 2; ++octave)
  {
    const auto candidate = midi + static_cast<float>(octave * 12);
    const auto distance = std::abs(candidate - reference);
    if (distance < foldedDistance)
    {
      foldedDistance = distance;
      foldedMidi = candidate;
    }
  }

  return foldedMidi;
}

constexpr std::array<int, 39> scaleMasks {
  0,
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(4) |
      pitchClassBit(5) | pitchClassBit(6) | pitchClassBit(7) | pitchClassBit(8) | pitchClassBit(9) |
      pitchClassBit(10) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(6) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(6) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(7) | pitchClassBit(9),
  pitchClassBit(0) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(6) | pitchClassBit(7) |
      pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(7) |
      pitchClassBit(9),
  pitchClassBit(0) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(6) | pitchClassBit(7) |
      pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(10) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(5) |
      pitchClassBit(7) | pitchClassBit(9) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(6) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(6) | pitchClassBit(8) |
      pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(6) |
      pitchClassBit(8) | pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(6) |
      pitchClassBit(7) | pitchClassBit(9) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(7) | pitchClassBit(8) |
      pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(6) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(8) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(5) | pitchClassBit(7) |
      pitchClassBit(9) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(4) | pitchClassBit(5) |
      pitchClassBit(6) | pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(6) |
      pitchClassBit(8) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(4) | pitchClassBit(5) | pitchClassBit(6) |
      pitchClassBit(8) | pitchClassBit(11),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(5) | pitchClassBit(7) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(7) | pitchClassBit(8),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(5) | pitchClassBit(7) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(5) | pitchClassBit(6) | pitchClassBit(10),
  pitchClassBit(0) | pitchClassBit(2) | pitchClassBit(3) | pitchClassBit(7) | pitchClassBit(9),
  pitchClassBit(0) | pitchClassBit(1) | pitchClassBit(3) | pitchClassBit(7) | pitchClassBit(8)
};
} // namespace

float FatTuneEngine::retuneMillisecondsForAmount(float amount)
{
  const auto clampedAmount = juce::jlimit(0.0f, 1.0f, amount);
  constexpr auto fastestRetuneMs = 0.0f;
  constexpr auto slowestRetuneMs = 400.0f;
  return fastestRetuneMs + std::pow(1.0f - clampedAmount, 2.65f) * (slowestRetuneMs - fastestRetuneMs);
}

void FatTuneEngine::prepare(double nextSampleRate, int samplesPerBlock)
{
  sampleRate = nextSampleRate > 1000.0 ? nextSampleRate : 48000.0;
  juce::ignoreUnused(samplesPerBlock);

  if (sampleRate < 64000.0)
  {
    bufferSize = 4096;
    fragmentSize = 128;
    latencySamples = 1024;
    detectorSize = 1024;
  }
  else if (sampleRate < 128000.0)
  {
    bufferSize = 8192;
    fragmentSize = 256;
    latencySamples = 2048;
    detectorSize = 2048;
  }
  else
  {
    bufferSize = 16384;
    fragmentSize = 512;
    latencySamples = 4096;
    detectorSize = 4096;
  }

  for (auto& channelBuffer : inputBuffer)
    channelBuffer.assign(static_cast<size_t>(bufferSize + 4), 0.0f);
  for (auto& channelBuffer : residualBuffer)
    channelBuffer.assign(static_cast<size_t>(bufferSize + 4), 0.0f);

  // LPC analysis window (Hann) and lag window (Gaussian, ~30 Hz bandwidth widening).
  lpcAnalysisLength = juce::jlimit(384, 1536, juce::roundToInt(sampleRate * 0.020));
  lpcAnalysisCapacity = lpcAnalysisLength;
  lpcAnalysisWindow.assign(static_cast<size_t>(lpcAnalysisLength), 0.0f);
  for (auto i = 0; i < lpcAnalysisLength; ++i)
    lpcAnalysisWindow[static_cast<size_t>(i)] =
        0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * static_cast<float>(i) /
                               static_cast<float>(juce::jmax(1, lpcAnalysisLength - 1)));
  lpcLagWindow.assign(static_cast<size_t>(lpcOrder + 1), 0.0f);
  {
    constexpr auto bandwidthHz = 30.0f;
    for (auto k = 0; k <= lpcOrder; ++k)
    {
      const auto v = juce::MathConstants<float>::twoPi * static_cast<float>(k) * bandwidthHz /
                     static_cast<float>(sampleRate);
      lpcLagWindow[static_cast<size_t>(k)] = std::exp(-0.5f * v * v);
    }
  }
  lpcAnalysisScratch.assign(static_cast<size_t>(lpcAnalysisLength), 0.0f);
  lpcUpdateInterval = juce::jlimit(32, 256, juce::roundToInt(sampleRate * 0.0025));
  lpcCrossfadeLength = juce::jlimit(16, 192, juce::roundToInt(sampleRate * 0.0015));

  crossfade.assign(static_cast<size_t>(fragmentSize), 0.0f);
  for (auto i = 0; i < fragmentSize; ++i)
    crossfade[static_cast<size_t>(i)] =
        0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * static_cast<float>(i) /
                                static_cast<float>(juce::jmax(1, fragmentSize))));

  detectorBuffer.assign(static_cast<size_t>(detectorSize), 0.0f);
  detectorScratch.assign(static_cast<size_t>(detectorSize), 0.0f);
  pitchCmndf.assign(static_cast<size_t>(detectorSize), 1.0f);
  detectorLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 1800.0f / static_cast<float>(sampleRate));
  shiftedBlendAttackAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0060 * sampleRate));
  shiftedBlendReleaseAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0025 * sampleRate));
  highPreserveAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0040 * sampleRate));
  highGuardLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 4700.0f / static_cast<float>(sampleRate));
  reset();
}

void FatTuneEngine::reset()
{
  for (auto& channelBuffer : inputBuffer)
    std::fill(channelBuffer.begin(), channelBuffer.end(), 0.0f);
  for (auto& channelBuffer : residualBuffer)
    std::fill(channelBuffer.begin(), channelBuffer.end(), 0.0f);
  for (auto& history : lpcSynthHistory)
    history.fill(0.0f);
  lpcCoeffs.fill(0.0f);
  lpcCoeffs[0] = 1.0f;
  lpcCoeffsPrev = lpcCoeffs;
  lpcCrossfadeRemaining = 0;
  lpcSampleCounter = 0;
  lpcReady = false;
  std::fill(detectorBuffer.begin(), detectorBuffer.end(), 0.0f);
  std::fill(detectorScratch.begin(), detectorScratch.end(), 0.0f);
  std::fill(pitchCmndf.begin(), pitchCmndf.end(), 1.0f);

  writeIndex = 0;
  filledSamples = 0;
  readIndex1 = wrapIndex(static_cast<float>(bufferSize - latencySamples), bufferSize);
  readIndex2 = readIndex1;
  fragmentIndex = 0;
  fragmentCounter = 0;
  crossfading = false;
  readAhead = 0;
  previousReadAhead = 0;
  fastMode = false;

  cycleSamples = static_cast<float>(fragmentSize);
  pitchErrorOctaves = 0.0f;
  pitchRatio = 1.0f;
  pitchRatioTarget = 1.0f;
  pitchRatioRampStep = 0.0f;
  pitchRatioRampRemaining = 0;
  pitchRatioOctaves = 0.0f;
  pitchRatioFollow = 0.42f;
  correctionFilter = 0.12f;
  correctionGain = 1.0f;
  correctionOvershoot = 1.0f;
  noteSwitchMarginOctaves = 0.018f;
  noteReleaseOctaves = 0.060f;
  noteTransitionFilter = 0.38f;
  noteSelectMidi = 0.0f;
  noteSelectFollow = 0.08f;
  hasNoteSelectMidi = false;
  correctionMidi = 0.0f;
  correctionMidiFollowFast = 0.26f;
  correctionMidiFollowSlow = 0.16f;
  hasCorrectionMidi = false;
  tuneLockStrength = 0.0f;
  centerLockGain = 1.0f;
  tonalCorrectionFloor = 0.35f;
  pitchTrendSemitones = 0.0f;
  detectorFollowFast = 0.45f;
  detectorFollowSlow = 0.28f;
  correctionConfigValid = false;
  configuredEnabled = false;
  configuredAmount = -1.0f;
  configuredKey = -1;
  configuredScale = -1;
  configuredCustomMask = -1;
  configuredFragmentSize = 0;
  configuredSampleRate = 0.0;
  configuredNoteMask = 0;
  analysisIntervalFragments = 4;
  noteHoldAnalyses = 1;
  noteReleaseAnalyses = 2;
  noteTransitionFramesRemaining = 0;
  stableNoteAnalyses = 0;
  targetLatchAnalyses = 0;
  onsetGuardAnalyses = 0;
  pitchDirection = 0;
  directionalRun = 0;
  lastNote = -1;
  pendingNote = -1;
  pendingNoteCount = 0;
  detectedNoteBits = 0;
  unvoicedCounter = 5;
  voiced = false;
  shiftedBlend = 0.0f;
  shiftedBlendTarget = 0.0f;
  highPreserveMix = 0.35f;
  highPreserveTarget = 0.35f;
  shiftedHighGuardLowpass.fill(0.0f);
  delayHighGuardLowpass.fill(0.0f);

  detectorWriteIndex = 0;
  detectorDecimationCounter = 0;
  analysisSampleCounter = 0;
  detectorLowpass = 0.0f;
  detectedFrequency = 0.0f;
  detectedClarity = 0.0f;
  smoothedMidi = 0.0f;
  hasSmoothedMidi = false;
  meters = {};
}

FatTuneEngine::Result FatTuneEngine::processSample(float left, float right, const Settings& settings,
                                                   bool forcePitchTracking)
{
  const auto mono = (left + right) * 0.5f;
  configureCorrection(settings);
  const auto noteMask = configuredNoteMask;
  const auto correctionActive = settings.enabled && noteMask != 0;

  inputBuffer[0][static_cast<size_t>(writeIndex)] = left;
  inputBuffer[1][static_cast<size_t>(writeIndex)] = right;

  for (auto channel = 0; channel < numChannels; ++channel)
  {
    const auto base = inputBuffer[static_cast<size_t>(channel)].data();
    base[bufferSize] = base[0];
    base[bufferSize + 1] = base[1];
    base[bufferSize + 2] = base[2];
    base[bufferSize + 3] = base[3];
  }

  writeResidualSample();
  maybeUpdateLpc();

  const auto trackingActive = correctionActive || forcePitchTracking;
  if (trackingActive)
  {
    pushDetectorSample(mono);
    if (!correctionActive && ++analysisSampleCounter >= fragmentSize * 4)
    {
      analysisSampleCounter = 0;
      analysePitch(noteMask);
    }
  }
  else
  {
    analysisSampleCounter = 0;
    meters = {};
  }

  const auto result = correctionActive ? readShiftedSample(true, noteMask) : readDelayOnly();

  writeIndex = (writeIndex + 1) % bufferSize;
  filledSamples = juce::jmin(bufferSize, filledSamples + 1);

  return result;
}

void FatTuneEngine::configureCorrection(const Settings& settings)
{
  const auto clampedAmount = juce::jlimit(0.0f, 100.0f, settings.amount);
  const auto clampedKey = juce::jlimit(0, 11, settings.key);
  const auto clampedScale = juce::jlimit(0, maxScaleCount - 1, settings.scale);
  const auto clampedCustomMask = settings.customMask & 0x0fff;

  if (correctionConfigValid && configuredEnabled == settings.enabled &&
      std::abs(configuredAmount - clampedAmount) < 0.001f && configuredKey == clampedKey &&
      configuredScale == clampedScale && configuredCustomMask == clampedCustomMask &&
      configuredFragmentSize == fragmentSize && std::abs(configuredSampleRate - sampleRate) < 0.001)
    return;

  correctionConfigValid = true;
  configuredEnabled = settings.enabled;
  configuredAmount = clampedAmount;
  configuredKey = clampedKey;
  configuredScale = clampedScale;
  configuredCustomMask = clampedCustomMask;
  configuredFragmentSize = fragmentSize;
  configuredSampleRate = sampleRate;
  configuredNoteMask = makeScaleMask(clampedKey, clampedScale, clampedCustomMask);

  const auto amountNorm = settings.enabled ? clampedAmount / 100.0f : 0.0f;
  const auto retuneSeconds = retuneMillisecondsForAmount(amountNorm) / 1000.0f;
  const auto pitchLock = std::pow(amountNorm, 1.35f);
  const auto hardLock = std::pow(amountNorm, 2.25f);
  tuneLockStrength = settings.enabled ? std::pow(amountNorm, 1.15f) : 0.0f;
  analysisIntervalFragments = amountNorm >= 0.70f ? 2 : 4;
  const auto analysisSamples = static_cast<float>(analysisIntervalFragments * fragmentSize);
  correctionFilter =
      retuneSeconds <= 0.000001f
          ? 1.0f
          : juce::jlimit(
                0.008f, 1.0f,
                1.0f - std::exp(-analysisSamples / static_cast<float>(retuneSeconds * sampleRate)));
  correctionFilter += (1.0f - correctionFilter) * tuneLockStrength * 0.88f;
  correctionGain = settings.enabled ? 1.0f : 0.0f;
  correctionOvershoot = 1.0f;
  noteSwitchMarginOctaves = (22.0f - tuneLockStrength * 15.0f) / 1200.0f;
  noteReleaseOctaves = (88.0f - tuneLockStrength * 32.0f) / 1200.0f;
  const auto maxTransitionFollow = 0.42f + tuneLockStrength * 0.58f;
  noteTransitionFilter =
      juce::jlimit(0.10f, maxTransitionFollow, correctionFilter * (1.18f + pitchLock * 0.38f));
  noteTransitionFilter += (1.0f - noteTransitionFilter) * tuneLockStrength * 0.78f;
  noteSelectFollow = juce::jlimit(0.08f, 0.55f, 0.13f + tuneLockStrength * 0.34f);
  correctionMidiFollowFast =
      juce::jlimit(0.14f, 0.88f, correctionFilter * (1.22f + pitchLock * 0.22f + hardLock * 0.34f));
  correctionMidiFollowSlow =
      juce::jlimit(0.10f, 0.72f, correctionFilter * (0.84f + pitchLock * 0.12f + hardLock * 0.26f));
  correctionMidiFollowFast += (1.0f - correctionMidiFollowFast) * tuneLockStrength * 0.92f;
  correctionMidiFollowSlow += (1.0f - correctionMidiFollowSlow) * tuneLockStrength * 0.86f;
  centerLockGain = 1.0f;
  tonalCorrectionFloor = settings.enabled ? 0.35f + tuneLockStrength * 0.65f : 0.35f;
  pitchRatioFollow =
      juce::jlimit(0.30f, 0.94f, correctionFilter * (1.75f + pitchLock * 0.45f + hardLock * 0.50f));
  pitchRatioFollow += (1.0f - pitchRatioFollow) * tuneLockStrength * 0.88f;
  detectorFollowFast = juce::jlimit(0.45f, 0.94f, 0.48f + pitchLock * 0.34f + hardLock * 0.12f);
  detectorFollowSlow = juce::jlimit(0.30f, 0.76f, 0.30f + pitchLock * 0.30f + hardLock * 0.14f);
  detectorFollowFast += (1.0f - detectorFollowFast) * tuneLockStrength * 0.55f;
  detectorFollowSlow += (1.0f - detectorFollowSlow) * tuneLockStrength * 0.45f;
  noteHoldAnalyses = tuneLockStrength > 0.68f ? 1 : 2;
  noteReleaseAnalyses = tuneLockStrength > 0.68f ? 2 : 3;
  fastMode = false;
}

void FatTuneEngine::pushDetectorSample(float sample)
{
  detectorLowpass += detectorLowpassAlpha * (sample - detectorLowpass);
  const auto filtered = detectorLowpass;
  if (++detectorDecimationCounter < detectorDecimation)
    return;

  detectorDecimationCounter = 0;
  detectorBuffer[static_cast<size_t>(detectorWriteIndex)] = filtered;
  detectorWriteIndex = (detectorWriteIndex + 1) % detectorSize;
}

void FatTuneEngine::analysePitch(int noteMask)
{
  if (detectorBuffer.empty() || detectorScratch.size() != detectorBuffer.size())
    return;

  auto mean = 0.0f;
  for (auto i = 0; i < detectorSize; ++i)
  {
    const auto index = (detectorWriteIndex + i) % detectorSize;
    const auto sample = detectorBuffer[static_cast<size_t>(index)];
    detectorScratch[static_cast<size_t>(i)] = sample;
    mean += sample;
  }

  mean /= static_cast<float>(detectorSize);

  auto energy = 0.0f;
  auto peak = 0.0f;
  for (auto& sample : detectorScratch)
  {
    sample -= mean;
    energy += sample * sample;
    peak = juce::jmax(peak, std::abs(sample));
  }

  const auto rms = std::sqrt(energy / static_cast<float>(detectorSize));
  if (rms < 0.0012f || peak < 0.004f)
  {
    detectedClarity *= 0.72f;
    if (detectedClarity < 0.18f)
    {
      voiced = false;
      detectedFrequency = 0.0f;
      hasSmoothedMidi = false;
      hasNoteSelectMidi = false;
      hasCorrectionMidi = false;
      pitchTrendSemitones = 0.0f;
      pitchDirection = 0;
      directionalRun = 0;
      onsetGuardAnalyses = 5;
    }
    return;
  }

  const auto detectorRate = static_cast<float>(sampleRate / static_cast<double>(detectorDecimation));
  const auto minTau = juce::jlimit(2, detectorSize / 2 - 2, juce::roundToInt(detectorRate / 1200.0f));
  const auto maxTau = juce::jlimit(minTau + 4, detectorSize / 2 - 1, juce::roundToInt(detectorRate / 60.0f));

  // YIN difference function d[tau] = sum_{i=0..W-1} (x[i] - x[i+tau])^2, with W = N - maxTau.
  // Computed in-place into pitchCmndf, then converted to the cumulative-mean-normalized
  // difference function (CMNDF).
  const auto windowLength = detectorSize - maxTau;
  if (windowLength <= 8 || static_cast<int>(pitchCmndf.size()) <= maxTau)
  {
    detectedClarity *= 0.72f;
    return;
  }

  pitchCmndf[0] = 1.0f;
  auto runningSum = 0.0f;
  for (auto tau = 1; tau <= maxTau; ++tau)
  {
    auto sum = 0.0f;
    for (auto i = 0; i < windowLength; ++i)
    {
      const auto diff = detectorScratch[static_cast<size_t>(i)] -
                        detectorScratch[static_cast<size_t>(i + tau)];
      sum += diff * diff;
    }
    runningSum += sum;
    pitchCmndf[static_cast<size_t>(tau)] = runningSum > 1.0e-12f
                                                ? sum * static_cast<float>(tau) / runningSum
                                                : 1.0f;
  }

  // YIN absolute threshold rule: pick the first tau in [minTau, maxTau-1] whose CMNDF dips
  // below `threshold` and is a local minimum. If none qualifies, fall back to the global
  // minimum in range.
  constexpr auto threshold = 0.12f;
  auto chosenTau = -1;
  for (auto tau = minTau; tau < maxTau; ++tau)
  {
    if (pitchCmndf[static_cast<size_t>(tau)] < threshold)
    {
      while (tau + 1 < maxTau &&
             pitchCmndf[static_cast<size_t>(tau + 1)] < pitchCmndf[static_cast<size_t>(tau)])
        ++tau;
      chosenTau = tau;
      break;
    }
  }

  if (chosenTau < 0)
  {
    auto fallback = minTau;
    for (auto tau = minTau + 1; tau <= maxTau; ++tau)
      if (pitchCmndf[static_cast<size_t>(tau)] < pitchCmndf[static_cast<size_t>(fallback)])
        fallback = tau;
    if (pitchCmndf[static_cast<size_t>(fallback)] < 0.30f)
      chosenTau = fallback;
  }

  if (chosenTau > 0 && hasSmoothedMidi)
  {
    // Octave-up guard: if the chosen tau is roughly half of the previous period, prefer the
    // longer period when it is similarly confident. Octave-up errors are the dominant
    // detector failure mode on vocals with strong even harmonics.
    const auto previousFreq = 440.0f * std::pow(2.0f, (smoothedMidi - 69.0f) / 12.0f);
    const auto previousTau = detectorRate / juce::jmax(1.0f, previousFreq);
    const auto doubledTau = chosenTau * 2;
    if (doubledTau < maxTau && previousTau > chosenTau * 1.5f &&
        pitchCmndf[static_cast<size_t>(doubledTau)] < threshold + 0.06f &&
        pitchCmndf[static_cast<size_t>(doubledTau)] <
            pitchCmndf[static_cast<size_t>(chosenTau)] * 1.30f)
    {
      auto refined = doubledTau;
      while (refined + 1 < maxTau &&
             pitchCmndf[static_cast<size_t>(refined + 1)] < pitchCmndf[static_cast<size_t>(refined)])
        ++refined;
      chosenTau = refined;
    }
  }

  auto bestTau = 0;
  auto bestValue = 0.0f;
  auto bestMidi = 0.0f;
  if (chosenTau > 0 && chosenTau < maxTau)
  {
    // Parabolic interpolation around the chosen lag for sub-sample accuracy.
    auto tauF = static_cast<float>(chosenTau);
    if (chosenTau > 0)
    {
      const auto a = pitchCmndf[static_cast<size_t>(chosenTau - 1)];
      const auto b = pitchCmndf[static_cast<size_t>(chosenTau)];
      const auto c = pitchCmndf[static_cast<size_t>(chosenTau + 1)];
      const auto den = a - 2.0f * b + c;
      if (std::abs(den) > 1.0e-12f)
        tauF = static_cast<float>(chosenTau) + 0.5f * (a - c) / den;
    }
    const auto frequency = detectorRate / juce::jmax(1.0e-3f, tauF);
    bestTau = chosenTau;
    bestValue = juce::jlimit(0.0f, 1.0f, 1.0f - pitchCmndf[static_cast<size_t>(chosenTau)]);
    bestMidi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    if (hasSmoothedMidi)
      bestMidi = foldMidiToReference(bestMidi, smoothedMidi);
  }

  if (bestTau <= 0 || bestValue < 0.55f)
  {
    detectedClarity *= 0.72f;
    if (++unvoicedCounter > 5)
    {
      voiced = false;
      pitchErrorOctaves = 0.0f;
      detectedFrequency = 0.0f;
      hasSmoothedMidi = false;
      hasNoteSelectMidi = false;
      hasCorrectionMidi = false;
      pitchTrendSemitones = 0.0f;
      pitchDirection = 0;
      directionalRun = 0;
      onsetGuardAnalyses = 5;
    }
    return;
  }

  juce::ignoreUnused(bestTau);
  auto nextMidi = bestMidi;

  if (hasSmoothedMidi && std::abs(nextMidi - smoothedMidi) > 7.0f)
    nextMidi = foldMidiToReference(nextMidi, smoothedMidi);

  if (hasSmoothedMidi)
  {
    const auto maxDetectorStep = bestValue > 0.82f ? 1.55f : 0.95f;
    const auto detectorStep = nextMidi - smoothedMidi;
    if (std::abs(detectorStep) > maxDetectorStep)
    {
      nextMidi = smoothedMidi + (detectorStep > 0.0f ? maxDetectorStep : -maxDetectorStep);
      bestValue *= 0.90f;
    }
  }

  if (hasSmoothedMidi)
  {
    const auto trendStep = nextMidi - smoothedMidi;
    pitchTrendSemitones += (trendStep - pitchTrendSemitones) * 0.32f;

    const auto nextDirection = std::abs(trendStep) > 0.018f ? (trendStep > 0.0f ? 1 : -1) : 0;
    if (nextDirection == 0)
    {
      directionalRun = juce::jmax(0, directionalRun - 1);
    }
    else if (nextDirection == pitchDirection)
    {
      directionalRun = juce::jmin(16, directionalRun + 1);
    }
    else
    {
      pitchDirection = nextDirection;
      directionalRun = 1;
    }
  }
  else
  {
    pitchTrendSemitones = 0.0f;
    pitchDirection = 0;
    directionalRun = 0;
  }

  const auto wasVoiced = voiced;
  if (!hasSmoothedMidi || std::abs(nextMidi - smoothedMidi) > 7.0f)
    smoothedMidi = nextMidi;
  else
    smoothedMidi += (nextMidi - smoothedMidi) * (bestValue > 0.82f ? detectorFollowFast : detectorFollowSlow);

  if (!hasCorrectionMidi || std::abs(nextMidi - correctionMidi) > 7.0f)
    correctionMidi = nextMidi;
  else
    correctionMidi += (nextMidi - correctionMidi) * (bestValue > 0.82f ? correctionMidiFollowFast
                                                                       : correctionMidiFollowSlow);

  if (!hasNoteSelectMidi || std::abs(smoothedMidi - noteSelectMidi) > 7.0f)
    noteSelectMidi = smoothedMidi;
  else
  {
    const auto slideSelectFollow =
        (directionalRun >= 3 && std::abs(pitchTrendSemitones) > 0.026f)
            ? juce::jmin(0.24f, noteSelectFollow + 0.10f)
            : noteSelectFollow;
    noteSelectMidi += (smoothedMidi - noteSelectMidi) * slideSelectFollow;
  }

  cycleSamples = static_cast<float>(sampleRate) / (440.0f * std::pow(2.0f, (smoothedMidi - 69.0f) / 12.0f));
  detectedFrequency = static_cast<float>(sampleRate) / cycleSamples;
  detectedClarity = bestValue;
  hasSmoothedMidi = true;
  hasNoteSelectMidi = true;
  hasCorrectionMidi = true;
  if (!wasVoiced)
    onsetGuardAnalyses = juce::jmax(onsetGuardAnalyses, 5);
  voiced = true;
  unvoicedCounter = 0;

  updateCorrection(noteMask);
}

void FatTuneEngine::updateCorrection(int noteMask)
{
  if (noteMask == 0 || detectedFrequency < 55.0f)
  {
    pitchErrorOctaves = 0.0f;
    lastNote = -1;
    pendingNote = -1;
    pendingNoteCount = 0;
    noteTransitionFramesRemaining = 0;
    stableNoteAnalyses = 0;
    targetLatchAnalyses = 0;
    hasNoteSelectMidi = false;
    hasCorrectionMidi = false;
    pitchTrendSemitones = 0.0f;
    pitchDirection = 0;
    directionalRun = 0;
    return;
  }

  const auto f = std::log2(static_cast<float>(sampleRate) / (cycleSamples * 440.0f));
  const auto noteSelectOctaves = hasNoteSelectMidi ? (noteSelectMidi - 69.0f) / 12.0f : f;
  const auto correctionOctaves = hasCorrectionMidi ? (correctionMidi - 69.0f) / 12.0f : f;
  auto bestAbs = 1.0f;
  auto bestDelta = 0.0f;
  auto bestNote = -1;

  for (auto note = 0, bit = 1; note < 12; ++note, bit <<= 1)
  {
    if ((noteMask & bit) == 0)
      continue;

    const auto noteOctaves = (static_cast<float>(note) - 9.0f) / 12.0f;
    auto delta = correctionOctaves - noteOctaves;
    delta -= std::floor(delta + 0.5f);
    auto selectionDelta = noteSelectOctaves - noteOctaves;
    selectionDelta -= std::floor(selectionDelta + 0.5f);
    const auto absDelta = std::abs(selectionDelta);

    if (absDelta < bestAbs)
    {
      bestAbs = absDelta;
      bestDelta = delta;
      bestNote = note;
    }
  }

  if (bestNote < 0)
  {
    pitchErrorOctaves = 0.0f;
    lastNote = -1;
    pendingNote = -1;
    pendingNoteCount = 0;
    noteTransitionFramesRemaining = 0;
    stableNoteAnalyses = 0;
    targetLatchAnalyses = 0;
    hasNoteSelectMidi = false;
    hasCorrectionMidi = false;
    pitchTrendSemitones = 0.0f;
    pitchDirection = 0;
    directionalRun = 0;
    return;
  }

  auto heldDelta = 0.0f;
  auto heldNoteIsReleased = true;
  if (lastNote >= 0 && lastNote < 12 && (noteMask & pitchClassBit(lastNote)) != 0)
  {
    const auto heldNoteOctaves = (static_cast<float>(lastNote) - 9.0f) / 12.0f;
    heldDelta = correctionOctaves - heldNoteOctaves;
    heldDelta -= std::floor(heldDelta + 0.5f);
    auto heldSelectionDelta = noteSelectOctaves - heldNoteOctaves;
    heldSelectionDelta -= std::floor(heldSelectionDelta + 0.5f);
    const auto heldSelectionAbs = std::abs(heldSelectionDelta);
    const auto newNoteHasConviction = bestAbs + noteSwitchMarginOctaves < heldSelectionAbs;
    heldNoteIsReleased = heldSelectionAbs > noteReleaseOctaves;
    if (heldNoteIsReleased && (directionalRun < 4 || std::abs(pitchTrendSemitones) < 0.032f) &&
        heldSelectionAbs < noteReleaseOctaves + noteSwitchMarginOctaves)
      heldNoteIsReleased = false;

    auto directionalRelease = false;
    if (!heldNoteIsReleased && pitchDirection != 0 && directionalRun >= 5 && detectedClarity > 0.72f &&
        std::abs(pitchTrendSemitones) > 0.040f)
    {
      auto directedNote = -1;
      auto directedSteps = 0;
      for (auto step = 1; step <= 12; ++step)
      {
        const auto candidate = (lastNote + pitchDirection * step + 120) % 12;
        if ((noteMask & pitchClassBit(candidate)) != 0)
        {
          directedNote = candidate;
          directedSteps = step;
          break;
        }
      }

      const auto correctionMotionSemitones = heldDelta * 12.0f * static_cast<float>(pitchDirection);
      const auto selectionMotionSemitones = heldSelectionDelta * 12.0f * static_cast<float>(pitchDirection);
      const auto earlySwitchSemitones =
          juce::jlimit(0.42f, 0.68f, 0.36f + static_cast<float>(directedSteps) * 0.080f);

      if (directedNote >= 0 && correctionMotionSemitones > earlySwitchSemitones &&
          selectionMotionSemitones > 0.22f)
      {
        const auto directedNoteOctaves = (static_cast<float>(directedNote) - 9.0f) / 12.0f;
        auto directedDelta = correctionOctaves - directedNoteOctaves;
        directedDelta -= std::floor(directedDelta + 0.5f);
        bestDelta = directedDelta;
        bestNote = directedNote;
        heldNoteIsReleased = true;
        directionalRelease = true;
      }
    }

    if (bestNote != lastNote && !directionalRelease && !heldNoteIsReleased && !newNoteHasConviction)
    {
      bestDelta = heldDelta;
      bestNote = lastNote;
    }
  }

  if (lastNote >= 0 && bestNote != lastNote)
  {
    if (pendingNote != bestNote)
    {
      pendingNote = bestNote;
      pendingNoteCount = 1;
    }
    else
    {
      ++pendingNoteCount;
    }

    auto requiredHoldAnalyses = heldNoteIsReleased ? 1 : noteHoldAnalyses;
    if (targetLatchAnalyses > 0 && tuneLockStrength < 0.72f)
      requiredHoldAnalyses = juce::jmax(requiredHoldAnalyses, 2);
    if (onsetGuardAnalyses > 0 && detectedClarity < 0.92f && tuneLockStrength < 0.84f)
      requiredHoldAnalyses = juce::jmax(requiredHoldAnalyses, 3);

    if (pendingNoteCount < requiredHoldAnalyses && lastNote >= 0 && lastNote < 12 &&
        (noteMask & pitchClassBit(lastNote)) != 0)
    {
      bestDelta = heldDelta;
      bestNote = lastNote;
    }
  }
  else
  {
    pendingNote = -1;
    pendingNoteCount = 0;
  }

  const auto noteChanged = lastNote >= 0 && bestNote != lastNote;
  stableNoteAnalyses = lastNote == bestNote ? juce::jmin(stableNoteAnalyses + 1, 64) : 0;

  if (lastNote == bestNote)
  {
    const auto filter = noteTransitionFramesRemaining > 0 ? noteTransitionFilter : correctionFilter;
    pitchErrorOctaves += filter * (bestDelta - pitchErrorOctaves);
  }
  else if (lastNote >= 0)
  {
    noteTransitionFramesRemaining = noteReleaseAnalyses;
    pitchErrorOctaves += noteTransitionFilter * (bestDelta - pitchErrorOctaves);
  }
  else
  {
    pitchErrorOctaves = bestDelta;
  }

  if (noteTransitionFramesRemaining > 0)
    --noteTransitionFramesRemaining;
  if (targetLatchAnalyses > 0)
    --targetLatchAnalyses;
  if (onsetGuardAnalyses > 0)
    --onsetGuardAnalyses;

  lastNote = bestNote;
  if (noteChanged)
  {
    const auto latchAnalyses =
        juce::roundToInt((1.0f - tuneLockStrength) * static_cast<float>(noteReleaseAnalyses + 1));
    targetLatchAnalyses = juce::jmax(targetLatchAnalyses, latchAnalyses);
    pendingNote = -1;
    pendingNoteCount = 0;
  }
  detectedNoteBits |= 1 << bestNote;

  const auto detectedMidi = 69.0f + 12.0f * std::log2(detectedFrequency / 440.0f);
  const auto targetMidi = findNearestScaleMidi(hasNoteSelectMidi ? noteSelectMidi : detectedMidi, 0, 0, noteMask);
  meters.frequency = detectedFrequency;
  meters.cents = juce::jlimit(-100.0f, 100.0f, (detectedMidi - targetMidi) * 100.0f);
  meters.confidence = juce::jlimit(0.0f, 100.0f, detectedClarity * 100.0f);
  meters.targetMidi = targetMidi;
}

void FatTuneEngine::updateJumpState(int noteMask)
{
  if (++fragmentCounter >= analysisIntervalFragments)
  {
    fragmentCounter = 0;
    analysePitch(noteMask);
    const auto confidenceLock = juce::jlimit(0.0f, 1.0f, (detectedClarity - 0.58f) / 0.28f);
    const auto voicedConfidence = voiced ? juce::jlimit(0.0f, 1.0f, (detectedClarity - 0.56f) / 0.22f) : 0.0f;
    shiftedBlendTarget = voiced ? voicedConfidence + (1.0f - voicedConfidence) * tuneLockStrength : 0.0f;
    const auto baseHighPreserve = 0.30f - tuneLockStrength * 0.18f;
    highPreserveTarget =
        juce::jlimit(0.08f, 0.90f,
                     baseHighPreserve +
                         (1.0f - voicedConfidence) * 0.60f * (1.0f - tuneLockStrength * 0.75f));
    const auto stableLock = juce::jlimit(0.0f, 1.0f, static_cast<float>(stableNoteAnalyses - 1) / 3.0f);
    const auto noteConviction =
        (voiced && noteTransitionFramesRemaining == 0 && targetLatchAnalyses == 0)
            ? stableLock * confidenceLock
            : 0.0f;
    const auto landingGain = 1.0f + (centerLockGain - 1.0f) * stableLock * confidenceLock;
    const auto appliedCorrectionGain =
        correctionGain * landingGain * (1.0f + (correctionOvershoot - 1.0f) * confidenceLock);
    auto targetRatioOctaves = -pitchErrorOctaves * appliedCorrectionGain;

    auto tonalWeight = juce::jlimit(tonalCorrectionFloor, 1.0f, (detectedClarity - 0.52f) / 0.23f);
    if (onsetGuardAnalyses > 0 && detectedClarity < 0.88f && noteConviction < 0.50f)
      tonalWeight *= 0.68f + tuneLockStrength * 0.32f;
    tonalWeight += (1.0f - tonalWeight) * noteConviction;
    targetRatioOctaves *= tonalWeight;

    constexpr auto maxCorrectionSemitones = 3.5f;
    const auto minPitchRatio = std::pow(2.0f, -maxCorrectionSemitones / 12.0f);
    const auto maxPitchRatio = std::pow(2.0f, maxCorrectionSemitones / 12.0f);
    const auto maxRatioOctaves = maxCorrectionSemitones / 12.0f;
    targetRatioOctaves = juce::jlimit(-maxRatioOctaves, maxRatioOctaves, targetRatioOctaves);
    auto ratioFollow = pitchRatioFollow;
    if (!voiced)
    {
      targetRatioOctaves = 0.0f;
      ratioFollow = 0.92f;
    }
    else if (noteConviction > 0.0f)
    {
      const auto correctionMoveSemitones = std::abs((targetRatioOctaves - pitchRatioOctaves) * 12.0f);
      const auto stableMove =
          juce::jlimit(0.0f, 1.0f, (0.70f - correctionMoveSemitones) / 0.70f);
      const auto lockedFollow = juce::jlimit(0.50f, 1.0f, pitchRatioFollow + 0.08f + noteConviction * 0.06f);
      ratioFollow += (lockedFollow - ratioFollow) * noteConviction * stableMove;
    }
    pitchRatioOctaves += ratioFollow * (targetRatioOctaves - pitchRatioOctaves);
    const auto nextPitchRatio = juce::jlimit(minPitchRatio, maxPitchRatio, std::pow(2.0f, pitchRatioOctaves));
    pitchRatioTarget = std::isfinite(nextPitchRatio) ? nextPitchRatio : 1.0f;
    pitchRatioRampRemaining = juce::jmax(1, fragmentSize);
    pitchRatioRampStep = (pitchRatioTarget - pitchRatio) / static_cast<float>(pitchRatioRampRemaining);
  }

  if (crossfading)
    readIndex1 = readIndex2;

  const auto periodJump = cycleSamples * static_cast<float>(juce::jmax(1, juce::roundToInt(std::ceil(
                                                                  static_cast<float>(fragmentSize) /
                                                                  juce::jmax(1.0f, cycleSamples)))));
  const auto targetDistance = static_cast<float>(latencySamples);
  const auto distance = wrapDistance(static_cast<float>(writeIndex), readIndex1, bufferSize);
  const auto tolerance = juce::jlimit(static_cast<float>(fragmentSize) * 0.60f, periodJump * 0.55f,
                                      static_cast<float>(fragmentSize) * 2.5f);

  if (!voiced)
  {
    crossfading = true;
    readIndex2 = wrapIndex(static_cast<float>(writeIndex) - targetDistance, bufferSize);
  }
  else if (distance < targetDistance - tolerance)
  {
    crossfading = true;
    readIndex2 = findBestSpliceIndex(readIndex1, readIndex1 - periodJump,
                                     juce::jlimit(8.0f, static_cast<float>(fragmentSize) * 0.75f,
                                                  cycleSamples * 0.18f));
  }
  else if (distance > targetDistance + tolerance)
  {
    crossfading = true;
    readIndex2 = findBestSpliceIndex(readIndex1, readIndex1 + periodJump,
                                     juce::jlimit(8.0f, static_cast<float>(fragmentSize) * 0.75f,
                                                  cycleSamples * 0.18f));
  }
  else
  {
    crossfading = false;
  }

  previousReadAhead = readAhead;
  readAhead = fastMode ? bufferSize * 7 / 16 : 0;
}

FatTuneEngine::Result FatTuneEngine::readShiftedSample(bool correctionActive, int noteMask)
{
  if (filledSamples < latencySamples + fragmentSize)
    return readDelayOnly();

  const auto ratioStep = correctionActive ? pitchRatio : 1.0f;
  Result result;

  // Read the shifted RESIDUAL (whitened input). The synthesis filter below imposes the
  // current spectral envelope so formants stay in place when pitch shifts.
  const auto useFormantPreservation = lpcReady;
  const auto readShiftedChannel = [this, useFormantPreservation](int channel, float index, int ahead)
  {
    return useFormantPreservation ? readCubicResidualWithAhead(channel, index, ahead)
                                  : readCubicWithAhead(channel, index, ahead);
  };

  if (crossfading)
  {
    const auto fade = crossfade[static_cast<size_t>(juce::jlimit(0, fragmentSize - 1, fragmentIndex))];
    result.left = readShiftedChannel(0, readIndex1, previousReadAhead) * (1.0f - fade) +
                  readShiftedChannel(0, readIndex2, readAhead) * fade;
    result.right = readShiftedChannel(1, readIndex1, previousReadAhead) * (1.0f - fade) +
                   readShiftedChannel(1, readIndex2, readAhead) * fade;
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
    readIndex2 = wrapIndex(readIndex2 + ratioStep, bufferSize);
  }
  else if (readAhead != previousReadAhead)
  {
    const auto fade = crossfade[static_cast<size_t>(juce::jlimit(0, fragmentSize - 1, fragmentIndex))];
    result.left = readShiftedChannel(0, readIndex1, previousReadAhead) * (1.0f - fade) +
                  readShiftedChannel(0, readIndex1, readAhead) * fade;
    result.right = readShiftedChannel(1, readIndex1, previousReadAhead) * (1.0f - fade) +
                   readShiftedChannel(1, readIndex1, readAhead) * fade;
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
  }
  else
  {
    result.left = readShiftedChannel(0, readIndex1, readAhead);
    result.right = readShiftedChannel(1, readIndex1, readAhead);
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
  }

  if (useFormantPreservation)
  {
    // All-pole synthesis y[n] = e[n] - sum a_k * y[n-k], using the same crossfaded
    // coefficients as the analysis path so the filter reduces to identity at unity ratio.
    const auto xfade = lpcCrossfadeRemaining > 0
                           ? 1.0f - static_cast<float>(lpcCrossfadeRemaining) /
                                        static_cast<float>(juce::jmax(1, lpcCrossfadeLength))
                           : 1.0f;
    auto synthesizeChannel = [this, xfade](int channel, float residualSample)
    {
      auto& history = lpcSynthHistory[static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel))];
      auto y = residualSample;
      for (auto k = 1; k <= lpcOrder; ++k)
      {
        const auto aCurr = lpcCoeffs[static_cast<size_t>(k)];
        const auto aPrev = lpcCoeffsPrev[static_cast<size_t>(k)];
        const auto aEff = aPrev + (aCurr - aPrev) * xfade;
        y -= aEff * history[static_cast<size_t>(k - 1)];
      }
      if (!std::isfinite(y))
      {
        y = 0.0f;
        history.fill(0.0f);
      }
      else
      {
        y = juce::jlimit(-4.0f, 4.0f, y);
      }
      for (auto k = lpcOrder - 1; k > 0; --k)
        history[static_cast<size_t>(k)] = history[static_cast<size_t>(k - 1)];
      history[0] = y;
      return y;
    };
    result.left = synthesizeChannel(0, result.left);
    result.right = synthesizeChannel(1, result.right);
  }

  if (++fragmentIndex >= fragmentSize)
  {
    fragmentIndex = 0;
    updateJumpState(noteMask);
  }

  if (pitchRatioRampRemaining > 0)
  {
    pitchRatio += pitchRatioRampStep;
    --pitchRatioRampRemaining;
    if (pitchRatioRampRemaining == 0)
    {
      pitchRatio = pitchRatioTarget;
      pitchRatioRampStep = 0.0f;
    }
  }

  const auto delayed = readDelayOnly();
  const auto blendAlpha = shiftedBlendTarget > shiftedBlend ? shiftedBlendAttackAlpha : shiftedBlendReleaseAlpha;
  shiftedBlend += (shiftedBlendTarget - shiftedBlend) * blendAlpha;
  highPreserveMix += (highPreserveTarget - highPreserveMix) * highPreserveAlpha;

  result.left = delayed.left + (result.left - delayed.left) * shiftedBlend;
  result.right = delayed.right + (result.right - delayed.right) * shiftedBlend;

  const auto preserveHighBand = [this](int channel, float shifted, float delayedSample)
  {
    const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
    shiftedHighGuardLowpass[channelIndex] +=
        highGuardLowpassAlpha * (shifted - shiftedHighGuardLowpass[channelIndex]);
    delayHighGuardLowpass[channelIndex] +=
        highGuardLowpassAlpha * (delayedSample - delayHighGuardLowpass[channelIndex]);

    const auto shiftedHigh = shifted - shiftedHighGuardLowpass[channelIndex];
    const auto delayedHigh = delayedSample - delayHighGuardLowpass[channelIndex];
    return shiftedHighGuardLowpass[channelIndex] + shiftedHigh * (1.0f - highPreserveMix) +
           delayedHigh * highPreserveMix;
  };

  result.left = preserveHighBand(0, result.left, delayed.left);
  result.right = preserveHighBand(1, result.right, delayed.right);

  if (!std::isfinite(result.left) || !std::isfinite(result.right))
    return readDelayOnly();

  return result;
}

FatTuneEngine::Result FatTuneEngine::readDelayOnly() const
{
  if (filledSamples < latencySamples + 4)
    return {};

  return {
    readDelaySample(0, static_cast<float>(latencySamples)),
    readDelaySample(1, static_cast<float>(latencySamples))
  };
}

float FatTuneEngine::findBestSpliceIndex(float referenceIndex, float targetIndex, float searchRadius) const
{
  if (filledSamples < latencySamples + fragmentSize)
    return wrapIndex(targetIndex, bufferSize);

  const auto radius = juce::jlimit(2, fragmentSize, juce::roundToInt(searchRadius));
  const auto windowRadius = juce::jlimit(8, 24, fragmentSize / 6);
  const auto reference = wrapIndex(referenceIndex, bufferSize);
  auto bestIndex = wrapIndex(targetIndex, bufferSize);
  auto bestScore = std::numeric_limits<float>::max();

  for (auto offset = -radius; offset <= radius; offset += 2)
  {
    const auto candidate = wrapIndex(targetIndex + static_cast<float>(offset), bufferSize);
    auto score = 0.0f;

    for (auto windowOffset = -windowRadius; windowOffset <= windowRadius; windowOffset += 4)
    {
      for (auto channel = 0; channel < numChannels; ++channel)
      {
        const auto referenceSample = readCubic(channel, reference + static_cast<float>(windowOffset));
        const auto candidateSample = readCubic(channel, candidate + static_cast<float>(windowOffset));
        const auto difference = referenceSample - candidateSample;
        score += difference * difference;
      }
    }

    const auto normalizedOffset = static_cast<float>(offset) / static_cast<float>(juce::jmax(1, radius));
    score += normalizedOffset * normalizedOffset * 0.000003f;

    if (score < bestScore)
    {
      bestScore = score;
      bestIndex = candidate;
    }
  }

  return bestIndex;
}

float FatTuneEngine::readCubic(int channel, float index) const
{
  const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
  const auto& buffer = inputBuffer[channelIndex];
  if (buffer.empty())
    return 0.0f;

  index = wrapIndex(index, bufferSize);
  const auto baseIndex = static_cast<int>(std::floor(index));
  const auto amount = index - static_cast<float>(baseIndex);

  std::array<float, 4> samples {};
  for (auto i = 0; i < 4; ++i)
  {
    const auto wrapped = (baseIndex + i - 1 + bufferSize) % bufferSize;
    samples[static_cast<size_t>(i)] = buffer[static_cast<size_t>(wrapped)];
  }

  return cubic(samples.data(), amount);
}

float FatTuneEngine::readCubicWithAhead(int channel, float index, int ahead) const
{
  return readCubic(channel, index + static_cast<float>(ahead));
}

float FatTuneEngine::readCubicResidual(int channel, float index) const
{
  const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
  const auto& buffer = residualBuffer[channelIndex];
  if (buffer.empty())
    return 0.0f;

  index = wrapIndex(index, bufferSize);
  const auto baseIndex = static_cast<int>(std::floor(index));
  const auto amount = index - static_cast<float>(baseIndex);

  std::array<float, 4> samples {};
  for (auto i = 0; i < 4; ++i)
  {
    const auto wrapped = (baseIndex + i - 1 + bufferSize) % bufferSize;
    samples[static_cast<size_t>(i)] = buffer[static_cast<size_t>(wrapped)];
  }

  return cubic(samples.data(), amount);
}

float FatTuneEngine::readCubicResidualWithAhead(int channel, float index, int ahead) const
{
  return readCubicResidual(channel, index + static_cast<float>(ahead));
}

float FatTuneEngine::readDelaySample(int channel, float delay) const
{
  return readCubic(channel, static_cast<float>(writeIndex) - delay);
}

void FatTuneEngine::writeResidualSample()
{
  const auto xfade = lpcCrossfadeRemaining > 0
                         ? 1.0f - static_cast<float>(lpcCrossfadeRemaining) /
                                      static_cast<float>(juce::jmax(1, lpcCrossfadeLength))
                         : 1.0f;

  for (auto channel = 0; channel < numChannels; ++channel)
  {
    const auto& inBuf = inputBuffer[static_cast<size_t>(channel)];
    const auto x = inBuf[static_cast<size_t>(writeIndex)];
    auto pred = 0.0f;
    for (auto k = 1; k <= lpcOrder; ++k)
    {
      const auto idx = (writeIndex - k + bufferSize) % bufferSize;
      const auto aCurr = lpcCoeffs[static_cast<size_t>(k)];
      const auto aPrev = lpcCoeffsPrev[static_cast<size_t>(k)];
      const auto aEff = aPrev + (aCurr - aPrev) * xfade;
      pred += aEff * inBuf[static_cast<size_t>(idx)];
    }
    auto residual = x + pred;
    if (!std::isfinite(residual))
      residual = 0.0f;
    residualBuffer[static_cast<size_t>(channel)][static_cast<size_t>(writeIndex)] = residual;
  }

  for (auto channel = 0; channel < numChannels; ++channel)
  {
    const auto base = residualBuffer[static_cast<size_t>(channel)].data();
    base[bufferSize] = base[0];
    base[bufferSize + 1] = base[1];
    base[bufferSize + 2] = base[2];
    base[bufferSize + 3] = base[3];
  }

  if (lpcCrossfadeRemaining > 0)
    --lpcCrossfadeRemaining;
}

void FatTuneEngine::maybeUpdateLpc()
{
  if (++lpcSampleCounter < lpcUpdateInterval)
    return;
  lpcSampleCounter = 0;
  updateLpcCoefficients();
}

void FatTuneEngine::updateLpcCoefficients()
{
  if (filledSamples < lpcAnalysisLength + lpcOrder)
    return;
  if (static_cast<int>(lpcAnalysisScratch.size()) < lpcAnalysisLength)
    return;
  if (static_cast<int>(lpcAnalysisWindow.size()) < lpcAnalysisLength)
    return;

  // Pull the last lpcAnalysisLength samples (mono mix), apply Hann window.
  for (auto i = 0; i < lpcAnalysisLength; ++i)
  {
    const auto idx = (writeIndex - lpcAnalysisLength + 1 + i + bufferSize) % bufferSize;
    const auto mono = 0.5f * (inputBuffer[0][static_cast<size_t>(idx)] +
                              inputBuffer[1][static_cast<size_t>(idx)]);
    lpcAnalysisScratch[static_cast<size_t>(i)] = mono * lpcAnalysisWindow[static_cast<size_t>(i)];
  }

  // Autocorrelation r[0..order] with Gaussian lag window for bandwidth widening.
  std::array<float, lpcOrder + 1> r {};
  for (auto k = 0; k <= lpcOrder; ++k)
  {
    auto sum = 0.0f;
    const auto limit = lpcAnalysisLength - k;
    for (auto i = 0; i < limit; ++i)
      sum += lpcAnalysisScratch[static_cast<size_t>(i)] *
             lpcAnalysisScratch[static_cast<size_t>(i + k)];
    r[static_cast<size_t>(k)] = sum * lpcLagWindow[static_cast<size_t>(k)];
  }

  if (r[0] < 1.0e-9f)
    return;

  std::array<float, lpcOrder + 1> a {};
  auto gain = 0.0f;
  if (!levinsonDurbin(r.data(), lpcOrder, a.data(), gain))
    return;

  for (auto k = 1; k <= lpcOrder; ++k)
    if (!std::isfinite(a[static_cast<size_t>(k)]) ||
        std::abs(a[static_cast<size_t>(k)]) > 4.0f)
      return;

  if (lpcReady)
    lpcCoeffsPrev = lpcCoeffs;
  else
    lpcCoeffsPrev = a;
  lpcCoeffs = a;
  lpcCoeffs[0] = 1.0f;
  lpcCrossfadeRemaining = lpcCrossfadeLength;
  lpcReady = true;
}

bool FatTuneEngine::levinsonDurbin(const float* r, int order, float* a, float& gain)
{
  for (auto k = 0; k <= order; ++k)
    a[k] = 0.0f;
  a[0] = 1.0f;

  auto E = r[0];
  if (E <= 1.0e-12f)
    return false;

  std::array<float, lpcOrder + 1> aPrevious {};
  for (auto i = 1; i <= order; ++i)
  {
    auto acc = r[i];
    for (auto j = 1; j < i; ++j)
      acc += a[j] * r[i - j];
    auto k_i = -acc / E;
    if (!std::isfinite(k_i) || std::abs(k_i) >= 0.999f)
      return false;

    for (auto j = 0; j <= order; ++j)
      aPrevious[static_cast<size_t>(j)] = a[j];
    for (auto j = 1; j < i; ++j)
      a[j] = aPrevious[static_cast<size_t>(j)] + k_i * aPrevious[static_cast<size_t>(i - j)];
    a[i] = k_i;

    E *= (1.0f - k_i * k_i);
    if (E <= 1.0e-20f)
      return false;
  }
  gain = E;
  return true;
}

float FatTuneEngine::processOnePoleLowpass(float input, float cutoffHz, float& state) const
{
  const auto alpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(sampleRate));
  state += alpha * (input - state);
  return state;
}

int FatTuneEngine::makeScaleMask(int key, int scale, int customMask) const
{
  auto mask = 0;

  if (scale == 0)
  {
    mask = customMask & 0x0fff;
  }
  else
  {
    const auto sourceMask = scaleMasks[static_cast<size_t>(juce::jlimit(0, maxScaleCount - 1, scale))];
    const auto root = juce::jlimit(0, 11, key);
    for (auto pitchClass = 0; pitchClass < 12; ++pitchClass)
      if ((sourceMask & pitchClassBit(pitchClass)) != 0)
        mask |= pitchClassBit((pitchClass + root) % 12);
  }

  if (mask == 0)
    mask = pitchClassBit(juce::jlimit(0, 11, key));

  return mask;
}

float FatTuneEngine::findNearestScaleMidi(float midi, int key, int scale, int customMask) const
{
  const auto mask = makeScaleMask(key, scale, customMask);
  const auto center = juce::roundToInt(midi);
  auto bestMidi = static_cast<float>(center);
  auto bestDistance = std::numeric_limits<float>::max();

  for (auto candidate = center - 24; candidate <= center + 24; ++candidate)
  {
    const auto pitchClass = (candidate % 12 + 12) % 12;
    if ((mask & pitchClassBit(pitchClass)) == 0)
      continue;

    const auto distance = std::abs(static_cast<float>(candidate) - midi);
    if (distance < bestDistance)
    {
      bestDistance = distance;
      bestMidi = static_cast<float>(candidate);
    }
  }

  return bestMidi;
}

float FatTuneEngine::wrapIndex(float index, int size)
{
  const auto floatSize = static_cast<float>(juce::jmax(1, size));
  while (index < 0.0f)
    index += floatSize;
  while (index >= floatSize)
    index -= floatSize;
  return index;
}

float FatTuneEngine::wrapDistance(float writeIndex, float readIndex, int size)
{
  auto distance = writeIndex - readIndex;
  const auto floatSize = static_cast<float>(juce::jmax(1, size));
  while (distance < 0.0f)
    distance += floatSize;
  while (distance >= floatSize)
    distance -= floatSize;
  return distance;
}

float FatTuneEngine::cubic(const float* samples, float amount)
{
  const auto b = 1.0f - amount;
  const auto c = amount * b;
  return (1.0f + 1.5f * c) * (samples[1] * b + samples[2] * amount) -
         0.5f * c * (samples[0] * b + samples[1] + samples[2] + samples[3] * amount);
}
