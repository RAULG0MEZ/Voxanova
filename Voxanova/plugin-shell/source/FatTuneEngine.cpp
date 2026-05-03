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
  return 3.5f + std::pow(1.0f - clampedAmount, 2.65f) * 396.5f;
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
    detectorSize = 2048;
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
  for (auto i = 0; i < fragmentSize; ++i)
    crossfade[static_cast<size_t>(i)] =
        0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * static_cast<float>(i) /
                                static_cast<float>(juce::jmax(1, fragmentSize))));

  detectorBuffer.assign(static_cast<size_t>(detectorSize), 0.0f);
  detectorScratch.assign(static_cast<size_t>(detectorSize), 0.0f);
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
  correctionFilter = 0.12f;
  correctionGain = 1.0f;
  correctionOvershoot = 1.0f;
  noteBias = 0.038f;
  detectorFollowFast = 0.45f;
  detectorFollowSlow = 0.28f;
  analysisIntervalFragments = 4;
  lastNote = -1;
  detectedNoteBits = 0;
  unvoicedCounter = 5;
  voiced = false;

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
  const auto noteMask = makeScaleMask(settings.key, settings.scale, settings.customMask);
  const auto correctionActive = settings.enabled && noteMask != 0;

  configureCorrection(settings);

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
  const auto amountNorm = settings.enabled ? juce::jlimit(0.0f, 1.0f, settings.amount / 100.0f) : 0.0f;
  const auto retuneSeconds = retuneMillisecondsForAmount(amountNorm) / 1000.0f;
  const auto pitchLock = std::pow(amountNorm, 1.35f);
  correctionFilter =
      juce::jlimit(0.015f, 1.0f, static_cast<float>((4.0 * fragmentSize) / (retuneSeconds * sampleRate)));
  correctionGain = settings.enabled ? 1.0f : 0.0f;
  correctionOvershoot = settings.enabled ? 1.0f + pitchLock * 0.085f : 1.0f;
  noteBias = juce::jlimit(0.004f, 0.50f / 13.0f, (0.50f / 13.0f) * (1.0f - 0.86f * pitchLock));
  detectorFollowFast = 0.45f + pitchLock * 0.42f;
  detectorFollowSlow = 0.30f + pitchLock * 0.39f;
  analysisIntervalFragments = amountNorm >= 0.70f ? 2 : 4;
  fastMode = false;
}

void FatTuneEngine::pushDetectorSample(float sample)
{
  const auto filtered = processOnePoleLowpass(sample, 1800.0f, detectorLowpass);
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
    }
    return;
  }

  const auto detectorRate = static_cast<float>(sampleRate / static_cast<double>(detectorDecimation));
  const auto minTau = juce::jlimit(2, detectorSize - 4, juce::roundToInt(detectorRate / 1200.0f));
  const auto maxTau = juce::jlimit(minTau + 2, detectorSize - 3, juce::roundToInt(detectorRate / 60.0f));

  auto bestTau = 0;
  auto bestValue = 0.0f;
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
      const auto midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
      const auto continuityPenalty = hasSmoothedMidi ? std::abs(midi - continuityMidi) * 0.032f : 0.0f;
      const auto lowTauPreference = static_cast<float>(tau) / static_cast<float>(maxTau) * 0.020f;
      const auto score = value + lowTauPreference - continuityPenalty;

      if (score > bestScore)
      {
        bestScore = score;
        bestTau = tau;
        bestValue = value;
      }
    }

    previousValue = value;
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
    }
    return;
  }

  const auto nextCycleSamples = static_cast<float>(bestTau * detectorDecimation);
  const auto nextFrequency = static_cast<float>(sampleRate) / nextCycleSamples;
  auto nextMidi = 69.0f + 12.0f * std::log2(nextFrequency / 440.0f);

  if (hasSmoothedMidi && std::abs(nextMidi - smoothedMidi) > 7.0f)
  {
    auto foldedMidi = nextMidi;
    auto foldedDistance = std::abs(foldedMidi - smoothedMidi);
    for (auto octave = -2; octave <= 2; ++octave)
    {
      const auto candidate = nextMidi + static_cast<float>(octave * 12);
      const auto distance = std::abs(candidate - smoothedMidi);
      if (distance < foldedDistance)
      {
        foldedDistance = distance;
        foldedMidi = candidate;
      }
    }

    if (foldedDistance < std::abs(nextMidi - smoothedMidi))
      nextMidi = foldedMidi;
  }

  if (!hasSmoothedMidi || std::abs(nextMidi - smoothedMidi) > 7.0f)
    smoothedMidi = nextMidi;
  else
    smoothedMidi += (nextMidi - smoothedMidi) * (bestValue > 0.82f ? detectorFollowFast : detectorFollowSlow);

  cycleSamples = static_cast<float>(sampleRate) / (440.0f * std::pow(2.0f, (smoothedMidi - 69.0f) / 12.0f));
  detectedFrequency = static_cast<float>(sampleRate) / cycleSamples;
  detectedClarity = bestValue;
  hasSmoothedMidi = true;
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
    return;
  }

  const auto f = std::log2(static_cast<float>(sampleRate) / (cycleSamples * 440.0f));
  auto bestAbs = 1.0f;
  auto bestDelta = 0.0f;
  auto bestNote = -1;

  for (auto note = 0, bit = 1; note < 12; ++note, bit <<= 1)
  {
    if ((noteMask & bit) == 0)
      continue;

    auto delta = f - (static_cast<float>(note) - 9.0f) / 12.0f;
    delta -= std::floor(delta + 0.5f);
    auto absDelta = std::abs(delta);
    if (note == lastNote)
      absDelta -= noteBias;

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
    return;
  }

  if (lastNote == bestNote)
    pitchErrorOctaves += correctionFilter * (bestDelta - pitchErrorOctaves);
  else
    pitchErrorOctaves = bestDelta;

  lastNote = bestNote;
  detectedNoteBits |= 1 << bestNote;

  const auto detectedMidi = 69.0f + 12.0f * std::log2(detectedFrequency / 440.0f);
  const auto targetMidi = findNearestScaleMidi(detectedMidi, 0, 0, noteMask);
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
    const auto appliedCorrectionGain = correctionGain * (1.0f + (correctionOvershoot - 1.0f) * confidenceLock);
    pitchRatio = std::pow(2.0f, -pitchErrorOctaves * appliedCorrectionGain);
    pitchRatio = juce::jlimit(0.50f, 2.0f, pitchRatio);
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
    readIndex2 = wrapIndex(readIndex1 - periodJump, bufferSize);
  }
  else if (distance > targetDistance + tolerance)
  {
    crossfading = true;
    readIndex2 = wrapIndex(readIndex1 + periodJump, bufferSize);
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

  if (crossfading)
  {
    const auto fade = crossfade[static_cast<size_t>(juce::jlimit(0, fragmentSize - 1, fragmentIndex))];
    result.left = readCubicWithAhead(0, readIndex1, previousReadAhead) * (1.0f - fade) +
                  readCubicWithAhead(0, readIndex2, readAhead) * fade;
    result.right = readCubicWithAhead(1, readIndex1, previousReadAhead) * (1.0f - fade) +
                   readCubicWithAhead(1, readIndex2, readAhead) * fade;
    readIndex1 = wrapIndex(readIndex1 + ratioStep, bufferSize);
    readIndex2 = wrapIndex(readIndex2 + ratioStep, bufferSize);
  }
  else if (readAhead != previousReadAhead)
  {
    const auto fade = crossfade[static_cast<size_t>(juce::jlimit(0, fragmentSize - 1, fragmentIndex))];
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
