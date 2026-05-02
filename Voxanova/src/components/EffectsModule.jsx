// Sección de efectos: Reverb + Delay con sus controles.
// Recibe los valores como objetos `reverb` y `delay`, y los setters
// para actualizarlos. Internamente usa un helper para no repetir
// la pirueta de "{...reverb, decay: nuevoValor}" cada vez.

import { useState, useRef, useEffect } from "react";
import { createPortal } from "react-dom";
import { cls, clamp } from "../utils/format.js";
import { initialDelayUi, initialReverbUi } from "../utils/initialState.js";
import { EffectKnob } from "./Knobs.jsx";
import { RackTitleItem } from "./RackHeader.jsx";

const NOTE_MODES = ["NOTE", "DOT", "TRIP"];
const BASE_DIVISIONS = ["1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"];
const BASE_BEATS = [4, 2, 1, 0.5, 0.25, 0.125, 0.0625];
const BASE_ICONS = ["○", "♩", "♩", "♪", "♬", "♬", "♬"];
const PREDELAY_DIVISIONS = ["NONE", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1"];
const PREDELAY_ICONS = ["○", "♬", "♬", "♬", "♪", "♩", "♩", "○"];
const MAX_DIV_INDEX = BASE_DIVISIONS.length - 1;
const MAX_PREDELAY_DIV_INDEX = PREDELAY_DIVISIONS.length - 1;
const MODE_SUFFIX = { NOTE: "", DOT: ".", TRIP: "T" };
const MODE_MULT = { NOTE: 1, DOT: 1.5, TRIP: 2 / 3 };
const REVERB_MODES = [
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
const DELAY_PATTERNS = ["NORMAL", "WIDE", "PING-PONG"];
const DELAY_STYLES = [
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
const BUS_OPTIONS = ["Bus 1", "Bus 2", "Bus 3", "Bus 4", "Bus 5", "Bus 6", "Bus 7", "Bus 8"];

// Convierte un índice base + modo NOTE/DOT/TRIP a ms a 120 BPM
function msForBase(baseIndex, mode = "NOTE", bpm = 120) {
  const beat = 60000 / bpm;
  const beats = BASE_BEATS[baseIndex] ?? 1;
  return Math.round(beat * beats * (MODE_MULT[mode] ?? 1));
}

// Encuentra el índice base más cercano a un valor ms dado el modo activo
function closestBase(ms, mode = "NOTE", bpm = 120) {
  const beat = 60000 / bpm;
  const mult = MODE_MULT[mode] ?? 1;
  let best = 0,
    bestDist = Infinity;
  BASE_BEATS.forEach((beats, i) => {
    const dist = Math.abs(ms - beat * beats * mult);
    if (dist < bestDist) {
      bestDist = dist;
      best = i;
    }
  });
  return best;
}

function formatDelayLowCut(value) {
  const hz = 20 * Math.pow(900, clamp(value, 0, 100) / 100);
  return hz >= 1000 ? `${(hz / 1000).toFixed(1)} kHz` : `${Math.round(hz)} Hz`;
}

function formatDelayHighCut(value) {
  const hz = 160 * Math.pow(125, clamp(value, 0, 100) / 100);
  return hz >= 1000 ? `${(hz / 1000).toFixed(1)} kHz` : `${Math.round(hz)} Hz`;
}

function formatReverbLowCut(value) {
  const hz = 20 * Math.pow(50, clamp(value, 0, 100) / 100);
  return hz >= 1000 ? `${(hz / 1000).toFixed(1)} kHz` : `${Math.round(hz)} Hz`;
}

function formatReverbHighCut(value) {
  const hz = 1500 * Math.pow(13.333333, clamp(value, 0, 100) / 100);
  return hz >= 1000 ? `${(hz / 1000).toFixed(1)} kHz` : `${Math.round(hz)} Hz`;
}

function formatReverbDecay(value) {
  const norm = clamp(value, 0, 100) / 100;
  const seconds = 0.2 + ((Math.pow(90, norm) - 1) / 89) * 17.8;
  return `${seconds.toFixed(seconds >= 10 ? 1 : 2)} s`;
}

function DelayDivisionPicker({
  divisionIndex,
  timeMs,
  noteMode,
  bpmActive,
  hostBpm = 120,
  onChange
}) {
  const [ctxOpen, setCtxOpen] = useState(false);
  const [ctxPos, setCtxPos] = useState({ bottom: 0, left: 0 });
  const safeBpm = clamp(hostBpm || 120, 20, 300);

  const openCtxMenu = (e) => {
    e.preventDefault();
    document.body.classList.add("is-dropdown-open");
    setCtxPos({ bottom: window.innerHeight - e.clientY + 8, left: e.clientX });
    setCtxOpen(true);
  };

  const closeCtxMenu = () => {
    document.body.classList.remove("is-dropdown-open");
    setCtxOpen(false);
  };

  return (
    <div className="fx-delay-time-control" onContextMenu={openCtxMenu}>
      {bpmActive ? (
        <EffectKnob
          className="fx-manual-knob indicator-dot"
          label="TIME"
          value={divisionIndex}
          min={0}
          max={MAX_DIV_INDEX}
          progressTone="bright"
          stepped
          stepCount={BASE_DIVISIONS.length}
          formatter={(v) => {
            const i = Math.round(clamp(v, 0, MAX_DIV_INDEX));
            return `${BASE_ICONS[i]} ${BASE_DIVISIONS[i]}${MODE_SUFFIX[noteMode]}`;
          }}
          onChange={(v) => onChange({ divisionIndex: Math.round(clamp(v, 0, MAX_DIV_INDEX)) })}
        />
      ) : (
        <EffectKnob
          className="fx-manual-knob indicator-dot"
          label="TIME"
          value={timeMs}
          min={1}
          max={2000}
          progressTone="bright"
          formatter={(v) => `${Math.round(v)} ms`}
          onChange={(v) => onChange({ timeMs: v })}
        />
      )}

      {ctxOpen &&
        createPortal(
          <>
            <div className="fx-division-overlay" onClick={closeCtxMenu} />
            <ul
              className="fx-division-list"
              role="listbox"
              style={{
                position: "fixed",
                bottom: ctxPos.bottom,
                left: ctxPos.left,
                transform: "translateX(-50%)"
              }}
            >
              {BASE_DIVISIONS.map((div, i) => (
                <li
                  key={div}
                  role="option"
                  className={bpmActive && divisionIndex === i ? "active" : undefined}
                  onClick={() => {
                    if (bpmActive) onChange({ divisionIndex: i });
                    else onChange({ timeMs: msForBase(i, noteMode, safeBpm) });
                    closeCtxMenu();
                  }}
                >
                  <span className="fx-note-icon fx-note-icon-sm">{BASE_ICONS[i]}</span>
                  {div}
                  {MODE_SUFFIX[noteMode]}
                </li>
              ))}
            </ul>
          </>,
          document.body
        )}
    </div>
  );
}

function DelayTimingButtons({
  divisionIndex,
  timeMs,
  noteMode,
  bpmActive,
  hostBpm = 120,
  onChange
}) {
  const safeBpm = clamp(hostBpm || 120, 20, 300);

  const handleBpmToggle = () => {
    const next = !bpmActive;
    if (next) {
      onChange({ bpmActive: true, divisionIndex: closestBase(timeMs, noteMode, safeBpm) });
    } else {
      onChange({ bpmActive: false, timeMs: msForBase(divisionIndex, noteMode, safeBpm) });
    }
  };

  return (
    <>
      {NOTE_MODES.map((mode) => (
        <button
          key={mode}
          className={cls("rack-chip fx-manual-btn", noteMode === mode && "is-on")}
          onClick={() => onChange({ noteMode: mode })}
        >
          {mode}
        </button>
      ))}
      <button
        className={cls("rack-chip fx-manual-btn fx-tempo-back", bpmActive && "is-on")}
        onClick={handleBpmToggle}
      >
        BPM
      </button>
    </>
  );
}

function DelayStylePicker({ value, onChange }) {
  const [open, setOpen] = useState(false);
  const btnRef = useRef(null);
  const [dropPos, setDropPos] = useState({ bottom: 0, left: 0 });

  useEffect(() => {
    return () => {
      document.body.classList.remove("is-dropdown-open");
    };
  }, []);

  const openDropdown = () => {
    if (btnRef.current) {
      const rect = btnRef.current.getBoundingClientRect();
      setDropPos({
        bottom: window.innerHeight - rect.top + 6,
        left: rect.left + rect.width / 2
      });
    }
    document.body.classList.add("is-dropdown-open");
    setOpen(true);
  };

  const closeDropdown = () => {
    document.body.classList.remove("is-dropdown-open");
    setOpen(false);
  };

  return (
    <div className="fx-division-wrapper fx-reverb-mode-wrapper fx-delay-style-wrapper">
      <button
        ref={btnRef}
        className="fx-mode-pill fx-reverb-mode-trigger"
        onClick={() => (open ? closeDropdown() : openDropdown())}
        aria-haspopup="listbox"
        aria-expanded={open}
      >
        {value}
        <span className="fx-bus-chevron">▾</span>
      </button>
      {open &&
        createPortal(
          <>
            <div className="fx-division-overlay" onClick={closeDropdown} />
            <ul
              className="fx-division-list fx-reverb-mode-list"
              role="listbox"
              style={{
                position: "fixed",
                bottom: dropPos.bottom,
                left: dropPos.left,
                transform: "translateX(-50%)"
              }}
            >
              {DELAY_STYLES.map((style) => (
                <li
                  key={style}
                  role="option"
                  aria-selected={value === style}
                  className={value === style ? "active" : undefined}
                  onClick={() => {
                    onChange(style);
                    closeDropdown();
                  }}
                >
                  {style}
                </li>
              ))}
            </ul>
          </>,
          document.body
        )}
    </div>
  );
}

function ReverbModePicker({ modeIndex, onModeChange, eqExpanded }) {
  const [open, setOpen] = useState(false);
  const btnRef = useRef(null);
  const [dropPos, setDropPos] = useState({ bottom: 0, left: 0 });

  useEffect(() => {
    if (eqExpanded && open) {
      document.body.classList.remove("is-dropdown-open");
      setOpen(false);
    }
  }, [eqExpanded]); // eslint-disable-line react-hooks/exhaustive-deps

  const openDropdown = () => {
    if (btnRef.current) {
      const rect = btnRef.current.getBoundingClientRect();
      setDropPos({
        bottom: window.innerHeight - rect.top + 6,
        left: rect.left + rect.width / 2
      });
    }
    document.body.classList.add("is-dropdown-open");
    setOpen(true);
  };

  const closeDropdown = () => {
    document.body.classList.remove("is-dropdown-open");
    setOpen(false);
  };

  return (
    <div className="fx-division-wrapper fx-reverb-mode-wrapper">
      <button
        ref={btnRef}
        className="fx-mode-pill fx-reverb-mode-trigger"
        onClick={openDropdown}
        aria-haspopup="listbox"
        aria-expanded={open}
      >
        {REVERB_MODES[modeIndex]}
        <span className="fx-bus-chevron">▾</span>
      </button>
      {open &&
        createPortal(
          <>
            <div className="fx-division-overlay" onClick={closeDropdown} />
            <ul
              className="fx-division-list fx-reverb-mode-list"
              role="listbox"
              style={{
                position: "fixed",
                bottom: dropPos.bottom,
                left: dropPos.left,
                transform: "translateX(-50%)"
              }}
            >
              {REVERB_MODES.map((mode, i) => (
                <li
                  key={mode}
                  role="option"
                  aria-selected={modeIndex === i}
                  className={modeIndex === i ? "active" : undefined}
                  onClick={() => {
                    onModeChange(i);
                    closeDropdown();
                  }}
                >
                  {mode}
                </li>
              ))}
            </ul>
          </>,
          document.body
        )}
    </div>
  );
}

// Helper: devuelve un setter que actualiza UNA sola propiedad de un objeto.
// Ej: setField(setReverb, reverb, "decay") devuelve un (val) => setReverb({...reverb, decay: val})
const setField = (setter, current, key) => (value) => setter({ ...current, [key]: value });
const getEffectState = (value) => (value === true ? "on" : value || "off");
const isEffectActive = (value) => getEffectState(value) !== "off";

function BusSelect({ value, onChange }) {
  const [open, setOpen] = useState(false);
  const btnRef = useRef(null);
  const [dropPos, setDropPos] = useState({ top: 0, left: 0 });

  useEffect(() => {
    return () => {
      document.body.classList.remove("is-dropdown-open");
    };
  }, []);

  const openBus = () => {
    if (btnRef.current) {
      const rect = btnRef.current.getBoundingClientRect();
      setDropPos({
        top: rect.bottom + 5,
        left: rect.left + rect.width / 2
      });
    }
    document.body.classList.add("is-dropdown-open");
    setOpen(true);
  };

  const closeBus = () => {
    document.body.classList.remove("is-dropdown-open");
    setOpen(false);
  };

  return (
    <div className="fx-bus-dropdown">
      <button
        ref={btnRef}
        className="rack-chip is-aux"
        onClick={() => (open ? closeBus() : openBus())}
        aria-haspopup="listbox"
        aria-expanded={open}
      >
        {value} <span className="fx-bus-chevron">▾</span>
      </button>
      {open &&
        createPortal(
          <>
            <div className="fx-division-overlay" onClick={closeBus} />
            <ul
              className="fx-bus-list"
              role="listbox"
              style={{
                position: "fixed",
                top: dropPos.top,
                left: dropPos.left,
                transform: "translateX(-50%)"
              }}
            >
              {BUS_OPTIONS.map((b) => (
                <li
                  key={b}
                  role="option"
                  aria-selected={b === value}
                  className={b === value ? "active" : undefined}
                  onClick={() => {
                    onChange(b);
                    closeBus();
                  }}
                >
                  {b}
                </li>
              ))}
            </ul>
          </>,
          document.body
        )}
    </div>
  );
}

function ReverbPanel({ reverb, setReverb }) {
  const noteMode = reverb.noteMode ?? initialReverbUi.noteMode;
  const bpmActive = reverb.bpmActive ?? initialReverbUi.bpmActive;
  const decaySync = reverb.decaySync ?? initialReverbUi.decaySync;
  const predelaySync = reverb.predelaySync ?? initialReverbUi.predelaySync;
  const decayDiv = reverb.decayDivisionIndex ?? initialReverbUi.decayDivisionIndex;
  const predelayDiv = reverb.predelayDivisionIndex ?? initialReverbUi.predelayDivisionIndex;

  const decayInBpm = bpmActive && decaySync;
  const predelayInBpm = bpmActive && predelaySync;

  const divFormatter = (v) => {
    const i = Math.round(clamp(v, 0, MAX_DIV_INDEX));
    return `${BASE_ICONS[i]} ${BASE_DIVISIONS[i]}${MODE_SUFFIX[noteMode]}`;
  };
  const predelayDivFormatter = (v) => {
    const i = Math.round(clamp(v, 0, MAX_PREDELAY_DIV_INDEX));
    if (i === 0) return "NONE";
    return `${PREDELAY_ICONS[i]} ${PREDELAY_DIVISIONS[i]}${MODE_SUFFIX[noteMode]}`;
  };
  const divOnChange = (key) => (v) => setReverb({ ...reverb, [key]: Math.round(clamp(v, 0, MAX_DIV_INDEX)) });
  const predelayDivOnChange = (v) =>
    setReverb({ ...reverb, predelayDivisionIndex: Math.round(clamp(v, 0, MAX_PREDELAY_DIV_INDEX)) });

  return (
    <div className="fx-panel fx-reverb-panel">
      <div className="fx-control-row fx-reverb-main-row">
        <EffectKnob
          className="fx-reverb-knob indicator-dot"
          label="MIX"
          value={reverb.mix}
          min={0}
          max={100}
          unit="%"
          progressTone="bright"
          onChange={setField(setReverb, reverb, "mix")}
        />
        {decayInBpm ? (
          <EffectKnob
            className="fx-big-knob fx-reverb-knob indicator-dot"
            label="DECAY"
            value={decayDiv}
            min={0}
            max={MAX_DIV_INDEX}
            progressTone="bright"
            stepped
            stepCount={BASE_DIVISIONS.length}
            formatter={divFormatter}
            onChange={divOnChange("decayDivisionIndex")}
          />
        ) : (
          <EffectKnob
            className="fx-big-knob fx-reverb-knob indicator-dot"
            label="DECAY"
            value={reverb.decay}
            min={0}
            max={100}
            progressTone="bright"
            formatter={formatReverbDecay}
            onChange={setField(setReverb, reverb, "decay")}
          />
        )}
        {predelayInBpm ? (
          <EffectKnob
            className="fx-reverb-knob indicator-dot"
            label="PREDELAY"
            value={predelayDiv}
            min={0}
            max={MAX_PREDELAY_DIV_INDEX}
            progressTone="bright"
            stepped
            stepCount={PREDELAY_DIVISIONS.length}
            formatter={predelayDivFormatter}
            onChange={predelayDivOnChange}
          />
        ) : (
          <EffectKnob
            className="fx-reverb-knob indicator-dot"
            label="PREDELAY"
            value={reverb.predelay}
            min={0}
            max={100}
            progressTone="bright"
            formatter={(v) => `${(v * 0.5).toFixed(2)} ms`}
            onChange={setField(setReverb, reverb, "predelay")}
          />
        )}
      </div>

      <div className="fx-manual-mode">
        <RoomSizeFader
          value={reverb.size}
          onChange={setField(setReverb, reverb, "size")}
        />
        <div className="fx-manual-controls">
          {NOTE_MODES.map((mode) => (
            <button
              key={mode}
              className={cls("rack-chip fx-manual-btn", noteMode === mode && "is-on")}
              onClick={() => setReverb({ ...reverb, noteMode: mode })}
            >
              {mode}
            </button>
          ))}
          <button
            className={cls("rack-chip fx-manual-btn fx-tempo-back", bpmActive && "is-on")}
            onClick={() => setReverb({ ...reverb, bpmActive: !bpmActive })}
          >
            BPM
          </button>
          <button
            className={cls("rack-chip fx-manual-btn", decaySync && "is-on")}
            onClick={() => setReverb({ ...reverb, decaySync: !decaySync })}
          >
            DECAY
          </button>
          <button
            className={cls("rack-chip fx-manual-btn", predelaySync && "is-on")}
            onClick={() => setReverb({ ...reverb, predelaySync: !predelaySync })}
          >
            PRE
          </button>
        </div>
      </div>

      <div className="fx-control-row">
        <EffectKnob
          className="fx-reverb-knob indicator-dot"
          label="LOW CUT"
          value={reverb.lowCut}
          min={0}
          max={100}
          progressTone="bright"
          formatter={formatReverbLowCut}
          onChange={setField(setReverb, reverb, "lowCut")}
        />
        <EffectKnob
          className="fx-reverb-knob indicator-dot"
          label="HIGH CUT"
          value={reverb.highCut}
          min={0}
          max={100}
          progressTone="bright"
          formatter={formatReverbHighCut}
          onChange={setField(setReverb, reverb, "highCut")}
        />
      </div>
    </div>
  );
}

function RoomSizeFader({ value, onChange }) {
  const railRef = useRef(null);
  const pointerRef = useRef({ active: false, id: -1 });
  const size = clamp(value, 0, 100);

  const updateFromClientX = (clientX) => {
    if (!railRef.current) return;
    const rect = railRef.current.getBoundingClientRect();
    const ratio = clamp((clientX - rect.left) / rect.width, 0, 1);
    onChange(Number((ratio * 100).toFixed(1)));
  };

  const onPointerDown = (event) => {
    pointerRef.current = { active: true, id: event.pointerId };
    railRef.current?.setPointerCapture(event.pointerId);
    document.body.classList.add("is-knob-dragging", "is-fader-dragging");
    updateFromClientX(event.clientX);
  };

  const onPointerMove = (event) => {
    if (!pointerRef.current.active || pointerRef.current.id !== event.pointerId) return;
    updateFromClientX(event.clientX);
  };

  const endDrag = (event) => {
    if (pointerRef.current.id !== event.pointerId) return;
    pointerRef.current = { active: false, id: -1 };
    document.body.classList.remove("is-knob-dragging", "is-fader-dragging");
  };

  return (
    <div className="fx-room-size-fader">
      <div className="fx-room-size-head">
        <span>SIZE</span>
        <strong>{Math.round(size)}%</strong>
      </div>
      <div
        ref={railRef}
        className="fx-room-size-rail"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={endDrag}
        onPointerCancel={endDrag}
      >
        <div className="fx-room-size-fill" style={{ width: `${size}%` }} />
        <div className="fx-room-size-thumb" style={{ left: `${size}%` }} />
      </div>
    </div>
  );
}

export function EffectsModule({
  reverb,
  setReverb,
  delay,
  setDelay,
  hostBpm,
  effectToggles,
  onEffectToggle,
  eqExpanded
}) {
  const [delayBus, setDelayBus] = useState(initialDelayUi.bus);
  const [reverbBus, setReverbBus] = useState(initialReverbUi.bus);
  const delayState = getEffectState(effectToggles?.delay);
  const reverbState = getEffectState(effectToggles?.reverb);
  const delayActive = isEffectActive(delayState);
  const reverbActive = isEffectActive(reverbState);
  const delayIsAux = delayState === "aux";
  const reverbIsAux = reverbState === "aux";
  const showPostReverb = delayState === "on";
  const reverbModeIndex = Math.round(
    clamp(reverb.modeIndex ?? initialReverbUi.modeIndex, 0, REVERB_MODES.length - 1)
  );
  const setEffectPower = (key, state) => {
    onEffectToggle?.(key, isEffectActive(state) ? "off" : "on");
  };
  const toggleEffectAux = (key, state) => {
    onEffectToggle?.(key, state === "aux" ? "on" : "aux");
  };

  return (
    <div className="fx-stack">
      <div className="rack-title-row fx-title-row">
        <div
          className={cls(
            "fx-header-cell",
            "fx-delay-header",
            !showPostReverb && !delayIsAux && "no-post",
            delayState === "off" && "is-bypassed",
            delayIsAux && "has-bus"
          )}
        >
          <div className="fx-header-main">
            <EffectTitle
              title={delayIsAux ? "AUX DELAY" : "DELAY"}
              active={delayActive}
              onToggle={() => setEffectPower("delay", delayState)}
            />
            {delayActive && (
              <button
                className={cls(
                  "rack-chip",
                  "rack-chip-mode",
                  delayIsAux && "is-aux"
                )}
                onClick={() => toggleEffectAux("delay", delayState)}
                aria-pressed={delayIsAux}
                aria-label={delayIsAux ? "Set Delay inline" : "Set Delay aux"}
              >
                Aux
              </button>
            )}
            {delayIsAux ? (
              <BusSelect value={delayBus} onChange={setDelayBus} />
            ) : (
              showPostReverb && (
                <button
                  className={cls(
                    "rack-chip",
                    "rack-chip-mode",
                    "fx-post-reverb",
                    delay.postReverb && "is-on"
                  )}
                  onClick={() => setDelay({ ...delay, postReverb: !delay.postReverb })}
                  aria-pressed={delay.postReverb}
                >
                  Post Reverb
                </button>
              )
            )}
          </div>
          <div className="fx-type-slot">
            <DelayStylePicker
              value={delay.style ?? "Clean"}
              onChange={(style) => setDelay({ ...delay, style })}
            />
          </div>
        </div>
        <div
          className={cls(
            "fx-header-cell",
            "fx-reverb-header",
            reverbState === "off" && "is-bypassed",
            reverbIsAux && "has-bus"
          )}
        >
          <div className="fx-header-main">
            <EffectTitle
              title={reverbIsAux ? "AUX REVERB" : "REVERB"}
              active={reverbActive}
              onToggle={() => setEffectPower("reverb", reverbState)}
            />
            {reverbActive && (
              <button
                className={cls(
                  "rack-chip",
                  "rack-chip-mode",
                  reverbIsAux && "is-aux"
                )}
                onClick={() => toggleEffectAux("reverb", reverbState)}
                aria-pressed={reverbIsAux}
                aria-label={reverbIsAux ? "Set Reverb inline" : "Set Reverb aux"}
              >
                Aux
              </button>
            )}
            {reverbIsAux && <BusSelect value={reverbBus} onChange={setReverbBus} />}
          </div>
          <div className="fx-type-slot">
            <ReverbModePicker
              modeIndex={reverbModeIndex}
              onModeChange={(nextMode) => setReverb({ ...reverb, modeIndex: nextMode })}
              eqExpanded={eqExpanded}
            />
          </div>
        </div>
      </div>
      <div className="fx-grid">
        <div
          className={cls(
            "fx-column",
            "fx-delay",
            delayState === "off" && "is-bypassed",
            delayIsAux && "is-aux"
          )}
        >
          <div className="fx-panel fx-delay-panel">
            <div className="fx-control-row fx-delay-main-row">
              <EffectKnob
                className="fx-delay-knob indicator-dot"
                label="MIX"
                value={delay.mix}
                min={0}
                max={100}
                unit="%"
                progressTone="bright"
                onChange={setField(setDelay, delay, "mix")}
              />
              <DelayDivisionPicker
                divisionIndex={delay.divisionIndex}
                timeMs={delay.timeMs}
                noteMode={delay.noteMode}
                bpmActive={delay.bpmActive}
                hostBpm={hostBpm}
                onChange={(changes) => setDelay({ ...delay, ...changes })}
              />
              <EffectKnob
                className="fx-delay-knob indicator-dot"
                label="FEEDBACK"
                value={delay.feedback}
                min={0}
                max={100}
                unit="%"
                progressTone="bright"
                onChange={setField(setDelay, delay, "feedback")}
              />
            </div>
            <div className="fx-delay-button-grid fx-manual-controls">
              <DelayTimingButtons
                divisionIndex={delay.divisionIndex}
                timeMs={delay.timeMs}
                noteMode={delay.noteMode}
                bpmActive={delay.bpmActive}
                hostBpm={hostBpm}
                onChange={(changes) => setDelay({ ...delay, ...changes })}
              />
              {DELAY_PATTERNS.map((mode) => (
                <button
                  key={mode}
                  className={cls("rack-chip fx-manual-btn fx-delay-pattern-btn", delay.mode === mode && "is-on")}
                  onClick={() => setDelay({ ...delay, mode })}
                >
                  {mode}
                </button>
              ))}
            </div>
            <div className="fx-control-row">
              <EffectKnob
                className="fx-delay-knob indicator-dot"
                label="LOW CUT"
                value={delay.lowCut}
                min={0}
                max={100}
                formatter={formatDelayLowCut}
                progressTone="bright"
                onChange={setField(setDelay, delay, "lowCut")}
              />
              <EffectKnob
                className="fx-delay-knob indicator-dot"
                label="HIGH CUT"
                value={delay.highCut}
                min={0}
                max={100}
                formatter={formatDelayHighCut}
                progressTone="bright"
                onChange={setField(setDelay, delay, "highCut")}
              />
            </div>
          </div>
        </div>

        <div
          className={cls(
            "fx-column",
            "fx-reverb",
            reverbState === "off" && "is-bypassed",
            reverbIsAux && "is-aux"
          )}
        >
          <ReverbPanel reverb={reverb} setReverb={setReverb} />
        </div>
      </div>
    </div>
  );
}

function EffectTitle({ title, active, onToggle }) {
  return <RackTitleItem title={title} active={active} onToggle={onToggle} />;
}
