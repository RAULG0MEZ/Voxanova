// Tarjeta genérica de un módulo. Según `module.type` renderiza:
// - "curve"      → MiniCurve (curvita decorativa del PRE/POST EQ)
// - "gateStereo" → Gate + Stereo (con sus toggles independientes)
// - "stereo"     → solo Stereo (perilla)
// - "effects"    → sección de Reverb + Delay
//
// Los compresores (PEAK TAMER, GLUE, IN YOUR FACE) NO usan este
// componente: viven dentro de `DynamicsRack` en Faders.jsx.

import { cls } from "../utils/format.js";
import { GateStereoFaders } from "./Faders.jsx";
import { StereoDial } from "./Knobs.jsx";
import { EffectsModule } from "./EffectsModule.jsx";

export function ModuleCard({
  module,
  active,
  onToggle,
  // controles de gate / stereo
  stereoWidth,
  onStereoChange,
  stereoLowBypass,
  onStereoLowBypassChange,
  stereoActive,
  onStereoToggle,
  gateThreshold,
  onGateThresholdChange,
  gateLevel,
  gateReduction,
  gateReductionDb,
  // efectos: objetos completos (no props sueltas)
  reverb,
  setReverb,
  delay,
  setDelay,
  hostBpm,
  effectToggles,
  onEffectToggle,
  eqExpanded
}) {
  const hasCustomRackTitle = module.type === "gateStereo" || module.type === "effects";
  const moduleIsActive = hasCustomRackTitle ? active || stereoActive : active;

  return (
    <article
      className={cls(
        "module-card",
        `module-${module.type}`,
        `is-${module.color}`,
        moduleIsActive ? "is-active" : "is-bypassed"
      )}
    >
      <div className={cls("module-accent", `accent-${module.color}`)} />
      {!hasCustomRackTitle && (
        <div className={cls("module-top", module.type === "curve" && "module-top-curve")}>
          <div className="module-title-wrap">
            <button
              className={cls("module-dot", `dot-${module.color}`, active && "active")}
              onClick={onToggle}
              aria-label={`${active ? "Disable" : "Enable"} ${module.title}`}
            />
            <h3>{module.title}</h3>
          </div>
          <div className="module-top-controls" />
        </div>
      )}

      <div className="module-content">
        {module.type === "curve" && <MiniCurve color={module.color} />}
        {module.type === "gateStereo" && (
          <GateStereoFaders
            color={module.color}
            gateThreshold={gateThreshold}
            onGateThresholdChange={onGateThresholdChange}
            gateLevel={gateLevel}
            gateReduction={gateReduction}
            gateReductionDb={gateReductionDb}
            gateActive={active}
            onGateToggle={onToggle}
            stereoWidth={stereoWidth}
            onStereoChange={onStereoChange}
            stereoLowBypass={stereoLowBypass}
            onStereoLowBypassChange={onStereoLowBypassChange}
            stereoActive={stereoActive}
            onStereoToggle={onStereoToggle}
          />
        )}
        {module.type === "stereo" && (
          <StereoModule value={stereoWidth} color={module.color} onChange={onStereoChange} />
        )}
        {module.type === "effects" && (
          <EffectsModule
            reverb={reverb}
            setReverb={setReverb}
            delay={delay}
            setDelay={setDelay}
            hostBpm={hostBpm}
            effectToggles={effectToggles}
            onEffectToggle={onEffectToggle}
            eqExpanded={eqExpanded}
          />
        )}
      </div>
    </article>
  );
}

function MiniCurve({ color }) {
  return (
    <div className="mini-curve">
      <svg viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
        <path
          d="M 5 82 C 20 84, 26 79, 35 60 C 42 46, 54 44, 64 60 C 74 78, 86 86, 95 84"
          className={cls("mini-curve-path", `curve-${color}`)}
        />
      </svg>
    </div>
  );
}

function StereoModule({ value, color, onChange }) {
  const displayValue = Math.round(value);

  return (
    <div className="stereo-module">
      <label className="knob-wrap">
        <StereoDial value={value} color={color} onChange={onChange} />
      </label>
      <div className={cls("dial-value", `text-${color}`)}>{displayValue}%</div>
    </div>
  );
}
