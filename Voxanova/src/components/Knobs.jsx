// Todas las perillas (knobs) de la UI.
// - Dial: la perilla genérica con drag vertical/horizontal.
// - EqKnob: variante con etiqueta y valor para el panel de EQ.
// - StereoDial: perilla simétrica para el control de stereo.

import { useCallback, useEffect, useRef } from "react";
import { cls, clamp } from "../utils/format.js";

export function Dial({
  value,
  min,
  max,
  color,
  glow = false,
  compact = false,
  stepped = false,
  stepCount = 0,
  detentCount = 0,
  step = 1,
  progressStyle,
  onChange
}) {
  const sweep = 270;
  const startAngle = -135;
  const normalized = clamp((value - min) / (max - min), 0, 1);
  const fillDeg = normalized * sweep;
  const angle = startAngle + normalized * sweep;
  const dialRef = useRef(null);
  const dragRef = useRef({ active: false, x: 0, y: 0, startValue: value });

  const toSteppedValue = useCallback(
    (raw) => {
      if (!stepped || stepCount < 2) return raw;
      const norm = clamp((raw - min) / (max - min), 0, 1);
      const snappedIndex = Math.round(norm * (stepCount - 1));
      return min + (snappedIndex / (stepCount - 1)) * (max - min);
    },
    [max, min, stepCount, stepped]
  );

  const quantizeValue = useCallback(
    (raw) => {
      if (!step || stepped) return raw;
      const snapped = Math.round((raw - min) / step) * step + min;
      return Number(clamp(snapped, min, max).toFixed(4));
    },
    [max, min, step, stepped]
  );

  useEffect(() => {
    if (!dragRef.current.active) {
      dragRef.current.startValue = value;
    }
  }, [value]);

  useEffect(() => {
    const updateFromPointer = (event) => {
      if (!dragRef.current.active) return;
      const drag = dragRef.current;
      const deltaY = drag.y - event.clientY;
      const deltaX = event.clientX - drag.x;
      const combined = deltaY + deltaX * 0.8;
      const sensitivity = (max - min) / 180;
      const raw = drag.startValue + combined * sensitivity;
      const next = quantizeValue(toSteppedValue(clamp(raw, min, max)));
      onChange(next);
    };
    const onMove = (event) => {
      if (!dragRef.current.active) return;
      updateFromPointer(event);
    };
    const onUp = () => {
      dragRef.current.active = false;
      document.body.classList.remove("is-knob-dragging");
    };

    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
    return () => {
      window.removeEventListener("pointermove", onMove);
      window.removeEventListener("pointerup", onUp);
      window.removeEventListener("pointercancel", onUp);
    };
  }, [max, min, onChange, quantizeValue, step, stepCount, stepped, toSteppedValue]);

  const onPointerDown = (event) => {
    event.preventDefault();
    event.currentTarget.setPointerCapture?.(event.pointerId);
    dragRef.current = {
      active: true,
      x: event.clientX,
      y: event.clientY,
      startValue: value
    };
    document.body.classList.add("is-knob-dragging");
  };

  return (
    <div
      ref={dialRef}
      className={cls(
        "dial",
        glow && "dial-glow",
        compact && "dial-compact",
        stepped && "dial-stepped"
      )}
      onPointerDown={onPointerDown}
    >
      {detentCount > 1 && (
        <div className="dial-detents" aria-hidden="true">
          {Array.from({ length: detentCount }).map((_, index) => {
            const detentAngle = -135 + (270 * index) / (detentCount - 1);
            return (
              <span
                key={`detent-${index}`}
                className="dial-detent"
                style={{ transform: `translate(-50%, -50%) rotate(${detentAngle}deg)` }}
              />
            );
          })}
        </div>
      )}
      <div className="dial-arc">
        <div
          className={cls("dial-progress", `dial-${color}`)}
          style={
            progressStyle ?? {
              background:
                fillDeg <= 0
                  ? "transparent"
                  : `conic-gradient(
                      from ${startAngle + 360}deg,
                      var(--${color}) 0deg ${fillDeg}deg,
                      transparent ${fillDeg}deg 360deg
                    )`
            }
          }
        />
      </div>
      <div className="dial-core">
        <div className="dial-indicator" style={{ transform: `rotate(${angle}deg)` }} />
      </div>
    </div>
  );
}

export function EqKnob({ className, label, value, min, max, step = 1, color, onChange, format }) {
  const normalized = clamp((value - min) / (max - min), 0, 1);
  const fillDeg = normalized * 270;
  const progressStyle = {
    background:
      fillDeg <= 0
        ? "transparent"
        : `conic-gradient(
            from 225deg,
            ${color} 0deg ${fillDeg}deg,
            transparent ${fillDeg}deg 360deg
          )`
  };

  return (
    <div className={cls("eq-knob", "indicator-dot", className)} style={{ "--band-color": color }}>
      <span>{label}</span>
      <Dial
        value={value}
        min={min}
        max={max}
        step={step}
        color="blue"
        compact
        progressStyle={progressStyle}
        onChange={onChange}
      />
      <strong>{format(value)}</strong>
    </div>
  );
}

export function EffectKnob({
  className,
  label,
  value,
  min = 0,
  max = 100,
  unit = "",
  formatter,
  progressTone = "default",
  stepped = false,
  stepCount = 0,
  onChange
}) {
  const normalized = clamp((value - min) / (max - min), 0, 1);
  const fillDeg = normalized * 270;
  const displayValue = formatter ? formatter(value) : `${Math.round(value)} ${unit}`.trim();
  const progressColor =
    progressTone === "bright" ? "rgba(10, 132, 255, 0.96)" : "rgba(10, 132, 255, 0.44)";
  const progressStyle = {
    background:
      fillDeg <= 0
        ? "transparent"
        : `conic-gradient(
            from 225deg,
            ${progressColor} 0deg ${fillDeg}deg,
            transparent ${fillDeg}deg 360deg
          )`
  };

  return (
    <div className={cls("eq-knob", className)}>
      <span>{label}</span>
      <Dial
        value={value}
        min={min}
        max={max}
        color="blue"
        compact
        stepped={stepped}
        stepCount={stepCount}
        detentCount={stepped ? stepCount : 0}
        progressStyle={progressStyle}
        onChange={onChange}
      />
      <strong>{displayValue}</strong>
    </div>
  );
}

export function StereoDial({ value, color, onChange }) {
  const normalized = clamp(value / 100, 0, 1);
  const leftEdge = 135 * (1 - normalized);
  const rightEdge = 135 * (1 + normalized);
  const dot1Angle = -normalized * 135;
  const dot2Angle = normalized * 135;
  const progressBackground =
    normalized <= 0
      ? "transparent"
      : `conic-gradient(
          from 225deg,
          rgba(255,255,255,0.08) 0deg ${leftEdge}deg,
          var(--${color}) ${leftEdge}deg ${rightEdge}deg,
          rgba(255,255,255,0.08) ${rightEdge}deg 270deg,
          rgba(255,255,255,0.02) 270deg 360deg
        )`;

  const dialRef = useRef(null);
  const dragRef = useRef({ active: false, x: 0, y: 0, startValue: value });

  useEffect(() => {
    if (!dragRef.current.active) {
      dragRef.current.startValue = value;
    }
  }, [value]);

  useEffect(() => {
    const onMove = (event) => {
      if (!dragRef.current.active) return;
      const drag = dragRef.current;
      const deltaY = drag.y - event.clientY;
      const deltaX = event.clientX - drag.x;
      const combined = deltaY + deltaX * 0.8;
      const raw = drag.startValue + combined * (100 / 180);
      onChange(clamp(Math.round(raw), 0, 100));
    };
    const onUp = () => {
      dragRef.current.active = false;
      document.body.classList.remove("is-knob-dragging");
    };
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
    return () => {
      window.removeEventListener("pointermove", onMove);
      window.removeEventListener("pointerup", onUp);
      window.removeEventListener("pointercancel", onUp);
    };
  }, [onChange]);

  const onPointerDown = (event) => {
    event.preventDefault();
    event.currentTarget.setPointerCapture?.(event.pointerId);
    dragRef.current = { active: true, x: event.clientX, y: event.clientY, startValue: value };
    document.body.classList.add("is-knob-dragging");
  };

  return (
    <div
      ref={dialRef}
      className={cls("dial stereo-dial", `dial-${color}`)}
      onPointerDown={onPointerDown}
    >
      <div className="dial-arc">
        <div
          className={cls("dial-progress", `dial-${color}`)}
          style={{
            background: progressBackground
          }}
        />
      </div>
      <div className="dial-core">
        <div className="stereo-dot" style={{ transform: `rotate(${dot1Angle}deg)` }} />
        <div className="stereo-dot" style={{ transform: `rotate(${dot2Angle}deg)` }} />
      </div>
    </div>
  );
}
