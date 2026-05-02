// Valores iniciales del plugin. Todos los números mágicos viven aquí
// para que sea obvio qué valor por defecto tiene cada control.
// Si quieres cambiar un valor inicial, edita SOLO este archivo.

export const initialIO = {
  inputLevel: 0.72,
  outputLevel: 0.78,
  inputGain: 0,
  outputGain: 0
};

export const initialCompressors = {
  peakTamer: { thr: 100, gr: 35 },
  glue: { thr: 100, gr: 25 },
  inYourFace: { thr: 0, gr: 18 }
};

export const initialGlueMultibandOn = false;

export const initialGlueBands = {
  low: 100,
  lowMid: 100,
  highMid: 100,
  high: 100
};

export const initialGateThreshold = 0;
export const initialStereoWidth = 0;
export const initialStereoLowBypass = 0;

export const initialSaturation = {
  pre: { mode: 0, amount: 0 },
  post: { mode: 0, amount: 0 }
};

export const initialReverb = {
  modeIndex: 0,
  noteMode: "NOTE",
  bpmActive: true,
  decaySync: true,
  predelaySync: true,
  decayDivisionIndex: 2,
  predelayDivisionIndex: 3,
  size: 68,
  decay: 72,
  mix: 0,
  predelay: 40,
  lowCut: 0,
  highCut: 100
};

export const initialDelay = {
  mix: 0,
  mode: "NORMAL",
  style: "Clean",
  postReverb: false,
  bpmActive: true,
  divisionIndex: 2,
  noteMode: "NOTE",
  timeMs: 500,
  feedback: 35,
  lowCut: 0,
  highCut: 100
};

export const initialEffectToggles = {
  reverb: "off",
  delay: "off"
};

export const initialDelayUi = {
  mode: "NORMAL",
  postReverb: false,
  bus: "Bus 1",
  divisionIndex: 2,
  manualMs: 500,
  noteMode: "NOTE",
  bpmActive: true
};

export const initialReverbUi = {
  bus: "Bus 1",
  modeIndex: 0,
  noteMode: "NOTE",
  bpmActive: true,
  decaySync: true,
  predelaySync: true,
  decayDivisionIndex: 2,
  predelayDivisionIndex: 3
};

export const PLUGIN_WIDTH = 1360;
export const PLUGIN_HEIGHT = 820;
