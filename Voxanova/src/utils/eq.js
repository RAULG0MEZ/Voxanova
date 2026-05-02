// Cálculos puros del ecualizador (sin React).
// Todas las funciones de mapeo frecuencia<->pixel, dB<->pixel,
// rangos de visualización y ganancia por banda viven aquí.

import { clamp } from "./format.js";

export const bandColors = ["#0a84ff", "#60758d", "#7f8ea3", "#3f7fa6"];

const ORANGE = "#ff8a45";
const GRAPH_LEFT = 33;
const GRAPH_RIGHT = 33;
const GRAPH_TOP = 24;
const GRAPH_BOTTOM = 52;

export function getBandColor(type, existingBands) {
  if (type === "Low Cut" || type === "High Shelf") return ORANGE;
  const regularCount = existingBands.filter(
    (b) => b.type !== "Low Cut" && b.type !== "High Shelf"
  ).length;
  return bandColors[regularCount % bandColors.length];
}

export const FILTER_TYPES = [
  "Bell",
  "Surfer Bell",
  "Low Cut",
  "High Cut",
  "Low Shelf",
  "High Shelf",
  "Notch",
  "Band Pass"
];

export function getEqGraphBounds(width, height) {
  const left = GRAPH_LEFT;
  const top = GRAPH_TOP;
  return {
    left,
    top,
    graphW: Math.max(240, width - left - GRAPH_RIGHT),
    graphH: Math.max(160, height - top - GRAPH_BOTTOM)
  };
}

export function xFromFreq(freq, width) {
  const { left, graphW } = getEqGraphBounds(width, 260);
  return left + (Math.log10(freq / 20) / Math.log10(1000)) * graphW;
}

export function freqFromX(x, width) {
  const { left, graphW } = getEqGraphBounds(width, 260);
  return 20 * 1000 ** clamp((x - left) / graphW, 0, 1);
}

export function yFromDb(db, height, range = 12) {
  const { top, graphH } = getEqGraphBounds(1000, height);
  return top + (graphH / 2) * (1 - db / range);
}

export function dbFromY(y, height, range = 12) {
  const { top, graphH } = getEqGraphBounds(1000, height);
  return range * (1 - 2 * ((y - top) / graphH));
}

export function getNeededEqRange(bands) {
  const maxGain = bands
    .filter((b) => !b.type?.includes("Cut"))
    .reduce((max, b) => Math.max(max, Math.abs(b.gain || 0)), 0);
  return maxGain > 12 ? 30 : 12;
}

export function getDisplayEqRange(range) {
  if (range < 4.5) return 3;
  if (range < 9) return 6;
  if (range < 21) return 12;
  return 30;
}

export function getEqGridStep(range) {
  return range <= 6 ? range / 3 : range <= 12 ? 3 : 6;
}

export function formatEqAxisValue(value) {
  const rounded = Math.round(value * 10) / 10;
  const text = Number.isInteger(rounded) ? `${rounded}` : rounded.toFixed(1);
  return rounded > 0 ? `+${text}` : text;
}

// Ganancia (en dB) que aporta UNA banda a una frecuencia dada.
// Esta es la única fuente de verdad para el cálculo de bandas:
// la usa el dibujo del canvas y también la detección de clicks.
export function bandShapeGainAt(band, freq) {
  const logD = Math.log10(freq / band.freq);
  const q = band.q;
  switch (band.type) {
    case "Low Shelf":
      return band.gain / (1 + (freq / band.freq) ** (q * 2));
    case "High Shelf":
      return band.gain / (1 + (band.freq / freq) ** (q * 2));
    case "Notch":
      return -Math.abs(band.gain) * Math.exp(-logD * logD * q * 10);
    case "Band Pass":
      return band.gain * Math.exp(-logD * logD * q * 2);
    case "Low Cut": {
      const n = (band.slope || 12) / 6;
      const cut = -10 * Math.log10(1 + (band.freq / freq) ** (2 * n));
      return cut + (band.gain || 0) * Math.exp(-logD * logD * q * 3);
    }
    case "High Cut": {
      const n = (band.slope || 12) / 6;
      const cut = -10 * Math.log10(1 + (freq / band.freq) ** (2 * n));
      return cut + (band.gain || 0) * Math.exp(-logD * logD * q * 3);
    }
    case "Bell":
    case "Surfer Bell":
    default:
      return band.gain * Math.exp(-logD * logD * q * 5);
  }
}

export function bandGainAt(band, freq) {
  if (!band.on) return 0;
  return bandShapeGainAt(band, freq);
}
