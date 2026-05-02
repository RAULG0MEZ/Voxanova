// Faders verticales (los deslizadores tipo consola).
// - VerticalFader: el fader genérico con escala lateral.
// - DynamicsRack: los 3 compresores (Peak Tamer, Glue, In Your Face)
//   en una sola tarjeta unificada.
// - GateStereoFaders: GATE + STEREO en una tarjeta con toggles separados.

import { useRef } from "react";
import { cls, clamp, formatDbFromPercent } from "../utils/format.js";
import { EffectKnob } from "./Knobs.jsx";
import { RackTitleItem } from "./RackHeader.jsx";

const COMPRESSOR_MIN_DB = -60;
const GLUE_BAND_MIN_DB = -48;
const COMPRESSOR_MAX_DB = 0;

export function VerticalFader({
  value,
  accent,
  grColor,
  levelColor,
  tall = false,
  levelValue = undefined,
  reductionValue = undefined,
  reductionDbValue = 0,
  readoutMode = "normal",
  minDb = -40,
  maxDb = 12,
  active = true,
  showScale = true,
  showReductionMeter = true,
  thumbTravelInset = 0,
  valueFill = false,
  onChange
}) {
  const railRef = useRef(null);
  const pointerRef = useRef({ active: false, id: -1 });
  const isPositivePercentFader = minDb >= 0;
  const faderPosition = isPositivePercentFader
    ? clamp(((value - minDb) / (maxDb - minDb)) * 100, 0, 100)
    : clamp(value, 0, 100);
  const thumbPosition =
    thumbTravelInset > 0
      ? thumbTravelInset + (faderPosition / 100) * (100 - thumbTravelInset * 2)
      : faderPosition;
  const scaleTicks = (() => {
    if (minDb === -80 && maxDb === 0) return [0, -20, -40, -60, -80];
    if (minDb === -60 && maxDb === 0) return [0, -12, -24, -36, -48, -60];
    if (minDb === 0 && maxDb === 100) return [100, 50, 0];
    if (minDb >= 0) return [maxDb, 80, 60, 40, 20, 0];
    if (minDb <= -24) return [maxDb, 0, -6, -12, -18, -24];
    return [maxDb, 0, -10, -20, -30];
  })();

  const visualLevelValue = active ? levelValue : levelValue != null ? 0 : undefined;
  const visualFillValue = valueFill ? (active ? faderPosition : 0) : visualLevelValue;
  const visualReductionValue = active ? reductionValue : reductionValue != null ? 0 : undefined;
  const visualReductionDbValue = active ? reductionDbValue : 0;
  const reductionAmount = visualReductionValue != null ? clamp(visualReductionValue, 0, 100) : 0;
  const isThresholdFader = minDb < 0 && visualLevelValue != null;
  const showThresholdReadout = isThresholdFader && readoutMode !== "hidden";
  const hasReductionReadout = visualReductionDbValue > 0.05;
  const reductionDbLabel =
    visualReductionDbValue > 0.05
      ? `-${visualReductionDbValue < 10 ? visualReductionDbValue.toFixed(1) : Math.round(visualReductionDbValue)}`
      : "0";
  const thresholdDbValue = minDb + (clamp(faderPosition, 0, 100) / 100) * (maxDb - minDb);
  const thresholdValueLabel = isThresholdFader
    ? `${thresholdDbValue >= 0 ? "+" : ""}${Math.round(thresholdDbValue)} dB`
    : "";
  const topValueLabel = isThresholdFader
    ? thresholdValueLabel
    : isPositivePercentFader
      ? `${Math.round(value)}%`
      : "";
  const showTopValue = (showThresholdReadout || isPositivePercentFader) && topValueLabel;

  const updateFromClientY = (clientY) => {
    if (!railRef.current) return;
    const rect = railRef.current.getBoundingClientRect();
    const ratio = clamp((rect.bottom - clientY) / rect.height, 0, 1);
    const nextValue = isPositivePercentFader ? minDb + ratio * (maxDb - minDb) : ratio * 100;
    onChange(Number(nextValue.toFixed(1)));
  };

  const onPointerDown = (event) => {
    pointerRef.current = { active: true, id: event.pointerId };
    railRef.current?.setPointerCapture(event.pointerId);
    document.body.classList.add("is-knob-dragging", "is-fader-dragging");
    updateFromClientY(event.clientY);
  };

  const onPointerMove = (event) => {
    if (!pointerRef.current.active || pointerRef.current.id !== event.pointerId) return;
    updateFromClientY(event.clientY);
  };

  const endDrag = (event) => {
    if (pointerRef.current.id !== event.pointerId) return;
    pointerRef.current = { active: false, id: -1 };
    document.body.classList.remove("is-knob-dragging", "is-fader-dragging");
  };

  return (
    <div className={cls("vertical-fader", tall && "tall")}>
      {showScale && scaleTicks.length > 0 && (
        <div className="fader-scale">
          {scaleTicks.map((tick) => {
            const pct = ((tick - minDb) / (maxDb - minDb)) * 100;
            return (
              <span key={tick} style={{ bottom: `${pct}%` }}>
                {isPositivePercentFader ? `${tick}%` : tick > 0 ? `+${tick}` : tick}
              </span>
            );
          })}
        </div>
      )}
      <div
        ref={railRef}
        className={cls("fader-rail", isThresholdFader && "fader-rail-threshold")}
        onPointerDown={onPointerDown}
        onPointerMove={onPointerMove}
        onPointerUp={endDrag}
        onPointerCancel={endDrag}
      >
        {visualFillValue != null && (
          <div className={cls("fader-level-meter", valueFill && "fader-value-fill", `level-${levelColor ?? accent}`)}>
            <div className="fader-level-bar" style={{ height: `${clamp(visualFillValue, 0, 100)}%` }} />
          </div>
        )}
        {isThresholdFader ? (
          <div
            className={cls("fader-threshold-marker", `threshold-${accent}`)}
            style={{ bottom: `${faderPosition}%` }}
          >
            <span className="fader-threshold-line fader-thumb-control">
              <span className="fader-thumb-control-line" />
            </span>
            <span className="fader-threshold-arrow" />
          </div>
        ) : (
          <div className={cls("fader-thumb", "fader-thumb-control", `thumb-${accent}`)} style={{ bottom: `${thumbPosition}%` }}>
            <span className="fader-thumb-control-line" />
          </div>
        )}
        {showReductionMeter && visualReductionValue != null && (
          <div className={cls("fader-gr-meter", `gr-${grColor ?? accent}`)}>
            <div className="fader-gr-bar" style={{ height: `${reductionAmount}%` }} />
          </div>
        )}
        {showThresholdReadout && hasReductionReadout && (
          <span className={cls("fader-gr-value-label", readoutMode === "compact" && "is-compact")}>
            {reductionDbLabel}
          </span>
        )}
      </div>
      {showTopValue && <span className="fader-value-label">{topValueLabel}</span>}
    </div>
  );
}

function GlueHorizontalBand({
  label,
  value,
  levelValue = 0,
  reductionValue = 0,
  active = true,
  onChange
}) {
  const railRef = useRef(null);
  const pointerRef = useRef({ active: false, id: -1 });
  const faderPosition = clamp(value, 0, 100);
  const visualLevelValue = active ? clamp(levelValue, 0, 100) : 0;
  const visualReductionValue = active ? clamp(reductionValue, 0, 100) : 0;
  const thresholdDbValue = formatDbFromPercent(faderPosition, GLUE_BAND_MIN_DB, COMPRESSOR_MAX_DB);

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
    <div className="glue-band-row">
      <span className="glue-band-name">{label}</span>
      <div className="glue-band-horizontal-meter">
        <div className="glue-band-gr-rail" aria-hidden="true">
          <div className="glue-band-gr-fill" style={{ width: `${visualReductionValue}%` }} />
        </div>
        <div
          ref={railRef}
          className="glue-band-main-rail"
          onPointerDown={onPointerDown}
          onPointerMove={onPointerMove}
          onPointerUp={endDrag}
          onPointerCancel={endDrag}
        >
          <div className="glue-band-level-fill" style={{ width: `${visualLevelValue}%` }} />
          <div className="glue-band-horizontal-thumb fader-thumb-control" style={{ left: `${faderPosition}%` }}>
            <span className="fader-thumb-control-line" />
          </div>
        </div>
      </div>
      <span className="glue-band-db">{thresholdDbValue} dB</span>
    </div>
  );
}

export function DynamicsRack({
  compressors,
  glueBands,
  glueMultibandOn,
  onGlueBandChange
}) {
  const getCompressorTitle = (compressor) =>
    compressor.title === "GLUE" && glueMultibandOn ? "MULTIBAND GLUE" : compressor.title;

  return (
    <article
      className={cls(
        "module-card",
        "module-compressorRack",
        glueMultibandOn && "is-glue-multiband"
      )}
    >
      <div className="rack-title-row">
        {compressors.map((compressor) => (
          <RackTitleItem
            key={compressor.title}
            title={getCompressorTitle(compressor)}
            color={compressor.color}
            active={compressor.active}
            label={compressor.title}
            onToggle={compressor.onToggle}
          />
        ))}
      </div>
      <div className="rack-control-row compressor-control-row">
        {compressors.map((compressor) => (
          <div key={`${compressor.title}-controls`} className="rack-control-cell">
            <span className="rack-control-spacer" aria-hidden="true" />
          </div>
        ))}
      </div>

      <div className="rack-fader-row compressor-rack-faders">
        {compressors.map((compressor) => (
          <div
            key={compressor.title}
            className={cls("rack-fader-cell", !compressor.active && "is-bypassed")}
          >
            {compressor.title === "GLUE" && glueMultibandOn ? (
              <>
                <div className="fader-pair compressor-meter-pair glue-multiband-faders glue-multiband-horizontal">
                  {[
                    ["low", "0-100"],
                    ["lowMid", "100-1k"],
                    ["highMid", "1-10k"],
                    ["high", "10k+"]
                  ].map(([key, label]) => (
                    <GlueHorizontalBand
                      key={key}
                      label={label}
                        value={glueBands?.[key] ?? compressor.value.thr}
                        levelValue={compressor.value.level}
                        reductionValue={compressor.value.gr}
                        active={compressor.active}
                        onChange={(thr) => onGlueBandChange?.(key, thr)}
                      />
                  ))}
                </div>
                <div className={cls("value-row", "compressor-values", `text-${compressor.color}`)}>
                  <span>Multiband</span>
                </div>
              </>
            ) : (
              <>
                <div className="fader-pair compressor-meter-pair">
                  <VerticalFader
                    value={compressor.value.thr}
                    levelValue={compressor.control === "mix" ? undefined : compressor.value.level}
                    levelColor="blue"
                    reductionValue={compressor.value.gr}
                    reductionDbValue={compressor.value.grDb}
                    active={compressor.active}
                    accent={compressor.color}
                    grColor="blue-soft"
                    readoutMode={compressor.control === "mix" ? "hidden" : "normal"}
                    showScale
                    showReductionMeter={compressor.control !== "mix"}
                    valueFill={compressor.control === "mix"}
                    minDb={compressor.control === "mix" ? 0 : COMPRESSOR_MIN_DB}
                    maxDb={compressor.control === "mix" ? 100 : COMPRESSOR_MAX_DB}
                    onChange={(thr) => compressor.onChange({ ...compressor.value, thr })}
                  />
                </div>
                <div className={cls("value-row", "compressor-values", `text-${compressor.color}`)}>
                  <span>
                    {compressor.control === "mix"
                      ? `${Math.round(compressor.value.thr)}%`
                      : `${formatDbFromPercent(compressor.value.thr, COMPRESSOR_MIN_DB, COMPRESSOR_MAX_DB)} dB`}
                  </span>
                </div>
              </>
            )}
          </div>
        ))}
      </div>
    </article>
  );
}

export function GateStereoFaders({
  color,
  gateThreshold,
  onGateThresholdChange,
  gateLevel = 0,
  gateReduction = 0,
  gateReductionDb = 0,
  gateActive = true,
  onGateToggle,
  stereoWidth,
  onStereoChange,
  stereoLowBypass = 0,
  onStereoLowBypassChange,
  stereoActive = true,
  onStereoToggle
}) {
  return (
    <div className="dual-fader gate-stereo-faders">
      <div className="rack-title-row gate-stereo-title-row">
        <RackTitleItem title="GATE" active={gateActive} onToggle={onGateToggle} />
        <RackTitleItem title="STEREO" active={stereoActive} onToggle={onStereoToggle} />
      </div>
      <div className="rack-control-row gate-stereo-control-row">
        <div className="rack-control-cell"><span className="rack-control-spacer" aria-hidden="true" /></div>
        <div className="rack-control-cell"><span className="rack-control-spacer" aria-hidden="true" /></div>
      </div>
      <div className="fader-pair rack-fader-row compressor-meter-pair gate-stereo-pair">
        <div className={cls("rack-fader-cell", !gateActive && "is-bypassed")}>
          <VerticalFader
            value={gateThreshold}
            accent={color}
            levelValue={gateLevel}
            levelColor={color}
            reductionValue={gateReduction}
            reductionDbValue={gateReductionDb}
            active={gateActive}
            grColor="blue-soft"
            tall
            minDb={-80}
            maxDb={0}
            onChange={onGateThresholdChange}
          />
        </div>
        <div className={cls("rack-fader-cell", "stereo-knob-cell", !stereoActive && "is-bypassed")}>
          <div className="stereo-knob-stack">
            <EffectKnob
              className="stereo-rack-knob stereo-width-knob indicator-dot"
              label="Width"
              value={stereoWidth}
              min={0}
              max={100}
              formatter={(v) => `${Math.round(v)}%`}
              progressTone="bright"
              onChange={onStereoChange}
            />
            <EffectKnob
              className="stereo-rack-knob stereo-low-bypass-knob indicator-dot"
              label="Low Bypass"
              value={stereoLowBypass}
              min={0}
              max={500}
              formatter={(v) => `${Math.round(v)} Hz`}
              progressTone="bright"
              onChange={onStereoLowBypassChange}
            />
          </div>
        </div>
      </div>
      <div className={cls("value-row", "compressor-values", `text-${color}`)}>
        <span>{formatDbFromPercent(gateThreshold, -80, 0)} dB</span>
        <span>{Math.round(stereoLowBypass)} Hz</span>
      </div>
    </div>
  );
}
