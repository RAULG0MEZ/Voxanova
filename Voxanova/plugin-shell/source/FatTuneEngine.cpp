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
    detectorSize = 512;
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

  crossfade.assign(static_cast<size_t>(fragmentSize), 0.0f);
  spliceFadeSamples = juce::jlimit(12, fragmentSize, juce::roundToInt(static_cast<float>(sampleRate) * 0.00065f));
  for (auto i = 0; i < fragmentSize; ++i)
    crossfade[static_cast<size_t>(i)] =
        0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * static_cast<float>(i) /
                                static_cast<float>(juce::jmax(1, fragmentSize))));

  detectorBuffer.assign(static_cast<size_t>(detectorSize), 0.0f);
  detectorScratch.assign(static_cast<size_t>(detectorSize), 0.0f);
  detectorLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 1800.0f / static_cast<float>(sampleRate));
  shiftedBlendAttackAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0060 * sampleRate));
  shiftedBlendReleaseAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0025 * sampleRate));
  formantPreserveAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0080 * sampleRate));
  formantLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 220.0f / static_cast<float>(sampleRate));
  formantHighpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 3600.0f / static_cast<float>(sampleRate));
  lpcFormantAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0100 * sampleRate));
  lpcCoeffAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0180 * sampleRate));
  lpcWindowSamples =
      juce::jlimit(256, juce::jmin(1024, bufferSize / 2), juce::roundToInt(static_cast<float>(sampleRate) * 0.014f));
  lpcWindow.assign(static_cast<size_t>(lpcWindowSamples), 1.0f);
  for (auto i = 0; i < lpcWindowSamples; ++i)
  {
    const auto phase = static_cast<float>(i) / static_cast<float>(juce::jmax(1, lpcWindowSamples - 1));
    lpcWindow[static_cast<size_t>(i)] =
        0.54f - 0.46f * std::cos(2.0f * juce::MathConstants<float>::pi * phase);
  }
  highPreserveAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0040 * sampleRate));
  highGuardLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 4700.0f / static_cast<float>(sampleRate));
  airBoostAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0100 * sampleRate));
  airBoostLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 4200.0f / static_cast<float>(sampleRate));
  sibilanceLowpassAlpha =
      1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 3200.0f / static_cast<float>(sampleRate));
  sibilanceEnvAttackAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0007 * sampleRate));
  sibilanceEnvReleaseAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0140 * sampleRate));
  sibilanceGuardAttackAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0009 * sampleRate));
  sibilanceGuardReleaseAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(0.0220 * sampleRate));
  reset();
}

void FatTuneEngine::reset()
{
  for (auto& channelBuffer : inputBuffer)
    std::fill(channelBuffer.begin(), channelBuffer.end(), 0.0f);
  std::fill(detectorBuffer.begin(), detectorBuffer.end(), 0.0f);
  std::fill(detectorScratch.begin(), detectorScratch.end(), 0.0f);

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
  noteCenterMidi = 0.0f;
  noteCenterFollow = 0.06f;
  hasNoteCenterMidi = false;
  correctionMidi = 0.0f;
  correctionMidiFollowFast = 0.26f;
  correctionMidiFollowSlow = 0.16f;
  hasCorrectionMidi = false;
  tuneLockStrength = 0.0f;
  vibratoPreserve = 0.85f;
  centerLockGain = 1.0f;
  tonalCorrectionFloor = 0.35f;
  correctionToleranceOctaves = 5.0f / 1200.0f;
  correctionKneeOctaves = 10.0f / 1200.0f;
  pitchTrendSemitones = 0.0f;
  detectorFollowFast = 0.45f;
  detectorFollowSlow = 0.28f;
  correctionConfigValid = false;
  configuredEnabled = false;
  configuredAmount = -1.0f;
  configuredKey = -1;
  configuredScale = -1;
  configuredCustomMask = -1;
  configuredVoiceType = -1;
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
  formantPreserveMix = 0.0f;
  formantPreserveTarget = 0.0f;
  lpcFormantMix = 0.0f;
  lpcFormantTarget = 0.0f;
  highPreserveMix = 0.35f;
  highPreserveTarget = 0.35f;
  airBoostGain = 0.0f;
  airBoostTarget = 0.0f;
  sibilanceLowpass = 0.0f;
  sibilanceLowEnv = 0.0f;
  sibilanceHighEnv = 0.0f;
  sibilanceGuard = 0.0f;
  shiftedFormantLowpass.fill(0.0f);
  shiftedFormantHighpass.fill(0.0f);
  delayFormantLowpass.fill(0.0f);
  delayFormantHighpass.fill(0.0f);
  for (auto channel = 0; channel < numChannels; ++channel)
  {
    dryLpc[static_cast<size_t>(channel)].fill(0.0f);
    dryLpcTarget[static_cast<size_t>(channel)].fill(0.0f);
    shiftedLpc[static_cast<size_t>(channel)].fill(0.0f);
    shiftedLpcTarget[static_cast<size_t>(channel)].fill(0.0f);
    dryLpc[static_cast<size_t>(channel)][0] = 1.0f;
    dryLpcTarget[static_cast<size_t>(channel)][0] = 1.0f;
    shiftedLpc[static_cast<size_t>(channel)][0] = 1.0f;
    shiftedLpcTarget[static_cast<size_t>(channel)][0] = 1.0f;
    lpcInputHistory[static_cast<size_t>(channel)].fill(0.0f);
    lpcOutputHistory[static_cast<size_t>(channel)].fill(0.0f);
  }
  shiftedHighGuardLowpass.fill(0.0f);
  delayHighGuardLowpass.fill(0.0f);
  airBoostLowpass.fill(0.0f);

  detectorWriteIndex = 0;
  detectorDecimationCounter = 0;
  analysisSampleCounter = 0;
  detectorLowpass = 0.0f;
  detectedFrequency = 0.0f;
  detectedClarity = 0.0f;
  detectorMinFrequency = 70.0f;
  detectorMaxFrequency = 720.0f;
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

  sibilanceLowpass += sibilanceLowpassAlpha * (mono - sibilanceLowpass);
  const auto tonalBand = sibilanceLowpass;
  const auto airBand = mono - tonalBand;
  const auto updateEnvelope = [](float target, float& state, float attackAlpha, float releaseAlpha)
  {
    state += (target - state) * (target > state ? attackAlpha : releaseAlpha);
  };
  updateEnvelope(std::abs(tonalBand), sibilanceLowEnv, sibilanceEnvAttackAlpha, sibilanceEnvReleaseAlpha);
  updateEnvelope(std::abs(airBand), sibilanceHighEnv, sibilanceEnvAttackAlpha, sibilanceEnvReleaseAlpha);
  const auto sibilanceTotal = sibilanceLowEnv + sibilanceHighEnv + 0.000001f;
  const auto airDominance = sibilanceHighEnv / sibilanceTotal;
  const auto sibilanceTarget =
      sibilanceTotal > 0.0012f ? juce::jlimit(0.0f, 1.0f, (airDominance - 0.54f) / 0.22f) : 0.0f;
  sibilanceGuard +=
      (sibilanceTarget - sibilanceGuard) *
      (sibilanceTarget > sibilanceGuard ? sibilanceGuardAttackAlpha : sibilanceGuardReleaseAlpha);

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
  const auto clampedVoiceType = juce::jlimit(0, 2, settings.voiceType);

  if (correctionConfigValid && configuredEnabled == settings.enabled &&
      std::abs(configuredAmount - clampedAmount) < 0.001f && configuredKey == clampedKey &&
      configuredScale == clampedScale && configuredCustomMask == clampedCustomMask &&
      configuredVoiceType == clampedVoiceType && configuredFragmentSize == fragmentSize &&
      std::abs(configuredSampleRate - sampleRate) < 0.001)
    return;

  correctionConfigValid = true;
  configuredEnabled = settings.enabled;
  configuredAmount = clampedAmount;
  configuredKey = clampedKey;
  configuredScale = clampedScale;
  configuredCustomMask = clampedCustomMask;
  configuredVoiceType = clampedVoiceType;
  configuredFragmentSize = fragmentSize;
  configuredSampleRate = sampleRate;
  configuredNoteMask = makeScaleMask(clampedKey, clampedScale, clampedCustomMask);

  switch (clampedVoiceType)
  {
    case 0: // Low male: keep the detector away from tenor/falsetto harmonics.
      detectorMinFrequency = 48.0f;
      detectorMaxFrequency = 360.0f;
      break;
    case 2: // Soprano/falsetto: reject low octave pulls while still allowing high lead notes.
      detectorMinFrequency = 118.0f;
      detectorMaxFrequency = 1180.0f;
      break;
    default:
      detectorMinFrequency = 70.0f;
      detectorMaxFrequency = 720.0f;
      break;
  }

  const auto amountNorm = settings.enabled ? clampedAmount / 100.0f : 0.0f;
  const auto backendAmountNorm = std::pow(amountNorm, 1.18f);
  const auto retuneSeconds = retuneMillisecondsForAmount(backendAmountNorm) / 1000.0f;
  const auto pitchLock = std::pow(backendAmountNorm, 1.35f);
  const auto hardLock = std::pow(backendAmountNorm, 2.25f);
  tuneLockStrength = settings.enabled ? std::pow(backendAmountNorm, 1.15f) : 0.0f;
  analysisIntervalFragments = backendAmountNorm >= 0.70f ? 1 : 3;
  const auto analysisSamples = static_cast<float>(analysisIntervalFragments * fragmentSize);
  correctionFilter =
      retuneSeconds <= 0.000001f
          ? 1.0f
          : juce::jlimit(
                0.008f, 1.0f,
                1.0f - std::exp(-analysisSamples / static_cast<float>(retuneSeconds * sampleRate)));
  correctionFilter += (1.0f - correctionFilter) * tuneLockStrength * 0.88f;
  correctionGain = settings.enabled ? 1.0f + hardLock * 0.06f : 0.0f;
  correctionOvershoot = 1.0f;
  noteSwitchMarginOctaves = (24.0f - tuneLockStrength * 16.0f) / 1200.0f;
  noteReleaseOctaves = (76.0f - tuneLockStrength * 36.0f) / 1200.0f;
  const auto maxTransitionFollow = 0.42f + tuneLockStrength * 0.58f;
  noteTransitionFilter =
      juce::jlimit(0.10f, maxTransitionFollow, correctionFilter * (1.18f + pitchLock * 0.38f));
  noteTransitionFilter += (1.0f - noteTransitionFilter) * tuneLockStrength * 0.78f;
  noteSelectFollow = juce::jlimit(0.06f, 0.18f, 0.085f + tuneLockStrength * 0.095f);
  noteCenterFollow = juce::jlimit(0.030f, 0.16f, 0.050f + tuneLockStrength * 0.060f);
  vibratoPreserve =
      settings.enabled ? juce::jlimit(0.06f, 0.90f, 0.86f - tuneLockStrength * 0.78f) : 1.0f;
  correctionMidiFollowFast =
      juce::jlimit(0.14f, 0.88f, correctionFilter * (1.22f + pitchLock * 0.22f + hardLock * 0.34f));
  correctionMidiFollowSlow =
      juce::jlimit(0.10f, 0.72f, correctionFilter * (0.84f + pitchLock * 0.12f + hardLock * 0.26f));
  correctionMidiFollowFast += (1.0f - correctionMidiFollowFast) * tuneLockStrength * 0.92f;
  correctionMidiFollowSlow += (1.0f - correctionMidiFollowSlow) * tuneLockStrength * 0.86f;
  centerLockGain = 1.0f;
  tonalCorrectionFloor = settings.enabled ? 0.35f + tuneLockStrength * 0.65f : 0.35f;
  const auto toleranceCents = settings.enabled ? 9.0f - tuneLockStrength * 8.4f : 12.0f;
  const auto kneeCents = settings.enabled ? 12.0f - tuneLockStrength * 10.6f : 18.0f;
  correctionToleranceOctaves = juce::jlimit(0.35f, 12.0f, toleranceCents) / 1200.0f;
  correctionKneeOctaves = juce::jlimit(1.0f, 18.0f, kneeCents) / 1200.0f;
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
      hasNoteCenterMidi = false;
      hasCorrectionMidi = false;
      pitchTrendSemitones = 0.0f;
      pitchDirection = 0;
      directionalRun = 0;
      onsetGuardAnalyses = 5;
    }
    return;
  }

  const auto detectorRate = static_cast<float>(sampleRate / static_cast<double>(detectorDecimation));
  const auto maxDetectHz = juce::jlimit(detectorMinFrequency + 20.0f, 1400.0f, detectorMaxFrequency);
  const auto minDetectHz = juce::jlimit(35.0f, maxDetectHz - 20.0f, detectorMinFrequency);
  const auto minTau = juce::jlimit(2, detectorSize - 4, juce::roundToInt(detectorRate / maxDetectHz));
  const auto maxTau = juce::jlimit(minTau + 2, detectorSize - 3, juce::roundToInt(detectorRate / minDetectHz));
  const auto normalizedAcfAt = [this](int tau)
  {
    auto acf = 0.0f;
    auto divisor = 0.0f;
    const auto limit = detectorSize - tau;

    for (auto i = 0; i < limit; ++i)
    {
      const auto a = detectorScratch[static_cast<size_t>(i)];
      const auto b = detectorScratch[static_cast<size_t>(i + tau)];
      acf += a * b;
      divisor += a * a + b * b;
    }

    return divisor > 0.0000001f ? (2.0f * acf) / divisor : 0.0f;
  };

  auto bestTau = 0;
  auto bestValue = 0.0f;
  auto bestMidi = 0.0f;
  auto bestRawMidi = 0.0f;
  auto previousValue = 0.0f;
  const auto continuityMidi = hasSmoothedMidi ? smoothedMidi : 0.0f;
  auto bestScore = -std::numeric_limits<float>::max();

  for (auto tau = minTau; tau <= maxTau; ++tau)
  {
    auto acf = 0.0f;
    auto divisor = 0.0f;
    const auto limit = detectorSize - tau;

    for (auto i = 0; i < limit; ++i)
    {
      const auto a = detectorScratch[static_cast<size_t>(i)];
      const auto b = detectorScratch[static_cast<size_t>(i + tau)];
      acf += a * b;
      divisor += a * a + b * b;
    }

    const auto value = divisor > 0.0000001f ? (2.0f * acf) / divisor : 0.0f;
    const auto nextTau = juce::jmin(maxTau, tau + 1);
    auto nextAcf = 0.0f;
    auto nextDivisor = 0.0f;
    if (tau < maxTau)
    {
      const auto nextLimit = detectorSize - nextTau;
      for (auto i = 0; i < nextLimit; ++i)
      {
        const auto a = detectorScratch[static_cast<size_t>(i)];
        const auto b = detectorScratch[static_cast<size_t>(i + nextTau)];
        nextAcf += a * b;
        nextDivisor += a * a + b * b;
      }
    }
    const auto nextValue = nextDivisor > 0.0000001f ? (2.0f * nextAcf) / nextDivisor : -1.0f;

    if (tau > minTau && value >= previousValue && value > nextValue && value > 0.56f)
    {
      const auto frequency = detectorRate / static_cast<float>(tau);
      const auto rawMidi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
      auto midi = rawMidi;
      if (hasSmoothedMidi)
        midi = foldMidiToReference(midi, continuityMidi);

      const auto continuityDistance = hasSmoothedMidi ? std::abs(midi - continuityMidi) : 0.0f;
      const auto continuityPenalty =
          hasSmoothedMidi
              ? continuityDistance * 0.075f + juce::jmax(0.0f, continuityDistance - 1.5f) * 0.12f
              : 0.0f;
      const auto lowTauPreference = static_cast<float>(tau) / static_cast<float>(maxTau) * 0.020f;
      const auto score = value + lowTauPreference - continuityPenalty;

      if (score > bestScore)
      {
        bestScore = score;
        bestTau = tau;
        bestValue = value;
        bestMidi = midi;
        bestRawMidi = rawMidi;
      }
    }

    previousValue = value;
  }

  if (bestTau > 0)
  {
    const auto baseTau = bestTau;
    const auto baseValue = bestValue;
    const auto baseMidi = bestMidi;
    const auto baseRawMidi = bestRawMidi;
    auto selectedTau = baseTau;
    auto selectedValue = baseValue;
    auto selectedMidi = baseMidi;

    for (const auto octaveMultiplier : { 2, 4 })
    {
      const auto octaveTau = baseTau * octaveMultiplier;
      if (octaveTau > maxTau)
        continue;

      const auto octaveValue = normalizedAcfAt(octaveTau);
      const auto voiceFundamentalBias =
          configuredVoiceType == 2 ? -0.04f : (configuredVoiceType == 0 ? 0.12f : 0.08f);
      const auto multiplierPenalty = octaveMultiplier == 2 ? 0.0f : 0.12f;
      const auto supportThreshold =
          (hasSmoothedMidi ? 0.68f : 0.78f) - voiceFundamentalBias + multiplierPenalty;
      if (octaveValue < baseValue * supportThreshold)
        continue;

      const auto octaveRawMidi =
          69.0f + 12.0f * std::log2((detectorRate / static_cast<float>(octaveTau)) / 440.0f);
      auto octaveMidi = octaveRawMidi;
      if (hasSmoothedMidi)
        octaveMidi = foldMidiToReference(octaveMidi, continuityMidi);

      const auto currentDistance = hasSmoothedMidi ? std::abs(selectedMidi - continuityMidi) : 0.0f;
      const auto octaveDistance = hasSmoothedMidi ? std::abs(octaveMidi - continuityMidi) : 0.0f;
      const auto octaveBias =
          configuredVoiceType == 2 ? 0.12f : (configuredVoiceType == 0 ? 0.70f : 0.52f);
      const auto currentLooksLikeHarmonic =
          configuredVoiceType != 2 && octaveMultiplier == 2 && octaveRawMidi < baseRawMidi - 6.5f &&
          octaveValue >= baseValue * (0.54f - voiceFundamentalBias * 0.40f);
      const auto octaveLooksCleaner =
          !hasSmoothedMidi || octaveDistance <= currentDistance + octaveBias || currentLooksLikeHarmonic;

      if (octaveLooksCleaner)
      {
        selectedTau = octaveTau;
        selectedValue = juce::jmin(baseValue, octaveValue);
        selectedMidi = octaveMidi;
      }
    }

    bestTau = selectedTau;
    bestValue = selectedValue;
    bestMidi = selectedMidi;
  }

  if (bestTau <= 0 || bestValue < 0.60f)
  {
    detectedClarity *= 0.72f;
    if (++unvoicedCounter > 5)
    {
      voiced = false;
      pitchErrorOctaves = 0.0f;
      detectedFrequency = 0.0f;
      hasSmoothedMidi = false;
      hasNoteSelectMidi = false;
      hasNoteCenterMidi = false;
      hasCorrectionMidi = false;
      pitchTrendSemitones = 0.0f;
      pitchDirection = 0;
      directionalRun = 0;
      onsetGuardAnalyses = 5;
    }
    return;
  }

  juce::ignoreUnused(bestTau);
  auto refinedTau = static_cast<float>(bestTau);
  if (bestTau > minTau && bestTau < maxTau)
  {
    const auto leftValue = normalizedAcfAt(bestTau - 1);
    const auto centerValue = normalizedAcfAt(bestTau);
    const auto rightValue = normalizedAcfAt(bestTau + 1);
    const auto denominator = leftValue - 2.0f * centerValue + rightValue;
    if (std::abs(denominator) > 0.000001f)
      refinedTau += juce::jlimit(-0.48f, 0.48f, 0.5f * (leftValue - rightValue) / denominator);
  }

  const auto refinedFrequency = detectorRate / juce::jmax(1.0f, refinedTau);
  auto nextMidi = 69.0f + 12.0f * std::log2(refinedFrequency / 440.0f);
  if (hasSmoothedMidi)
    nextMidi = foldMidiToReference(nextMidi, continuityMidi);

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

  if (!hasNoteCenterMidi || std::abs(smoothedMidi - noteCenterMidi) > 7.0f)
  {
    noteCenterMidi = smoothedMidi;
    hasNoteCenterMidi = true;
  }
  else
  {
    const auto centerError = smoothedMidi - noteCenterMidi;
    const auto centerAbs = std::abs(centerError);
    const auto transitionRegisterMidi = hasNoteCenterMidi ? noteCenterMidi : smoothedMidi;
    const auto upperTransitionRegister = juce::jlimit(0.0f, 1.0f, (transitionRegisterMidi - 68.0f) / 8.0f);
    const auto fastRetuneAgility = juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.60f) / 0.18f);
    const auto midRetuneAgility =
        juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.42f) / 0.18f) *
        (1.0f - upperTransitionRegister * 0.80f);
    const auto slowHighRetuneAgility =
        upperTransitionRegister * juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.18f) / 0.12f) *
        juce::jlimit(0.0f, 1.0f, (0.42f - tuneLockStrength) / 0.14f) *
        juce::jlimit(0.0f, 1.0f, (centerAbs - 0.42f) / 0.30f);
    const auto transitionAgility =
        juce::jmax(fastRetuneAgility, juce::jmax(midRetuneAgility, slowHighRetuneAgility * 0.75f));
    const auto slideTrendThreshold = 0.035f - transitionAgility * 0.017f;
    const auto slideDistanceThreshold = 0.72f - transitionAgility * 0.34f;
    const auto slideIntent =
        (directionalRun >= (transitionAgility > 0.35f ? 2 : 4) &&
         std::abs(pitchTrendSemitones) > slideTrendThreshold) ||
        centerAbs > slideDistanceThreshold;
    auto centerFollow = noteCenterFollow;

    if (slideIntent)
      centerFollow = juce::jlimit(centerFollow, 0.56f + transitionAgility * 0.16f,
                                  centerFollow + 0.16f + transitionAgility * 0.10f +
                                      centerAbs * (0.12f + transitionAgility * 0.06f));
    else
      centerFollow *= centerAbs < 0.55f ? 0.25f : 0.50f;

    const auto confidenceFollow = juce::jlimit(0.40f, 1.0f, (bestValue - 0.56f) / 0.34f);
    noteCenterMidi += centerError * centerFollow * confidenceFollow;
  }

  const auto noteSelectSource = hasNoteCenterMidi ? noteCenterMidi : smoothedMidi;
  if (!hasNoteSelectMidi || std::abs(noteSelectSource - noteSelectMidi) > 7.0f)
    noteSelectMidi = noteSelectSource;
  else
  {
    const auto transitionRegisterMidi = hasNoteCenterMidi ? noteCenterMidi : noteSelectSource;
    const auto upperTransitionRegister = juce::jlimit(0.0f, 1.0f, (transitionRegisterMidi - 68.0f) / 8.0f);
    const auto selectAbs = std::abs(noteSelectSource - noteSelectMidi);
    const auto fastRetuneAgility = juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.60f) / 0.18f);
    const auto midRetuneAgility =
        juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.42f) / 0.18f) *
        (1.0f - upperTransitionRegister * 0.80f);
    const auto slowHighRetuneAgility =
        upperTransitionRegister * juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.18f) / 0.12f) *
        juce::jlimit(0.0f, 1.0f, (0.42f - tuneLockStrength) / 0.14f) *
        juce::jlimit(0.0f, 1.0f, (selectAbs - 0.34f) / 0.28f);
    const auto transitionAgility =
        juce::jmax(fastRetuneAgility, juce::jmax(midRetuneAgility, slowHighRetuneAgility * 0.75f));
    const auto slideTrendThreshold = 0.035f - transitionAgility * 0.017f;
    const auto slideSelectFollow =
        (directionalRun >= (transitionAgility > 0.35f ? 2 : 3) &&
         std::abs(pitchTrendSemitones) > slideTrendThreshold)
            ? juce::jmin(0.20f + transitionAgility * 0.24f,
                         noteSelectFollow + 0.08f + transitionAgility * 0.14f)
            : noteSelectFollow;
    noteSelectMidi += (noteSelectSource - noteSelectMidi) * slideSelectFollow;
  }

  cycleSamples = static_cast<float>(sampleRate) / (440.0f * std::pow(2.0f, (smoothedMidi - 69.0f) / 12.0f));
  detectedFrequency = static_cast<float>(sampleRate) / cycleSamples;
  detectedClarity = bestValue;
  hasSmoothedMidi = true;
  hasNoteSelectMidi = true;
  hasNoteCenterMidi = true;
  hasCorrectionMidi = true;
  if (!wasVoiced)
    onsetGuardAnalyses = juce::jmax(onsetGuardAnalyses, 5);
  voiced = true;
  unvoicedCounter = 0;

  updateCorrection(noteMask);
}

void FatTuneEngine::updateCorrection(int noteMask)
{
  if (noteMask == 0 || detectedFrequency < juce::jmax(38.0f, detectorMinFrequency * 0.85f))
  {
    pitchErrorOctaves = 0.0f;
    lastNote = -1;
    pendingNote = -1;
    pendingNoteCount = 0;
    noteTransitionFramesRemaining = 0;
    stableNoteAnalyses = 0;
    targetLatchAnalyses = 0;
    hasNoteSelectMidi = false;
    hasNoteCenterMidi = false;
    hasCorrectionMidi = false;
    pitchTrendSemitones = 0.0f;
    pitchDirection = 0;
    directionalRun = 0;
    return;
  }

  const auto f = std::log2(static_cast<float>(sampleRate) / (cycleSamples * 440.0f));
  const auto detectedMidiEstimate = 69.0f + 12.0f * f;
  const auto centerMidi =
      hasNoteCenterMidi ? noteCenterMidi : (hasNoteSelectMidi ? noteSelectMidi : detectedMidiEstimate);
  const auto instantaneousMidi = hasCorrectionMidi ? correctionMidi : centerMidi;
  const auto vibratoSemitones = juce::jlimit(-0.65f, 0.65f, instantaneousMidi - centerMidi);
  const auto correctionMidiForRatio = centerMidi + vibratoSemitones * (1.0f - vibratoPreserve);
  const auto upperRegisterConviction =
      juce::jlimit(0.0f, 1.0f, (centerMidi - 68.0f) / 10.0f) *
      juce::jlimit(0.0f, 1.0f, (detectedClarity - 0.66f) / 0.22f) *
      juce::jlimit(0.0f, 1.0f, (0.98f - tuneLockStrength) / 0.42f);
  const auto noteSelectOctaves = (centerMidi - 69.0f) / 12.0f;
  const auto correctionOctaves = (correctionMidiForRatio - 69.0f) / 12.0f;
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
    hasNoteCenterMidi = false;
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
    const auto effectiveSwitchMargin =
        noteSwitchMarginOctaves * (1.0f + upperRegisterConviction * 0.55f);
    const auto effectiveReleaseOctaves =
        noteReleaseOctaves * (1.0f + upperRegisterConviction * 0.35f);
    const auto newNoteHasConviction = bestAbs + effectiveSwitchMargin < heldSelectionAbs;
    heldNoteIsReleased = heldSelectionAbs > effectiveReleaseOctaves;
    if (heldNoteIsReleased && (directionalRun < 4 || std::abs(pitchTrendSemitones) < 0.032f) &&
        heldSelectionAbs < effectiveReleaseOctaves + effectiveSwitchMargin)
      heldNoteIsReleased = false;

    auto directionalRelease = false;
    const auto upperReleaseRegister = juce::jlimit(0.0f, 1.0f, (centerMidi - 68.0f) / 8.0f);
    const auto fastReleaseAgility = juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.60f) / 0.18f);
    const auto midReleaseAgility =
        juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.42f) / 0.18f) *
        (1.0f - upperReleaseRegister * 0.80f);
    const auto slowHighReleaseAgility =
        upperReleaseRegister * juce::jlimit(0.0f, 1.0f, (tuneLockStrength - 0.18f) / 0.12f) *
        juce::jlimit(0.0f, 1.0f, (0.42f - tuneLockStrength) / 0.14f);
    const auto releaseAgility =
        juce::jmax(fastReleaseAgility, juce::jmax(midReleaseAgility, slowHighReleaseAgility * 0.65f));
    const auto releaseDirectionalRun = releaseAgility > 0.35f ? 3 : 5;
    const auto releaseTrendThreshold = 0.040f - releaseAgility * 0.014f;

    if (!heldNoteIsReleased && pitchDirection != 0 && directionalRun >= releaseDirectionalRun &&
        detectedClarity > 0.72f - releaseAgility * 0.02f &&
        std::abs(pitchTrendSemitones) > releaseTrendThreshold)
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
          juce::jlimit(0.42f, 0.78f,
                       0.36f + static_cast<float>(directedSteps) * 0.080f +
                           upperRegisterConviction * 0.10f);

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
    if (upperRegisterConviction > 0.35f && tuneLockStrength < 0.58f && !heldNoteIsReleased)
      requiredHoldAnalyses = juce::jmax(requiredHoldAnalyses, 2);

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
  const auto targetMidi = findNearestScaleMidi(
      hasNoteCenterMidi ? noteCenterMidi : (hasNoteSelectMidi ? noteSelectMidi : detectedMidi), 0, 0, noteMask);
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
    shiftedBlendTarget = voiced ? 1.0f : 0.0f;
    formantPreserveTarget = 0.0f;
    const auto sibilanceProtect =
        juce::jlimit(0.0f, 1.0f, sibilanceGuard * (1.18f - confidenceLock * 0.30f));
    highPreserveTarget = juce::jlimit(0.0f, 0.68f, sibilanceProtect * 0.68f);
    airBoostTarget = 0.0f;
    const auto stableLock = juce::jlimit(0.0f, 1.0f, static_cast<float>(stableNoteAnalyses - 1) / 3.0f);
    const auto noteConviction =
        (voiced && noteTransitionFramesRemaining == 0 && targetLatchAnalyses == 0)
            ? stableLock * confidenceLock
            : 0.0f;
    const auto correctionCenterMidi = hasNoteCenterMidi ? noteCenterMidi : smoothedMidi;
    const auto upperRegisterTrim =
        juce::jlimit(0.0f, 1.0f, (correctionCenterMidi - 72.0f) / 8.0f) *
        juce::jlimit(0.0f, 1.0f, (0.92f - tuneLockStrength) / 0.20f);
    const auto registerAwareCorrectionGain =
        1.0f + (correctionGain - 1.0f) * (1.0f - upperRegisterTrim);
    const auto landingGain = 1.0f + (centerLockGain - 1.0f) * stableLock * confidenceLock;
    const auto appliedCorrectionGain =
        registerAwareCorrectionGain * landingGain * (1.0f + (correctionOvershoot - 1.0f) * confidenceLock);
    const auto applyCorrectionTolerance = [this](float errorOctaves)
    {
      const auto absError = std::abs(errorOctaves);
      if (absError <= correctionToleranceOctaves)
        return 0.0f;

      const auto kneeEnd = correctionToleranceOctaves + correctionKneeOctaves;
      if (absError >= kneeEnd)
        return errorOctaves;

      const auto kneePosition = (absError - correctionToleranceOctaves) / correctionKneeOctaves;
      const auto kneeGain = kneePosition * kneePosition * (3.0f - 2.0f * kneePosition);
      return errorOctaves * kneeGain;
    };
    auto targetRatioOctaves = -applyCorrectionTolerance(pitchErrorOctaves) * appliedCorrectionGain;

    auto tonalWeight = juce::jlimit(tonalCorrectionFloor, 1.0f, (detectedClarity - 0.52f) / 0.23f);
    if (onsetGuardAnalyses > 0 && detectedClarity < 0.88f && noteConviction < 0.50f)
      tonalWeight *= 0.68f + tuneLockStrength * 0.32f;
    tonalWeight += (1.0f - tonalWeight) * noteConviction;
    targetRatioOctaves *= tonalWeight;
    updateLpcFormantModels(wrapIndex(static_cast<float>(writeIndex) - static_cast<float>(latencySamples), bufferSize),
                           wrapIndex(readIndex1 + static_cast<float>(readAhead), bufferSize),
                           std::abs(targetRatioOctaves * 12.0f), confidenceLock);

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
  const auto spliceFade = juce::jlimit(1, fragmentSize, spliceFadeSamples);
  const auto spliceFadeForFrame = [this, spliceFade]()
  {
    if (fragmentIndex >= spliceFade)
      return 1.0f;

    const auto phase = static_cast<float>(fragmentIndex) / static_cast<float>(juce::jmax(1, spliceFade));
    return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * phase));
  };
  Result result;

  if (crossfading)
  {
    const auto fade = spliceFadeForFrame();
    result.left = readCubicWithAhead(0, readIndex1, previousReadAhead) * (1.0f - fade) +
                  readCubicWithAhead(0, readIndex2, readAhead) * fade;
    result.right = readCubicWithAhead(1, readIndex1, previousReadAhead) * (1.0f - fade) +
                   readCubicWithAhead(1, readIndex2, readAhead) * fade;
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
    readIndex2 = wrapIndex(readIndex2 + ratioStep, bufferSize);
  }
  else if (readAhead != previousReadAhead)
  {
    const auto fade = spliceFadeForFrame();
    result.left = readCubicWithAhead(0, readIndex1, previousReadAhead) * (1.0f - fade) +
                  readCubicWithAhead(0, readIndex1, readAhead) * fade;
    result.right = readCubicWithAhead(1, readIndex1, previousReadAhead) * (1.0f - fade) +
                   readCubicWithAhead(1, readIndex1, readAhead) * fade;
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
  }
  else
  {
    result.left = readCubicWithAhead(0, readIndex1, readAhead);
    result.right = readCubicWithAhead(1, readIndex1, readAhead);
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
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
  formantPreserveMix += (formantPreserveTarget - formantPreserveMix) * formantPreserveAlpha;
  lpcFormantMix += (lpcFormantTarget - lpcFormantMix) * lpcFormantAlpha;
  highPreserveMix += (highPreserveTarget - highPreserveMix) * highPreserveAlpha;
  airBoostGain += (airBoostTarget - airBoostGain) * airBoostAlpha;

  result.left = delayed.left + (result.left - delayed.left) * shiftedBlend;
  result.right = delayed.right + (result.right - delayed.right) * shiftedBlend;
  result.left = applyLpcFormantTransfer(0, result.left);
  result.right = applyLpcFormantTransfer(1, result.right);

  const auto preserveFormantBand = [this](int channel, float shifted, float delayedSample)
  {
    const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
    shiftedFormantLowpass[channelIndex] +=
        formantLowpassAlpha * (shifted - shiftedFormantLowpass[channelIndex]);
    shiftedFormantHighpass[channelIndex] +=
        formantHighpassAlpha * (shifted - shiftedFormantHighpass[channelIndex]);
    delayFormantLowpass[channelIndex] +=
        formantLowpassAlpha * (delayedSample - delayFormantLowpass[channelIndex]);
    delayFormantHighpass[channelIndex] +=
        formantHighpassAlpha * (delayedSample - delayFormantHighpass[channelIndex]);

    const auto shiftedBand = shiftedFormantHighpass[channelIndex] - shiftedFormantLowpass[channelIndex];
    const auto delayedBand = delayFormantHighpass[channelIndex] - delayFormantLowpass[channelIndex];
    return shifted + (delayedBand - shiftedBand) * formantPreserveMix;
  };

  result.left = preserveFormantBand(0, result.left, delayed.left);
  result.right = preserveFormantBand(1, result.right, delayed.right);

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

  const auto restoreAir = [this](int channel, float sample)
  {
    const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
    airBoostLowpass[channelIndex] += airBoostLowpassAlpha * (sample - airBoostLowpass[channelIndex]);
    const auto high = sample - airBoostLowpass[channelIndex];
    return airBoostLowpass[channelIndex] + high * (1.0f + airBoostGain);
  };

  result.left = restoreAir(0, result.left);
  result.right = restoreAir(1, result.right);

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

void FatTuneEngine::updateLpcFormantModels(float dryIndex, float shiftedIndex, float correctionSemitones,
                                           float confidence)
{
  const auto correctionWeight = juce::jlimit(0.0f, 1.0f, (correctionSemitones - 0.18f) / 1.55f);
  const auto sibilanceDodge = 1.0f - juce::jlimit(0.0f, 1.0f, sibilanceGuard * 1.30f);
  const auto target = voiced && filledSamples > latencySamples + lpcWindowSamples && confidence > 0.32f
                          ? correctionWeight * confidence * sibilanceDodge * 0.24f
                          : 0.0f;

  if (target < 0.006f)
  {
    lpcFormantTarget = 0.0f;
    return;
  }

  auto allModelsValid = true;
  for (auto channel = 0; channel < numChannels; ++channel)
  {
    auto dryCoefficients = std::array<float, lpcOrder + 1> {};
    auto shiftedCoefficients = std::array<float, lpcOrder + 1> {};
    const auto dryValid = computeLpcForChannel(channel, dryIndex, dryCoefficients);
    const auto shiftedValid = computeLpcForChannel(channel, shiftedIndex, shiftedCoefficients);

    if (!dryValid || !shiftedValid)
    {
      allModelsValid = false;
      break;
    }

    dryLpcTarget[static_cast<size_t>(channel)] = dryCoefficients;
    shiftedLpcTarget[static_cast<size_t>(channel)] = shiftedCoefficients;
  }

  lpcFormantTarget = allModelsValid ? juce::jlimit(0.0f, 0.24f, target) : 0.0f;
}

bool FatTuneEngine::computeLpcForChannel(int channel, float centerIndex,
                                         std::array<float, lpcOrder + 1>& coefficients) const
{
  coefficients.fill(0.0f);
  coefficients[0] = 1.0f;

  if (lpcWindow.empty() || lpcWindowSamples <= lpcOrder + 8 || filledSamples < latencySamples + lpcWindowSamples)
    return false;

  const auto channelIndex = juce::jlimit(0, numChannels - 1, channel);
  const auto windowSamples = juce::jlimit(lpcOrder + 8, 1024, lpcWindowSamples);
  std::array<float, 1024> frame {};
  const auto startIndex = centerIndex - static_cast<float>(windowSamples - 1);

  auto mean = 0.0;
  for (auto i = 0; i < windowSamples; ++i)
  {
    const auto sample = readCubic(channelIndex, startIndex + static_cast<float>(i));
    frame[static_cast<size_t>(i)] = sample;
    mean += static_cast<double>(sample);
  }
  mean /= static_cast<double>(windowSamples);

  for (auto i = 0; i < windowSamples; ++i)
  {
    const auto window = lpcWindow[static_cast<size_t>(i)];
    frame[static_cast<size_t>(i)] = static_cast<float>((static_cast<double>(frame[static_cast<size_t>(i)]) - mean) *
                                                       static_cast<double>(window));
  }

  std::array<double, lpcOrder + 1> autocorrelation {};
  for (auto lag = 0; lag <= lpcOrder; ++lag)
  {
    auto sum = 0.0;
    for (auto i = lag; i < windowSamples; ++i)
      sum += static_cast<double>(frame[static_cast<size_t>(i)]) *
             static_cast<double>(frame[static_cast<size_t>(i - lag)]);
    autocorrelation[static_cast<size_t>(lag)] = sum;
  }

  if (autocorrelation[0] < 0.00000018)
    return false;

  autocorrelation[0] *= 1.0008;
  std::array<double, lpcOrder + 1> a {};
  std::array<double, lpcOrder + 1> previous {};
  a[0] = 1.0;
  auto predictionError = autocorrelation[0];

  for (auto order = 1; order <= lpcOrder; ++order)
  {
    auto reflection = autocorrelation[static_cast<size_t>(order)];
    for (auto i = 1; i < order; ++i)
      reflection += a[static_cast<size_t>(i)] * autocorrelation[static_cast<size_t>(order - i)];

    reflection = -reflection / juce::jmax(predictionError, 0.000000001);
    if (!std::isfinite(reflection))
      return false;

    reflection = juce::jlimit(-0.82, 0.82, reflection);
    previous = a;
    a[static_cast<size_t>(order)] = reflection;
    for (auto i = 1; i < order; ++i)
      a[static_cast<size_t>(i)] =
          previous[static_cast<size_t>(i)] + reflection * previous[static_cast<size_t>(order - i)];

    predictionError *= 1.0 - reflection * reflection;
    if (predictionError < autocorrelation[0] * 0.00001)
      break;
  }

  for (auto i = 0; i <= lpcOrder; ++i)
    coefficients[static_cast<size_t>(i)] =
        static_cast<float>(juce::jlimit(-3.0, 3.0, a[static_cast<size_t>(i)]));

  coefficients[0] = 1.0f;
  return true;
}

float FatTuneEngine::applyLpcFormantTransfer(int channel, float shifted)
{
  const auto channelIndex = static_cast<size_t>(juce::jlimit(0, numChannels - 1, channel));
  auto& dryCoefficients = dryLpc[channelIndex];
  auto& shiftedCoefficients = shiftedLpc[channelIndex];
  const auto& dryTargets = dryLpcTarget[channelIndex];
  const auto& shiftedTargets = shiftedLpcTarget[channelIndex];

  for (auto i = 1; i <= lpcOrder; ++i)
  {
    dryCoefficients[static_cast<size_t>(i)] +=
        (dryTargets[static_cast<size_t>(i)] - dryCoefficients[static_cast<size_t>(i)]) * lpcCoeffAlpha;
    shiftedCoefficients[static_cast<size_t>(i)] +=
        (shiftedTargets[static_cast<size_t>(i)] - shiftedCoefficients[static_cast<size_t>(i)]) * lpcCoeffAlpha;
  }

  auto& inputHistory = lpcInputHistory[channelIndex];
  auto& outputHistory = lpcOutputHistory[channelIndex];
  auto residual = static_cast<double>(shifted);
  for (auto i = 1; i <= lpcOrder; ++i)
    residual += static_cast<double>(shiftedCoefficients[static_cast<size_t>(i)]) *
                static_cast<double>(inputHistory[static_cast<size_t>(i - 1)]);

  auto synthesized = residual;
  for (auto i = 1; i <= lpcOrder; ++i)
    synthesized -= static_cast<double>(dryCoefficients[static_cast<size_t>(i)]) *
                   static_cast<double>(outputHistory[static_cast<size_t>(i - 1)]);

  if (!std::isfinite(synthesized))
    synthesized = shifted;

  const auto maxDelta = 0.18 + std::abs(static_cast<double>(shifted)) * 1.70;
  synthesized = static_cast<double>(shifted) +
                juce::jlimit(-maxDelta, maxDelta, synthesized - static_cast<double>(shifted));

  for (auto i = lpcOrder - 1; i > 0; --i)
  {
    inputHistory[static_cast<size_t>(i)] = inputHistory[static_cast<size_t>(i - 1)];
    outputHistory[static_cast<size_t>(i)] = outputHistory[static_cast<size_t>(i - 1)];
  }
  inputHistory[0] = shifted;
  outputHistory[0] = static_cast<float>(synthesized);

  const auto mixed = static_cast<double>(shifted) + (synthesized - static_cast<double>(shifted)) *
                                                       static_cast<double>(lpcFormantMix);
  return std::isfinite(mixed) ? static_cast<float>(mixed) : shifted;
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

    for (auto channel = 0; channel < numChannels; ++channel)
    {
      const auto referenceSample = readCubic(channel, reference);
      const auto candidateSample = readCubic(channel, candidate);
      const auto referenceSlope = readCubic(channel, reference + 1.0f) - readCubic(channel, reference - 1.0f);
      const auto candidateSlope = readCubic(channel, candidate + 1.0f) - readCubic(channel, candidate - 1.0f);
      const auto sampleError = referenceSample - candidateSample;
      const auto slopeError = referenceSlope - candidateSlope;
      score += sampleError * sampleError * 10.0f + slopeError * slopeError * 2.5f;
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

float FatTuneEngine::readDelaySample(int channel, float delay) const
{
  return readCubic(channel, static_cast<float>(writeIndex) - delay);
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
