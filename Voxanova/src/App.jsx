// Componente raíz: define el estado global del plugin y compone la UI.
// Los valores iniciales viven en utils/initialState.js.
// Toda la lógica visual vive en src/components/ y src/utils/.

import { useCallback, useEffect, useRef, useState } from "react";
import packageMeta from "../package.json";
import {
  PLUGIN_WIDTH,
  PLUGIN_HEIGHT,
  initialIO,
  initialCompressors,
  initialGlueBands,
  initialGlueMultibandOn,
  initialGateThreshold,
  initialStereoWidth,
  initialStereoLowBypass,
  initialSaturation,
  initialReverb,
  initialDelay,
  initialEffectToggles
} from "./utils/initialState.js";
import { EqPanel } from "./components/EqPanel.jsx";
import { ModuleCard } from "./components/ModuleCard.jsx";
import { DynamicsRack } from "./components/Faders.jsx";

const modules = [
  { title: "PRE-EQ", color: "cyan", type: "curve", active: true },
  { title: "PEAK TAMER", color: "blue-soft", type: "dualFader", active: true },
  { title: "GLUE", color: "blue", type: "dualFader", active: true },
  { title: "IN YOUR FACE", color: "blue-deep", type: "dualFader", active: true },
  { title: "POST-EQ", color: "cyan", type: "curve", active: true },
  { title: "GATE", color: "blue", type: "gateStereo", active: true },
  { title: "STEREO", color: "blue", type: "stereo", active: false },
  { title: "EFX", color: "blue", type: "effects", active: true }
];

const windowSizeOptions = [
  { id: "mini", label: "Mini", scale: 0.65 },
  { id: "compact", label: "Compact", scale: 0.8 },
  { id: "default", label: "Default", scale: 1 },
  { id: "large", label: "Large", scale: 1.15 }
];

const appVersion = packageMeta.version;
const nativeVersion = appVersion;
const basePageSizeLabel = `${PLUGIN_WIDTH} x ${PLUGIN_HEIGHT}`;
const COMPRESSOR_MIN_DB = -60;
const GLUE_BAND_MIN_DB = -48;
const COMPRESSOR_MAX_DB = 0;
const moduleParameterIds = {
  "PEAK TAMER": "peakEnabled",
  GLUE: "glueEnabled",
  "IN YOUR FACE": "faceEnabled",
  GATE: "gateEnabled",
  STEREO: "stereoEnabled"
};
const effectParameterIds = {
  delay: "delayEnabled",
  reverb: "reverbEnabled"
};
const delayModeToNative = { NORMAL: 0, WIDE: 1, "PING-PONG": 2 };
const nativeToDelayMode = ["NORMAL", "WIDE", "PING-PONG"];
const delayNoteModeToNative = { NOTE: 0, DOT: 1, TRIP: 2 };
const nativeToDelayNoteMode = ["NOTE", "DOT", "TRIP"];
const delayStyleOptions = [
  "Clean",
  "Digital",
  "Tape",
  "Studio Tape",
  "Old Tape",
  "Cheap Tape",
  "Analog",
  "Radio",
  "Telephone",
  "Dirty",
  "Ambient"
];
const delayStyleToNative = Object.fromEntries(delayStyleOptions.map((style, index) => [style, index]));
const saturationParameterMap = {
  pre: { mode: "preSaturationMode", amount: "preSaturationAmount" },
  post: { mode: "postSaturationMode", amount: "postSaturationAmount" }
};
const reverbModeOptions = [
  "Concert Hall",
  "Bright Hall",
  "Plate",
  "Room",
  "Chamber",
  "Random Space",
  "Chorus Space",
  "Ambience",
  "Sanctuary",
  "Dirty Hall",
  "Dirty Plate",
  "Smooth Plate",
  "Smooth Room",
  "Smooth Random",
  "Nonlin",
  "Chaotic Chamber",
  "Chaotic Hall",
  "Chaotic Neutral",
  "Cathedral",
  "Palace",
  "Chamber1979",
  "Hall1984"
];
const reverbParameterMap = {
  mix: "reverbMix",
  decay: "reverbDecay",
  size: "reverbSize",
  predelay: "reverbPredelay",
  lowCut: "reverbLowCut",
  highCut: "reverbHighCut",
  modeIndex: "reverbMode",
  bpmActive: "reverbSync",
  noteMode: "reverbNoteMode",
  decaySync: "reverbDecaySync",
  predelaySync: "reverbPredelaySync",
  decayDivisionIndex: "reverbDecayDivision",
  predelayDivisionIndex: "reverbPredelayDivision"
};
const delayParameterMap = {
  mix: "delayMix",
  feedback: "delayFeedback",
  lowCut: "delayLowCut",
  highCut: "delayHighCut",
  bpmActive: "delaySync",
  divisionIndex: "delayDivision",
  noteMode: "delayNoteMode",
  timeMs: "delayTimeMs",
  mode: "delayMode",
  postReverb: "delayPostReverb",
  style: "delayStyle"
};

const emptyMeters = {
  input: [0, 0],
  output: [0, 0],
  inputChannels: 2,
  outputChannels: 2
};

const emptyReductions = {
  gate: 0,
  peak: 0,
  glue: 0,
  face: 0
};

const emptyReductionDbs = {
  gate: 0,
  peak: 0,
  glue: 0,
  face: 0
};

const emptyFaderLevels = {
  gate: 0,
  peak: 0,
  glue: 0,
  face: 0
};

function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

function getStoredWindowSizeId() {
  try {
    return localStorage.getItem("voxanova-window-size") || "default";
  } catch {
    return "default";
  }
}

function getWindowSizeOption(id) {
  return windowSizeOptions.find((option) => option.id === id) || windowSizeOptions[1];
}

function formatMeterDb(level) {
  if (level <= 0.001) return "-inf dB";
  const db = level * 72 - 60;
  return `${db.toFixed(1)} dB`;
}

function percentToDb(value, min, max) {
  return min + (clamp(value, 0, 100) / 100) * (max - min);
}

function dbToPercent(value, min, max) {
  return clamp(((value - min) / (max - min)) * 100, 0, 100);
}

function nativeApiUrl(path, params) {
  const root = window.__JUCE__?.backend ? window.location.origin : "";
  const query = new URLSearchParams({ ...params, t: String(Date.now()) });
  return `${root}${path}?${query.toString()}`;
}

function pokeNativeResource(url) {
  const image = new Image();
  image.src = url;

  const frame = document.createElement("iframe");
  frame.style.display = "none";
  frame.src = url;
  document.body.appendChild(frame);
  window.setTimeout(() => frame.remove(), 1000);
}

const pendingNativeParameters = new Map();
let pendingNativeParameterFrame = 0;

function flushNativeParameters() {
  pendingNativeParameterFrame = 0;
  const entries = Array.from(pendingNativeParameters.entries());
  pendingNativeParameters.clear();

  entries.forEach(([id, value]) => {
    transmitNativeParameter(id, value);
  });
}

function transmitNativeParameter(id, value) {
  const url = nativeApiUrl("/api/setParameter", { id, value: String(value) });

  window.__voxanovaParameterQueue = window.__voxanovaParameterQueue || [];
  window.__voxanovaParameterQueue.push({ id, value });

  try {
    window.__JUCE__?.backend?.emitEvent?.("voxanovaSetParameter", { id, value });
  } catch {
    // Si el puente nativo no existe, usamos los fallbacks de recurso.
  }

  pokeNativeResource(url);

  fetch(url, {
    cache: "no-store"
  })
    .then((response) => (response.ok ? response.json() : Promise.reject(response)))
    .then((payload) => {
      if (payload?.ok === false) throw new Error(payload.error || "native parameter rejected");
    })
    .catch(() => {
      // En navegador normal no existe el endpoint nativo; la UI sigue funcionando visualmente.
    });
}

function sendNativeParameter(id, value) {
  pendingNativeParameters.set(id, value);

  if (pendingNativeParameterFrame) return;

  const scheduleFrame = window.requestAnimationFrame || ((callback) => window.setTimeout(callback, 16));
  pendingNativeParameterFrame = scheduleFrame(flushNativeParameters);
}

function sendNativeEditorSize(option) {
  const width = Math.round(PLUGIN_WIDTH * option.scale);
  const height = Math.round(PLUGIN_HEIGHT * option.scale);
  const url = nativeApiUrl("/api/setEditorSize", {
    scale: String(option.scale),
    width: String(width),
    height: String(height)
  });

  try {
    window.__JUCE__?.backend?.emitEvent?.("voxanovaSetEditorSize", {
      scale: option.scale,
      width,
      height
    });
  } catch {
    // Si el puente nativo no existe, usamos los fallbacks de recurso.
  }

  pokeNativeResource(url);

  fetch(url, { cache: "no-store" }).catch(() => {
    // En navegador normal no existe el endpoint nativo; el preview usa escala visual.
  });
}

function App() {
  // Toggles de cada módulo (encendido/apagado), inicializados desde el array.
  const [toggles, setToggles] = useState(() =>
    Object.fromEntries(modules.map((m) => [m.title, m.active]))
  );

  // Niveles y gains de entrada/salida.
  const [meters, setMeters] = useState(emptyMeters);
  const [reductions, setReductions] = useState(emptyReductions);
  const [reductionDbs, setReductionDbs] = useState(emptyReductionDbs);
  const [faderLevels, setFaderLevels] = useState(emptyFaderLevels);
  const [inputGain, setInputGain] = useState(initialIO.inputGain);
  const [outputGain, setOutputGain] = useState(initialIO.outputGain);

  // Compresores: peakTamer, glue, inYourFace.
  const [peakTamer, setPeakTamer] = useState(initialCompressors.peakTamer);
  const [glue, setGlue] = useState(initialCompressors.glue);
  const [glueMultibandOn, setGlueMultibandOn] = useState(initialGlueMultibandOn);
  const [glueBands, setGlueBands] = useState(() => ({ ...initialGlueBands }));
  const [inYourFace, setInYourFace] = useState(initialCompressors.inYourFace);

  // Gate y Stereo (controles individuales).
  const [gateThreshold, setGateThreshold] = useState(initialGateThreshold);
  const [stereoWidth, setStereoWidth] = useState(initialStereoWidth);
  const [stereoLowBypass, setStereoLowBypass] = useState(initialStereoLowBypass);
  const [saturation, setSaturation] = useState(initialSaturation);

  // Efectos como objetos agrupados.
  const [reverb, setReverb] = useState(initialReverb);
  const [delay, setDelay] = useState(initialDelay);
  const [hostBpm, setHostBpm] = useState(120);
  const [effectToggles, setEffectToggles] = useState(initialEffectToggles);

  // Escala visual (responsive).
  const [pluginScale, setPluginScale] = useState(1);
  const [menuOpen, setMenuOpen] = useState(false);
  const [windowSizeId, setWindowSizeId] = useState(getStoredWindowSizeId);
  const menuRef = useRef(null);
  const pendingNativeValuesRef = useRef({});
  const resizeDragRef = useRef({ active: false, id: -1, x: 0, y: 0, scale: 1 });
  const pendingEditorScaleRef = useRef(null);
  const pendingEditorFrameRef = useRef(0);

  // Estado expandido del EQ: el rack debe poder recuperar prioridad cuando el usuario vuelve a él.
  const [eqExpanded, setEqExpanded] = useState(false);
  const releaseEqExpansion = useCallback(() => {
    setEqExpanded((current) => (current ? false : current));
  }, []);

  // Meters reales desde JUCE. En navegador normal se quedan en cero.
  useEffect(() => {
    const onMeterUpdate = (event) => {
      const payload = event.detail || {};
      setMeters({
        input: [payload.inputL ?? 0, payload.inputR ?? payload.inputL ?? 0],
        output: [payload.outputL ?? 0, payload.outputR ?? payload.outputL ?? 0],
        inputChannels: payload.inputChannels ?? 2,
        outputChannels: payload.outputChannels ?? 2
      });
      setReductions({
        gate: payload.gateGr ?? 0,
        peak: payload.peakGr ?? 0,
        glue: payload.glueGr ?? 0,
        face: payload.faceGr ?? 0
      });
      setReductionDbs({
        gate: payload.gateGrDb ?? 0,
        peak: payload.peakGrDb ?? 0,
        glue: payload.glueGrDb ?? 0,
        face: payload.faceGrDb ?? 0
      });
      setFaderLevels({
        gate: payload.gateLevel ?? 0,
        peak: payload.peakLevel ?? 0,
        glue: payload.glueLevel ?? 0,
        face: payload.faceLevel ?? 0
      });
      if (typeof payload.hostBpm === "number") {
        setHostBpm(clamp(payload.hostBpm, 20, 300));
      }

      const now = performance.now();
      const pending = pendingNativeValuesRef.current;
      const isPending = (id) => pending[id] && pending[id].until >= now;

      if (typeof payload.inputGain === "number" && !isPending("inputGain")) {
        setInputGain(payload.inputGain);
      }

      if (typeof payload.outputGain === "number" && !isPending("outputGain")) {
        setOutputGain(payload.outputGain);
      }

      if (typeof payload.gateThreshold === "number" && !isPending("gateThreshold")) {
        setGateThreshold(dbToPercent(payload.gateThreshold, -80, 0));
      }

      if (typeof payload.stereoWidth === "number" && !isPending("stereoWidth")) {
        setStereoWidth(clamp(payload.stereoWidth, 0, 100));
      }

      if (typeof payload.stereoLowBypass === "number" && !isPending("stereoLowBypass")) {
        setStereoLowBypass(clamp(payload.stereoLowBypass, 0, 500));
      }

      Object.entries(saturationParameterMap).forEach(([stage, ids]) => {
        if (typeof payload[ids.mode] === "number" && !isPending(ids.mode)) {
          setSaturation((current) => ({
            ...current,
            [stage]: { ...current[stage], mode: Math.round(clamp(payload[ids.mode], 0, 3)) }
          }));
        }

        if (typeof payload[ids.amount] === "number" && !isPending(ids.amount)) {
          setSaturation((current) => ({
            ...current,
            [stage]: { ...current[stage], amount: clamp(payload[ids.amount], 0, 100) }
          }));
        }
      });

      Object.entries(moduleParameterIds).forEach(([title, parameterId]) => {
        if (typeof payload[parameterId] === "number" && !isPending(parameterId)) {
          setToggles((current) => ({ ...current, [title]: payload[parameterId] >= 0.5 }));
        }
      });

      if (typeof payload.peakThreshold === "number" && !isPending("peakThreshold")) {
        setPeakTamer((current) => ({
          ...current,
          thr: dbToPercent(payload.peakThreshold, COMPRESSOR_MIN_DB, COMPRESSOR_MAX_DB)
        }));
      }

      if (typeof payload.glueThreshold === "number" && !isPending("glueThreshold")) {
        setGlue((current) => ({
          ...current,
          thr: dbToPercent(payload.glueThreshold, COMPRESSOR_MIN_DB, COMPRESSOR_MAX_DB)
        }));
      }

      if (typeof payload.glueMultiband === "number" && !isPending("glueMultiband")) {
        setGlueMultibandOn(payload.glueMultiband >= 0.5);
      }

      const glueBandPayloadMap = {
        low: "glueLowThreshold",
        lowMid: "glueLowMidThreshold",
        highMid: "glueHighMidThreshold",
        high: "glueAirThreshold"
      };

      Object.entries(glueBandPayloadMap).forEach(([band, parameterId]) => {
        if (typeof payload[parameterId] === "number" && !isPending(parameterId)) {
          setGlueBands((current) => ({
            ...current,
            [band]: dbToPercent(payload[parameterId], GLUE_BAND_MIN_DB, COMPRESSOR_MAX_DB)
          }));
        }
      });

      if (typeof payload.faceThreshold === "number" && !isPending("faceThreshold")) {
        setInYourFace((current) => ({
          ...current,
          thr: clamp(payload.faceThreshold, 0, 100)
        }));
      }

      if (typeof payload.reverbMix === "number" && !isPending("reverbMix")) {
        setReverb((current) => ({ ...current, mix: clamp(payload.reverbMix, 0, 100) }));
      }

      if (typeof payload.reverbDecay === "number" && !isPending("reverbDecay")) {
        setReverb((current) => ({ ...current, decay: clamp(payload.reverbDecay, 0, 100) }));
      }

      if (typeof payload.reverbSize === "number" && !isPending("reverbSize")) {
        setReverb((current) => ({ ...current, size: clamp(payload.reverbSize, 0, 100) }));
      }

      if (typeof payload.reverbPredelay === "number" && !isPending("reverbPredelay")) {
        setReverb((current) => ({ ...current, predelay: clamp(payload.reverbPredelay, 0, 100) }));
      }

      if (typeof payload.reverbLowCut === "number" && !isPending("reverbLowCut")) {
        setReverb((current) => ({ ...current, lowCut: clamp(payload.reverbLowCut, 0, 100) }));
      }

      if (typeof payload.reverbHighCut === "number" && !isPending("reverbHighCut")) {
        setReverb((current) => ({ ...current, highCut: clamp(payload.reverbHighCut, 0, 100) }));
      }

      if (typeof payload.reverbMode === "number" && !isPending("reverbMode")) {
        setReverb((current) => ({
          ...current,
          modeIndex: Math.round(clamp(payload.reverbMode, 0, reverbModeOptions.length - 1))
        }));
      }

      if (typeof payload.reverbSync === "number" && !isPending("reverbSync")) {
        setReverb((current) => ({ ...current, bpmActive: payload.reverbSync >= 0.5 }));
      }

      if (typeof payload.reverbNoteMode === "number" && !isPending("reverbNoteMode")) {
        setReverb((current) => ({
          ...current,
          noteMode: nativeToDelayNoteMode[Math.round(clamp(payload.reverbNoteMode, 0, 2))] ?? "NOTE"
        }));
      }

      if (typeof payload.reverbDecaySync === "number" && !isPending("reverbDecaySync")) {
        setReverb((current) => ({ ...current, decaySync: payload.reverbDecaySync >= 0.5 }));
      }

      if (typeof payload.reverbPredelaySync === "number" && !isPending("reverbPredelaySync")) {
        setReverb((current) => ({ ...current, predelaySync: payload.reverbPredelaySync >= 0.5 }));
      }

      if (typeof payload.reverbDecayDivision === "number" && !isPending("reverbDecayDivision")) {
        setReverb((current) => ({
          ...current,
          decayDivisionIndex: Math.round(clamp(payload.reverbDecayDivision, 0, 6))
        }));
      }

      if (typeof payload.reverbPredelayDivision === "number" && !isPending("reverbPredelayDivision")) {
        setReverb((current) => ({
          ...current,
          predelayDivisionIndex: Math.round(clamp(payload.reverbPredelayDivision, 0, 7))
        }));
      }

      if (typeof payload.delayMix === "number" && !isPending("delayMix")) {
        setDelay((current) => ({ ...current, mix: clamp(payload.delayMix, 0, 100) }));
      }

      if (typeof payload.delayFeedback === "number" && !isPending("delayFeedback")) {
        setDelay((current) => ({ ...current, feedback: clamp(payload.delayFeedback, 0, 100) }));
      }

      if (typeof payload.delayLowCut === "number" && !isPending("delayLowCut")) {
        setDelay((current) => ({ ...current, lowCut: clamp(payload.delayLowCut, 0, 100) }));
      }

      if (typeof payload.delayHighCut === "number" && !isPending("delayHighCut")) {
        setDelay((current) => ({ ...current, highCut: clamp(payload.delayHighCut, 0, 100) }));
      }

      if (typeof payload.delaySync === "number" && !isPending("delaySync")) {
        setDelay((current) => ({ ...current, bpmActive: payload.delaySync >= 0.5 }));
      }

      if (typeof payload.delayDivision === "number" && !isPending("delayDivision")) {
        setDelay((current) => ({ ...current, divisionIndex: Math.round(clamp(payload.delayDivision, 0, 6)) }));
      }

      if (typeof payload.delayNoteMode === "number" && !isPending("delayNoteMode")) {
        setDelay((current) => ({
          ...current,
          noteMode: nativeToDelayNoteMode[Math.round(clamp(payload.delayNoteMode, 0, 2))] ?? "NOTE"
        }));
      }

      if (typeof payload.delayTimeMs === "number" && !isPending("delayTimeMs")) {
        setDelay((current) => ({ ...current, timeMs: clamp(payload.delayTimeMs, 1, 2000) }));
      }

      if (typeof payload.delayMode === "number" && !isPending("delayMode")) {
        setDelay((current) => ({
          ...current,
          mode: nativeToDelayMode[Math.round(clamp(payload.delayMode, 0, 2))] ?? "NORMAL"
        }));
      }

      if (typeof payload.delayPostReverb === "number" && !isPending("delayPostReverb")) {
        setDelay((current) => ({ ...current, postReverb: payload.delayPostReverb >= 0.5 }));
      }

      if (typeof payload.delayStyle === "number" && !isPending("delayStyle")) {
        setDelay((current) => ({
          ...current,
          style: delayStyleOptions[Math.round(clamp(payload.delayStyle, 0, delayStyleOptions.length - 1))] ?? "Clean"
        }));
      }

      Object.entries(effectParameterIds).forEach(([key, parameterId]) => {
        if (typeof payload[parameterId] === "number" && !isPending(parameterId)) {
          setEffectToggles((current) => ({
            ...current,
            [key]: payload[parameterId] >= 0.5 ? current[key] === "aux" ? "aux" : "on" : "off"
          }));
        }
      });
    };

    window.addEventListener("voxanovaMeterUpdate", onMeterUpdate);

    return () => {
      window.removeEventListener("voxanovaMeterUpdate", onMeterUpdate);
    };
  }, []);

  // Escala el plugin para que quepa en la ventana sin recortarse.
  useEffect(() => {
    const updateScale = () => {
      const viewportWidth = window.innerWidth;
      const viewportHeight = window.innerHeight;
      const nativeScale = Math.min(viewportWidth / PLUGIN_WIDTH, viewportHeight / PLUGIN_HEIGHT);

      if (window.__JUCE__?.backend) {
        setPluginScale(clamp(nativeScale, 0.6, 1.2));
        return;
      }

      if (viewportWidth >= PLUGIN_WIDTH && viewportHeight >= PLUGIN_HEIGHT) {
        setPluginScale(1);
        return;
      }

      const maxWidth = Math.max(viewportWidth - 24, 320);
      const maxHeight = Math.max(viewportHeight - 24, 320);
      setPluginScale(Math.min(maxWidth / PLUGIN_WIDTH, maxHeight / PLUGIN_HEIGHT, 1));
    };
    updateScale();
    window.addEventListener("resize", updateScale);
    return () => window.removeEventListener("resize", updateScale);
  }, []);

  useEffect(() => {
    const closeMenu = (event) => {
      if (!menuRef.current?.contains(event.target)) setMenuOpen(false);
    };

    const closeWithEscape = (event) => {
      if (event.key === "Escape") setMenuOpen(false);
    };

    document.addEventListener("pointerdown", closeMenu);
    document.addEventListener("keydown", closeWithEscape);
    return () => {
      document.removeEventListener("pointerdown", closeMenu);
      document.removeEventListener("keydown", closeWithEscape);
    };
  }, []);

  useEffect(() => {
    const selectedOption = getWindowSizeOption(windowSizeId);

    try {
      localStorage.setItem("voxanova-window-size", selectedOption.id);
    } catch {
      // LocalStorage puede no existir en algunos hosts; el preset sigue funcionando en sesión.
    }

    const timer = window.setTimeout(() => sendNativeEditorSize(selectedOption), 80);
    return () => window.clearTimeout(timer);
  }, [windowSizeId]);

  useEffect(
    () => () => {
      if (pendingEditorFrameRef.current) {
        window.cancelAnimationFrame(pendingEditorFrameRef.current);
      }
    },
    []
  );

  const scheduleEditorScale = useCallback((scale) => {
    pendingEditorScaleRef.current = scale;

    if (pendingEditorFrameRef.current) return;

    pendingEditorFrameRef.current = window.requestAnimationFrame(() => {
      pendingEditorFrameRef.current = 0;
      const nextScale = pendingEditorScaleRef.current;
      if (nextScale == null) return;
      sendNativeEditorSize({ id: "custom", scale: nextScale });
    });
  }, []);

  const getResizeLimit = useCallback(() => 1.25, []);

  const applyResizeScale = useCallback(
    (scale) => {
      const nextScale = Number(clamp(scale, 0.38, getResizeLimit()).toFixed(3));
      setPluginScale(nextScale);
      scheduleEditorScale(nextScale);
    },
    [getResizeLimit, scheduleEditorScale]
  );

  const updateResizeFromPointer = useCallback(
    (clientX, clientY) => {
      const drag = resizeDragRef.current;
      if (!drag.active) return;
      const deltaX = clientX - drag.x;
      const deltaY = clientY - drag.y;
      const deltaScale = (deltaX / PLUGIN_WIDTH + deltaY / PLUGIN_HEIGHT) / 2;
      applyResizeScale(drag.scale + deltaScale);
    },
    [applyResizeScale]
  );

  const moveWindowResize = (event) => {
    const pointerId = event.pointerId ?? "mouse";
    if (!resizeDragRef.current.active || resizeDragRef.current.id !== pointerId) return;
    event.preventDefault();
    updateResizeFromPointer(event.clientX, event.clientY);
  };

  const endWindowResize = (event) => {
    const pointerId = event.pointerId ?? "mouse";
    if (resizeDragRef.current.id !== pointerId) return;
    updateResizeFromPointer(event.clientX, event.clientY);
    resizeDragRef.current = { active: false, id: -1, x: 0, y: 0, scale: pluginScale };
    document.body.classList.remove("is-window-resizing");
    document.removeEventListener("mousemove", moveWindowResize);
    document.removeEventListener("mouseup", endWindowResize);
  };

  const startWindowResize = (event) => {
    if (event.type === "pointerdown" && event.pointerType === "mouse") return;
    if (resizeDragRef.current.active) return;
    event.preventDefault();
    event.stopPropagation();
    const pointerId = event.pointerId ?? "mouse";
    resizeDragRef.current = {
      active: true,
      id: pointerId,
      x: event.clientX,
      y: event.clientY,
      scale: pluginScale
    };
    event.currentTarget.setPointerCapture?.(pointerId);
    if (pointerId === "mouse") {
      document.addEventListener("mousemove", moveWindowResize);
      document.addEventListener("mouseup", endWindowResize);
    }
    document.body.classList.add("is-window-resizing");
  };

  const toggleModule = (title) =>
    setToggles((current) => {
      const enabled = !current[title];
      const parameterId = moduleParameterIds[title];

      if (parameterId) {
        const nativeValue = enabled ? 1 : 0;
        markPendingNativeValue(parameterId, nativeValue);
        sendNativeParameter(parameterId, nativeValue);
      }

      return { ...current, [title]: enabled };
    });

  const toggleGlueModule = () => {
    if (!toggles.GLUE) {
      setNativeGlueMultiband(false);
      setToggles((current) => ({ ...current, GLUE: true }));
      markPendingNativeValue("glueEnabled", 1);
      sendNativeParameter("glueEnabled", 1);
      return;
    }

    if (!glueMultibandOn) {
      setNativeGlueMultiband(true);
      return;
    }

    setNativeGlueMultiband(false);
    setToggles((current) => ({ ...current, GLUE: false }));
    markPendingNativeValue("glueEnabled", 0);
    sendNativeParameter("glueEnabled", 0);
  };

  const toggleEffect = (key, desiredState) =>
    setEffectToggles((current) => {
      const currentValue = current[key] === true ? "on" : current[key] || "off";
      const nextValue =
        desiredState ?? (currentValue === "on" ? "aux" : currentValue === "aux" ? "off" : "on");
      const parameterId = effectParameterIds[key];

      if (parameterId) {
        const nativeValue = nextValue === "off" ? 0 : 1;
        markPendingNativeValue(parameterId, nativeValue);
        sendNativeParameter(parameterId, nativeValue);
      }

      return { ...current, [key]: nextValue };
    });

  const markPendingNativeValue = (id, value) => {
    pendingNativeValuesRef.current[id] = { value, until: performance.now() + 1500 };
  };

  const setNativeInputGain = (value) => {
    setInputGain(value);
    markPendingNativeValue("inputGain", value);
    sendNativeParameter("inputGain", value);
  };

  const setNativeOutputGain = (value) => {
    setOutputGain(value);
    markPendingNativeValue("outputGain", value);
    sendNativeParameter("outputGain", value);
  };

  const setNativeGateThreshold = (value) => {
    setGateThreshold(value);
    const nativeValue = percentToDb(value, -80, 0);
    markPendingNativeValue("gateThreshold", nativeValue);
    sendNativeParameter("gateThreshold", nativeValue);
  };

  const setNativeStereoWidth = (value) => {
    const nativeValue = clamp(value, 0, 100);
    setStereoWidth(nativeValue);
    markPendingNativeValue("stereoWidth", nativeValue);
    sendNativeParameter("stereoWidth", nativeValue);
  };

  const setNativeStereoLowBypass = (value) => {
    const nativeValue = clamp(value, 0, 500);
    setStereoLowBypass(nativeValue);
    markPendingNativeValue("stereoLowBypass", nativeValue);
    sendNativeParameter("stereoLowBypass", nativeValue);
  };

  const setNativeSaturation = (stage, nextValue) => {
    const ids = saturationParameterMap[stage];
    if (!ids) return;

    const nextMode = Math.round(clamp(nextValue.mode ?? 0, 0, 3));
    const nextAmount = clamp(nextValue.amount ?? 0, 0, 100);
    const nextStage = { mode: nextMode, amount: nextAmount };

    setSaturation((current) => ({ ...current, [stage]: nextStage }));

    if (nextMode !== saturation[stage].mode) {
      markPendingNativeValue(ids.mode, nextMode);
      sendNativeParameter(ids.mode, nextMode);
    }

    if (nextAmount !== saturation[stage].amount) {
      markPendingNativeValue(ids.amount, nextAmount);
      sendNativeParameter(ids.amount, nextAmount);
    }
  };

  const setNativeCompressorThreshold = (parameterId, setter) => (value) => {
    setter(value);
    const nativeValue = percentToDb(value.thr, COMPRESSOR_MIN_DB, COMPRESSOR_MAX_DB);
    markPendingNativeValue(parameterId, nativeValue);
    sendNativeParameter(parameterId, nativeValue);
  };

  const setNativeGlueMultiband = (enabled) => {
    setGlueMultibandOn(enabled);
    const nativeValue = enabled ? 1 : 0;
    markPendingNativeValue("glueMultiband", nativeValue);
    sendNativeParameter("glueMultiband", nativeValue);
  };

  const setNativeGlueBandThreshold = (band, value) => {
    const parameterByBand = {
      low: "glueLowThreshold",
      lowMid: "glueLowMidThreshold",
      highMid: "glueHighMidThreshold",
      high: "glueAirThreshold"
    };
    const parameterId = parameterByBand[band];

    setGlueBands((current) => ({ ...current, [band]: value }));

    if (!parameterId) return;

    const nativeValue = percentToDb(value, GLUE_BAND_MIN_DB, COMPRESSOR_MAX_DB);
    markPendingNativeValue(parameterId, nativeValue);
    sendNativeParameter(parameterId, nativeValue);
  };

  const setNativeFaceMix = (value) => {
    const nativeValue = clamp(value, 0, 100);
    setInYourFace((current) => ({ ...current, thr: nativeValue }));
    markPendingNativeValue("faceThreshold", nativeValue);
    sendNativeParameter("faceThreshold", nativeValue);
  };

  const setNativeReverb = (nextReverb) => {
    setReverb(nextReverb);

    Object.entries(reverbParameterMap).forEach(([key, parameterId]) => {
      if (nextReverb[key] === reverb[key]) return;

      let nativeValue = nextReverb[key];

      if (key === "bpmActive" || key === "decaySync" || key === "predelaySync") {
        nativeValue = nativeValue ? 1 : 0;
      } else if (key === "noteMode") {
        nativeValue = delayNoteModeToNative[nativeValue] ?? delayNoteModeToNative.NOTE;
      }

      if (typeof nativeValue !== "number") return;

      markPendingNativeValue(parameterId, nativeValue);
      sendNativeParameter(parameterId, nativeValue);
    });
  };

  const setNativeDelay = (nextDelay) => {
    setDelay(nextDelay);

    Object.entries(delayParameterMap).forEach(([key, parameterId]) => {
      if (nextDelay[key] === delay[key]) return;

      let nativeValue = nextDelay[key];

      if (key === "bpmActive" || key === "postReverb") {
        nativeValue = nativeValue ? 1 : 0;
      } else if (key === "mode") {
        nativeValue = delayModeToNative[nativeValue] ?? delayModeToNative.NORMAL;
      } else if (key === "noteMode") {
        nativeValue = delayNoteModeToNative[nativeValue] ?? delayNoteModeToNative.NOTE;
      } else if (key === "style") {
        nativeValue = delayStyleToNative[nativeValue] ?? delayStyleToNative.Clean;
      }

      if (typeof nativeValue !== "number") return;

      markPendingNativeValue(parameterId, nativeValue);
      sendNativeParameter(parameterId, nativeValue);
    });
  };

  const setWindowSize = (option) => {
    setWindowSizeId(option.id);
    setMenuOpen(false);
    sendNativeEditorSize(option);

    if (window.__JUCE__?.backend) return;

    const previewScale = Math.min(
      option.scale,
      Math.max((window.innerWidth - 24) / PLUGIN_WIDTH, 0.6),
      Math.max((window.innerHeight - 24) / PLUGIN_HEIGHT, 0.6)
    );
    setPluginScale(previewScale);
  };

  // Props compartidas para los ModuleCard (gate/stereo y effects).
  const sharedModuleProps = {
    stereoWidth,
    onStereoChange: setNativeStereoWidth,
    stereoLowBypass,
    onStereoLowBypassChange: setNativeStereoLowBypass,
    gateThreshold,
    onGateThresholdChange: setNativeGateThreshold,
    gateLevel: faderLevels.gate,
    gateReduction: reductions.gate,
    gateReductionDb: reductionDbs.gate,
    reverb,
    setReverb: setNativeReverb,
    delay,
    setDelay: setNativeDelay,
    hostBpm,
    effectToggles,
    onEffectToggle: toggleEffect,
    eqExpanded
  };

  // Configuración del rack de compresores (los 3 dualFader en uno).
  const compressorRack = [
    {
      title: modules[1].title,
      color: modules[1].color,
      active: toggles[modules[1].title],
      value: { ...peakTamer, gr: reductions.peak, grDb: reductionDbs.peak, level: faderLevels.peak },
      onToggle: () => toggleModule(modules[1].title),
      onChange: setNativeCompressorThreshold("peakThreshold", setPeakTamer)
    },
    {
      title: modules[2].title,
      color: modules[2].color,
      active: toggles[modules[2].title],
      value: { ...glue, gr: reductions.glue, grDb: reductionDbs.glue, level: faderLevels.glue },
      onToggle: toggleGlueModule,
      onChange: setNativeCompressorThreshold("glueThreshold", setGlue)
    },
    {
      title: modules[3].title,
      color: modules[3].color,
      active: toggles[modules[3].title],
      control: "mix",
      value: { ...inYourFace, gr: reductions.face, grDb: reductionDbs.face, level: faderLevels.face },
      onToggle: () => toggleModule(modules[3].title),
      onChange: (nextValue) => setNativeFaceMix(nextValue.thr)
    }
  ];

  return (
    <main className="app-shell">
      <div
        className="plugin-viewport"
        style={{
          width: `${Math.round(PLUGIN_WIDTH * pluginScale)}px`,
          height: `${Math.round(PLUGIN_HEIGHT * pluginScale)}px`
        }}
      >
        <section
          className="plugin-frame"
          style={{
            width: `${PLUGIN_WIDTH}px`,
            height: `${PLUGIN_HEIGHT}px`,
            zoom: pluginScale
          }}
        >
          <header className="topbar">
            <div className="brand">
              <span className="brand-main">VOXANOVA</span>
              <span className="brand-sub">VOCAL CHAIN</span>
            </div>
            <div className="window-menu" ref={menuRef}>
              <button
                className={`menu-button ${menuOpen ? "is-open" : ""}`}
                aria-label="Menu"
                aria-expanded={menuOpen}
                type="button"
                onClick={() => setMenuOpen((open) => !open)}
              >
                <span />
                <span />
                <span />
              </button>

              {menuOpen && (
                <div className="app-menu-panel" role="menu" aria-label="Voxanova menu">
                  <section className="app-menu-section">
                    <span className="app-menu-title">About</span>
                    <p className="app-menu-copy">
                      Voxanova Vocal Chain. Clean vocal processing UI for Logic, AU, VST3 and
                      Standalone.
                    </p>
                  </section>

                  <section className="app-menu-section">
                    <span className="app-menu-title">Version</span>
                    <div className="app-menu-row">
                      <span>App</span>
                      <em>v{appVersion}</em>
                    </div>
                    <div className="app-menu-row">
                      <span>Native</span>
                      <em>v{nativeVersion}</em>
                    </div>
                    <div className="app-menu-row">
                      <span>Base UI</span>
                      <em>{basePageSizeLabel}</em>
                    </div>
                  </section>

                  <section className="app-menu-section">
                    <span className="app-menu-title">Page Size</span>
                    {windowSizeOptions.map((option) => (
                      <button
                        className={`window-size-option ${option.id === windowSizeId ? "active" : ""}`}
                        key={option.id}
                        type="button"
                        role="menuitemradio"
                        aria-checked={option.id === windowSizeId}
                        onClick={() => setWindowSize(option)}
                      >
                        <span>{option.label}</span>
                        <em>{Math.round(option.scale * 100)}%</em>
                      </button>
                    ))}
                  </section>
                </div>
              )}
            </div>
          </header>

          <div className="content-grid">
            <section className="center-stage">
              <EqPanel
                expanded={eqExpanded}
                onExpand={setEqExpanded}
                saturation={saturation}
                onSaturationChange={setNativeSaturation}
                inputMeter={{
                  title: "IN",
                  value: formatMeterDb(Math.max(...meters.input)),
                  gain: inputGain,
                  onGainChange: setNativeInputGain,
                  levels: meters.input,
                  channelCount: meters.inputChannels,
                  color: "blue"
                }}
                outputMeter={{
                  title: "OUT",
                  value: formatMeterDb(Math.max(...meters.output)),
                  gain: outputGain,
                  onGainChange: setNativeOutputGain,
                  levels: meters.output,
                  channelCount: meters.outputChannels,
                  color: "orange"
                }}
              />

              <section
                className="module-strip"
                onMouseEnter={releaseEqExpansion}
                onPointerEnter={releaseEqExpansion}
                onPointerMove={releaseEqExpansion}
                onFocusCapture={releaseEqExpansion}
              >
                <section className="compressor-stack">
                  <DynamicsRack
                    compressors={compressorRack}
                    glueBands={glueBands}
                    glueMultibandOn={glueMultibandOn}
                    onGlueBandChange={setNativeGlueBandThreshold}
                  />
                </section>

                <section className="utility-stack">
                  <ModuleCard
                    module={modules[5]}
                    active={toggles[modules[5].title]}
                    onToggle={() => toggleModule(modules[5].title)}
                    stereoActive={toggles.STEREO}
                    onStereoToggle={() => toggleModule("STEREO")}
                    {...sharedModuleProps}
                  />
                </section>

                <ModuleCard
                  module={modules[7]}
                  active={toggles[modules[7].title]}
                  onToggle={() => toggleModule(modules[7].title)}
                  {...sharedModuleProps}
                />
              </section>
            </section>
          </div>
          <button
            type="button"
            className="plugin-resize-grip"
            aria-label="Resize plugin window"
            title="Resize"
            onPointerDown={startWindowResize}
            onPointerMove={moveWindowResize}
            onPointerUp={endWindowResize}
            onPointerCancel={endWindowResize}
            onMouseDown={startWindowResize}
          >
            <span />
            <span />
            <span />
          </button>
        </section>
      </div>
    </main>
  );
}

export default App;
