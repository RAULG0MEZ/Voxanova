import { useEffect, useMemo, useState } from "react";
import packageMeta from "../package.json";
import {
  PLUGIN_HEIGHT,
  PLUGIN_WIDTH,
  booleanParameters,
  defaultValues,
  delayDivisions,
  delayModes,
  delayStyles,
  noteModes,
  reverbModes,
  reverbPredelayDivisions,
  saturationModes
} from "./pluginContract.js";
import { hasNativeBackend, sendNativeEditorSize, sendNativeParameter } from "./nativeBridge.js";

const booleanParameterSet = new Set(booleanParameters);
const sizeOptions = [
  { id: "compact", label: "Compact", scale: 0.82 },
  { id: "default", label: "Default", scale: 1 },
  { id: "large", label: "Large", scale: 1.12 }
];

const emptyMeters = {
  input: [0, 0],
  output: [0, 0],
  inputChannels: 2,
  outputChannels: 2,
  peakLevel: 0,
  glueLevel: 0,
  faceLevel: 0,
  gateLevel: 0,
  peakReduction: 0,
  glueReduction: 0,
  faceReduction: 0,
  gateReduction: 0,
  peakReductionDb: 0,
  glueReductionDb: 0,
  faceReductionDb: 0,
  gateReductionDb: 0,
  hostBpm: 120
};

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function toNumber(value) {
  return Number.parseFloat(value);
}

function dbLabel(value) {
  if (value <= -79.95) return "-inf dB";
  return `${value > 0 ? "+" : ""}${Number(value).toFixed(1)} dB`;
}

function shortDbLabel(value) {
  if (value <= -79.95) return "-inf";
  return `${Math.round(value)} dB`;
}

function percentLabel(value) {
  return `${Math.round(value)}%`;
}

function hzLabel(value) {
  return `${Math.round(value)} Hz`;
}

function msLabel(value) {
  return `${Math.round(value)} ms`;
}

function meterDbLabel(level) {
  if (level <= 0.001) return "-inf";
  return `${(level * 72 - 60).toFixed(1)} dB`;
}

function reverbDecayLabel(value) {
  const norm = clamp(value / 100, 0, 1);
  const seconds = 0.2 + ((90 ** norm - 1) / 89) * 17.8;
  return `${seconds >= 10 ? seconds.toFixed(1) : seconds.toFixed(2)} s`;
}

function controlPercent(value, min, max) {
  return clamp(((value - min) / (max - min)) * 100, 0, 100);
}

function optionList(labels) {
  return labels.map((label, value) => ({ label, value }));
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

function Toggle({ active, label, onChange }) {
  return (
    <button
      className={`toggle ${active ? "is-on" : ""}`}
      type="button"
      aria-pressed={active}
      onClick={() => onChange(!active)}
    >
      <span />
      {label}
    </button>
  );
}

function Meter({ label, levels, channels, tone = "blue" }) {
  const visible = channels > 1 ? [levels[0] ?? 0, levels[1] ?? 0] : [levels[0] ?? 0];
  const peak = Math.max(0, ...visible);

  return (
    <div className={`meter meter-${tone}`}>
      <div className="meter-head">
        <span>{label}</span>
        <strong>{meterDbLabel(peak)}</strong>
      </div>
      <div className={`meter-tube ${channels > 1 ? "is-stereo" : "is-mono"}`}>
        {visible.map((level, index) => (
          <span key={index} style={{ height: `${clamp(level, 0, 1) * 100}%` }} />
        ))}
      </div>
    </div>
  );
}

function SliderControl({
  label,
  value,
  min,
  max,
  step = 1,
  disabled = false,
  formatter = percentLabel,
  onChange
}) {
  const percent = controlPercent(value, min, max);

  return (
    <label className={`control ${disabled ? "is-disabled" : ""}`}>
      <span className="control-label">{label}</span>
      <span className="control-value">{formatter(value)}</span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        disabled={disabled}
        style={{ "--fill": `${percent}%` }}
        onChange={(event) => onChange(toNumber(event.target.value))}
      />
    </label>
  );
}

function KnobControl({
  label,
  value,
  min,
  max,
  step = 1,
  disabled = false,
  formatter = percentLabel,
  onChange
}) {
  const percent = controlPercent(value, min, max);
  const angle = -135 + (percent / 100) * 270;

  return (
    <label className={`knob-control ${disabled ? "is-disabled" : ""}`}>
      <span className="knob-label">{label}</span>
      <span className="knob-shell" style={{ "--arc": `${percent}%`, "--angle": `${angle}deg` }}>
        <span className="knob-face">
          <span />
        </span>
      </span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        disabled={disabled}
        onChange={(event) => onChange(toNumber(event.target.value))}
      />
      <strong>{formatter(value)}</strong>
    </label>
  );
}

function SelectControl({ label, value, options, disabled = false, onChange }) {
  return (
    <label className={`select-control ${disabled ? "is-disabled" : ""}`}>
      <span>{label}</span>
      <select
        disabled={disabled}
        value={value}
        onChange={(event) => onChange(toNumber(event.target.value))}
      >
        {options.map((option) => (
          <option key={option.value} value={option.value}>
            {option.label}
          </option>
        ))}
      </select>
    </label>
  );
}

function Reduction({ label, value, db }) {
  return (
    <div className="reduction">
      <span>{label}</span>
      <div>
        <span style={{ width: `${clamp(value, 0, 100)}%` }} />
      </div>
      <strong>{db > 0.05 ? `-${db.toFixed(db >= 10 ? 0 : 1)} dB` : "0 dB"}</strong>
    </div>
  );
}

function Panel({ title, kicker, active = true, action, children }) {
  return (
    <section className={`panel ${active ? "" : "is-muted"}`}>
      <header className="panel-head">
        <div>
          <span>{kicker}</span>
          <h2>{title}</h2>
        </div>
        {action}
      </header>
      {children}
    </section>
  );
}

function DynamicsStrip({ title, enabled, amountLabel, level, reduction, reductionDb, children }) {
  return (
    <div className={`dynamics-strip ${enabled ? "" : "is-muted"}`}>
      <div className="strip-title">
        <h3>{title}</h3>
        <span>{amountLabel}</span>
      </div>
      {children}
      <Reduction label="Gain reduction" value={reduction} db={reductionDb} />
      <div className="activity-line">
        <span style={{ width: `${clamp(level, 0, 1) * 100}%` }} />
      </div>
    </div>
  );
}

export default function App() {
  const [values, setValues] = useState(defaultValues);
  const [meters, setMeters] = useState(emptyMeters);
  const [scale, setScale] = useState(1);
  const [nativeOnline, setNativeOnline] = useState(hasNativeBackend);

  const selectOptions = useMemo(
    () => ({
      saturation: optionList(saturationModes),
      note: optionList(noteModes),
      delayDivision: optionList(delayDivisions),
      reverbPredelayDivision: optionList(reverbPredelayDivisions),
      delayMode: optionList(delayModes),
      delayStyle: optionList(delayStyles),
      reverbMode: optionList(reverbModes)
    }),
    []
  );

  useEffect(() => {
    const updateNativeStatus = () => setNativeOnline(hasNativeBackend());
    updateNativeStatus();
    const timer = window.setInterval(updateNativeStatus, 1000);
    return () => window.clearInterval(timer);
  }, []);

  useEffect(() => {
    const onMeterUpdate = (event) => {
      const payload = event.detail || {};

      setMeters({
        input: [payload.inputL ?? 0, payload.inputR ?? payload.inputL ?? 0],
        output: [payload.outputL ?? 0, payload.outputR ?? payload.outputL ?? 0],
        inputChannels: payload.inputChannels ?? 2,
        outputChannels: payload.outputChannels ?? 2,
        peakLevel: payload.peakLevel ?? 0,
        glueLevel: payload.glueLevel ?? 0,
        faceLevel: payload.faceLevel ?? 0,
        gateLevel: payload.gateLevel ?? 0,
        peakReduction: payload.peakGr ?? 0,
        glueReduction: payload.glueGr ?? 0,
        faceReduction: payload.faceGr ?? 0,
        gateReduction: payload.gateGr ?? 0,
        peakReductionDb: payload.peakGrDb ?? 0,
        glueReductionDb: payload.glueGrDb ?? 0,
        faceReductionDb: payload.faceGrDb ?? 0,
        gateReductionDb: payload.gateGrDb ?? 0,
        hostBpm: payload.hostBpm ?? 120
      });

      setValues((current) => updateValuesFromPayload(current, payload));
    };

    window.addEventListener("voxanovaMeterUpdate", onMeterUpdate);
    return () => window.removeEventListener("voxanovaMeterUpdate", onMeterUpdate);
  }, []);

  const setParam = (id, value) => {
    const nextValue = booleanParameterSet.has(id) ? Boolean(value) : value;
    setValues((current) => ({ ...current, [id]: nextValue }));
    sendNativeParameter(id, booleanParameterSet.has(id) ? (nextValue ? 1 : 0) : nextValue);
  };

  const setWindowScale = (nextScale) => {
    setScale(nextScale);
    sendNativeEditorSize(
      nextScale,
      Math.round(PLUGIN_WIDTH * nextScale),
      Math.round(PLUGIN_HEIGHT * nextScale)
    );
  };

  const disabled = {
    peak: !values.peakEnabled,
    glue: !values.glueEnabled,
    face: !values.faceEnabled,
    gate: !values.gateEnabled,
    stereo: !values.stereoEnabled,
    delay: !values.delayEnabled,
    reverb: !values.reverbEnabled
  };

  return (
    <main className="app-shell">
      <div
        className="plugin-viewport"
        style={{
          width: `${Math.round(PLUGIN_WIDTH * scale)}px`,
          height: `${Math.round(PLUGIN_HEIGHT * scale)}px`
        }}
      >
        <div
          className="plugin-frame"
          style={{
            width: `${PLUGIN_WIDTH}px`,
            height: `${PLUGIN_HEIGHT}px`,
            transform: `scale(${scale})`
          }}
        >
          <header className="topbar">
            <div className="brand-lockup">
              <span>VOXANOVA</span>
              <strong>Vocal Chain</strong>
            </div>
            <div className="system-status">
              <span>{nativeOnline ? "Native linked" : "Browser preview"}</span>
              <span>{Math.round(meters.hostBpm)} BPM</span>
              <span>v{packageMeta.version}</span>
            </div>
            <div className="size-switcher" aria-label="Window size">
              {sizeOptions.map((option) => (
                <button
                  key={option.id}
                  type="button"
                  className={scale === option.scale ? "is-active" : ""}
                  onClick={() => setWindowScale(option.scale)}
                >
                  {option.label}
                </button>
              ))}
            </div>
          </header>

          <section className="meter-row">
            <Meter
              label="Input"
              levels={meters.input}
              channels={meters.inputChannels}
              tone="blue"
            />
            <div className="gain-grid">
              <KnobControl
                label="Input Gain"
                value={values.inputGain}
                min={-24}
                max={24}
                step={0.1}
                formatter={dbLabel}
                onChange={(value) => setParam("inputGain", value)}
              />
              <KnobControl
                label="Output Gain"
                value={values.outputGain}
                min={-24}
                max={24}
                step={0.1}
                formatter={dbLabel}
                onChange={(value) => setParam("outputGain", value)}
              />
            </div>
            <Meter
              label="Output"
              levels={meters.output}
              channels={meters.outputChannels}
              tone="orange"
            />
          </section>

          <section className="main-grid">
            <Panel
              title="Dynamics"
              kicker="Control"
              action={
                <span className="panel-meter">
                  {meterDbLabel(Math.max(meters.peakLevel, meters.glueLevel, meters.faceLevel))}
                </span>
              }
            >
              <div className="dynamics-grid">
                <DynamicsStrip
                  title="Peak Tamer"
                  enabled={values.peakEnabled}
                  amountLabel={shortDbLabel(values.peakThreshold)}
                  level={meters.peakLevel}
                  reduction={meters.peakReduction}
                  reductionDb={meters.peakReductionDb}
                >
                  <Toggle
                    label={values.peakEnabled ? "On" : "Off"}
                    active={values.peakEnabled}
                    onChange={(value) => setParam("peakEnabled", value)}
                  />
                  <SliderControl
                    label="Threshold"
                    value={values.peakThreshold}
                    min={-60}
                    max={0}
                    step={0.1}
                    formatter={dbLabel}
                    disabled={disabled.peak}
                    onChange={(value) => setParam("peakThreshold", value)}
                  />
                </DynamicsStrip>

                <DynamicsStrip
                  title="Glue"
                  enabled={values.glueEnabled}
                  amountLabel={
                    values.glueMultiband ? "Multiband" : shortDbLabel(values.glueThreshold)
                  }
                  level={meters.glueLevel}
                  reduction={meters.glueReduction}
                  reductionDb={meters.glueReductionDb}
                >
                  <div className="toggle-pair">
                    <Toggle
                      label={values.glueEnabled ? "On" : "Off"}
                      active={values.glueEnabled}
                      onChange={(value) => setParam("glueEnabled", value)}
                    />
                    <Toggle
                      label="Bands"
                      active={values.glueMultiband}
                      onChange={(value) => setParam("glueMultiband", value)}
                    />
                  </div>
                  {values.glueMultiband ? (
                    <div className="mini-band-grid">
                      {[
                        ["Low", "glueLowThreshold"],
                        ["Low Mid", "glueLowMidThreshold"],
                        ["High Mid", "glueHighMidThreshold"],
                        ["Air", "glueAirThreshold"]
                      ].map(([label, id]) => (
                        <SliderControl
                          key={id}
                          label={label}
                          value={values[id]}
                          min={-48}
                          max={0}
                          step={0.1}
                          formatter={shortDbLabel}
                          disabled={disabled.glue}
                          onChange={(value) => setParam(id, value)}
                        />
                      ))}
                    </div>
                  ) : (
                    <SliderControl
                      label="Threshold"
                      value={values.glueThreshold}
                      min={-60}
                      max={0}
                      step={0.1}
                      formatter={dbLabel}
                      disabled={disabled.glue}
                      onChange={(value) => setParam("glueThreshold", value)}
                    />
                  )}
                </DynamicsStrip>

                <DynamicsStrip
                  title="In Your Face"
                  enabled={values.faceEnabled}
                  amountLabel={percentLabel(values.faceThreshold)}
                  level={meters.faceLevel}
                  reduction={meters.faceReduction}
                  reductionDb={meters.faceReductionDb}
                >
                  <Toggle
                    label={values.faceEnabled ? "On" : "Off"}
                    active={values.faceEnabled}
                    onChange={(value) => setParam("faceEnabled", value)}
                  />
                  <SliderControl
                    label="Amount"
                    value={values.faceThreshold}
                    min={0}
                    max={100}
                    formatter={percentLabel}
                    disabled={disabled.face}
                    onChange={(value) => setParam("faceThreshold", value)}
                  />
                </DynamicsStrip>
              </div>
            </Panel>

            <Panel title="Utility" kicker="Input shape">
              <div className="utility-grid">
                <div className={`utility-card ${disabled.gate ? "is-muted" : ""}`}>
                  <div className="card-head">
                    <h3>Gate</h3>
                    <Toggle
                      label={values.gateEnabled ? "On" : "Off"}
                      active={values.gateEnabled}
                      onChange={(value) => setParam("gateEnabled", value)}
                    />
                  </div>
                  <SliderControl
                    label="Threshold"
                    value={values.gateThreshold}
                    min={-80}
                    max={0}
                    step={0.1}
                    formatter={dbLabel}
                    disabled={disabled.gate}
                    onChange={(value) => setParam("gateThreshold", value)}
                  />
                  <Reduction
                    label="Gate action"
                    value={meters.gateReduction}
                    db={meters.gateReductionDb}
                  />
                </div>

                <div className={`utility-card ${disabled.stereo ? "is-muted" : ""}`}>
                  <div className="card-head">
                    <h3>Stereo</h3>
                    <Toggle
                      label={values.stereoEnabled ? "On" : "Off"}
                      active={values.stereoEnabled}
                      onChange={(value) => setParam("stereoEnabled", value)}
                    />
                  </div>
                  <KnobControl
                    label="Width"
                    value={values.stereoWidth}
                    min={0}
                    max={100}
                    formatter={percentLabel}
                    disabled={disabled.stereo}
                    onChange={(value) => setParam("stereoWidth", value)}
                  />
                  <SliderControl
                    label="Low Bypass"
                    value={values.stereoLowBypass}
                    min={0}
                    max={500}
                    formatter={hzLabel}
                    disabled={disabled.stereo}
                    onChange={(value) => setParam("stereoLowBypass", value)}
                  />
                </div>
              </div>
            </Panel>

            <Panel title="Tone" kicker="Saturation">
              <div className="tone-grid">
                {[
                  ["Pre", "preSaturationMode", "preSaturationAmount"],
                  ["Post", "postSaturationMode", "postSaturationAmount"]
                ].map(([label, modeId, amountId]) => (
                  <div className="tone-card" key={label}>
                    <h3>{label}</h3>
                    <SelectControl
                      label="Mode"
                      value={values[modeId]}
                      options={selectOptions.saturation}
                      onChange={(value) => setParam(modeId, value)}
                    />
                    <KnobControl
                      label="Drive"
                      value={values[amountId]}
                      min={0}
                      max={100}
                      formatter={percentLabel}
                      disabled={values[modeId] === 0}
                      onChange={(value) => setParam(amountId, value)}
                    />
                  </div>
                ))}
              </div>
            </Panel>

            <Panel
              title="Delay"
              kicker="Time"
              active={values.delayEnabled}
              action={
                <Toggle
                  label={values.delayEnabled ? "On" : "Off"}
                  active={values.delayEnabled}
                  onChange={(value) => setParam("delayEnabled", value)}
                />
              }
            >
              <div className="fx-grid">
                <KnobControl
                  label="Mix"
                  value={values.delayMix}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayMix", value)}
                />
                <KnobControl
                  label="Feedback"
                  value={values.delayFeedback}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayFeedback", value)}
                />
                <SelectControl
                  label="Division"
                  value={values.delayDivision}
                  options={selectOptions.delayDivision}
                  disabled={disabled.delay || !values.delaySync}
                  onChange={(value) => setParam("delayDivision", value)}
                />
                <SelectControl
                  label="Feel"
                  value={values.delayNoteMode}
                  options={selectOptions.note}
                  disabled={disabled.delay || !values.delaySync}
                  onChange={(value) => setParam("delayNoteMode", value)}
                />
                <SliderControl
                  label="Manual Time"
                  value={values.delayTimeMs}
                  min={1}
                  max={2000}
                  formatter={msLabel}
                  disabled={disabled.delay || values.delaySync}
                  onChange={(value) => setParam("delayTimeMs", value)}
                />
                <SelectControl
                  label="Mode"
                  value={values.delayMode}
                  options={selectOptions.delayMode}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayMode", value)}
                />
                <SelectControl
                  label="Style"
                  value={values.delayStyle}
                  options={selectOptions.delayStyle}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayStyle", value)}
                />
                <div className="toggle-stack">
                  <Toggle
                    label="Sync"
                    active={values.delaySync}
                    onChange={(value) => setParam("delaySync", value)}
                  />
                  <Toggle
                    label="Post Reverb"
                    active={values.delayPostReverb}
                    onChange={(value) => setParam("delayPostReverb", value)}
                  />
                </div>
                <SliderControl
                  label="Low Cut"
                  value={values.delayLowCut}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayLowCut", value)}
                />
                <SliderControl
                  label="High Cut"
                  value={values.delayHighCut}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.delay}
                  onChange={(value) => setParam("delayHighCut", value)}
                />
              </div>
            </Panel>

            <Panel
              title="Reverb"
              kicker="Space"
              active={values.reverbEnabled}
              action={
                <Toggle
                  label={values.reverbEnabled ? "On" : "Off"}
                  active={values.reverbEnabled}
                  onChange={(value) => setParam("reverbEnabled", value)}
                />
              }
            >
              <div className="fx-grid reverb-grid">
                <SelectControl
                  label="Type"
                  value={values.reverbMode}
                  options={selectOptions.reverbMode}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbMode", value)}
                />
                <KnobControl
                  label="Mix"
                  value={values.reverbMix}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbMix", value)}
                />
                <KnobControl
                  label="Size"
                  value={values.reverbSize}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbSize", value)}
                />
                <SliderControl
                  label="Decay"
                  value={values.reverbDecay}
                  min={0}
                  max={100}
                  formatter={reverbDecayLabel}
                  disabled={disabled.reverb || values.reverbDecaySync}
                  onChange={(value) => setParam("reverbDecay", value)}
                />
                <SliderControl
                  label="Predelay"
                  value={values.reverbPredelay}
                  min={0}
                  max={100}
                  formatter={msLabel}
                  disabled={disabled.reverb || values.reverbPredelaySync}
                  onChange={(value) => setParam("reverbPredelay", value)}
                />
                <SelectControl
                  label="Decay Sync"
                  value={values.reverbDecayDivision}
                  options={selectOptions.delayDivision}
                  disabled={disabled.reverb || !values.reverbDecaySync}
                  onChange={(value) => setParam("reverbDecayDivision", value)}
                />
                <SelectControl
                  label="Predelay Sync"
                  value={values.reverbPredelayDivision}
                  options={selectOptions.reverbPredelayDivision}
                  disabled={disabled.reverb || !values.reverbPredelaySync}
                  onChange={(value) => setParam("reverbPredelayDivision", value)}
                />
                <SelectControl
                  label="Feel"
                  value={values.reverbNoteMode}
                  options={selectOptions.note}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbNoteMode", value)}
                />
                <div className="toggle-stack">
                  <Toggle
                    label="BPM"
                    active={values.reverbSync}
                    onChange={(value) => setParam("reverbSync", value)}
                  />
                  <Toggle
                    label="Decay Sync"
                    active={values.reverbDecaySync}
                    onChange={(value) => setParam("reverbDecaySync", value)}
                  />
                  <Toggle
                    label="Pre Sync"
                    active={values.reverbPredelaySync}
                    onChange={(value) => setParam("reverbPredelaySync", value)}
                  />
                </div>
                <SliderControl
                  label="Low Cut"
                  value={values.reverbLowCut}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbLowCut", value)}
                />
                <SliderControl
                  label="High Cut"
                  value={values.reverbHighCut}
                  min={0}
                  max={100}
                  formatter={percentLabel}
                  disabled={disabled.reverb}
                  onChange={(value) => setParam("reverbHighCut", value)}
                />
              </div>
            </Panel>
          </section>
        </div>
      </div>
    </main>
  );
}
