// Medidores de nivel IN / OUT.
// MeterSection conserva la columna lateral legacy; EqInlineMeter monta gain IN/OUT dentro del EQ.
// LevelBar es una sola barra animada.

import { useRef } from "react";
import { clamp, formatGain } from "../utils/format.js";
import { getEqGridStep } from "../utils/eq.js";
import { Dial } from "./Knobs.jsx";

const meterMarks = [
  "+12",
  "+6",
  "0",
  "-6",
  "-12",
  "-18",
  "-24",
  "-30",
  "-36",
  "-42",
  "-48",
  "-54",
  "-60"
];
const gainMin = -24;
const gainMax = 12;
const gainStep = 0.5;
const meterVisualMin = -60;
const meterVisualMax = 12;

function gainToEqPercent(value, displayRange) {
  const range = Math.max(1, displayRange);
  const safeValue = clamp(value, -range, range);
  return ((safeValue + range) / (range * 2)) * 100;
}

function eqPercentToGain(percent, displayRange) {
  const safePercent = clamp(percent, 0, 1);
  const range = Math.max(1, displayRange);
  return -range + safePercent * range * 2;
}

function formatEqGainMark(value) {
  const rounded = Math.round(value * 10) / 10;
  const text = Number.isInteger(rounded) ? `${rounded}` : rounded.toFixed(1);
  return rounded > 0 ? `+${text}` : text;
}

function getEqGainMarks(displayRange) {
  const step = getEqGridStep(displayRange);
  const marks = [];
  for (let value = -displayRange; value < displayRange - step / 2; value += step) {
    marks.push(Math.abs(value) < 0.0001 ? 0 : Number(value.toFixed(1)));
  }
  return marks;
}

function dbToMeterPosition(dbValue) {
  return ((dbValue - meterVisualMin) / (meterVisualMax - meterVisualMin)) * 100;
}

function dbToMeterTop(dbValue) {
  return 100 - dbToMeterPosition(dbValue);
}

function getVisibleLevels(levels, channelCount) {
  const isStereo = channelCount > 1;
  return {
    isStereo,
    visibleLevels: isStereo ? [levels[0] ?? 0, levels[1] ?? 0] : [levels[0] ?? 0]
  };
}

function MeterBars({ levels, channelCount, color }) {
  const { isStereo, visibleLevels } = getVisibleLevels(levels, channelCount);

  return (
    <div className={`dual-bars ${isStereo ? "meter-bars-stereo" : "meter-bars-mono"}`}>
      {visibleLevels.map((levelValue, index) => (
        <div className="meter-channel" key={isStereo ? index : "mono"}>
          <LevelBar level={levelValue} color={color} />
        </div>
      ))}
    </div>
  );
}

function MeterScale() {
  return (
    <div className="meter-scale">
      {meterMarks.map((mark) => (
        <span key={mark} style={{ top: `${dbToMeterTop(Number(mark))}%` }}>
          {mark}
        </span>
      ))}
    </div>
  );
}

function MeterGainKnob({ gain, onGainChange, color }) {
  const visibleGain = clamp(gain, gainMin, gainMax);

  const setGain = (nextValue) => {
    onGainChange(Number(clamp(nextValue, gainMin, gainMax).toFixed(1)));
  };

  const changeGainBy = (delta) => {
    setGain(visibleGain + delta);
  };

  const onGainWheel = (event) => {
    event.preventDefault();
    changeGainBy(event.deltaY > 0 ? -0.5 : 0.5);
  };

  return (
    <div
      className="meter-gain-knob"
      onDoubleClick={() => setGain(0)}
      onWheel={onGainWheel}
      title={`Gain ${formatGain(visibleGain)}`}
    >
      <Dial
        value={visibleGain}
        min={gainMin}
        max={gainMax}
        step={0.5}
        color={color}
        compact
        onChange={setGain}
      />
      <strong>{formatGain(visibleGain)}</strong>
    </div>
  );
}

function MeterCore({ title, value, gain, onGainChange, levels, channelCount, color }) {
  return (
    <>
      <div className="meter-header">
        <span>{title}</span>
        <div className="db-chip">{value}</div>
      </div>

      <div className="meter-body">
        <div className="meter-meter-wrap">
          <MeterBars levels={levels} channelCount={channelCount} color={color} />
          <MeterScale />
        </div>
      </div>

      <MeterGainKnob gain={gain} onGainChange={onGainChange} color={color} />
    </>
  );
}

export function MeterSection({
  title,
  value,
  gain,
  onGainChange,
  levels = [0, 0],
  channelCount = 2,
  color = "orange"
}) {
  return (
    <aside className={`meter-section meter-${title.toLowerCase()}`}>
      <MeterCore
        title={title}
        value={value}
        gain={gain}
        onGainChange={onGainChange}
        levels={levels}
        channelCount={channelCount}
        color={color}
      />
    </aside>
  );
}

export function EqInlineMeter({
  title,
  gain,
  onGainChange,
  levels = [0, 0],
  channelCount = 2,
  color = "orange",
  displayRange = 12
}) {
  const railRef = useRef(null);
  const pointerRef = useRef({ active: false, id: -1 });
  const visibleGain = clamp(gain, gainMin, gainMax);
  const safeDisplayRange = Math.max(1, displayRange);
  const gainPct = gainToEqPercent(visibleGain, safeDisplayRange);
  const showScaleMarks = title !== "IN";
  const scaleMarks = getEqGainMarks(safeDisplayRange);
  const { visibleLevels } = getVisibleLevels(levels, channelCount);
  const signalLevel = clamp(Math.max(0, ...visibleLevels), 0, 1);

  const setGain = (nextValue) => {
    const snapped = Math.round(clamp(nextValue, gainMin, gainMax) / gainStep) * gainStep;
    onGainChange(Number(snapped.toFixed(1)));
  };

  const updateFromClientY = (clientY) => {
    if (!railRef.current) return;
    const rect = railRef.current.getBoundingClientRect();
    const ratio = clamp((rect.bottom - clientY) / rect.height, 0, 1);
    setGain(eqPercentToGain(ratio, safeDisplayRange));
  };

  const onPointerDown = (event) => {
    event.preventDefault();
    event.stopPropagation();
    pointerRef.current = { active: true, id: event.pointerId };
    railRef.current?.setPointerCapture(event.pointerId);
    document.body.classList.add("is-knob-dragging", "is-fader-dragging");
    updateFromClientY(event.clientY);
  };

  const onPointerMove = (event) => {
    if (!pointerRef.current.active || pointerRef.current.id !== event.pointerId) return;
    event.stopPropagation();
    updateFromClientY(event.clientY);
  };

  const endDrag = (event) => {
    if (pointerRef.current.id !== event.pointerId) return;
    event.stopPropagation();
    pointerRef.current = { active: false, id: -1 };
    document.body.classList.remove("is-knob-dragging", "is-fader-dragging");
  };

  const onWheel = (event) => {
    event.preventDefault();
    event.stopPropagation();
    setGain(visibleGain + (event.deltaY > 0 ? -gainStep : gainStep));
  };

  const onDoubleClick = (event) => {
    event.preventDefault();
    event.stopPropagation();
    setGain(0);
  };

  return (
    <div
      className={`eq-io-gain eq-io-gain-${title.toLowerCase()} eq-io-gain-${color}`}
      onMouseDown={(event) => event.stopPropagation()}
      onPointerDown={(event) => event.stopPropagation()}
      onContextMenu={(event) => event.stopPropagation()}
      onWheel={onWheel}
      onDoubleClick={onDoubleClick}
    >
      <span className="eq-io-gain-name">{title}</span>
      <div
        ref={railRef}
        className="eq-io-gain-rail"
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={endDrag}
        onPointerCancel={endDrag}
      >
        <span
          className="eq-io-gain-signal"
          style={{ height: `${signalLevel * 100}%` }}
          aria-hidden="true"
        />
        {showScaleMarks && <span className="eq-io-gain-zero" style={{ bottom: "50%" }} />}
        {showScaleMarks &&
          scaleMarks.map((mark) => (
            <span className="eq-io-gain-mark" key={mark} style={{ bottom: `${gainToEqPercent(mark, safeDisplayRange)}%` }}>
              {formatEqGainMark(mark)}
            </span>
          ))}
        <span
          className="eq-io-gain-handle"
          style={{ bottom: `${gainPct}%` }}
          aria-label={`${title} ${formatGain(visibleGain)}`}
        />
      </div>
      <div className="eq-io-gain-readout">
        <span>{title}</span>
      </div>
    </div>
  );
}

export function LevelBar({ level, color = "orange" }) {
  return (
    <div className="level-bar">
      <div
        className={`level-fill level-fill-${color}`}
        style={{
          height: `${level * 100}%`
        }}
      />
    </div>
  );
}
