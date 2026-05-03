import React from "react";
import { BypassBtn, PillGroup, Select } from "./controls.jsx";
import { Knob } from "./knob.jsx";
import { auxOutputOptions, delayDivisions, delayStyles, reverbModes, reverbPredelayDivisions } from "../pluginContract.js";

// modules.jsx — Compressor modules with proper threshold/GR/output gain logic

function ModuleHeader({ on, setOn, name }) {
  return (
    <div className="mod-header">
      <BypassBtn on={on} onChange={setOn} />
      <span className="mod-name" title={name}>{name}</span>
    </div>
  );
}

const WAVE_CANVAS_WIDTH = 600;
const WAVE_CANVAS_HEIGHT = 210;
const WAVE_CANVAS_STYLE = { width: '100%', height: '70px', display: 'block' };

function prepareWaveCanvas(canvas, ctx) {
  const rect = canvas.getBoundingClientRect();
  const dpr = Math.max(1, Math.min(3, window.devicePixelRatio || 1));
  const cssWidth = Math.max(1, rect.width || WAVE_CANVAS_WIDTH);
  const cssHeight = Math.max(1, rect.height || 70);
  const pixelWidth = Math.round(cssWidth * dpr);
  const pixelHeight = Math.round(cssHeight * dpr);

  if (canvas.width !== pixelWidth || canvas.height !== pixelHeight) {
    canvas.width = pixelWidth;
    canvas.height = pixelHeight;
  }

  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, cssWidth, cssHeight);
  ctx.imageSmoothingEnabled = true;
  ctx.imageSmoothingQuality = 'high';

  return { W: cssWidth, H: cssHeight };
}

function drawThresholdLines(ctx, W, H, thrY, color) {
  ctx.save();
  ctx.strokeStyle = color;
  ctx.globalAlpha = 0.98;
  ctx.lineWidth = 1;
  ctx.lineCap = 'butt';
  ctx.beginPath();
  ctx.moveTo(0, Math.round(thrY) + 0.5);
  ctx.lineTo(W, Math.round(thrY) + 0.5);
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(0, Math.round(H - thrY) + 0.5);
  ctx.lineTo(W, Math.round(H - thrY) + 0.5);
  ctx.stroke();
  ctx.restore();
}

function normalizeWaveform(samples) {
  if (!Array.isArray(samples) || samples.length < 2) return [];
  return samples.map((sample) => {
    const numeric = Number(sample);
    if (!Number.isFinite(numeric)) return 0;
    return Math.max(-1, Math.min(1, numeric));
  });
}

function waveformIsSilent(samples) {
  return !samples.some((sample) => Math.abs(sample) > 0.0005);
}

function sampleWaveform(samples, position) {
  if (samples.length === 0) return 0;
  const scaled = Math.max(0, Math.min(1, position)) * (samples.length - 1);
  const left = Math.floor(scaled);
  const right = Math.min(samples.length - 1, left + 1);
  const mix = scaled - left;
  return samples[left] + (samples[right] - samples[left]) * mix;
}

const WAVE_VERTICAL_SCALE = 0.92;

function dbToLinearDistance(db, halfH) {
  return Math.pow(10, Math.min(0, Number(db) || 0) / 20) * halfH * WAVE_VERTICAL_SCALE;
}

function clampPercent(value) {
  return Math.max(0, Math.min(100, Number(value) || 0));
}

function gainReductionPercent(reduction, grDb, maxDb = 20) {
  const meter = Number(reduction);
  if (Number.isFinite(meter) && meter > 0) {
    return clampPercent(meter > 1 ? meter : meter * 100);
  }

  return clampPercent((Math.max(0, Number(grDb) || 0) / maxDb) * 100);
}

function thresholdToY(thresholdDb, halfH) {
  return halfH - dbToLinearDistance(thresholdDb, halfH);
}

function makeLinearMagnitudeTarget(samples, W, halfH) {
  if (samples.length < 2) return [];
  const steps = Math.max(48, Math.round(W));
  return Array.from({ length: steps }, (_, index) => {
    const t = steps <= 1 ? 0 : index / (steps - 1);
    return Math.min(1, Math.abs(sampleWaveform(samples, t))) * halfH * WAVE_VERTICAL_SCALE;
  });
}

function smoothMagnitudes(ref, target) {
  if (!target.length) {
    ref.current = [];
    return [];
  }

  if (!Array.isArray(ref.current) || ref.current.length !== target.length) {
    ref.current = [...target];
    return ref.current;
  }

  ref.current = ref.current.map((current, index) => {
    const delta = target[index] - current;
    const coeff = delta > 0 ? 0.82 : 0.38;
    return current + delta * coeff;
  });
  return ref.current;
}

function pointsFromMagnitudes(magnitudes, W, halfH) {
  const last = Math.max(1, magnitudes.length - 1);
  return magnitudes.map((mag, index) => {
    const x = (index / last) * W;
    return {
      x,
      mag,
      top: halfH - mag,
      bottom: halfH + mag,
    };
  });
}

function drawFilledWave(ctx, points, fill, alpha) {
  if (points.length < 2) return;

  ctx.save();
  ctx.fillStyle = fill;
  ctx.globalAlpha = alpha;
  ctx.beginPath();
  points.forEach((point, index) => {
    index === 0 ? ctx.moveTo(point.x, point.top) : ctx.lineTo(point.x, point.top);
  });
  for (let index = points.length - 1; index >= 0; index--) {
    ctx.lineTo(points[index].x, points[index].bottom);
  }
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}

function formatReverbDecay(value) {
  const normalized = Math.max(0, Math.min(1, Number(value || 0) / 100));
  const seconds = 0.2 + ((90 ** normalized - 1) / 89) * 17.8;
  return seconds >= 10 ? `${seconds.toFixed(1)}s` : `${seconds.toFixed(2)}s`;
}

function formatPredelayMs(value) {
  return `${Math.round(Math.max(0, Number(value) || 0) * 0.5)}ms`;
}

// ─── Compressor Waveform Viz ──────────────────────────────────────────────────
function CompWaveViz({ threshold, active, waveform }) {
  const canvasRef = React.useRef(null);
  const waveformRef = React.useRef(normalizeWaveform(waveform));
  const magnitudeRef = React.useRef([]);

  React.useEffect(() => {
    waveformRef.current = normalizeWaveform(waveform);
  }, [waveform]);

  React.useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let raf;

    const draw = () => {
      const { W, H } = prepareWaveCanvas(canvas, ctx);

      const halfH = H / 2;
      const thrY = thresholdToY(threshold, halfH);
      const thresholdLimit = halfH - thrY;
      const rootStyle = getComputedStyle(document.documentElement);
      const accentColor = rootStyle.getPropertyValue('--accent').trim() || '#4a90d9';
      const warmColor = rootStyle.getPropertyValue('--warm').trim() || '#e07840';
      const darkMode = document.body?.dataset?.mode === 'dark';
      const waveFillColor = darkMode ? accentColor : (rootStyle.getPropertyValue('--ink-1').trim() || '#151821');
      const idleColor = darkMode ? 'rgba(255,255,255,0.15)' : 'rgba(0,0,0,0.10)';
      const waveformSamples = waveformRef.current;

      // The center stays clean; the shaded area lives outside the threshold.
      ctx.save();
      ctx.fillStyle = warmColor;
      ctx.globalAlpha = darkMode ? 0.13 : 0.10;
      ctx.fillRect(0, 0, W, thrY);
      ctx.fillRect(0, H - thrY, W, thrY);
      ctx.restore();

      if (!active || waveformSamples.length < 2 || waveformIsSilent(waveformSamples)) {
        magnitudeRef.current = [];
        ctx.strokeStyle = idleColor;
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, halfH); ctx.lineTo(W, halfH); ctx.stroke();
        drawThresholdLines(ctx, W, H, thrY, warmColor);
        raf = requestAnimationFrame(draw);
        return;
      }

      const target = makeLinearMagnitudeTarget(waveformSamples, W, halfH);
      const pts = pointsFromMagnitudes(smoothMagnitudes(magnitudeRef, target), W, halfH);

      drawFilledWave(ctx, pts, waveFillColor, darkMode ? 0.44 : 0.64);

      // Fill only the waveform portions that live outside the threshold.
      const limit = thresholdLimit;
      let i = 0;
      while (i < pts.length) {
        while (i < pts.length && pts[i].mag <= limit) i++;
        if (i >= pts.length) break;
        const start = i;
        while (i < pts.length && pts[i].mag > limit) i++;
        const end = i;

        ctx.save();
        ctx.fillStyle = warmColor;
        ctx.globalAlpha = darkMode ? 0.34 : 0.30;

        ctx.beginPath();
        ctx.moveTo(pts[start].x, pts[start].top);
        for (let k = start + 1; k < end; k++) ctx.lineTo(pts[k].x, pts[k].top);
        for (let k = end - 1; k >= start; k--) ctx.lineTo(pts[k].x, thrY);
        ctx.closePath();
        ctx.fill();

        ctx.beginPath();
        ctx.moveTo(pts[start].x, H - thrY);
        for (let k = start; k < end; k++) ctx.lineTo(pts[k].x, pts[k].bottom);
        for (let k = end - 1; k >= start; k--) ctx.lineTo(pts[k].x, H - thrY);
        ctx.closePath();
        ctx.fill();

        ctx.restore();
      }

      drawThresholdLines(ctx, W, H, thrY, warmColor);

      raf = requestAnimationFrame(draw);
    };

    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
  }, [active, threshold]);

  return (
    <canvas
      ref={canvasRef}
      width={WAVE_CANVAS_WIDTH}
      height={WAVE_CANVAS_HEIGHT}
      style={WAVE_CANVAS_STYLE}
    />
  );
}

function RackThresholdSlider({ label = 'THR', value, min = -60, max = 0, onChange, format, className = '' }) {
  const norm = Math.max(0, Math.min(1, (value - min) / (max - min)));
  const display = format ? format(value) : `${value.toFixed(0)} dB`;

  const onPointerDown = (e) => {
    e.preventDefault();
    const target = e.currentTarget;
    target.setPointerCapture(e.pointerId);
    const update = (ev) => {
      const rect = target.getBoundingClientRect();
      onChange(min + Math.max(0, Math.min(1, (ev.clientX - rect.left) / rect.width)) * (max - min));
    };
    update(e);
    const move = ev => update(ev);
    const up = () => {
      window.removeEventListener('pointermove', move);
      window.removeEventListener('pointerup', up);
    };
    window.addEventListener('pointermove', move);
    window.addEventListener('pointerup', up);
  };

  return (
    <div className={`rev-size-row rack-threshold-row${className ? ` ${className}` : ''}`}>
      <span className="rev-size-lbl rack-threshold-lbl">{label}</span>
      <div className="rev-slider rack-threshold-slider">
        <div className="rev-slider-track" onPointerDown={onPointerDown}>
          <div className="rev-slider-fill" style={{ width: `${norm * 100}%` }} />
          <div className="rev-slider-handle" style={{ left: `${norm * 100}%` }} />
        </div>
        <span className="rev-size-val rack-threshold-val">{display}</span>
      </div>
    </div>
  );
}

// ─── Compressor Module ────────────────────────────────────────────────────────
function CompModule({ name, threshold, setThreshold, on, setOn, signalActive, waveform, reduction = 0, reductionDb = 0 }) {
  const grDb = Math.max(0, Math.abs(Number(reductionDb) || 0));
  const grPct = gainReductionPercent(reduction, grDb);

  return (
    <div className={`comp-unit${!on ? ' bypassed' : ''}`}>
      <div className="comp-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <span className="mod-name">{name}</span>
      </div>
      <div className="comp-unit-body">
        <div className="comp-wave-wrap">
          <CompWaveViz threshold={threshold} active={signalActive && on} waveform={waveform} />
        </div>
        <RackThresholdSlider value={threshold} onChange={setThreshold} />
        <div className="comp-fader-row gr">
          <span className="comp-fader-lbl">GR</span>
          <div className="comp-gr-track">
            <div className="comp-gr-fill" style={{ width: `${grPct}%` }} />
          </div>
          <span className="comp-gr-val">{grDb > 0.1 ? `−${grDb.toFixed(1)}` : '0.0'}</span>
        </div>
      </div>
    </div>
  );
}

function MultiButterThreshold({ label, value, onChange, grValue = 0 }) {
  const grPct = Math.min(100, (grValue / 20) * 100);
  return (
    <div className="multi-band-control">
      <RackThresholdSlider label={label} value={value} onChange={onChange} className="multi-thr-row" />
      <div className="multi-gr-row">
        <span className="multi-gr-lbl">GR</span>
        <div className="multi-gr-track">
          <div className="multi-gr-fill" style={{ width: `${grPct}%` }} />
        </div>
        <span className="multi-gr-val">{grValue > 0.1 ? `-${grValue.toFixed(1)}` : '0.0'}</span>
      </div>
    </div>
  );
}

function ButterCompModule({ mode, setMode, threshold, setThreshold, multiThresholds, setMultiThresholds, on, setOn, signalActive, waveform, reduction = 0, reductionDb = 0, bandReductionDbs = [] }) {
  const isMulti = mode === 'MULTI BUTTER';
  const bands = ['LOW', 'L MID', 'H MID', 'HIGH'];
  const grDb = Math.max(0, Math.abs(Number(reductionDb) || 0));
  const grPct = gainReductionPercent(reduction, grDb);
  const updateBand = (idx, value) => {
    setMultiThresholds(current => current.map((v, i) => i === idx ? value : v));
  };

  return (
    <div className={`comp-unit butter-unit${!on ? ' bypassed' : ''}${isMulti ? ' multi' : ''}`}>
      <div className="comp-unit-header butter-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <div className="butter-mode-tabs">
          <PillGroup
            value={mode}
            onChange={setMode}
            options={['BUTTER COMP', 'MULTI BUTTER']}
            stretch
          />
        </div>
      </div>
      {isMulti ? (
        <div className="comp-unit-body multi-butter-body">
          {bands.map((band, idx) => (
            <MultiButterThreshold
              key={band}
              label={band}
              value={multiThresholds[idx]}
              onChange={value => updateBand(idx, value)}
              grValue={Math.max(0, Math.abs(Number(bandReductionDbs[idx]) || 0))}
            />
          ))}
        </div>
      ) : (
        <div className="comp-unit-body">
        <div className="comp-wave-wrap">
          <CompWaveViz threshold={threshold} active={signalActive && on} waveform={waveform} />
        </div>
          <RackThresholdSlider value={threshold} onChange={setThreshold} />
          <div className="comp-fader-row gr">
            <span className="comp-fader-lbl">GR</span>
            <div className="comp-gr-track">
              <div className="comp-gr-fill" style={{ width: `${grPct}%` }} />
            </div>
            <span className="comp-gr-val">{grDb > 0.1 ? `−${grDb.toFixed(1)}` : '0.0'}</span>
          </div>
        </div>
      )}
    </div>
  );
}

// ─── In Your Face waveform viz ────────────────────────────────────────────────
function IYFViz({ active, amount, waveform }) {
  const canvasRef = React.useRef(null);
  const waveformRef = React.useRef(normalizeWaveform(waveform));
  const magnitudeRef = React.useRef([]);

  React.useEffect(() => {
    waveformRef.current = normalizeWaveform(waveform);
  }, [waveform]);

  React.useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let raf;
    const draw = () => {
      const { W, H } = prepareWaveCanvas(canvas, ctx);
      const halfH = H / 2;
      const rootStyle = getComputedStyle(document.documentElement);
      const accentColor = rootStyle.getPropertyValue('--accent').trim() || '#4a90d9';
      const darkMode = document.body?.dataset?.mode === 'dark';
      const waveFillColor = darkMode ? accentColor : (rootStyle.getPropertyValue('--ink-1').trim() || '#151821');
      const idleColor = darkMode ? 'rgba(255,255,255,0.14)' : 'rgba(0,0,0,0.09)';
      const waveformSamples = waveformRef.current;

      if (!active || waveformSamples.length < 2 || waveformIsSilent(waveformSamples)) {
        magnitudeRef.current = [];
        ctx.strokeStyle = idleColor;
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, halfH); ctx.lineTo(W, halfH); ctx.stroke();
        raf = requestAnimationFrame(draw);
        return;
      }

      const amountNorm = Math.max(0, Math.min(100, Number(amount) || 0)) / 100;
      const target = makeLinearMagnitudeTarget(waveformSamples, W, halfH);
      const pts = pointsFromMagnitudes(smoothMagnitudes(magnitudeRef, target), W, halfH);
      drawFilledWave(ctx, pts, waveFillColor, (darkMode ? 0.40 : 0.58) + amountNorm * 0.08);

      raf = requestAnimationFrame(draw);
    };
    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
  }, [active, amount]);
  return <canvas ref={canvasRef} width={WAVE_CANVAS_WIDTH} height={WAVE_CANVAS_HEIGHT} style={WAVE_CANVAS_STYLE} />;
}

// ─── In Your Face (presence/saturation) ──────────────────────────────────────
function PctModule({ name, value, onChange, on, setOn, signalActive, compact = false, waveform }) {
  if (compact) {
    return (
      <div className={`comp-unit comp-unit-mini${!on ? ' bypassed' : ''}`}>
        <div className="comp-unit-header">
          <BypassBtn on={on} onChange={setOn} />
          <span className="mod-name">{name}</span>
          <DriveBars value={value} active={signalActive && on} />
        </div>
        <div className="comp-unit-body">
          <RackThresholdSlider
            label="AMT"
            value={value}
            min={0}
            max={100}
            onChange={onChange}
            format={(v) => `${Math.round(v)}%`}
          />
        </div>
      </div>
    );
  }

  return (
    <div className={`comp-unit${!on ? ' bypassed' : ''}`}>
      <div className="comp-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <span className="mod-name">{name}</span>
        <div className="comp-unit-meta">
          <span className="comp-meta-val">{Math.round(value)}%</span>
        </div>
      </div>
      <div className="comp-unit-body">
        <div className="comp-wave-wrap">
          <IYFViz active={signalActive && on} amount={value} waveform={waveform} />
        </div>
        <div className="comp-fader-row">
          <span className="comp-fader-lbl">AMT</span>
          <div className="comp-fader-track"
            onPointerDown={(e) => {
              e.preventDefault();
              const target = e.currentTarget;
              target.setPointerCapture(e.pointerId);
              const upd = (ev) => {
                const r = target.getBoundingClientRect();
                onChange(Math.max(0, Math.min(100, ((ev.clientX - r.left) / r.width) * 100)));
              };
              upd(e);
              const move = ev => upd(ev);
              const up = () => { window.removeEventListener('pointermove', move); window.removeEventListener('pointerup', up); };
              window.addEventListener('pointermove', move);
              window.addEventListener('pointerup', up);
            }}
          >
            <div className="comp-fader-fill" style={{ width: `${value}%` }} />
            <div className="comp-fader-handle" style={{ left: `${value}%` }} />
          </div>
        </div>
      </div>
    </div>
  );
}

function DriveBars({ value, active }) {
  const bars = 5;
  const lit = Math.round((Math.max(0, Math.min(100, value)) / 100) * bars);

  return (
    <div className={`drive-bars${active ? ' active' : ''}`} aria-hidden>
      {Array.from({ length: bars }).map((_, i) => (
        <span
          key={i}
          className={`drive-bar${i < lit ? ' lit' : ''}${i >= 3 && i < lit ? ' hot' : ''}`}
          style={{ height: `${28 + i * 16}%` }}
        />
      ))}
    </div>
  );
}

function GateLED({ threshold, active }) {
  const [open, setOpen] = React.useState(false);

  React.useEffect(() => {
    if (!active) {
      setOpen(false);
      return;
    }

    let raf;
    const tick = () => {
      const now = performance.now() * 0.001;
      const env = 0.55 + 0.4 * Math.sin(now * 1.7) * Math.cos(now * 0.9);
      const thrNorm = (Math.max(-80, Math.min(0, threshold)) + 80) / 80;
      setOpen(env > thrNorm * 0.92);
      raf = requestAnimationFrame(tick);
    };

    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [threshold, active]);

  return (
    <div className="gate-led-wrap" aria-hidden>
      <span className={`gate-led${active ? (open ? ' open' : ' shut') : ''}`} />
      <span className="gate-led-label">{active ? (open ? 'PASS' : 'CUT') : '—'}</span>
    </div>
  );
}

const DESSER_BAND_MIN = 2500;
const DESSER_BAND_MAX = 12000;
const DESSER_MIN_GAP = 400;
const DESSER_BAND_TICKS = [3000, 4000, 6000, 8000, 10000];

function DesserBand({ low, high, amount = 0, spectrum = [], onLowChange, onHighChange, active }) {
  const trackRef = React.useRef(null);
  const min = DESSER_BAND_MIN;
  const max = DESSER_BAND_MAX;
  const lowNorm = (low - min) / (max - min);
  const highNorm = (high - min) / (max - min);
  const amountNorm = Math.max(0, Math.min(1, Number(amount) / 100 || 0));
  const activityLines = React.useMemo(() => {
    if (!Array.isArray(spectrum) || spectrum.length < 4 || !active || amountNorm <= 0.01) return [];

    const lines = [];
    const minLog = Math.log10(20);
    const maxLog = Math.log10(20000);
    const lowEdge = Math.max(DESSER_BAND_MIN, Number(low) || DESSER_BAND_MIN);
    const highEdge = Math.min(DESSER_BAND_MAX, Number(high) || DESSER_BAND_MAX);
    spectrum.forEach((value, index) => {
      const t = spectrum.length <= 1 ? 0 : index / (spectrum.length - 1);
      const freq = 10 ** (minLog + t * (maxLog - minLog));
      if (freq < lowEdge || freq > highEdge) return;

      const numeric = Math.max(0, Math.min(1, Number(value) || 0));
      const intensity = Math.max(0, Math.min(1, (numeric - 0.028) * 4.8)) * (0.22 + amountNorm * 0.78);
      if (intensity <= 0.03) return;

      lines.push({
        freq,
        x: ((freq - DESSER_BAND_MIN) / (DESSER_BAND_MAX - DESSER_BAND_MIN)) * 100,
        height: 24 + intensity * 56,
        alpha: 0.14 + intensity * 0.72,
        delay: (index % 7) * 34,
      });
    });

    return lines
      .sort((a, b) => b.alpha - a.alpha)
      .slice(0, 16)
      .sort((a, b) => a.freq - b.freq);
  }, [active, amountNorm, high, low, spectrum]);

  const dragThumb = (which) => (e) => {
    e.preventDefault();
    e.stopPropagation();
    const track = trackRef.current;
    if (!track) return;

    const update = (ev) => {
      const r = track.getBoundingClientRect();
      const x = Math.max(0, Math.min(1, (ev.clientX - r.left) / r.width));
      const value = Math.round(min + x * (max - min));
      if (which === 'low') {
        onLowChange(Math.max(min, Math.min(value, high - DESSER_MIN_GAP)));
      } else {
        onHighChange(Math.min(max, Math.max(value, low + DESSER_MIN_GAP)));
      }
    };

    update(e);
    const move = ev => update(ev);
    const up = () => {
      window.removeEventListener('pointermove', move);
      window.removeEventListener('pointerup', up);
    };
    window.addEventListener('pointermove', move);
    window.addEventListener('pointerup', up);
  };

  return (
    <div className={`desser-band${active ? ' active' : ''}`}>
      <div className="desser-band-track" ref={trackRef}>
        {DESSER_BAND_TICKS.map((frequency) => {
          const pos = ((frequency - min) / (max - min)) * 100;
          return <span key={frequency} className="desser-band-tick" style={{ left: `${pos}%` }} />;
        })}
        <div className="desser-activity" aria-hidden="true">
          {activityLines.map((line) => (
            <span
              key={`${Math.round(line.freq)}-${Math.round(line.alpha * 100)}`}
              className="desser-activity-line"
              style={{
                left: `${line.x}%`,
                height: `${line.height}%`,
                opacity: line.alpha,
                animationDelay: `${line.delay}ms`,
              }}
            />
          ))}
        </div>
        <div
          className="desser-band-region"
          style={{ left: `${lowNorm * 100}%`, width: `${Math.max(0, highNorm - lowNorm) * 100}%` }}
        />
        <button
          type="button"
          className="desser-thumb low"
          style={{ left: `${lowNorm * 100}%` }}
          onPointerDown={dragThumb('low')}
          aria-label={`Low edge ${(low / 1000).toFixed(1)} kHz`}
          title={`${(low / 1000).toFixed(1)} kHz`}
        />
        <button
          type="button"
          className="desser-thumb high"
          style={{ left: `${highNorm * 100}%` }}
          onPointerDown={dragThumb('high')}
          aria-label={`High edge ${(high / 1000).toFixed(1)} kHz`}
          title={`${(high / 1000).toFixed(1)} kHz`}
        />
      </div>
      <div className="desser-band-labels">
        <span>3k</span>
        <span>5k</span>
        <span>8k</span>
        <span>12k</span>
      </div>
    </div>
  );
}

// ─── Gate ─────────────────────────────────────────────────────────────────────
function GateModule({ threshold, setThreshold, on, setOn, signalActive, compact = false, waveform }) {
  if (compact) {
    return (
      <div className={`comp-unit comp-unit-mini${!on ? ' bypassed' : ''}`}>
        <div className="comp-unit-header">
          <BypassBtn on={on} onChange={setOn} />
          <span className="mod-name">GATE KEEPER</span>
          <GateLED threshold={threshold} active={signalActive && on} />
        </div>
        <div className="comp-unit-body">
          <RackThresholdSlider value={threshold} min={-80} onChange={setThreshold} />
        </div>
      </div>
    );
  }

  return (
    <div className={`comp-unit${!on ? ' bypassed' : ''}`}>
      <div className="comp-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <span className="mod-name">GATE KEEPER (SHUT UP!)</span>
      </div>
      <div className="comp-unit-body">
        <div className="comp-wave-wrap">
          <GateWaveViz threshold={threshold} active={signalActive && on} waveform={waveform} />
        </div>
        <RackThresholdSlider value={threshold} min={-80} onChange={setThreshold} />
      </div>
    </div>
  );
}

function DeEsserModule({ low = 5500, high = 8500, reduction, setLow, setHigh, setReduction, on, setOn, signalActive, spectrum }) {
  return (
    <div className={`comp-unit comp-unit-mini desser-unit${!on ? ' bypassed' : ''}`}>
      <div className="comp-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <span className="mod-name">DE-ESSER</span>
        <span className="desser-range-readout">
          {(low / 1000).toFixed(1)}–{(high / 1000).toFixed(1)} kHz
        </span>
      </div>
      <div className="comp-unit-body">
        <DesserBand
          low={low}
          high={high}
          amount={reduction}
          spectrum={spectrum}
          onLowChange={setLow}
          onHighChange={setHigh}
          active={signalActive && on}
        />
        <RackThresholdSlider
          label="RED"
          value={reduction}
          min={0}
          max={100}
          onChange={setReduction}
          format={(v) => `−${Math.round(v)}%`}
        />
      </div>
    </div>
  );
}

// Gate viz: signal that drops to silence when below threshold (cuts off)
function GateWaveViz({ threshold, active, waveform }) {
  const canvasRef = React.useRef(null);
  const waveformRef = React.useRef(normalizeWaveform(waveform));
  const magnitudeRef = React.useRef([]);

  React.useEffect(() => {
    waveformRef.current = normalizeWaveform(waveform);
  }, [waveform]);

  React.useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let raf;
    const draw = () => {
      const { W, H } = prepareWaveCanvas(canvas, ctx);
      const halfH = H / 2;
      const thrY = thresholdToY(threshold, halfH);
      const thresholdLimit = halfH - thrY;
      const rootStyle = getComputedStyle(document.documentElement);
      const accentColor = rootStyle.getPropertyValue('--accent').trim() || '#4a90d9';
      const warmColor = rootStyle.getPropertyValue('--warm').trim() || '#e07840';
      const darkMode = document.body?.dataset?.mode === 'dark';
      const waveFillColor = darkMode ? accentColor : (rootStyle.getPropertyValue('--ink-1').trim() || '#151821');
      const idleColor = darkMode ? 'rgba(255,255,255,0.14)' : 'rgba(0,0,0,0.09)';

      ctx.save();
      ctx.fillStyle = warmColor;
      ctx.globalAlpha = darkMode ? 0.12 : 0.09;
      ctx.fillRect(0, thrY, W, Math.max(0, H - thrY * 2));
      ctx.restore();

      const waveformSamples = waveformRef.current;

      if (!active || waveformSamples.length < 2 || waveformIsSilent(waveformSamples)) {
        magnitudeRef.current = [];
        ctx.strokeStyle = idleColor;
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, halfH); ctx.lineTo(W, halfH); ctx.stroke();
        drawThresholdLines(ctx, W, H, thrY, warmColor);
        raf = requestAnimationFrame(draw);
        return;
      }

      const target = makeLinearMagnitudeTarget(waveformSamples, W, halfH);
      const pts = pointsFromMagnitudes(smoothMagnitudes(magnitudeRef, target), W, halfH);
      const limit = thresholdLimit;

      let i = 0;
      while (i < pts.length) {
        const p = pts[i];
        const passed = p.mag >= limit;
        let j = i + 1;
        while (j < pts.length && (pts[j].mag >= limit) === passed) j++;
        if (passed) {
          ctx.save();
          ctx.fillStyle = waveFillColor;
          ctx.globalAlpha = darkMode ? 0.44 : 0.64;
          ctx.beginPath();
          ctx.moveTo(pts[i].x, pts[i].top);
          for (let k = i + 1; k < j; k++) ctx.lineTo(pts[k].x, pts[k].top);
          for (let k = j - 1; k >= i; k--) ctx.lineTo(pts[k].x, pts[k].bottom);
          ctx.closePath();
          ctx.fill();
          ctx.restore();
        } else {
          // closed: flat line at center
          ctx.beginPath();
          ctx.moveTo(pts[i].x, halfH);
          ctx.lineTo(pts[j - 1].x, halfH);
          ctx.strokeStyle = idleColor;
          ctx.lineWidth = 2.5;
          ctx.lineJoin = 'round';
          ctx.lineCap = 'round';
          ctx.stroke();
        }
        i = j;
      }

      drawThresholdLines(ctx, W, H, thrY, warmColor);

      raf = requestAnimationFrame(draw);
    };
    raf = requestAnimationFrame(draw);
    return () => cancelAnimationFrame(raf);
  }, [active, threshold]);
  return <canvas ref={canvasRef} width={WAVE_CANVAS_WIDTH} height={WAVE_CANVAS_HEIGHT} style={WAVE_CANVAS_STYLE} />;
}

// ─── Stereo ───────────────────────────────────────────────────────────────────
function StereoModule({ width, setWidth, lowBypass, setLowBypass, on, setOn }) {
  return (
    <div className={`stereo-unit${!on ? ' bypassed' : ''}`}>
      <ModuleHeader on={on} setOn={setOn} name="STEREOIDS" />
      <div className="stereo-body">
        <div className="stereo-knob-col">
          <Knob value={width} onChange={setWidth} min={0} max={100} size={48}
                defaultValue={100} color="var(--accent)" format={v => `${Math.round(v)}%`} disabled={!on} />
          <div className="stereo-knob-label">WIDTH</div>
        </div>
        <div className="stereo-knob-col">
          <Knob value={lowBypass} onChange={setLowBypass} min={0} max={500} size={42}
                defaultValue={0} color="var(--neutral-knob)"
                format={v => v < 1000 ? `${Math.round(v)}Hz` : `${(v/1000).toFixed(1)}k`}
                disabled={!on} />
          <div className="stereo-knob-label">LOW BYPASS</div>
        </div>
      </div>
    </div>
  );
}

function ExtAuxSelect({ value = 'TRACK', onChange, disabled }) {
  const [open, setOpen] = React.useState(false);
  const ref = React.useRef(null);

  React.useEffect(() => {
    const onDoc = (e) => {
      if (ref.current && !ref.current.contains(e.target)) setOpen(false);
    };
    document.addEventListener('mousedown', onDoc);
    return () => document.removeEventListener('mousedown', onDoc);
  }, []);

  return (
    <div className={`fx-aux${open ? ' open' : ''}${disabled ? ' disabled' : ''}`} ref={ref}>
      <button
        className="fx-aux-btn"
        type="button"
        disabled={disabled}
        onClick={() => setOpen(o => !o)}
        aria-haspopup="listbox"
        aria-expanded={open}
      >
        <span className="fx-aux-label">SEND TO</span>
        <span className="fx-aux-value">{value}</span>
        <svg width="8" height="8" viewBox="0 0 10 10" aria-hidden="true">
          <path d="M2 4l3 3 3-3" stroke="currentColor" strokeWidth="1.3" fill="none" strokeLinecap="round" strokeLinejoin="round" />
        </svg>
      </button>
      {open && (
        <div className="fx-aux-menu" role="listbox">
          {auxOutputOptions.map(option => (
            <button
              key={option}
              type="button"
              className={`fx-aux-item${option === value ? ' active' : ''}`}
              onClick={() => { onChange(option); setOpen(false); }}
              role="option"
              aria-selected={option === value}
            >
              {option}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}

// ─── Delay ────────────────────────────────────────────────────────────────────
function DelayModule({ state, set, on, setOn }) {
  const modes = ['NOTE', 'DOT', 'TRIP'];
  const types = ['NORMAL', 'WIDE', 'PING-PONG'];
  const bpmOn = state.bpm !== false;
  const postReverb = state.postReverb === true;
  const timeValue = bpmOn ? state.timeIdx : state.timeMs;
  const timeMax = bpmOn ? delayDivisions.length - 1 : 2000;
  const timeFormat = bpmOn
    ? (v) => delayDivisions[Math.round(v)] || '1/4'
    : (v) => `${Math.round(v)}ms`;

  return (
    <div className={`module fx-module${!on ? ' bypassed' : ''}`}>
      <div className="fx-head">
        <ModuleHeader on={on} setOn={setOn} name="DELAY" />
        <Select value={state.preset} onChange={v => set({ preset: v })} options={delayStyles} floating />
      </div>
      <ExtAuxSelect value={state.aux} onChange={v => set({ aux: v })} disabled={!on} />
      <div className="fx-knobs">
        <Knob value={state.mix} onChange={v => set({ mix: v })} min={0} max={100} size={40}
              color="var(--neutral-knob)" disabled={!on} format={v => `${Math.round(v)}%`} label="MIX" defaultValue={20} />
        <Knob value={timeValue} onChange={v => bpmOn ? set({ timeIdx: v }) : set({ timeMs: v })} min={bpmOn ? 0 : 1} max={timeMax} size={48}
              color="var(--accent)" disabled={!on} format={timeFormat} label="TIME" defaultValue={bpmOn ? 2 : 500} />
        <Knob value={state.feedback} onChange={v => set({ feedback: v })} min={0} max={100} size={40}
              color="var(--accent-soft)" disabled={!on} format={v => `${Math.round(v)}%`} label="FEEDBACK" defaultValue={25} />
      </div>
      <div className="fx-tab-align-spacer" aria-hidden="true" />
      <div className="fx-pills-row">
        <PillGroup value={state.mode} onChange={v => set({ mode: v })} options={modes} stretch />
      </div>
      <div className="fx-pills-row">
        <PillGroup value={state.type} onChange={v => set({ type: v })} options={types} stretch />
      </div>
      <div className="fx-ping fx-ping-dual">
        <button
          className={`ping-btn${bpmOn ? ' active' : ''}`}
          onClick={() => set({ bpm: !bpmOn })}
          disabled={!on}
        >BPM</button>
        <button
          className={`ping-btn${postReverb ? ' active' : ''}`}
          onClick={() => set({ postReverb: !postReverb })}
          disabled={!on}
        >POST RVB</button>
      </div>
      <div className="fx-cuts">
        <Knob value={state.lowCut} onChange={v => set({ lowCut: v })} min={20} max={500} size={34}
              color="var(--accent)" disabled={!on} format={v => `${Math.round(v)}Hz`} label="LOW CUT" />
        <Knob value={state.highCut} onChange={v => set({ highCut: v })} min={2000} max={20000} size={34}
              color="var(--accent)" disabled={!on} format={v => v >= 1000 ? `${(v/1000).toFixed(1)}k` : `${Math.round(v)}`} label="HIGH CUT" />
      </div>
    </div>
  );
}

// ─── Reverb ───────────────────────────────────────────────────────────────────
function ReverbModule({ state, set, on, setOn }) {
  const modes = ['NOTE', 'DOT', 'TRIP'];
  const bpmOn = state.bpm !== false;
  const decaySync = state.decaySync !== false;
  const preDelaySync = state.preDelaySync !== false;
  const decaySynced = bpmOn && decaySync;
  const preDelaySynced = bpmOn && preDelaySync;
  const decayValue = decaySynced ? state.decayIdx : state.decay;
  const preDelayValue = preDelaySynced ? state.preDelayIdx : state.preDelay;

  return (
    <div className={`module fx-module${!on ? ' bypassed' : ''}`}>
      <div className="fx-head">
        <ModuleHeader on={on} setOn={setOn} name="REVERB" />
        <Select value={state.preset} onChange={v => set({ preset: v })} options={reverbModes} floating />
      </div>
      <ExtAuxSelect value={state.aux} onChange={v => set({ aux: v })} disabled={!on} />
      <div className="fx-knobs">
        <Knob value={state.mix} onChange={v => set({ mix: v })} min={0} max={100} size={40}
              color="var(--neutral-knob)" disabled={!on} format={v => `${Math.round(v)}%`} label="MIX" defaultValue={18} />
        <Knob value={decayValue} onChange={v => decaySynced ? set({ decayIdx: v }) : set({ decay: v })} min={0} max={decaySynced ? delayDivisions.length - 1 : 100} size={48}
              color="var(--accent)" disabled={!on} format={v => decaySynced ? (delayDivisions[Math.round(v)] || '1/4') : formatReverbDecay(v)} label="DECAY" defaultValue={decaySynced ? 2 : 72} />
        <Knob value={preDelayValue} onChange={v => preDelaySynced ? set({ preDelayIdx: v }) : set({ preDelay: v })} min={0} max={preDelaySynced ? reverbPredelayDivisions.length - 1 : 100} size={40}
              color="var(--accent-soft)" disabled={!on} format={v => preDelaySynced ? (reverbPredelayDivisions[Math.round(v)] || 'None') : formatPredelayMs(v)} label="PRE DLY" defaultValue={0} />
      </div>
      <div className="rev-size-row">
        <span className="rev-size-lbl">SIZE</span>
        <div className="rev-slider">
          <div className="rev-slider-track"
            onPointerDown={(e) => {
              e.preventDefault();
              const target = e.currentTarget;
              target.setPointerCapture(e.pointerId);
              const upd = (ev) => {
                const r = target.getBoundingClientRect();
                set({ size: Math.max(0, Math.min(100, ((ev.clientX - r.left) / r.width) * 100)) });
              };
              upd(e);
              const move = ev => upd(ev);
              const up = () => { window.removeEventListener('pointermove', move); window.removeEventListener('pointerup', up); };
              window.addEventListener('pointermove', move);
              window.addEventListener('pointerup', up);
            }}
          >
            <div className="rev-slider-fill" style={{ width: `${state.size}%` }} />
            <div className="rev-slider-handle" style={{ left: `${state.size}%` }} />
          </div>
          <span className="rev-size-val">{Math.round(state.size)}%</span>
        </div>
      </div>
      <div className="fx-pills-row">
        <PillGroup value={state.mode} onChange={v => set({ mode: v })} options={modes} stretch />
      </div>
      <div className="fx-ping fx-ping-triple">
        <button
          className={`ping-btn${bpmOn ? ' active' : ''}`}
          onClick={() => set({ bpm: !bpmOn, preDelaySync: !bpmOn ? true : preDelaySync })}
          disabled={!on}
        >BPM</button>
        <button
          className={`ping-btn${decaySync ? ' active' : ''}`}
          onClick={() => set({ decaySync: !decaySync, bpm: !decaySync ? true : bpmOn })}
          disabled={!on}
        >DECAY</button>
        <button
          className={`ping-btn${preDelaySync ? ' active' : ''}`}
          onClick={() => set({ preDelaySync: !preDelaySync, bpm: !preDelaySync ? true : bpmOn })}
          disabled={!on}
        >PRE</button>
      </div>
      <div className="fx-cuts">
        <Knob value={state.lowCut} onChange={v => set({ lowCut: v })} min={20} max={500} size={34}
              color="var(--accent)" disabled={!on} format={v => `${Math.round(v)}Hz`} label="LOW CUT" />
        <Knob value={state.highCut} onChange={v => set({ highCut: v })} min={2000} max={20000} size={34}
              color="var(--accent)" disabled={!on} format={v => v >= 1000 ? `${(v/1000).toFixed(1)}k` : `${Math.round(v)}`} label="HIGH CUT" />
      </div>
    </div>
  );
}

export { CompModule, ButterCompModule, PctModule, GateModule, DeEsserModule, StereoModule, DelayModule, ReverbModule, ExtAuxSelect };
