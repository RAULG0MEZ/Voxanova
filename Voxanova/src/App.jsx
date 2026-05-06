import React from "react";
import {
  booleanParameters,
  autoTuneNotes,
  autoTuneScales,
  autoTuneVoiceTypes,
  defaultValues,
  delayDivisions,
  delayStyles,
  EQ_FILTER_TYPES,
  reverbPredelayDivisions,
  reverbModes,
  PLUGIN_HEIGHT,
  PLUGIN_WIDTH
} from "./pluginContract.js";
import { hasNativeBackend, sendNativeEditorSize, sendNativeEqBands, sendNativeParameter } from "./nativeBridge.js";

import { useTweaks, TweaksPanel, TweakSection, TweakSelect } from "./v2/tweaksPanel.jsx";
import { EQCurve } from "./v2/eqCurve.jsx";
import { CompModule, ButterCompModule, PctModule, GateModule, DeEsserModule, StereoModule, DelayModule, ReverbModule } from "./v2/modules.jsx";
import { AutoTuneModule } from "./v2/ioModules.jsx";
import { resetOnAltClick, resetOnDoubleClick } from "./v2/controlReset.js";
import { handleWheelValue } from "./v2/wheelControl.js";

const { useState, useEffect, useRef, useCallback, useMemo } = React;

const booleanParameterSet = new Set(booleanParameters);
const noteModeToValue = { NOTE: 0, DOT: 1, TRIP: 2 };
const noteValueToMode = ["NOTE", "DOT", "TRIP"];
const delayTypeToValue = { NORMAL: 0, WIDE: 1, "PING-PONG": 2 };
const delayValueToType = ["NORMAL", "WIDE", "PING-PONG"];
const delayDivisionMax = delayDivisions.length - 1;
const reverbPredelayDivisionMax = reverbPredelayDivisions.length - 1;
const FULL_SPECTRUM_TYPE = "Full Spectrum";
const FULL_SPECTRUM_MIN_RATIO = 1.015;
const glueBandIds = ["glueLowThreshold", "glueLowMidThreshold", "glueHighMidThreshold", "glueAirThreshold"];
const eqFilterTypeSet = new Set(EQ_FILTER_TYPES);
const eqDynamicTypeSet = new Set(["Bell", "Surfer Bell", "Low Shelf", "High Shelf", "Band Pass", FULL_SPECTRUM_TYPE]);
const eqCutTypeSet = new Set(["Low Cut", "High Cut"]);

const REAL_EQ_SOURCE_HEIGHT = 351;
const REAL_EQ_SOURCE_Y = 82;
const DEFAULT_SPECTRUM_MAX_FREQUENCY = 20000;
const REAL_RACK_SOURCE_Y = 433;

const layoutTargets = {
  normal: { eqGraphHeight: 320, realEqHeight: 351, realRackHeight: 347, eqSectionPadTop: 12, eqSectionPadBottom: 6 },
  eq: { eqGraphHeight: 674, realEqHeight: 698, realRackHeight: 0, eqSectionPadTop: 10, eqSectionPadBottom: 2 },
};

const EQ_RACK_RESIZE_RANGE = layoutTargets.eq.realEqHeight - layoutTargets.normal.realEqHeight;
const resolveLayoutKey = (focus) => (focus === 'eq' ? focus : 'normal');
const mixLayoutValue = (from, to, t) => from + (to - from) * t;

function getLayoutMetrics(progress) {
  const eased = clamp(Number(progress) || 0, 0, 1);
  const from = layoutTargets.normal;
  const target = layoutTargets.eq;

  return {
    eqGraphHeight: mixLayoutValue(from.eqGraphHeight, target.eqGraphHeight, eased),
    realEqHeight: mixLayoutValue(from.realEqHeight, target.realEqHeight, eased),
    realRackHeight: mixLayoutValue(from.realRackHeight, target.realRackHeight, eased),
    eqSectionPadTop: mixLayoutValue(from.eqSectionPadTop, target.eqSectionPadTop, eased),
    eqSectionPadBottom: mixLayoutValue(from.eqSectionPadBottom, target.eqSectionPadBottom, eased),
  };
}

function getLayoutStyle(metrics) {
  const eqScale = metrics.realEqHeight / REAL_EQ_SOURCE_HEIGHT;

  return {
    '--eq-graph-height': `${metrics.eqGraphHeight.toFixed(3)}px`,
    '--eq-section-pad-top': `${metrics.eqSectionPadTop.toFixed(3)}px`,
    '--eq-section-pad-bottom': `${metrics.eqSectionPadBottom.toFixed(3)}px`,
    '--real-eq-height': `${metrics.realEqHeight.toFixed(3)}px`,
    '--real-rack-height': `${metrics.realRackHeight.toFixed(3)}px`,
    '--real-eq-bg-height': `${(PLUGIN_HEIGHT * eqScale).toFixed(3)}px`,
    '--real-eq-bg-y': `${(-REAL_EQ_SOURCE_Y * eqScale).toFixed(3)}px`,
    '--real-rack-bg-height': `${PLUGIN_HEIGHT}px`,
    '--real-rack-bg-y': `${-REAL_RACK_SOURCE_Y}px`,
  };
}

const reverbModeByV2Preset = {
  "Concert Hall": 0,
  Plate: 2,
  Room: 3,
  Chamber: 4,
  Cathedral: 18,
  Spring: 2,
};
const v2PresetByReverbMode = {
  0: "Concert Hall",
  2: "Plate",
  3: "Room",
  4: "Chamber",
  18: "Cathedral",
};
const DEFAULT_EQ_BELL_Q = 1;
const DEFAULT_EQ_Q = 5;
const DEFAULT_EQ_SHELF_Q = 1.3;
const DEFAULT_EQ_LOW_CUT_SLOPE = 30;
const DEFAULT_FULL_SPECTRUM_SLOPE = 8;
const EQ_BAND_SATURATION_MAX_MODE = 3;
function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function boolFromParam(value) {
  return value === true || value >= 0.5;
}

function valueToRange(percent, min, max) {
  return min + (clamp(Number(percent) || 0, 0, 100) / 100) * (max - min);
}

function rangeToPercent(value, min, max) {
  return clamp(((Number(value) - min) / (max - min)) * 100, 0, 100);
}

function defaultEqQForType(type) {
  if (type === "Bell") return DEFAULT_EQ_BELL_Q;
  if (type === FULL_SPECTRUM_TYPE) return DEFAULT_FULL_SPECTRUM_SLOPE;
  return type === "Low Shelf" || type === "High Shelf" ? DEFAULT_EQ_SHELF_Q : DEFAULT_EQ_Q;
}

function defaultEqSlopeForType(type) {
  return type === "Low Cut" ? DEFAULT_EQ_LOW_CUT_SLOPE : 12;
}

function normalizeEqSlope(value, type = "Bell") {
  if (String(value).toLowerCase() === "wall") return "wall";
  const numeric = Number(value);
  return clamp(Math.round(Number.isFinite(numeric) && numeric > 0 ? numeric : defaultEqSlopeForType(type)), 6, 96);
}

function fullSpectrumFallbackRange(freq) {
  const center = clamp(Number(freq) || 1000, 20, 20000);
  const halfOctaves = 1;
  return {
    low: clamp(center / (2 ** halfOctaves), 20, 20000),
    high: clamp(center * (2 ** halfOctaves), 20, 20000)
  };
}

function normalizeFullSpectrumRange(point, freq) {
  const fallback = fullSpectrumFallbackRange(freq);
  let low = Number(point?.rangeLow);
  let high = Number(point?.rangeHigh);

  if (!Number.isFinite(low) || !Number.isFinite(high) || low <= 0 || high <= 0 || high <= low * FULL_SPECTRUM_MIN_RATIO) {
    low = fallback.low;
    high = fallback.high;
  }

  low = clamp(low, 20, 20000);
  high = clamp(high, 20, 20000);

  if (high <= low * FULL_SPECTRUM_MIN_RATIO) {
    const center = clamp(Number(freq) || Math.sqrt(Math.max(20, low) * Math.max(20, high)), 20, 20000);
    const halfRatio = Math.sqrt(FULL_SPECTRUM_MIN_RATIO);
    low = clamp(center / halfRatio, 20, 20000 / FULL_SPECTRUM_MIN_RATIO);
    high = clamp(center * halfRatio, low * FULL_SPECTRUM_MIN_RATIO, 20000);
  }

  const center = clamp(Math.sqrt(low * high), 20, 20000);
  return { low, high, center };
}

function normalizeEqPoint(point) {
  const type = point?.type || "Bell";
  if (type === "Desser") return null;

  const safeType = eqFilterTypeSet.has(type) ? type : "Bell";
  const surfRatio = Number(point?.surfRatio);
  const q = Number(point?.q);
  const rawGain = clamp(Number(point?.gain) || 0, -30, 30);
  const gain = eqCutTypeSet.has(safeType) ? clamp(rawGain, 0, 30) : rawGain;
  const comp = clamp(Number(point?.comp) || 0, -30, 30);
  const hasExplicitCompEnabled = Object.prototype.hasOwnProperty.call(point || {}, "compEnabled");
  const explicitCompEnabled = point?.compEnabled === true || point?.compEnabled === "true" || Number(point?.compEnabled) >= 0.5;
  const legacyCompEnabled = Math.abs(comp) > 0.05 && Math.abs(comp - gain) > 0.05;
  const compThreshold = Number(point?.compThreshold);
  const compAttack = Number(point?.compAttack);
  const compRelease = Number(point?.compRelease);
  const compRatio = Number(point?.compRatio);
  const saturationMode = Number(point?.saturationMode ?? point?.satMode);
  const saturationAmount = Number(point?.saturationAmount ?? point?.satAmount);
  const rangeLowSlope = Number(point?.rangeLowSlope ?? point?.lowSlope);
  const rangeHighSlope = Number(point?.rangeHighSlope ?? point?.highSlope);

  const safeFreq = clamp(Math.round(Number(point?.freq) || 1000), 20, 20000);
  const safeQ = clamp(Number.isFinite(q) && q > 0 ? q : defaultEqQForType(safeType), 0.1, 50);
  const fullSpectrumRange = safeType === FULL_SPECTRUM_TYPE
    ? normalizeFullSpectrumRange(point, safeFreq)
    : null;
  const safeSaturationMode = eqDynamicTypeSet.has(safeType)
    ? clamp(Math.round(Number.isFinite(saturationMode) ? saturationMode : 0), 0, EQ_BAND_SATURATION_MAX_MODE)
    : 0;

  return {
    type: safeType,
    freq: fullSpectrumRange ? Math.round(fullSpectrumRange.center) : safeFreq,
    gain,
    q: safeQ,
    slope: normalizeEqSlope(point?.slope, safeType),
    threshold: clamp(Number(point?.threshold) || -24, -60, 0),
    intensity: clamp(Number(point?.intensity) || 50, 0, 100),
    deessMode: point?.deessMode === "wider" ? "wider" : "split",
    on: point?.on !== false,
    solo: point?.solo === true || point?.solo === "true" || Number(point?.solo) >= 0.5,
    placement: point?.placement || "stereo",
    comp,
    compEnabled: eqDynamicTypeSet.has(safeType) && (hasExplicitCompEnabled ? explicitCompEnabled : legacyCompEnabled),
    compThreshold: clamp(Number.isFinite(compThreshold) ? compThreshold : -18, -60, 0),
    compAttack: clamp(Number.isFinite(compAttack) ? compAttack : 12, 0.1, 200),
    compRelease: clamp(Number.isFinite(compRelease) ? compRelease : 140, 5, 1000),
    compRatio: clamp(Number.isFinite(compRatio) ? compRatio : 4, 1, 20),
    saturationMode: safeSaturationMode,
    saturationAmount: safeSaturationMode > 0
      ? clamp(Number.isFinite(saturationAmount) ? saturationAmount : 20, 0, 100)
      : 0,
    ...(fullSpectrumRange ? {
      rangeLow: Math.round(fullSpectrumRange.low),
      rangeHigh: Math.round(fullSpectrumRange.high),
      rangeLowSlope: clamp(Number.isFinite(rangeLowSlope) && rangeLowSlope > 0 ? rangeLowSlope : safeQ, 0.1, 50),
      rangeHighSlope: clamp(Number.isFinite(rangeHighSlope) && rangeHighSlope > 0 ? rangeHighSlope : safeQ, 0.1, 50)
    } : {}),
    ...(safeType === "Surfer Bell" && Number.isFinite(surfRatio) && surfRatio > 0 ? { surfRatio } : {})
  };
}

function normalizeEqPoints(points) {
  return Array.isArray(points)
    ? points.map(normalizeEqPoint).filter(Boolean).sort((a, b) => a.freq - b.freq)
    : [];
}

function eqBandsFromPayload(payload) {
  let raw = payload?.eqBands;

  if (typeof raw === "string") {
    try {
      raw = JSON.parse(raw);
    } catch {
      raw = null;
    }
  }

  if (!raw || typeof raw !== "object") return null;

  return {
    pre: normalizeEqPoints(raw.pre),
    post: normalizeEqPoints(raw.post)
  };
}

function updateValuesFromPayload(current, payload) {
  let changed = false;
  const next = { ...current };

  Object.keys(defaultValues).forEach((id) => {
    if (typeof payload[id] !== "number") return;
    changed = true;
    next[id] = booleanParameterSet.has(id) ? payload[id] >= 0.5 : payload[id];
  });

  return changed ? next : current;
}

function presetIndex(labels, label) {
  return Math.max(0, labels.findIndex((item) => item.toLowerCase() === String(label).toLowerCase()));
}

const makeEmptyWaveform = () => Array.from({ length: 256 }, () => 0);
const makeEmptySpectrum = () => Array.from({ length: 256 }, () => 0);
const makeEmptyEqDetectorDb = () => Array.from({ length: 128 }, () => -120);

const emptyMeters = {
  inputLevel: 0,
  outputLevel: 0,
  visualSilence: true,
  inputWaveform: makeEmptyWaveform(),
  peakWaveform: makeEmptyWaveform(),
  peakOutputWaveform: makeEmptyWaveform(),
  glueWaveform: makeEmptyWaveform(),
  glueOutputWaveform: makeEmptyWaveform(),
  faceWaveform: makeEmptyWaveform(),
  faceOutputWaveform: makeEmptyWaveform(),
  gateWaveform: makeEmptyWaveform(),
  gateOutputWaveform: makeEmptyWaveform(),
  inputSpectrum: makeEmptySpectrum(),
  preCompSpectrum: makeEmptySpectrum(),
  postCompSpectrum: makeEmptySpectrum(),
  preEqDetectorDb: makeEmptyEqDetectorDb(),
  postEqDetectorDb: makeEmptyEqDetectorDb(),
  gateReduction: 0,
  peakReduction: 0,
  glueReduction: 0,
  glueBandReduction: [0, 0, 0, 0],
  faceReduction: 0,
  gateReductionDb: 0,
  peakReductionDb: 0,
  glueReductionDb: 0,
  glueBandReductionDb: [0, 0, 0, 0],
  faceReductionDb: 0,
  gateLevel: 0,
  peakLevel: 0,
  glueLevel: 0,
  faceLevel: 0,
  tuneFrequency: 0,
  tuneCents: 0,
  tuneConfidence: 0,
  tuneTargetMidi: 0,
  spectrumMaxFrequency: DEFAULT_SPECTRUM_MAX_FREQUENCY,
  sampleRate: 48000,
};

function numberOrZero(value) {
  const numeric = Number(value);
  return Number.isFinite(numeric) ? numeric : 0;
}

function fadeMeterValue(value, factor = 0.72, floor = 0.0008) {
  const next = numberOrZero(value) * factor;
  return Math.abs(next) <= floor ? 0 : next;
}

function fadeMeterArray(values, fallbackLength = 0, factor = 0.72, floor = 0.0008) {
  const source = Array.isArray(values) ? values : Array.from({ length: fallbackLength }, () => 0);
  return source.map((value) => fadeMeterValue(value, factor, floor));
}

function fadeDetectorDbArray(values, fallbackLength = 0, factor = 0.76) {
  const source = Array.isArray(values) ? values : Array.from({ length: fallbackLength }, () => -120);
  return source.map((value) => {
    const next = -120 + (numberOrZero(value) + 120) * factor;
    return next <= -119.5 ? -120 : next;
  });
}

function metersFromPayload(current, payload) {
  const arrayFromPayload = (id) => (
    Array.isArray(payload[id]) ? payload[id].map(numberOrZero) : current[id]
  );
  const parsedMaxFrequency = numberOrZero(payload.spectrumMaxFrequency);
  const safeSpectrumMaxFrequency = Number.isFinite(parsedMaxFrequency) && parsedMaxFrequency >= 20.0
    ? Math.min(20000, parsedMaxFrequency)
    : current.spectrumMaxFrequency;
  const parsedSampleRate = numberOrZero(payload.sampleRate);
  const safeSampleRate = Number.isFinite(parsedSampleRate) && parsedSampleRate >= 1000
    ? parsedSampleRate
    : current.sampleRate;
  const visualSilence = payload.visualSilence === true || payload.visualSilence === 1;
  const meterStale = payload.meterStale === true || payload.meterStale === 1;

  if (visualSilence) {
    return {
      inputLevel: fadeMeterValue(current.inputLevel, 0.62, 0.002),
      outputLevel: fadeMeterValue(current.outputLevel, 0.62, 0.002),
      visualSilence: true,
      inputWaveform: fadeMeterArray(current.inputWaveform, 256, 0.70),
      peakWaveform: fadeMeterArray(current.peakWaveform, 256, 0.70),
      peakOutputWaveform: fadeMeterArray(current.peakOutputWaveform, 256, 0.70),
      glueWaveform: fadeMeterArray(current.glueWaveform, 256, 0.70),
      glueOutputWaveform: fadeMeterArray(current.glueOutputWaveform, 256, 0.70),
      faceWaveform: fadeMeterArray(current.faceWaveform, 256, 0.70),
      faceOutputWaveform: fadeMeterArray(current.faceOutputWaveform, 256, 0.70),
      gateWaveform: fadeMeterArray(current.gateWaveform, 256, 0.70),
      gateOutputWaveform: fadeMeterArray(current.gateOutputWaveform, 256, 0.70),
      inputSpectrum: meterStale
        ? fadeMeterArray(current.inputSpectrum, 256, 0.92, 0.0008)
        : arrayFromPayload("inputSpectrum"),
      preCompSpectrum: meterStale
        ? fadeMeterArray(current.preCompSpectrum, 256, 0.92, 0.0008)
        : arrayFromPayload("preCompSpectrum"),
      postCompSpectrum: meterStale
        ? fadeMeterArray(current.postCompSpectrum, 256, 0.92, 0.0008)
        : arrayFromPayload("postCompSpectrum"),
      preEqDetectorDb: meterStale
        ? fadeDetectorDbArray(current.preEqDetectorDb, 128)
        : arrayFromPayload("preEqDetectorDb"),
      postEqDetectorDb: meterStale
        ? fadeDetectorDbArray(current.postEqDetectorDb, 128)
        : arrayFromPayload("postEqDetectorDb"),
      spectrumMaxFrequency: safeSpectrumMaxFrequency,
      sampleRate: safeSampleRate,
      gateReduction: fadeMeterValue(current.gateReduction, 0.58, 0.001),
      peakReduction: fadeMeterValue(current.peakReduction, 0.58, 0.001),
      glueReduction: fadeMeterValue(current.glueReduction, 0.58, 0.001),
      glueBandReduction: fadeMeterArray(current.glueBandReduction, 4, 0.58, 0.001),
      faceReduction: fadeMeterValue(current.faceReduction, 0.58, 0.001),
      gateReductionDb: fadeMeterValue(current.gateReductionDb, 0.58, 0.05),
      peakReductionDb: fadeMeterValue(current.peakReductionDb, 0.58, 0.05),
      glueReductionDb: fadeMeterValue(current.glueReductionDb, 0.58, 0.05),
      glueBandReductionDb: fadeMeterArray(current.glueBandReductionDb, 4, 0.58, 0.05),
      faceReductionDb: fadeMeterValue(current.faceReductionDb, 0.58, 0.05),
      gateLevel: fadeMeterValue(current.gateLevel, 0.62, 0.1),
      peakLevel: fadeMeterValue(current.peakLevel, 0.62, 0.1),
      glueLevel: fadeMeterValue(current.glueLevel, 0.62, 0.1),
      faceLevel: fadeMeterValue(current.faceLevel, 0.62, 0.1),
      tuneFrequency: fadeMeterValue(current.tuneFrequency, 0.62, 0.5),
      tuneCents: fadeMeterValue(current.tuneCents, 0.62, 0.2),
      tuneConfidence: fadeMeterValue(current.tuneConfidence, 0.50, 1.0),
      tuneTargetMidi: current.tuneConfidence > 1 ? numberOrZero(current.tuneTargetMidi) : 0,
    };
  }

  return {
    inputLevel: Math.max(numberOrZero(payload.inputL), numberOrZero(payload.inputR)),
    outputLevel: Math.max(numberOrZero(payload.outputL), numberOrZero(payload.outputR)),
    visualSilence,
    inputWaveform: arrayFromPayload("inputWaveform"),
    peakWaveform: arrayFromPayload("peakWaveform"),
    peakOutputWaveform: arrayFromPayload("peakOutputWaveform"),
    glueWaveform: arrayFromPayload("glueWaveform"),
    glueOutputWaveform: arrayFromPayload("glueOutputWaveform"),
    faceWaveform: arrayFromPayload("faceWaveform"),
    faceOutputWaveform: arrayFromPayload("faceOutputWaveform"),
    gateWaveform: arrayFromPayload("gateWaveform"),
    gateOutputWaveform: arrayFromPayload("gateOutputWaveform"),
    inputSpectrum: arrayFromPayload("inputSpectrum"),
    preCompSpectrum: arrayFromPayload("preCompSpectrum"),
    postCompSpectrum: arrayFromPayload("postCompSpectrum"),
    preEqDetectorDb: arrayFromPayload("preEqDetectorDb"),
    postEqDetectorDb: arrayFromPayload("postEqDetectorDb"),
    spectrumMaxFrequency: safeSpectrumMaxFrequency,
    sampleRate: safeSampleRate,
    gateReduction: numberOrZero(payload.gateGr),
    peakReduction: numberOrZero(payload.peakGr),
    glueReduction: numberOrZero(payload.glueGr),
    glueBandReduction: arrayFromPayload("glueBandGr"),
    faceReduction: numberOrZero(payload.faceGr),
    gateReductionDb: numberOrZero(payload.gateGrDb),
    peakReductionDb: numberOrZero(payload.peakGrDb),
    glueReductionDb: numberOrZero(payload.glueGrDb),
    glueBandReductionDb: arrayFromPayload("glueBandGrDb"),
    faceReductionDb: numberOrZero(payload.faceGrDb),
    gateLevel: numberOrZero(payload.gateLevel),
    peakLevel: numberOrZero(payload.peakLevel),
    glueLevel: numberOrZero(payload.glueLevel),
    faceLevel: numberOrZero(payload.faceLevel),
    tuneFrequency: numberOrZero(payload.tuneFrequency),
    tuneCents: numberOrZero(payload.tuneCents),
    tuneConfidence: numberOrZero(payload.tuneConfidence),
    tuneTargetMidi: numberOrZero(payload.tuneTargetMidi),
  };
}

// Voxanova V2 — main app
const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "theme": "real-noir",
  "showWaveform": true,
  "signalActive": true
}/*EDITMODE-END*/;

// Night-only theme catalogue. Each entry maps a `data-theme` value to its
// display label, mode flag, and a swatch gradient used in the picker.
// The actual color palette lives in styles.css under [data-theme="…"].
const THEMES = [
  { id: 'midnight', label: 'Midnight', mode: 'dark',  accent: 'oklch(68% 0.16 248)',
    swatch: 'linear-gradient(135deg, oklch(22% 0.014 268) 0%, oklch(40% 0.14 248) 100%)' },
  { id: 'plum',     label: 'Plum',     mode: 'dark',  accent: 'oklch(72% 0.18 320)',
    swatch: 'linear-gradient(135deg, oklch(20% 0.024 310) 0%, oklch(46% 0.18 320) 100%)' },
  { id: 'forest',   label: 'Forest',   mode: 'dark',  accent: 'oklch(72% 0.16 165)',
    swatch: 'linear-gradient(135deg, oklch(20% 0.020 168) 0%, oklch(46% 0.14 165) 100%)' },
  { id: 'obsidian', label: 'Obsidian', mode: 'dark',  accent: 'oklch(92% 0 0)',
    swatch: 'linear-gradient(135deg, oklch(15% 0 0) 0%, oklch(50% 0 0) 100%)' },
  { id: 'real',     label: 'Real',     mode: 'dark',  accent: 'oklch(82% 0.045 96)',
    swatch: 'linear-gradient(135deg, oklch(12% 0.004 96) 0%, oklch(42% 0.006 96) 52%, oklch(18% 0.004 96) 100%)' },
  { id: 'real-noir', label: 'Real Noir', mode: 'dark', accent: 'oklch(82% 0.030 112)',
    swatch: 'linear-gradient(135deg, oklch(4% 0.002 112) 0%, oklch(18% 0.004 112) 48%, oklch(7% 0.002 112) 100%)' },
];

const THEME_BY_ID = Object.fromEntries(THEMES.map(t => [t.id, t]));

const PRESET_CATEGORIES = [
  {
    label: 'Lead',
    presets: ['Default', 'Pop Vocal', 'Velvet Lead', 'Airy Hook', 'Smooth Ballad']
  },
  {
    label: 'Rap',
    presets: ['Rap Vibe', 'Drill Tight', 'Trap Bright', 'Punch Verse', 'Adlib Bite']
  },
  {
    label: 'Color',
    presets: ['Lo-Fi', 'Broadcast', 'Tape Phone', 'Dusty Stack', 'Neon Doubler']
  },
  {
    label: 'FX',
    presets: ['Wide Chorus', 'Delay Throw', 'Hall Lead', 'Parallel Crunch', 'Night Radio']
  }
];

const FLAT_PRESETS = PRESET_CATEGORIES.flatMap((category) =>
  category.presets.map((name) => ({ name, category: category.label }))
);

// Map any legacy theme value (lavender/dark) to a current id so existing
// EDITMODE blocks keep working.
function resolveThemeId(id) {
  if (THEME_BY_ID[id]) return id;
  if (id === 'lavender') return 'real-noir';
  if (id === 'dark') return 'midnight';
  return 'real-noir';
}

function ThemeMenu({ themeId, onSelect, onClose }) {
  const ref = useRef(null);
  useEffect(() => {
    const onDoc = (e) => { if (ref.current && !ref.current.contains(e.target)) onClose(); };
    const onKey = (e) => { if (e.key === 'Escape') onClose(); };
    document.addEventListener('mousedown', onDoc);
    document.addEventListener('keydown', onKey);
    return () => {
      document.removeEventListener('mousedown', onDoc);
      document.removeEventListener('keydown', onKey);
    };
  }, [onClose]);
  return (
    <div className="theme-menu" ref={ref} role="menu">
      <div className="theme-menu-section">
        <div className="theme-menu-label">Night</div>
        <div className="theme-grid">
          {THEMES.map(t => (
            <button
              key={t.id}
              type="button"
              className={`theme-swatch${themeId === t.id ? ' active' : ''}`}
              style={{ background: t.swatch }}
              onClick={() => onSelect(t.id)}
              title={t.label}
              aria-label={`Theme: ${t.label}`}
            >
              <span className="theme-swatch-dot" style={{ background: t.accent }} />
              <span className="theme-swatch-name">{t.label}</span>
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}

function useUserPresets() {
  const [presets, setPresets] = useState(() => {
    try {
      return JSON.parse(localStorage.getItem('voxanova-user-presets') || '[]');
    } catch {
      return [];
    }
  });

  const savePreset = useCallback((name, data) => {
    const preset = { id: Date.now().toString(), name, savedAt: Date.now(), ...data };
    setPresets((current) => {
      const next = [...current, preset];
      localStorage.setItem('voxanova-user-presets', JSON.stringify(next));
      return next;
    });
  }, []);

  const deletePreset = useCallback((id) => {
    setPresets((current) => {
      const next = current.filter((p) => p.id !== id);
      localStorage.setItem('voxanova-user-presets', JSON.stringify(next));
      return next;
    });
  }, []);

  return { presets, savePreset, deletePreset };
}

function SavePresetModal({ onSave, onClose }) {
  const [name, setName] = useState('');
  const inputRef = useRef(null);

  useEffect(() => {
    inputRef.current?.focus();
    const onKey = (e) => { if (e.key === 'Escape') onClose(); };
    document.addEventListener('keydown', onKey);
    return () => document.removeEventListener('keydown', onKey);
  }, [onClose]);

  const handleSubmit = (e) => {
    e.preventDefault();
    const trimmed = name.trim();
    if (!trimmed) return;
    onSave(trimmed);
    onClose();
  };

  return (
    <div className="plugin-info-backdrop" onMouseDown={onClose}>
      <section className="save-preset-modal" role="dialog" aria-modal="true" aria-label="Save preset" onMouseDown={(e) => e.stopPropagation()}>
        <button type="button" className="plugin-info-close" aria-label="Close" onClick={onClose}>
          <svg width="12" height="12" viewBox="0 0 12 12" aria-hidden="true">
            <path d="M3 3l6 6M9 3L3 9" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
          </svg>
        </button>
        <div className="save-preset-title">Save Preset</div>
        <div className="save-preset-sub">Name your configuration to recall it later</div>
        <form onSubmit={handleSubmit} className="save-preset-form">
          <input
            ref={inputRef}
            className="save-preset-input"
            type="text"
            placeholder="Preset name…"
            value={name}
            onChange={(e) => setName(e.target.value)}
            maxLength={48}
            autoComplete="off"
            spellCheck={false}
          />
          <div className="save-preset-actions">
            <button type="button" className="save-preset-btn cancel" onClick={onClose}>Cancel</button>
            <button type="submit" className="save-preset-btn confirm" disabled={!name.trim()}>Save</button>
          </div>
        </form>
      </section>
    </div>
  );
}

function PresetMenu({ current, onSelect, onClose, userPresets, onDeleteUserPreset, onSelectUserPreset }) {
  const ref = useRef(null);

  useEffect(() => {
    const onDoc = (e) => { if (ref.current && !ref.current.contains(e.target)) onClose(); };
    const onKey = (e) => { if (e.key === 'Escape') onClose(); };
    document.addEventListener('mousedown', onDoc);
    document.addEventListener('keydown', onKey);
    return () => {
      document.removeEventListener('mousedown', onDoc);
      document.removeEventListener('keydown', onKey);
    };
  }, [onClose]);

  return (
    <div className="preset-menu" ref={ref} role="menu">
      {userPresets.length > 0 && (
        <div className="preset-menu-section">
          <div className="preset-menu-label">My Presets</div>
          <div className="preset-menu-options preset-menu-options-user">
            {userPresets.map((preset) => (
              <div key={preset.id} className="preset-menu-user-row">
                <button
                  type="button"
                  className="preset-menu-option preset-menu-option-user"
                  onClick={() => { onSelectUserPreset(preset); onClose(); }}
                >
                  <span>{preset.name}</span>
                </button>
                <button
                  type="button"
                  className="preset-menu-delete"
                  aria-label={`Delete preset ${preset.name}`}
                  onClick={(e) => { e.stopPropagation(); onDeleteUserPreset(preset.id); }}
                >
                  <svg width="8" height="8" viewBox="0 0 8 8" aria-hidden="true">
                    <path d="M1.5 1.5l5 5M6.5 1.5l-5 5" stroke="currentColor" strokeWidth="1.3" strokeLinecap="round" />
                  </svg>
                </button>
              </div>
            ))}
          </div>
        </div>
      )}
      {PRESET_CATEGORIES.map((category) => (
        <div className="preset-menu-section" key={category.label}>
          <div className="preset-menu-label">{category.label}</div>
          <div className="preset-menu-options">
            {category.presets.map((name) => (
              <button
                key={name}
                type="button"
                className={`preset-menu-option${name === current ? ' active' : ''}`}
                onClick={() => {
                  onSelect(name);
                  onClose();
                }}
              >
                <span>{name}</span>
                {name === current && <span className="preset-menu-dot" />}
              </button>
            ))}
          </div>
        </div>
      ))}
    </div>
  );
}

function PluginInfoModal({ nativeOnline, onClose }) {
  useEffect(() => {
    const onKey = (e) => { if (e.key === 'Escape') onClose(); };
    document.addEventListener('keydown', onKey);
    return () => document.removeEventListener('keydown', onKey);
  }, [onClose]);

  return (
    <div className="plugin-info-backdrop" onMouseDown={onClose}>
      <section className="plugin-info-modal" role="dialog" aria-modal="true" onMouseDown={(e) => e.stopPropagation()}>
        <button type="button" className="plugin-info-close" aria-label="Close" onClick={onClose}>
          <svg width="12" height="12" viewBox="0 0 12 12" aria-hidden="true">
            <path d="M3 3l6 6M9 3L3 9" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
          </svg>
        </button>
        <div className="plugin-info-title">Voxanova</div>
        <div className="plugin-info-sub">Vocal Chain · {nativeOnline ? 'Native' : 'Browser'}</div>
        <div className="plugin-info-grid">
          <span>Format</span><strong>AU · VST3</strong>
          <span>Engine</span><strong>EQ · Dynamics · Tune · FX</strong>
          <span>Build</span><strong>2026.05</strong>
          <span>State</span><strong>{nativeOnline ? 'Connected' : 'Preview'}</strong>
        </div>
      </section>
    </div>
  );
}

function FooterLevelMeter({ active, level = 0 }) {
  const [displayLevel, setDisplayLevel] = useState(0);

  useEffect(() => {
    const target = active ? Math.max(0, Math.min(1, Number(level) || 0)) : 0;
    if (target <= 0.002) {
      setDisplayLevel(0);
      return;
    }

    setDisplayLevel(prev => Math.max(target, prev * 0.62 + target * 0.38));
  }, [active, level]);

  const segments = 28;
  const litSegments = displayLevel > 0.002 ? Math.ceil(displayLevel * segments) : 0;
  return (
    <div className="footer-meter" aria-hidden="true">
      {Array.from({ length: segments }).map((_, i) => {
        const lit = active && i < litSegments;
        const hot = i >= segments - 2;
        const warm = i >= segments - 6;
        return <span key={i} className={`footer-meter-seg${warm ? ' warm' : ''}${hot ? ' hot' : ''}${lit ? ' on' : ''}`} />;
      })}
    </div>
  );
}

function FooterGainSlider({ label, value, onChange, defaultValue = 0 }) {
  const min = -24;
  const max = 24;
  const safeValue = clamp(Number(value) || 0, min, max);
  const pct = ((safeValue - min) / (max - min)) * 100;

  const onPointerDown = (event) => {
    if (resetOnAltClick(event, () => onChange(defaultValue))) return;
    event.preventDefault();
    const target = event.currentTarget;
    target.setPointerCapture?.(event.pointerId);

    const update = (ev) => {
      const rect = target.getBoundingClientRect();
      const next = min + clamp((ev.clientX - rect.left) / rect.width, 0, 1) * (max - min);
      onChange(Number(next.toFixed(1)));
    };

    update(event);

    const move = (ev) => update(ev);
    const up = () => {
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
    };

    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up);
  };

  return (
    <div className="rev-slider footer-gain-control">
      <div
        className="rev-slider-track"
        onPointerDown={onPointerDown}
        onDoubleClick={(event) => resetOnDoubleClick(event, () => onChange(defaultValue))}
        role="slider"
        aria-label={`${label} gain`}
        aria-valuemin={min}
        aria-valuemax={max}
        aria-valuenow={safeValue}
        onWheel={(event) => handleWheelValue(event, safeValue, { min, max, step: 0.1 }, onChange)}
      >
        <div className="rev-slider-fill" style={{ width: `${pct}%` }} />
        <div className="rev-slider-handle" style={{ left: `${pct}%` }} />
      </div>
    </div>
  );
}

function FooterGainControl({ label, value, onChange, active, level = 0, output = false, defaultValue = 0 }) {
  const gainText = `${value >= 0 ? '+' : ''}${value.toFixed(1)} dB`;
  const slider = <FooterGainSlider label={label} value={value} onChange={onChange} defaultValue={defaultValue} />;
  const meter = <FooterLevelMeter active={active} level={level} />;
  return (
    <div className={`footer-channel${output ? ' output' : ' input'}`}>
      <span className="footer-channel-name">{label}</span>
      {output ? slider : meter}
      {output ? meter : slider}
      <span className="footer-gain-value">{gainText}</span>
    </div>
  );
}

function getPluginFrameRect() {
  const frame = document.querySelector('.plugin-frame');
  return frame?.getBoundingClientRect?.() || {
    width: window.innerWidth || PLUGIN_WIDTH,
    height: window.innerHeight || PLUGIN_HEIGHT,
  };
}

function getScaleForPluginRect(rect) {
  const frameWidth = Math.max(0, rect?.width || PLUGIN_WIDTH);
  const frameHeight = Math.max(0, rect?.height || PLUGIN_HEIGHT);
  const minScale = hasNativeBackend() ? 0.5 : 0.25;
  return clamp(Math.min(frameWidth / PLUGIN_WIDTH, frameHeight / PLUGIN_HEIGHT), minScale, 2);
}

function useLockedPluginViewport() {
  useEffect(() => {
    let raf = 0;
    let observer = null;

    const syncViewport = () => {
      if (raf) cancelAnimationFrame(raf);
      raf = requestAnimationFrame(() => {
        raf = 0;
        const nextScale = getScaleForPluginRect(getPluginFrameRect());
        document.documentElement.style.setProperty('--plugin-ui-scale', nextScale.toFixed(4));
      });
    };

    const frame = document.querySelector('.plugin-frame');
    if (frame && typeof ResizeObserver !== 'undefined') {
      observer = new ResizeObserver(syncViewport);
      observer.observe(frame);
    }

    syncViewport();
    window.addEventListener('resize', syncViewport);
    return () => {
      if (raf) cancelAnimationFrame(raf);
      observer?.disconnect();
      window.removeEventListener('resize', syncViewport);
    };
  }, []);
}

function EditorResizeGrip({ enabled }) {
  const dragRef = useRef(null);
  const rafRef = useRef(0);
  const lastSentRef = useRef({ scale: 0, at: 0 });

  const sendResize = useCallback((scale) => {
    const nextScale = Math.round(clamp(scale, 0.5, 2) * 100) / 100;
    const now = performance.now();
    const last = lastSentRef.current;

    if (Math.abs(nextScale - last.scale) < 0.01 && now - last.at < 90) return;

    lastSentRef.current = { scale: nextScale, at: now };
    sendNativeEditorSize(
      nextScale,
      Math.round(PLUGIN_WIDTH * nextScale),
      Math.round(PLUGIN_HEIGHT * nextScale)
    );
  }, []);

  const onPointerDown = useCallback((event) => {
    if (!enabled) return;

    event.preventDefault();
    const startScale = getScaleForPluginRect(getPluginFrameRect());
    dragRef.current = {
      startX: event.clientX,
      startY: event.clientY,
      startScale,
      latestScale: startScale,
    };

    event.currentTarget.setPointerCapture?.(event.pointerId);
    document.body.classList.add('is-plugin-resizing');

    const flushResize = (scale) => {
      if (rafRef.current) cancelAnimationFrame(rafRef.current);
      rafRef.current = requestAnimationFrame(() => {
        rafRef.current = 0;
        sendResize(scale);
      });
    };

    const onMove = (moveEvent) => {
      const drag = dragRef.current;
      if (!drag) return;

      const dxScale = (moveEvent.clientX - drag.startX) / PLUGIN_WIDTH;
      const dyScale = (moveEvent.clientY - drag.startY) / PLUGIN_HEIGHT;
      const dominantDelta = Math.abs(dxScale) > Math.abs(dyScale) ? dxScale : dyScale;
      drag.latestScale = drag.startScale + dominantDelta;
      flushResize(drag.latestScale);
    };

    const onUp = () => {
      if (dragRef.current) sendResize(dragRef.current.latestScale);
      dragRef.current = null;
      document.body.classList.remove('is-plugin-resizing');
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
      window.removeEventListener('pointercancel', onUp);
    };

    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
    window.addEventListener('pointercancel', onUp);
  }, [enabled, sendResize]);

  return (
    <button
      type="button"
      className={`plugin-resize-grip${enabled ? '' : ' disabled'}`}
      onPointerDown={onPointerDown}
      aria-label="Resize plugin"
      title="Resize plugin"
    >
      <span />
      <span />
      <span />
    </button>
  );
}

function App() {
  useLockedPluginViewport();
  const [tweaks, setTweak] = useTweaks(TWEAK_DEFAULTS);
  const themeId = resolveThemeId(tweaks.theme);
  const themeMeta = THEME_BY_ID[themeId];
  const [themeMenuOpen, setThemeMenuOpen] = useState(false);
  const [eqExpansion, setEqExpansion] = useState(0);
  const [rackResizeActive, setRackResizeActive] = useState(false);
  const rackResizeRef = useRef(null);
  const pluginRef = useRef(null);

  useEffect(() => {
    if (tweaks.theme !== themeId && !THEME_BY_ID[tweaks.theme]) {
      setTweak('theme', themeId);
    }
  }, [setTweak, themeId, tweaks.theme]);

  useEffect(() => {
    const root = document.documentElement;
    const body = document.body;
    root.dataset.theme = themeId;
    body.dataset.theme = themeId;
    root.dataset.mode = themeMeta.mode;
    body.dataset.mode = themeMeta.mode;
  }, [themeId, themeMeta.mode]);

  const [values, setValues] = useState(defaultValues);
  const [meters, setMeters] = useState(emptyMeters);
  const [nativeOnline, setNativeOnline] = useState(hasNativeBackend());
  const [nativeEqBands, setNativeEqBands] = useState(null);

  useEffect(() => {
    sendNativeEditorSize(1, PLUGIN_WIDTH, PLUGIN_HEIGHT);
  }, []);

  useEffect(() => {
    const updateNativeStatus = () => setNativeOnline(hasNativeBackend());
    updateNativeStatus();
    const timer = window.setInterval(updateNativeStatus, 1000);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => {
    const onMeterUpdate = (event) => {
      const payload = event.detail || {};
      const eqBands = eqBandsFromPayload(payload);
      if (eqBands) setNativeEqBands(eqBands);
      setMeters((current) => metersFromPayload(current, payload));
      setValues((current) => updateValuesFromPayload(current, payload));
    };

    window.addEventListener("voxanovaMeterUpdate", onMeterUpdate);
    return () => window.removeEventListener("voxanovaMeterUpdate", onMeterUpdate);
  }, []);

  const setParam = useCallback((id, value) => {
    const nextValue = booleanParameterSet.has(id) ? Boolean(value) : value;
    setValues((current) => ({ ...current, [id]: nextValue }));
    sendNativeParameter(id, booleanParameterSet.has(id) ? (nextValue ? 1 : 0) : nextValue);
  }, []);

  // EQ state
  const [eqMode, setEqMode] = useState('pre');
  const [eqScaleByMode, setEqScaleByMode] = useState({ pre: 12, post: 12 }); // dB range ± display per EQ view
  const [eqScaleOpen, setEqScaleOpen] = useState(false);
  const eqScaleRef = useRef(null);
  const scaleOptions = [3, 6, 12, 30];
  const eqScale = eqScaleByMode[eqMode] ?? 12;
  const setActiveEqScale = useCallback((nextOrUpdater) => {
    setEqScaleByMode((current) => {
      const currentScale = current[eqMode] ?? 12;
      const nextScale = typeof nextOrUpdater === 'function' ? nextOrUpdater(currentScale) : nextOrUpdater;
      return {
        ...current,
        [eqMode]: nextScale,
      };
    });
  }, [eqMode]);
  const [eqSaturation, setEqSaturation] = useState({
    pre: { mode: defaultValues.preSaturationMode, amount: defaultValues.preSaturationAmount },
    post: { mode: defaultValues.postSaturationMode, amount: defaultValues.postSaturationAmount },
  });
  const activeEqSaturation = {
    mode: values[`${eqMode}SaturationMode`] ?? eqSaturation[eqMode]?.mode ?? 0,
    amount: values[`${eqMode}SaturationAmount`] ?? eqSaturation[eqMode]?.amount ?? 0,
  };
  const setActiveEqSaturation = (patch) => {
    setEqSaturation(current => ({
      ...current,
      [eqMode]: {
        ...(current[eqMode] || { mode: 0, amount: 0 }),
        ...patch,
      },
    }));
    if (Object.prototype.hasOwnProperty.call(patch, 'mode')) {
      setParam(`${eqMode}SaturationMode`, patch.mode);
    }
    if (Object.prototype.hasOwnProperty.call(patch, 'amount')) {
      setParam(`${eqMode}SaturationAmount`, patch.amount);
    }
  };

  useEffect(() => {
    const onDoc = (e) => { if (eqScaleRef.current && !eqScaleRef.current.contains(e.target)) setEqScaleOpen(false); };
    document.addEventListener('mousedown', onDoc);
    return () => document.removeEventListener('mousedown', onDoc);
  }, []);

  const [eqPostPoints, setEqPostPoints] = useState([]);
  const [eqPrePoints, setEqPrePoints] = useState([]);
  const eqTouchedRef = useRef({ pre: false, post: false });
  const eqHydratedRef = useRef({ pre: false, post: false });
  const lastSentEqJsonRef = useRef("");

  const setEqPrePointsFromUi = useCallback((nextOrUpdater) => {
    eqTouchedRef.current.pre = true;
    setEqPrePoints(nextOrUpdater);
  }, []);

  const setEqPostPointsFromUi = useCallback((nextOrUpdater) => {
    eqTouchedRef.current.post = true;
    setEqPostPoints(nextOrUpdater);
  }, []);

  useEffect(() => {
    if (!nativeEqBands) return;

    if (!eqTouchedRef.current.pre && !eqHydratedRef.current.pre) {
      setEqPrePoints(nativeEqBands.pre);
      eqHydratedRef.current.pre = true;
    }

    if (!eqTouchedRef.current.post && !eqHydratedRef.current.post) {
      setEqPostPoints(nativeEqBands.post);
      eqHydratedRef.current.post = true;
    }
  }, [nativeEqBands]);

  useEffect(() => {
    if (nativeOnline && !nativeEqBands && !eqTouchedRef.current.pre && !eqTouchedRef.current.post) return;

    const payload = {
      pre: normalizeEqPoints(eqPrePoints),
      post: normalizeEqPoints(eqPostPoints)
    };
    const json = JSON.stringify(payload);
    if (json === lastSentEqJsonRef.current) return;

    lastSentEqJsonRef.current = json;
    sendNativeEqBands(payload);
  }, [eqPrePoints, eqPostPoints, nativeEqBands, nativeOnline]);

  // Native-backed module state. The V2 controls stay visual, but every mapped
  // change goes through the same JUCE bridge as the previous frontend.
  const peakOn = boolFromParam(values.peakEnabled);
  const peakThresh = values.peakThreshold;
  const setPeakOn = useCallback((value) => setParam("peakEnabled", value), [setParam]);
  const setPeakThresh = useCallback((value) => setParam("peakThreshold", value), [setParam]);

  const glueOn = boolFromParam(values.glueEnabled);
  const glueThresh = values.glueThreshold;
  const butterMode = boolFromParam(values.glueMultiband) ? 'MULTI BUTTER' : 'BUTTER COMP';
  const multiButterThresh = [
    values.glueLowThreshold,
    values.glueLowMidThreshold,
    values.glueHighMidThreshold,
    values.glueAirThreshold,
  ];
  const setGlueOn = useCallback((value) => setParam("glueEnabled", value), [setParam]);
  const setGlueThresh = useCallback((value) => setParam("glueThreshold", value), [setParam]);
  const setButterMode = useCallback((value) => setParam("glueMultiband", value === 'MULTI BUTTER'), [setParam]);
  const setMultiButterThresh = useCallback((nextOrUpdater) => {
    const current = [
      values.glueLowThreshold,
      values.glueLowMidThreshold,
      values.glueHighMidThreshold,
      values.glueAirThreshold,
    ];
    const next = typeof nextOrUpdater === "function" ? nextOrUpdater(current) : nextOrUpdater;
    glueBandIds.forEach((id, index) => {
      if (typeof next?.[index] === "number") setParam(id, next[index]);
    });
  }, [setParam, values.glueAirThreshold, values.glueHighMidThreshold, values.glueLowMidThreshold, values.glueLowThreshold]);

  const iyfOn = boolFromParam(values.faceEnabled);
  const iyf = values.faceThreshold;
  const setIyfOn = useCallback((value) => setParam("faceEnabled", value), [setParam]);
  const setIyf = useCallback((value) => setParam("faceThreshold", value), [setParam]);

  const gateOn = boolFromParam(values.gateEnabled);
  const gateThresh = values.gateThreshold;
  const setGateOn = useCallback((value) => setParam("gateEnabled", value), [setParam]);
  const setGateThresh = useCallback((value) => setParam("gateThreshold", value), [setParam]);

  const deEsserOn = boolFromParam(values.deEsserEnabled);
  const deEsserAmount = values.deEsserAmount;
  const setDeEsserOn = useCallback((value) => setParam("deEsserEnabled", value), [setParam]);
  const setDeEsserAmount = useCallback((value) => setParam("deEsserAmount", value), [setParam]);
  const deEsserLow = clamp(Number(values.deEsserLow) || 5500, 2500, 11600);
  const deEsserHigh = clamp(Number(values.deEsserHigh) || 8500, deEsserLow + 400, 12000);
  const setDeEsserLow = useCallback((value) => {
    const nextLow = clamp(Number(value) || 5500, 2500, 11600);
    setParam("deEsserLow", nextLow);
    if (nextLow > deEsserHigh - 400) setParam("deEsserHigh", Math.min(12000, nextLow + 400));
  }, [deEsserHigh, setParam]);
  const setDeEsserHigh = useCallback((value) => {
    setParam("deEsserHigh", clamp(Number(value) || 8500, deEsserLow + 400, 12000));
  }, [deEsserLow, setParam]);

  const stereoOn = boolFromParam(values.stereoEnabled);
  const stereoWidth = values.stereoWidth;
  const lowBypass = values.stereoLowBypass;
  const setStereoOn = useCallback((value) => setParam("stereoEnabled", value), [setParam]);
  const setStereoWidth = useCallback((value) => setParam("stereoWidth", value), [setParam]);
  const setLowBypass = useCallback((value) => setParam("stereoLowBypass", value), [setParam]);

  const delayOn = boolFromParam(values.delayEnabled);
  const delay = useMemo(() => ({
    preset: delayStyles[clamp(Math.round(values.delayStyle), 0, delayStyles.length - 1)] || 'Clean',
    mix: values.delayMix,
    timeIdx: clamp(Math.round(values.delayDivision), 0, delayDivisionMax),
    timeMs: values.delayTimeMs,
    feedback: values.delayFeedback,
    mode: noteValueToMode[clamp(Math.round(values.delayNoteMode), 0, 2)] || 'NOTE',
    type: delayValueToType[clamp(Math.round(values.delayMode), 0, 2)] || 'NORMAL',
    bpm: boolFromParam(values.delaySync),
    postReverb: boolFromParam(values.delayPostReverb),
    lowCut: valueToRange(values.delayLowCut, 20, 500),
    highCut: valueToRange(values.delayHighCut, 2000, 20000),
  }), [values.delayDivision, values.delayFeedback, values.delayHighCut, values.delayLowCut, values.delayMix, values.delayMode, values.delayNoteMode, values.delayPostReverb, values.delayStyle, values.delaySync, values.delayTimeMs]);
  const setDelayOn = useCallback((value) => setParam("delayEnabled", value), [setParam]);
  const setDelay = useCallback((patch) => {
    if (Object.prototype.hasOwnProperty.call(patch, "preset")) setParam("delayStyle", presetIndex(delayStyles, patch.preset));
    if (Object.prototype.hasOwnProperty.call(patch, "mix")) setParam("delayMix", patch.mix);
    if (Object.prototype.hasOwnProperty.call(patch, "timeIdx")) setParam("delayDivision", clamp(Math.round(patch.timeIdx), 0, delayDivisionMax));
    if (Object.prototype.hasOwnProperty.call(patch, "timeMs")) setParam("delayTimeMs", patch.timeMs);
    if (Object.prototype.hasOwnProperty.call(patch, "feedback")) setParam("delayFeedback", patch.feedback);
    if (Object.prototype.hasOwnProperty.call(patch, "mode")) setParam("delayNoteMode", noteModeToValue[patch.mode] ?? 0);
    if (Object.prototype.hasOwnProperty.call(patch, "type")) setParam("delayMode", delayTypeToValue[patch.type] ?? 0);
    if (Object.prototype.hasOwnProperty.call(patch, "bpm")) setParam("delaySync", patch.bpm);
    if (Object.prototype.hasOwnProperty.call(patch, "postReverb")) setParam("delayPostReverb", patch.postReverb);
    if (Object.prototype.hasOwnProperty.call(patch, "lowCut")) setParam("delayLowCut", rangeToPercent(patch.lowCut, 20, 500));
    if (Object.prototype.hasOwnProperty.call(patch, "highCut")) setParam("delayHighCut", rangeToPercent(patch.highCut, 2000, 20000));
  }, [setParam]);

  const reverbOn = boolFromParam(values.reverbEnabled);
  const reverb = useMemo(() => ({
    preset: v2PresetByReverbMode[Math.round(values.reverbMode)] || reverbModes[clamp(Math.round(values.reverbMode), 0, reverbModes.length - 1)] || 'Concert Hall',
    mix: values.reverbMix,
    decay: values.reverbDecay,
    decayIdx: clamp(Math.round(values.reverbDecayDivision), 0, delayDivisionMax),
    preDelay: values.reverbPredelay,
    preDelayIdx: clamp(Math.round(values.reverbPredelayDivision), 0, reverbPredelayDivisionMax),
    mode: noteValueToMode[clamp(Math.round(values.reverbNoteMode), 0, 2)] || 'NOTE',
    bpm: boolFromParam(values.reverbSync),
    decaySync: boolFromParam(values.reverbDecaySync),
    preDelaySync: boolFromParam(values.reverbPredelaySync),
    size: values.reverbSize,
    lowCut: valueToRange(values.reverbLowCut, 20, 500),
    highCut: valueToRange(values.reverbHighCut, 2000, 20000),
  }), [values.reverbDecay, values.reverbDecayDivision, values.reverbDecaySync, values.reverbHighCut, values.reverbLowCut, values.reverbMix, values.reverbMode, values.reverbNoteMode, values.reverbPredelay, values.reverbPredelayDivision, values.reverbPredelaySync, values.reverbSize, values.reverbSync]);
  const setReverbOn = useCallback((value) => setParam("reverbEnabled", value), [setParam]);
  const setReverb = useCallback((patch) => {
    if (Object.prototype.hasOwnProperty.call(patch, "preset")) setParam("reverbMode", reverbModeByV2Preset[patch.preset] ?? presetIndex(reverbModes, patch.preset));
    if (Object.prototype.hasOwnProperty.call(patch, "mix")) setParam("reverbMix", patch.mix);
    if (Object.prototype.hasOwnProperty.call(patch, "decay")) setParam("reverbDecay", patch.decay);
    if (Object.prototype.hasOwnProperty.call(patch, "decayIdx")) setParam("reverbDecayDivision", clamp(Math.round(patch.decayIdx), 0, delayDivisionMax));
    if (Object.prototype.hasOwnProperty.call(patch, "preDelay")) setParam("reverbPredelay", patch.preDelay);
    if (Object.prototype.hasOwnProperty.call(patch, "preDelayIdx")) setParam("reverbPredelayDivision", clamp(Math.round(patch.preDelayIdx), 0, reverbPredelayDivisionMax));
    if (Object.prototype.hasOwnProperty.call(patch, "mode")) setParam("reverbNoteMode", noteModeToValue[patch.mode] ?? 0);
    if (Object.prototype.hasOwnProperty.call(patch, "bpm")) setParam("reverbSync", patch.bpm);
    if (Object.prototype.hasOwnProperty.call(patch, "decaySync")) setParam("reverbDecaySync", patch.decaySync);
    if (Object.prototype.hasOwnProperty.call(patch, "preDelaySync")) setParam("reverbPredelaySync", patch.preDelaySync);
    if (Object.prototype.hasOwnProperty.call(patch, "size")) setParam("reverbSize", patch.size);
    if (Object.prototype.hasOwnProperty.call(patch, "lowCut")) setParam("reverbLowCut", rangeToPercent(patch.lowCut, 20, 500));
    if (Object.prototype.hasOwnProperty.call(patch, "highCut")) setParam("reverbHighCut", rangeToPercent(patch.highCut, 2000, 20000));
  }, [setParam]);

  // AutoTune
  const atOn = boolFromParam(values.tuneEnabled);
  const atAmount = values.tuneAmount;
  const atKey = autoTuneNotes[clamp(Math.round(values.tuneKey), 0, autoTuneNotes.length - 1)] || 'C';
  const atScale = autoTuneScales[clamp(Math.round(values.tuneScale), 0, autoTuneScales.length - 1)] || 'MAJ';
  const atVoiceType = autoTuneVoiceTypes[clamp(Math.round(values.tuneVoiceType), 0, autoTuneVoiceTypes.length - 1)] || 'TENOR';
  const atCustomMask = values.tuneCustomNotes;
  const setAtOn = useCallback((value) => setParam("tuneEnabled", value), [setParam]);
  const setAtAmount = useCallback((value) => setParam("tuneAmount", value), [setParam]);
  const setAtKey = useCallback((value) => setParam("tuneKey", presetIndex(autoTuneNotes, value)), [setParam]);
  const setAtScale = useCallback((value) => setParam("tuneScale", presetIndex(autoTuneScales, value)), [setParam]);
  const setAtVoiceType = useCallback((value) => setParam("tuneVoiceType", presetIndex(autoTuneVoiceTypes, value)), [setParam]);
  const setAtCustomMask = useCallback((value) => setParam("tuneCustomNotes", value), [setParam]);
  const liveTunePitch = useMemo(() => ({
    frequency: meters.tuneFrequency,
    cents: meters.tuneCents,
    confidence: meters.tuneConfidence,
    targetMidi: meters.tuneTargetMidi,
  }), [meters.tuneCents, meters.tuneConfidence, meters.tuneFrequency, meters.tuneTargetMidi]);

  const inputGain = values.inputGain;
  const outputGain = values.outputGain;
  const setInputGain = useCallback((value) => setParam("inputGain", value), [setParam]);
  const setOutputGain = useCallback((value) => setParam("outputGain", value), [setParam]);
  const layoutMetrics = useMemo(() => getLayoutMetrics(eqExpansion), [eqExpansion]);
  const visualLayoutKey = resolveLayoutKey(eqExpansion > 0.01 ? 'eq' : null);
  const eqGraphHeight = layoutMetrics.eqGraphHeight;
  const rackMaximized = eqExpansion >= 0.985;
  const activeEqPointCount = eqMode === 'pre' ? eqPrePoints.length : eqPostPoints.length;
  const layoutStyle = useMemo(
    () => getLayoutStyle(layoutMetrics),
    [layoutMetrics]
  );

  const setClampedEqExpansion = useCallback((nextOrUpdater) => {
    setEqExpansion((current) => {
      const next = typeof nextOrUpdater === 'function' ? nextOrUpdater(current) : nextOrUpdater;
      return clamp(Number(next) || 0, 0, 1);
    });
  }, []);

  const onRackResizePointerDown = useCallback((event) => {
    if (event.button !== undefined && event.button !== 0) return;
    if (rackResizeRef.current) return;
    if (resetOnAltClick(event, () => setClampedEqExpansion(0))) return;

    event.preventDefault();
    event.stopPropagation();

    const pluginRect = pluginRef.current?.getBoundingClientRect?.();
    const uiScale = pluginRect?.height
      ? Math.max(0.1, pluginRect.height / PLUGIN_HEIGHT)
      : Math.max(0.1, getScaleForPluginRect(getPluginFrameRect()));
    const drag = {
      startY: event.clientY,
      startExpansion: eqExpansion,
      uiScale,
    };

    rackResizeRef.current = drag;
    setRackResizeActive(true);
    document.body.classList.add('is-rack-resizing');
    event.currentTarget.setPointerCapture?.(event.pointerId);

    const onMove = (moveEvent) => {
      const currentDrag = rackResizeRef.current;
      if (!currentDrag) return;

      moveEvent.preventDefault();
      const deltaY = (moveEvent.clientY - currentDrag.startY) / currentDrag.uiScale;
      setClampedEqExpansion(currentDrag.startExpansion + deltaY / EQ_RACK_RESIZE_RANGE);
    };

    const onUp = () => {
      rackResizeRef.current = null;
      setRackResizeActive(false);
      document.body.classList.remove('is-rack-resizing');
      window.removeEventListener('pointermove', onMove);
      window.removeEventListener('pointerup', onUp);
      window.removeEventListener('pointercancel', onUp);
    };

    window.addEventListener('pointermove', onMove);
    window.addEventListener('pointerup', onUp);
    window.addEventListener('pointercancel', onUp);
  }, [eqExpansion, setClampedEqExpansion]);

  const onRackResizeMouseDown = useCallback((event) => {
    if (event.button !== undefined && event.button !== 0) return;
    if (rackResizeRef.current) return;
    if (resetOnAltClick(event, () => setClampedEqExpansion(0))) return;

    event.preventDefault();
    event.stopPropagation();

    const pluginRect = pluginRef.current?.getBoundingClientRect?.();
    const uiScale = pluginRect?.height
      ? Math.max(0.1, pluginRect.height / PLUGIN_HEIGHT)
      : Math.max(0.1, getScaleForPluginRect(getPluginFrameRect()));
    const drag = {
      startY: event.clientY,
      startExpansion: eqExpansion,
      uiScale,
    };

    rackResizeRef.current = drag;
    setRackResizeActive(true);
    document.body.classList.add('is-rack-resizing');

    const onMove = (moveEvent) => {
      const currentDrag = rackResizeRef.current;
      if (!currentDrag) return;

      moveEvent.preventDefault();
      const deltaY = (moveEvent.clientY - currentDrag.startY) / currentDrag.uiScale;
      setClampedEqExpansion(currentDrag.startExpansion + deltaY / EQ_RACK_RESIZE_RANGE);
    };

    const onUp = () => {
      rackResizeRef.current = null;
      setRackResizeActive(false);
      document.body.classList.remove('is-rack-resizing');
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };

    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
  }, [eqExpansion, setClampedEqExpansion]);

  const onRackResizeKeyDown = useCallback((event) => {
    const smallStep = 8 / EQ_RACK_RESIZE_RANGE;
    const largeStep = 24 / EQ_RACK_RESIZE_RANGE;

    if (event.key === 'ArrowDown') {
      event.preventDefault();
      setClampedEqExpansion((current) => current + smallStep);
    } else if (event.key === 'ArrowUp') {
      event.preventDefault();
      setClampedEqExpansion((current) => current - smallStep);
    } else if (event.key === 'PageDown') {
      event.preventDefault();
      setClampedEqExpansion((current) => current + largeStep);
    } else if (event.key === 'PageUp') {
      event.preventDefault();
      setClampedEqExpansion((current) => current - largeStep);
    } else if (event.key === 'Home') {
      event.preventDefault();
      setClampedEqExpansion(0);
    } else if (event.key === 'End') {
      event.preventDefault();
      setClampedEqExpansion(1);
    }
  }, [setClampedEqExpansion]);

  const [presetIdx, setPresetIdx] = useState(0);
  const [presetOpen, setPresetOpen] = useState(false);
  const [savePresetOpen, setSavePresetOpen] = useState(false);
  const [infoOpen, setInfoOpen] = useState(false);
  const [ab, setAb] = useState('A');
  const activePreset = FLAT_PRESETS[presetIdx]?.name || 'Default';
  const selectPreset = useCallback((name) => {
    const index = FLAT_PRESETS.findIndex((preset) => preset.name === name);
    if (index >= 0) setPresetIdx(index);
    if (name !== 'Default') return;

    setValues(defaultValues);
    setEqMode('pre');
    setEqSaturation({
      pre: { mode: defaultValues.preSaturationMode, amount: defaultValues.preSaturationAmount },
      post: { mode: defaultValues.postSaturationMode, amount: defaultValues.postSaturationAmount },
    });
    setEqPrePointsFromUi([]);
    setEqPostPointsFromUi([]);
    Object.keys(defaultValues).forEach((id) => {
      const value = defaultValues[id];
      sendNativeParameter(id, booleanParameterSet.has(id) ? (value ? 1 : 0) : value);
    });
    sendNativeEqBands({ pre: [], post: [] });
  }, [setEqPostPointsFromUi, setEqPrePointsFromUi]);

  const { presets: userPresets, savePreset, deletePreset } = useUserPresets();

  const handleSavePreset = useCallback((name) => {
    savePreset(name, {
      values,
      eqPrePoints,
      eqPostPoints,
    });
  }, [savePreset, values, eqPrePoints, eqPostPoints]);

  const handleLoadUserPreset = useCallback((preset) => {
    setValues(preset.values);
    setEqPrePointsFromUi(normalizeEqPoints(preset.eqPrePoints || []));
    setEqPostPointsFromUi(normalizeEqPoints(preset.eqPostPoints || []));
    Object.keys(defaultValues).forEach((id) => {
      if (typeof preset.values[id] !== 'undefined') {
        const value = preset.values[id];
        sendNativeParameter(id, booleanParameterSet.has(id) ? (value ? 1 : 0) : value);
      }
    });
  }, [setEqPrePointsFromUi, setEqPostPointsFromUi]);

  return (
    <div className="plugin-frame">
    <div
      className="plugin"
      ref={pluginRef}
      style={layoutStyle}
      data-layout-focus={visualLayoutKey}
      data-rack-resizing={rackResizeActive ? "true" : "false"}
      data-rack-maximized={rackMaximized ? "true" : "false"}
      data-eq-empty={activeEqPointCount === 0 ? "true" : "false"}
      data-stack-density="normal"
    >
      <div className="real-bg" aria-hidden="true">
        <div className="real-bg-slice real-bg-header" />
        <div className="real-bg-slice real-bg-eq" />
        <div className="real-bg-slice real-bg-rack" />
        <div className="real-bg-slice real-bg-footer" />
      </div>
      {/* ── Header ── */}
      <div className="plugin-header">
        <div className="brand">
          <button type="button" className="brand-mark" onClick={() => setInfoOpen(true)} aria-haspopup="dialog">
            Voxanova
            <span className="brand-dot" />
          </button>
          <div className="brand-sub">Vocal Chain · {nativeOnline ? 'Native' : 'Browser'}</div>
        </div>
        <div className="preset-center">
          <div className="preset-wrap">
            <div className="preset-bar-row">
              <button
                type="button"
                className={`preset-bar${presetOpen ? ' open' : ''}`}
                aria-haspopup="menu"
                aria-expanded={presetOpen}
                onClick={() => setPresetOpen((open) => !open)}
              >
                <span className="preset-name">{activePreset}</span>
                <svg className="preset-chevron" width="10" height="10" viewBox="0 0 10 10" aria-hidden="true">
                  <path d="M2 4l3 3 3-3" stroke="currentColor" strokeWidth="1.3" fill="none" strokeLinecap="round" strokeLinejoin="round"/>
                </svg>
              </button>
              <button
                type="button"
                className="preset-save-btn"
                aria-label="Save current settings as preset"
                title="Save preset"
                onClick={() => { setPresetOpen(false); setSavePresetOpen(true); }}
              >
                <svg width="11" height="11" viewBox="0 0 11 11" aria-hidden="true">
                  <path d="M5.5 1v9M1 5.5h9" stroke="currentColor" strokeWidth="1.4" strokeLinecap="round" />
                </svg>
              </button>
            </div>
            {presetOpen && (
              <PresetMenu
                current={activePreset}
                onSelect={selectPreset}
                onClose={() => setPresetOpen(false)}
                userPresets={userPresets}
                onDeleteUserPreset={deletePreset}
                onSelectUserPreset={handleLoadUserPreset}
              />
            )}
          </div>
        </div>
        <div className="header-actions">
          <div className="ab-compare">
            <button className={ab === 'A' ? 'active' : ''} onClick={() => setAb('A')}>A</button>
            <button className={ab === 'B' ? 'active' : ''} onClick={() => setAb('B')}>B</button>
          </div>
          <button
            className={`icon-btn${tweaks.signalActive ? ' active' : ''}`}
            onClick={(event) => {
              if (resetOnAltClick(event, () => setTweak('signalActive', TWEAK_DEFAULTS.signalActive))) return;
              setTweak('signalActive', !tweaks.signalActive);
            }}
            onDoubleClick={(event) => resetOnDoubleClick(event, () => setTweak('signalActive', TWEAK_DEFAULTS.signalActive))}
            title="Toggle signal"
          >
            <svg width="14" height="14" viewBox="0 0 14 14" fill="none">
              <path d="M2 7h2l1.5-3 3 6 1.5-3h2" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" strokeLinejoin="round" />
            </svg>
          </button>
          <div className="theme-menu-wrap">
            <button
              className={`icon-btn${themeMenuOpen ? ' active' : ''}`}
              title="Theme & display"
              aria-haspopup="menu"
              aria-expanded={themeMenuOpen}
              onClick={(event) => {
                if (resetOnAltClick(event, () => { setTweak('theme', TWEAK_DEFAULTS.theme); setThemeMenuOpen(false); })) return;
                setThemeMenuOpen(o => !o);
              }}
              onDoubleClick={(event) => resetOnDoubleClick(event, () => { setTweak('theme', TWEAK_DEFAULTS.theme); setThemeMenuOpen(false); })}
            >
              <svg width="14" height="14" viewBox="0 0 14 14"><path d="M2 4h10M2 7h10M2 10h10" stroke="currentColor" strokeWidth="1.2" strokeLinecap="round" /></svg>
            </button>
            {themeMenuOpen && (
              <ThemeMenu
                themeId={themeId}
                onSelect={(id) => { setTweak('theme', id); setThemeMenuOpen(false); }}
                onClose={() => setThemeMenuOpen(false)}
              />
            )}
          </div>
        </div>
      </div>
      {infoOpen && <PluginInfoModal nativeOnline={nativeOnline} onClose={() => setInfoOpen(false)} />}
      {savePresetOpen && <SavePresetModal onSave={handleSavePreset} onClose={() => setSavePresetOpen(false)} />}

      {/* ── EQ ── */}
      <div className="eq-section">
        <EQCurve
          postPoints={eqPostPoints}
          setPostPoints={setEqPostPointsFromUi}
          prePoints={eqPrePoints}
          setPrePoints={setEqPrePointsFromUi}
          mode={eqMode}
          setMode={setEqMode}
          showWaveform={tweaks.showWaveform}
          scale={eqScale}
          scaleOpen={eqScaleOpen}
          setScaleOpen={setEqScaleOpen}
          scaleOptions={scaleOptions}
          setScale={setActiveEqScale}
          scaleRef={eqScaleRef}
          saturation={activeEqSaturation}
          onSaturationChange={setActiveEqSaturation}
          detectedFrequency={meters.tuneFrequency}
          spectrumData={eqMode === 'pre' ? meters.preCompSpectrum : meters.postCompSpectrum}
          detectorData={eqMode === 'pre' ? meters.preEqDetectorDb : meters.postEqDetectorDb}
          graphHeight={eqGraphHeight}
          spectrumMaxFrequency={Math.max(20, Math.min(20000, Number(meters.spectrumMaxFrequency) || 20000))}
          sampleRate={meters.sampleRate}
        />
        <button
          type="button"
          className={`rack-toggle-btn${rackMaximized ? ' show-rack-btn' : ' hide-rack-btn'}`}
          onClick={(event) => {
            if (resetOnAltClick(event, () => setClampedEqExpansion(0))) return;
            setClampedEqExpansion(rackMaximized ? 0 : 1);
          }}
          onDoubleClick={(event) => resetOnDoubleClick(event, () => setClampedEqExpansion(0))}
          aria-label={rackMaximized ? "Show rack" : "Hide rack"}
        >
          <svg className="rack-toggle-icon" width="10" height="10" viewBox="0 0 10 10" aria-hidden="true">
            {rackMaximized ? (
              <path d="M2 6l3-3 3 3M5 3v5" fill="none" stroke="currentColor" strokeWidth="1.25" strokeLinecap="round" strokeLinejoin="round" />
            ) : (
              <path d="M2 4l3 3 3-3M5 2v5" fill="none" stroke="currentColor" strokeWidth="1.25" strokeLinecap="round" strokeLinejoin="round" />
            )}
          </svg>
          {rackMaximized ? 'Show Rack' : 'Hide Rack'}
        </button>
        <div
          className="rack-resize-handle"
          role="separator"
          aria-orientation="horizontal"
          aria-label="Resize EQ"
          aria-valuemin={0}
          aria-valuemax={100}
          aria-valuenow={Math.round(eqExpansion * 100)}
          tabIndex={0}
          title="Resize EQ"
          onPointerDown={onRackResizePointerDown}
          onMouseDown={onRackResizeMouseDown}
          onDoubleClick={(event) => resetOnDoubleClick(event, () => setClampedEqExpansion(0))}
          onKeyDown={onRackResizeKeyDown}
        />
      </div>

      {/* ── Modules ── */}
      <div
        className="modules-row"
        data-stack-density="normal"
      >
        {/* Peak Sniper + Butter Comp stacked */}
        <div className="module module-stack">
          <CompModule
            name="PEAK SNIPER"
            threshold={peakThresh} setThreshold={setPeakThresh}
            on={peakOn} setOn={setPeakOn}
            signalActive={tweaks.signalActive}
            waveform={meters.peakWaveform}
            reduction={meters.peakReduction}
            reductionDb={meters.peakReductionDb}
          />
          <div className="stack-divider" />
          <ButterCompModule
            mode={butterMode} setMode={setButterMode}
            threshold={glueThresh} setThreshold={setGlueThresh}
            multiThresholds={multiButterThresh} setMultiThresholds={setMultiButterThresh}
            on={glueOn} setOn={setGlueOn}
            signalActive={tweaks.signalActive}
            waveform={meters.glueWaveform}
            reduction={meters.glueReduction}
            reductionDb={meters.glueReductionDb}
            bandReductionDbs={meters.glueBandReductionDb}
          />
        </div>

        {/* Vocal cleanup stacked */}
        <div className="module module-stack vocal-stack">
          <PctModule
            name="IN YOUR F** FACE"
            value={iyf} onChange={setIyf}
            on={iyfOn} setOn={setIyfOn}
            signalActive={tweaks.signalActive}
            waveform={meters.faceWaveform}
            compact
          />
          <div className="stack-divider" />
          <DeEsserModule
            low={deEsserLow} setLow={setDeEsserLow}
            high={deEsserHigh} setHigh={setDeEsserHigh}
            reduction={deEsserAmount} setReduction={setDeEsserAmount}
            on={deEsserOn} setOn={setDeEsserOn}
            signalActive={tweaks.signalActive}
            spectrum={meters.preCompSpectrum}
          />
          <div className="stack-divider" />
          <GateModule
            threshold={gateThresh} setThreshold={setGateThresh}
            on={gateOn} setOn={setGateOn}
            signalActive={tweaks.signalActive}
            waveform={meters.gateWaveform}
            compact
          />
        </div>

        {/* Fairy Dust Tune + Stereoids stacked */}
        <div className="module module-stack tune-stack">
          <AutoTuneModule
            amount={atAmount} setAmount={setAtAmount}
            key_={atKey} setKey={setAtKey}
            scale_={atScale} setScale={setAtScale}
            voiceType={atVoiceType} setVoiceType={setAtVoiceType}
            customMask={atCustomMask} setCustomMask={setAtCustomMask}
            on={atOn} setOn={setAtOn}
            signalActive={tweaks.signalActive}
            livePitch={liveTunePitch}
          />
          <div className="stack-divider" />
          <StereoModule
            width={stereoWidth} setWidth={setStereoWidth}
            lowBypass={lowBypass} setLowBypass={setLowBypass}
            on={stereoOn} setOn={setStereoOn}
          />
        </div>

        {/* FX */}
        <div className="modules-group two-col">
          <DelayModule state={delay} set={setDelay} on={delayOn} setOn={setDelayOn} />
          <ReverbModule state={reverb} set={setReverb} on={reverbOn} setOn={setReverbOn} />
        </div>
      </div>

      {/* ── Footer I/O ── */}
      <div className="footer-meta">
        <FooterGainControl
          label="INPUT"
          value={inputGain}
          onChange={setInputGain}
          defaultValue={defaultValues.inputGain}
          active={tweaks.signalActive}
          level={meters.inputLevel}
        />
        <FooterGainControl
          label="OUTPUT"
          value={outputGain}
          onChange={setOutputGain}
          defaultValue={defaultValues.outputGain}
          active={tweaks.signalActive}
          level={meters.outputLevel}
          output
        />
      </div>

      {/* ── Tweaks ── */}
      <TweaksPanel title="Tweaks">
        <TweakSection label="Theme" />
        <TweakSelect
          label="Palette"
          value={themeId}
          onChange={(v) => setTweak('theme', v)}
          defaultValue={TWEAK_DEFAULTS.theme}
          options={THEMES.map(t => ({ value: t.id, label: `${t.label} · Night` }))}
        />
      </TweaksPanel>
      <EditorResizeGrip enabled={nativeOnline} />
    </div>
    </div>
  );
}

export default App;
