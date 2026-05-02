// Panel flotante con los controles de UNA banda del EQ.
// La forma orgánica (montaña simétrica) se aplica vía clip-path en CSS.
// El drop-shadow en CSS sigue el contorno exacto de la montaña.

import { cls } from "../utils/format.js";
import { FILTER_TYPES } from "../utils/eq.js";
import { EqKnob } from "./Knobs.jsx";

function PowerIcon() {
  return (
    <svg viewBox="0 0 18 18" aria-hidden="true" focusable="false">
      <path d="M9 2.4v6.2" />
      <path d="M5.4 4.9a6 6 0 1 0 7.2 0" />
    </svg>
  );
}

export function BandPanel({
  band,
  left,
  onChange,
  onDelete,
  isVisible,
  isEmphasis,
  onHoverChange,
  onRequestClose
}) {
  const hasSlope = band.type.includes("Cut");

  return (
    <div
      className={cls(
        "eq-band-panel",
        hasSlope && "has-slope",
        !band.on && "is-band-off",
        isVisible ? "is-visible" : "is-hidden",
        isEmphasis ? "is-emphasis" : "is-faded"
      )}
      style={{ "--panel-left": left, "--band-color": band.color }}
      onMouseEnter={() => onHoverChange(true)}
      onMouseLeave={() => {
        onHoverChange(false);
        onRequestClose?.();
      }}
    >
      <button
        className={cls("band-power", band.on && "active")}
        aria-pressed={band.on}
        aria-label={band.on ? "Apagar banda" : "Encender banda"}
        title={band.on ? "Apagar banda" : "Encender banda"}
        onClick={() => onChange("on", !band.on)}
      >
        <PowerIcon />
      </button>
      <label>
        <select value={band.type} onChange={(event) => onChange("type", event.target.value)}>
          {FILTER_TYPES.map((type) => (
            <option key={type} value={type}>
              {type}
            </option>
          ))}
        </select>
        <span>TYPE</span>
      </label>
      <EqKnob
        label="FREQ"
        value={band.freq}
        min={20}
        max={20000}
        color={band.color}
        onChange={(value) => onChange("freq", value)}
        format={(value) =>
          value >= 1000 ? `${(value / 1000).toFixed(2)} kHz` : `${Math.round(value)} Hz`
        }
      />
      <EqKnob
        className="gain-knob"
        label="GAIN"
        value={band.gain || 0}
        min={-18}
        max={18}
        step={0.5}
        color={band.color}
        onChange={(value) => onChange("gain", value)}
        format={(value) => `${value > 0 ? "+" : ""}${value.toFixed(1)} dB`}
      />
      <EqKnob
        label="Q"
        value={band.q}
        min={0.1}
        max={band.type.includes("Cut") || band.type.includes("Shelf") ? 10 : 50}
        step={0.1}
        color={band.color}
        onChange={(value) => onChange("q", value)}
        format={(value) => value.toFixed(value >= 10 ? 0 : 1)}
      />
      {band.type.includes("Cut") && (
        <label className="band-slope">
          <select
            value={band.slope || 12}
            onChange={(event) => onChange("slope", Number(event.target.value))}
          >
            {[6, 12, 18, 24, 36, 48, 72, 96].map((slope) => (
              <option key={slope} value={slope}>
                {slope} dB/oct
              </option>
            ))}
          </select>
          <span>SLOPE</span>
        </label>
      )}
      <button className="band-delete" onClick={onDelete}>
        X
      </button>
    </div>
  );
}
