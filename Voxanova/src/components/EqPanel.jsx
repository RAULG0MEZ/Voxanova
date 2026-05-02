// Ecualizador. El componente más grande de la app:
// - Dibuja el canvas con la curva total y por banda.
// - Maneja el click para crear bandas, drag para mover, click derecho para menú.
// - Usa funciones puras de utils/eq.js para todos los cálculos.

import { useCallback, useEffect, useRef, useState } from "react";
import { cls, clamp, formatFreq } from "../utils/format.js";
import {
  getBandColor,
  getEqGraphBounds,
  xFromFreq,
  freqFromX,
  yFromDb,
  dbFromY,
  bandGainAt,
  bandShapeGainAt,
  getNeededEqRange,
  getDisplayEqRange,
  getEqGridStep,
  FILTER_TYPES
} from "../utils/eq.js";
import { BandPanel } from "./BandPanel.jsx";
import { EqInlineMeter } from "./Meters.jsx";

function SatIcon({ mode }) {
  if (mode === 0) {
    return (
      <svg
        viewBox="0 0 12 12"
        width="11"
        height="11"
        fill="none"
        stroke="currentColor"
        strokeWidth="1.4"
      >
        <circle cx="6" cy="6" r="4" />
      </svg>
    );
  }

  const configs = [
    [{ x: 6, f: 1 }],
    [
      { x: 4, f: 1 },
      { x: 8, f: -1 }
    ],
    [
      { x: 3, f: 1 },
      { x: 6.5, f: -1 },
      { x: 10, f: 1 }
    ]
  ][mode - 1];

  return (
    <svg
      viewBox="0 0 12 12"
      width="11"
      height="11"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.5"
      strokeLinecap="round"
    >
      {configs.map(({ x, f }, i) => (
        <path
          key={i}
          d={`M ${x} 11 C ${x - f * 2} 9.5, ${x + f * 2} 8, ${x} 6.5 C ${x - f * 2} 5, ${x + f * 2} 3.5, ${x} 2`}
        />
      ))}
    </svg>
  );
}

const getCreatedFilterType = (freq) =>
  freq < 50
    ? "Low Cut"
    : freq <= 100
      ? "Low Shelf"
      : freq >= 15000
        ? "High Cut"
        : freq > 5000
          ? "High Shelf"
          : "Bell";

const EQ_SLOPE_OPTIONS = [6, 12, 18, 24, 36, 48, 72, 96];
const EQ_PLACEMENT_OPTIONS = [
  ["stereo", "Stereo"],
  ["left", "Left"],
  ["right", "Right"],
  ["mid", "Mid"],
  ["side", "Side"]
];

function getTrackedTextWidth(ctx, text, tracking) {
  const chars = [...text];
  return chars.reduce((width, char) => width + ctx.measureText(char).width, 0) + Math.max(chars.length - 1, 0) * tracking;
}

function drawTrackedText(ctx, text, x, y, tracking = 0) {
  if (tracking <= 0) {
    ctx.fillText(text, x, y);
    return;
  }

  const chars = [...text];
  const align = ctx.textAlign;
  const width = getTrackedTextWidth(ctx, text, tracking);
  let cursor = x;

  if (align === "center") cursor -= width / 2;
  if (align === "right" || align === "end") cursor -= width;

  ctx.save();
  ctx.textAlign = "left";
  chars.forEach((char) => {
    ctx.fillText(char, cursor, y);
    cursor += ctx.measureText(char).width + tracking;
  });
  ctx.restore();
}

const SATURATION_LABELS = ["Off", "1073", "Tape", "Tube"];

export function EqPanel({ expanded = false, onExpand, inputMeter, outputMeter, saturation, onSaturationChange }) {
  const eqPanelRef = useRef(null);
  const canvasRef = useRef(null);
  const [activeEq, setActiveEq] = useState("pre");
  const activeSaturation = saturation?.[activeEq] ?? { mode: 0, amount: 0 };
  const satMode = activeSaturation.mode ?? 0;
  const satAmount = activeSaturation.amount ?? 0;
  const setSatMode = (updater) => {
    const nextMode = typeof updater === "function" ? updater(satMode) : updater;
    onSaturationChange?.(activeEq, { ...activeSaturation, mode: nextMode });
  };
  const setSatAmount = (val) => onSaturationChange?.(activeEq, { ...activeSaturation, amount: val });
  const [eqBands, setEqBands] = useState({ pre: [], post: [] });
  const [selectedBand, setSelectedBand] = useState(null);
  const [dbRange, setDbRange] = useState(12);
  const [rangeBase, setRangeBase] = useState(12);
  const [rangeMenuOpen, setRangeMenuOpen] = useState(false);
  const [hoverPos, setHoverPos] = useState(null);
  const [hoveredBandIndex, setHoveredBandIndex] = useState(null);
  const [contextMenu, setContextMenu] = useState(null);
  const [panelHover, setPanelHover] = useState(false);
  const [panelVisible, setPanelVisible] = useState(false);
  const [panelRenderBand, setPanelRenderBand] = useState(null);
  const [isBandDragging, setIsBandDragging] = useState(false);
  const panelHideTimerRef = useRef(null);
  const dragRef = useRef(null);
  const dragOffsetRef = useRef(0);
  const dbRangeRef = useRef(dbRange);
  const rangeBaseRef = useRef(rangeBase);
  const rangeDisplayRef = useRef(dbRange);
  const barsRef = useRef(Array.from({ length: 120 }, () => Math.random() * 0.2));
  const bands = eqBands[activeEq];
  const bandsRef = useRef(bands);
  const selectedBandRef = useRef(selectedBand);
  const hoverRef = useRef(null);
  const hoverIntensityRef = useRef({});
  const setBands = useCallback(
    (updater) => {
      setEqBands((current) => {
        const currentBands = current[activeEq];
        const nextBands = typeof updater === "function" ? updater(currentBands) : updater;
        return { ...current, [activeEq]: nextBands };
      });
    },
    [activeEq]
  );

  useEffect(() => {
    bandsRef.current = bands;
    selectedBandRef.current = selectedBand;
  }, [bands, selectedBand, activeEq]);

  useEffect(() => {
    dbRangeRef.current = dbRange;
  }, [dbRange]);

  useEffect(() => {
    rangeBaseRef.current = rangeBase;
  }, [rangeBase]);

  useEffect(() => {
    setSelectedBand(null);
    setContextMenu(null);
    setRangeMenuOpen(false);
    hoverIntensityRef.current = {};
  }, [activeEq]);

  useEffect(() => {
    setDbRange(rangeBase);
  }, [rangeBase]);

  useEffect(() => {
    if (panelHideTimerRef.current) {
      window.clearTimeout(panelHideTimerRef.current);
      panelHideTimerRef.current = null;
    }
    if (selectedBand !== null) {
      setPanelRenderBand(selectedBand);
      setPanelVisible(true);
      return undefined;
    }
    if (hoveredBandIndex !== null) {
      setPanelRenderBand(hoveredBandIndex);
      setPanelVisible(true);
      return undefined;
    }
    setPanelVisible(false);
    panelHideTimerRef.current = window.setTimeout(() => {
      setPanelRenderBand(null);
    }, 260);
    return () => {
      if (panelHideTimerRef.current) {
        window.clearTimeout(panelHideTimerRef.current);
        panelHideTimerRef.current = null;
      }
    };
  }, [selectedBand, hoveredBandIndex]);

  useEffect(() => {
    if (selectedBand === null || panelHover || isBandDragging || contextMenu) return undefined;
    const timeout = window.setTimeout(() => {
      setSelectedBand(null);
    }, 1800);
    return () => window.clearTimeout(timeout);
  }, [contextMenu, isBandDragging, panelHover, selectedBand]);

  useEffect(() => {
    const onPointerDown = (event) => {
      if (!eqPanelRef.current || eqPanelRef.current.contains(event.target)) return;
      setSelectedBand(null);
      setContextMenu(null);
      setRangeMenuOpen(false);
    };

    window.addEventListener("pointerdown", onPointerDown);
    return () => window.removeEventListener("pointerdown", onPointerDown);
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return undefined;

    let animation;
    const drawPath = (ctx, start, end, xToFreq, gainToY, calcGain, step = 2) => {
      for (let px = start; px <= end; px += step) {
        const y = gainToY(calcGain(xToFreq(px)));
        if (px === start) ctx.moveTo(px, y);
        else ctx.lineTo(px, y);
      }
    };

    const draw = () => {
      const rect = canvas.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      const width = Math.max(320, Math.floor(rect.width));
      const height = Math.max(220, Math.floor(rect.height));

      if (canvas.width !== width * dpr || canvas.height !== height * dpr) {
        canvas.width = width * dpr;
        canvas.height = height * dpr;
      }

      const ctx = canvas.getContext("2d");
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, width, height);

      const activeBands = bandsRef.current;
      const selected = selectedBandRef.current;
      rangeDisplayRef.current += (dbRangeRef.current - rangeDisplayRef.current) * 0.085;
      if (Math.abs(dbRangeRef.current - rangeDisplayRef.current) < 0.02) {
        rangeDisplayRef.current = dbRangeRef.current;
      }

      const maxDb = rangeDisplayRef.current;
      const labelRange = getDisplayEqRange(maxDb);
      const { left, top, graphW, graphH } = getEqGraphBounds(width, height);
      const freqToX = (freq) => xFromFreq(freq, width);
      const xToFreq = (x) => freqFromX(x, width);
      const gainToY = (gain) => top + (graphH / 2) * (1 - gain / maxDb);
      const totalEqGain = (freq) =>
        activeBands.reduce((sum, band) => sum + bandGainAt(band, freq), 0);
      const zeroLineInset = 26;

      ctx.fillStyle = "rgba(242, 247, 253, 0.82)";
      ctx.fillRect(0, 0, width, height);

      const zeroY = gainToY(0);
      const bg = ctx.createLinearGradient(0, 0, 0, height);
      bg.addColorStop(0, "rgba(255,255,255,0.78)");
      bg.addColorStop(clamp(zeroY / height, 0, 1), "rgba(232,241,252,0.5)");
      bg.addColorStop(1, "rgba(213,227,245,0.38)");
      ctx.fillStyle = bg;
      ctx.fillRect(0, 0, width, height);

      ctx.save();
      ctx.beginPath();
      ctx.rect(left, top, graphW, graphH);
      ctx.clip();

      const freqs = [
        20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000,
        3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000
      ];
      freqs.forEach((freq) => {
        const x = freqToX(freq);
        const decadeStart = freq < 100 ? 20 : freq < 1000 ? 100 : freq < 10000 ? 1000 : 10000;
        const decadeEnd = freq < 100 ? 100 : freq < 1000 ? 1000 : freq < 10000 ? 10000 : 20000;
        const pos = Math.log10(freq / decadeStart) / Math.log10(decadeEnd / decadeStart);
        const hover = hoverRef.current
          ? Math.max(0, 1 - Math.abs(x - hoverRef.current.x) / (width * 0.14)) * 0.2
          : 0;
        ctx.strokeStyle = `rgba(68,86,108,${0.04 + pos * 0.06 + hover * 0.45})`;
        ctx.lineWidth = freq === 100 || freq === 1000 || freq === 10000 ? 0.65 : 0.34;
        ctx.beginPath();
        ctx.moveTo(x, top);
        ctx.lineTo(x, top + graphH);
        ctx.stroke();
      });

      const dbStep = getEqGridStep(labelRange);
      for (let db = -labelRange; db <= labelRange; db += dbStep) {
        const y = gainToY(db);
        const isZero = db === 0;
        ctx.strokeStyle = isZero ? "rgba(31,48,70,0.34)" : "rgba(68,86,108,0.065)";
        ctx.lineWidth = isZero ? 1 : 0.42;
        ctx.beginPath();
        const lineInset = isZero ? zeroLineInset : 0;
        ctx.moveTo(left + lineInset, y);
        ctx.lineTo(left + graphW - lineInset, y);
        ctx.stroke();
      }

      const bars = barsRef.current;
      for (let i = 0; i < bars.length; i += 1) {
        const freq = 20 * 1000 ** (i / bars.length);
        let target = 0.08 + Math.sin(Date.now() / 1050 + i * 0.37) * 0.05 + Math.random() * 0.028;
        if (freq > 80 && freq < 9000) {
          target += 0.34 * Math.exp(-(Math.log10(freq / 620) ** 2) * 1.35);
        }
        bars[i] += (target - bars[i]) * 0.08;
      }
      ctx.beginPath();
      ctx.moveTo(left, top + graphH);
      bars.forEach((value, index) => {
        const x = left + (index / (bars.length - 1)) * graphW;
        ctx.lineTo(x, top + graphH - value * graphH * 0.48);
      });
      ctx.lineTo(left + graphW, top + graphH);
      ctx.closePath();
      const fill = ctx.createLinearGradient(0, top, 0, top + graphH);
      fill.addColorStop(0, "rgba(80,103,130,0.18)");
      fill.addColorStop(0.52, "rgba(80,103,130,0.08)");
      fill.addColorStop(1, "rgba(80,103,130,0.01)");
      ctx.fillStyle = fill;
      ctx.fill();
      ctx.beginPath();
      bars.forEach((value, index) => {
        const x = left + (index / (bars.length - 1)) * graphW;
        const y = top + graphH - value * graphH * 0.48;
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.strokeStyle = "rgba(62,83,108,0.36)";
      ctx.lineWidth = 1;
      ctx.stroke();

      activeBands.forEach((band, index) => {
        const previous = hoverIntensityRef.current[index] || 0;
        const hover = hoverRef.current;
        let targetHover = 0;
        if (hover) {
          const g = bandShapeGainAt(band, xToFreq(hover.x));
          targetHover = band.on && Math.abs(g) > 0.25 && Math.abs(gainToY(g) - hover.y) < 70 ? 1 : 0;
        }
        const nextHover = previous + (targetHover - previous) * 0.12;
        hoverIntensityRef.current[index] = nextHover;
        const alpha = band.on
          ? Math.round(35 + nextHover * 65)
              .toString(16)
              .padStart(2, "0")
          : "24";
        ctx.beginPath();
        ctx.moveTo(left, zeroY);
        for (let px = left; px <= left + graphW; px += 2) {
          const freq = xToFreq(px);
          ctx.lineTo(px, gainToY(bandShapeGainAt(band, freq)));
        }
        ctx.lineTo(left + graphW, zeroY);
        ctx.closePath();
        const bandFill = ctx.createLinearGradient(0, top, 0, top + graphH);
        if (band.on) {
          bandFill.addColorStop(0, `${band.color}${alpha}`);
          bandFill.addColorStop(0.55, `${band.color}24`);
          bandFill.addColorStop(1, `${band.color}05`);
        } else {
          bandFill.addColorStop(0, `rgba(82, 96, 112, ${nextHover > 0.4 ? 0.18 : 0.12})`);
          bandFill.addColorStop(0.55, "rgba(82, 96, 112, 0.07)");
          bandFill.addColorStop(1, "rgba(82, 96, 112, 0.015)");
        }
        ctx.fillStyle = bandFill;
        ctx.fill();
        ctx.beginPath();
        drawPath(ctx, left, left + graphW, xToFreq, gainToY, (freq) => bandShapeGainAt(band, freq), 2);
        ctx.strokeStyle = band.on
          ? `${band.color}${nextHover > 0.4 ? "b8" : "70"}`
          : "rgba(66, 78, 92, 0.36)";
        ctx.lineWidth = band.on ? 1.25 + nextHover * 0.5 : 1;
        ctx.stroke();
      });

      ctx.beginPath();
      drawPath(ctx, left + zeroLineInset, left + graphW - zeroLineInset, xToFreq, gainToY, totalEqGain, 1);
      ctx.strokeStyle = "rgba(23, 42, 64, 0.68)";
      ctx.lineWidth = 1.8;
      ctx.shadowBlur = 0;
      ctx.stroke();

      const hover = hoverRef.current;
      if (hover && dragRef.current === null) {
        const hoverFreq = xToFreq(hover.x);
        const hoverY = gainToY(totalEqGain(hoverFreq));
        if (Math.abs(hover.y - hoverY) < 18) {
          const color = getBandColor(getCreatedFilterType(hoverFreq), activeBands);
          ctx.beginPath();
          ctx.arc(hover.x, hoverY, 5.5, 0, Math.PI * 2);
          ctx.fillStyle = color;
          ctx.fill();
          ctx.strokeStyle = "rgba(0,0,0,0.55)";
          ctx.lineWidth = 1;
          ctx.stroke();
        }
      }

      activeBands.forEach((band, index) => {
        const pointFreq = band.freq;
        const x =
          freqToX(pointFreq) + (band.type === "Surfer Bell" ? Math.sin(Date.now() / 800) * 12 : 0);
        const y = gainToY(bandShapeGainAt(band, pointFreq));
        const isSelected = index === selected;
        if (isSelected) {
          ctx.beginPath();
          ctx.arc(x, y, 14, 0, Math.PI * 2);
          ctx.fillStyle = band.on ? `${band.color}22` : "rgba(66, 78, 92, 0.12)";
          ctx.fill();
        }
        ctx.beginPath();
        ctx.arc(x, y, isSelected ? 7.5 : 5.5, 0, Math.PI * 2);
        ctx.fillStyle = band.on ? "rgba(250,253,255,0.96)" : "rgba(229, 235, 242, 0.58)";
        ctx.fill();
        ctx.strokeStyle = band.on
          ? isSelected
            ? band.color
            : "rgba(70,88,108,0.72)"
          : "rgba(66, 78, 92, 0.44)";
        ctx.lineWidth = isSelected ? 2 : 1.2;
        ctx.stroke();
      });

      ctx.restore();

      ctx.font = "500 12px Montserrat, 'Roboto Condensed', system-ui, sans-serif";
      ctx.textAlign = "center";
      ctx.fillStyle = "rgba(38,54,72,0.44)";
      [
        ["0", 20],
        ["50", 50],
        ["100", 100],
        ["200", 200],
        ["500", 500],
        ["1k", 1000],
        ["2k", 2000],
        ["5k", 5000],
        ["10k", 10000]
      ].forEach(([label, freq]) => drawTrackedText(ctx, label, freqToX(freq), height - 13, 0.18));

      if (bands.length === 0) {
        ctx.textAlign = "center";
        ctx.font = "600 12px Montserrat, 'Roboto Condensed', system-ui, sans-serif";
        ctx.fillStyle = "rgba(38,54,72,0.28)";
        drawTrackedText(ctx, "CLICK TO ADD EQ BAND", left + graphW / 2, height * 0.42, 1.1);
      }

      animation = window.requestAnimationFrame(draw);
    };

    animation = window.requestAnimationFrame(draw);
    return () => window.cancelAnimationFrame(animation);
  }, [bands]);

  useEffect(() => {
    const onMove = (event) => {
      if (dragRef.current === null || !canvasRef.current) return;
      const rect = canvasRef.current.getBoundingClientRect();
      const x = event.clientX - rect.left;
      const y = event.clientY - rect.top;
      const adjustedY = y - dragOffsetRef.current;
      const draggedIndex = dragRef.current;
      const draggedBand = bandsRef.current[draggedIndex];
      const canExpand = !draggedBand?.type.includes("Cut");
      const rawGainAtTwelve = dbFromY(adjustedY, rect.height, 12);
      const nextRange = canExpand && Math.abs(rawGainAtTwelve) > 12 ? 30 : rangeBaseRef.current;
      const rawGain = canExpand
        ? rawGainAtTwelve
        : dbFromY(adjustedY, rect.height, dbRangeRef.current);
      if (nextRange !== dbRangeRef.current) setDbRange(nextRange);
      // El panel se expande cuando la bolita cruza el límite visible de ±12dB
      if (canExpand && Math.abs(rawGainAtTwelve) > 12) onExpand?.(true);
      setBands((current) =>
        current.map((band, index) =>
          index === draggedIndex
            ? {
                ...band,
                freq: clamp(freqFromX(x, rect.width), 20, 20000),
                gain: band.type.includes("Cut") ? band.gain : clamp(rawGain, -30, 30)
              }
            : band
        )
      );
    };
    const onUp = () => {
      if (dragRef.current === null) return;
      dragRef.current = null;
      setIsBandDragging(false);
      const needed = getNeededEqRange(bandsRef.current);
      setRangeBase(needed);
    };

    window.addEventListener("mousemove", onMove);
    window.addEventListener("mouseup", onUp);
    return () => {
      window.removeEventListener("mousemove", onMove);
      window.removeEventListener("mouseup", onUp);
    };
  }, [onExpand, setBands]);

  const handleMouseDown = (event) => {
    const rect = canvasRef.current.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    const viewRange = rangeDisplayRef.current || dbRangeRef.current;

    const hit = bands.findIndex((band) => {
      const bx = xFromFreq(band.freq, rect.width);
      const by = yFromDb(bandShapeGainAt(band, band.freq), rect.height, viewRange);
      return Math.hypot(x - bx, y - by) < 18;
    });

    if (hit >= 0) {
      setSelectedBand(hit);
      dragRef.current = hit;
      setIsBandDragging(true);
      dragOffsetRef.current = bands[hit].type.includes("Cut")
        ? 0
        : y - yFromDb(bandShapeGainAt(bands[hit], bands[hit].freq), rect.height, viewRange);
      return;
    }

    const rawFreq = freqFromX(x, rect.width);

    // Solo crear banda nueva si el click cae sobre la línea negra total (±18px).
    // Si no, y el cursor está dentro del relleno de color de una banda existente, la selecciona.
    const totalGain = bands.reduce((sum, band) => sum + bandGainAt(band, rawFreq), 0);
    const totalY = yFromDb(totalGain, rect.height, viewRange);
    if (Math.abs(y - totalY) >= 18) {
      const zeroY = yFromDb(0, rect.height, viewRange);
      const nearCurve = bands.findIndex((band) => {
        const gainAtX = bandShapeGainAt(band, rawFreq);
        if (Math.abs(gainAtX) < 0.5) return false;
        const curveY = yFromDb(gainAtX, rect.height, viewRange);
        return y >= Math.min(curveY, zeroY) && y <= Math.max(curveY, zeroY);
      });
      if (nearCurve >= 0) {
        setSelectedBand(nearCurve);
        return;
      }
    }

    const freq = Math.round(clamp(rawFreq, 20, 20000));
    const type = getCreatedFilterType(freq);
    const color = getBandColor(type, bands);
    const existingCutIndex = type.includes("Cut")
      ? bands.findIndex((band) => band.type === type)
      : -1;

    if (existingCutIndex >= 0) {
      setSelectedBand(existingCutIndex);
      dragRef.current = existingCutIndex;
      setIsBandDragging(true);
      dragOffsetRef.current = 0;
      setBands((current) =>
        current.map((band, index) =>
          index === existingCutIndex
            ? {
                ...band,
                freq
              }
            : band
        )
      );
      return;
    }

    const next = {
      freq,
      gain: type.includes("Cut")
        ? 0
        : Number(clamp(dbFromY(y, rect.height, viewRange), -viewRange, viewRange).toFixed(1)),
      q: 1,
      type,
      slope: 12,
      dynamic: false,
      on: true,
      placement: "stereo",
      color
    };
    setBands((current) => [...current, next]);
    setSelectedBand(bands.length);
    dragRef.current = bands.length;
    setIsBandDragging(true);
  };

  const handleContextMenu = (event) => {
    event.preventDefault();
    const rect = canvasRef.current.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    const viewRange = rangeDisplayRef.current || dbRangeRef.current;
    const hit = bands.findIndex(
      (band) =>
        Math.hypot(
          x - xFromFreq(band.freq, rect.width),
          y - yFromDb(bandShapeGainAt(band, band.freq), rect.height, viewRange)
        ) < 18
    );
    if (hit >= 0) {
      setSelectedBand(hit);
      setContextMenu({ x, y, index: hit });
    }
  };

  const updateBandAt = (targetIndex, key, value) => {
    if (targetIndex === null || targetIndex < 0) return;
    if (
      key === "type" &&
      (value === "Low Cut" || value === "High Cut") &&
      bands.some((band, index) => index !== targetIndex && band.type === value)
    ) {
      return;
    }
    setBands((current) =>
      current.map((band, index) =>
        index === targetIndex
          ? {
              ...band,
              [key]: key === "freq" ? clamp(value, 20, 20000) : value
            }
          : band
      )
    );
  };

  const updateBand = (key, value) => {
    updateBandAt(selectedBand, key, value);
  };

  const closeBandPanel = () => {
    if (isBandDragging || document.body.classList.contains("is-knob-dragging")) return;
    setSelectedBand(null);
    setPanelHover(false);
  };

  const deleteBand = () => {
    setBands((current) => current.filter((_, index) => index !== selectedBand));
    setSelectedBand(null);
    setContextMenu(null);
  };

  const contextBand = contextMenu ? bands[contextMenu.index] : null;
  const canvasRect = canvasRef.current?.getBoundingClientRect();
  const canvasWidth = canvasRect?.width || 1000;
  const canvasHeight = canvasRect?.height || 260;
  const getBandPanelLeft = (band, isSelectedPanel = false) => {
    const hasSlope = band.type.includes("Cut");
    const nodeX = xFromFreq(band.freq, canvasWidth);
    const baseLeft = `${clamp(
      (nodeX / canvasWidth) * 100,
      hasSlope ? 25 : 22,
      hasSlope ? 75 : 78
    )}%`;

    if (!isSelectedPanel) return baseLeft;

    const viewRange = rangeDisplayRef.current || dbRangeRef.current;
    const nodeY = yFromDb(bandShapeGainAt(band, band.freq), canvasHeight, viewRange);
    const zeroY = yFromDb(0, canvasHeight, viewRange);
    const panelWidth = Math.min(hasSlope ? 700 : 660, Math.max(canvasWidth - 28, 280));
    const panelHalf = panelWidth / 2;
    const panelHeight = 142;
    const panelBottom = 24;
    const panelTop = canvasHeight - panelBottom - panelHeight;
    const nodeIsMovingDownArea = nodeY > zeroY + 12;
    const nodeIsNearPanel = nodeY >= panelTop - 18;

    if (!nodeIsMovingDownArea || !nodeIsNearPanel) return baseLeft;

    const leftCenter = panelHalf + 14;
    const rightCenter = canvasWidth - panelHalf - 14;
    const targetCenter =
      Math.abs(nodeX - leftCenter) > Math.abs(nodeX - rightCenter) ? leftCenter : rightCenter;

    return `${clamp(targetCenter, panelHalf, canvasWidth - panelHalf)}px`;
  };
  const selectRange = (range) => {
    setRangeBase(range);
    setRangeMenuOpen(false);
  };

  return (
    <section
      ref={eqPanelRef}
      className={cls("eq-panel", expanded && "is-expanded")}
      aria-label="EQ graph"
      onMouseLeave={() => {
        if (isBandDragging || document.body.classList.contains("is-knob-dragging")) return;
        setSelectedBand(null);
        setContextMenu(null);
        setPanelHover(false);
      }}
    >
      <canvas
        ref={canvasRef}
        className="eq-canvas"
        onMouseDown={handleMouseDown}
        onContextMenu={handleContextMenu}
        onMouseMove={(event) => {
          const rect = canvasRef.current.getBoundingClientRect();
          const next = { x: event.clientX - rect.left, y: event.clientY - rect.top };
          hoverRef.current = next;
          setHoverPos(next);
          if (dragRef.current === null) {
            const viewRange = rangeDisplayRef.current || dbRangeRef.current;
            const activeBands = bandsRef.current;
            const freq = freqFromX(next.x, rect.width);
            const zeroY = yFromDb(0, rect.height, viewRange);
            const idx = activeBands.findIndex((band) => {
              const gainAtX = bandShapeGainAt(band, freq);
              if (Math.abs(gainAtX) < 0.5) return false;
              const curveY = yFromDb(gainAtX, rect.height, viewRange);
              return next.y >= Math.min(curveY, zeroY) && next.y <= Math.max(curveY, zeroY);
            });
            setHoveredBandIndex(idx >= 0 ? idx : null);
          }
        }}
        onMouseLeave={() => {
          hoverRef.current = null;
          setHoverPos(null);
          setHoveredBandIndex(null);
        }}
      />
      <div className="eq-edge-fade" aria-hidden="true" />
      {inputMeter && <EqInlineMeter {...inputMeter} displayRange={getDisplayEqRange(dbRange)} />}
      {outputMeter && <EqInlineMeter {...outputMeter} displayRange={getDisplayEqRange(dbRange)} />}
      <div className="eq-top-bar">
        <div className="eq-mode-select" aria-label="EQ stage">
          {[
            ["pre", "PRE COMP", "eq-mode-pre"],
            ["post", "POST COMP", "eq-mode-post"]
          ].map(([value, label, colorCls]) => (
            <button
              key={value}
              className={cls(colorCls, value === activeEq ? "active" : "")}
              onClick={() => setActiveEq(value)}
            >
              {label}
            </button>
          ))}
        </div>
        <div className="eq-sat-control">
          <button
            className={cls("eq-sat-btn", satMode > 0 && `eq-sat-mode-${satMode}`)}
            onClick={() => setSatMode((m) => (m + 1) % 4)}
            aria-label={satMode === 0 ? "Activar saturación" : `Saturación ${SATURATION_LABELS[satMode]}`}
            title={satMode === 0 ? "Saturation off" : SATURATION_LABELS[satMode]}
            style={
              satMode > 0
                ? {
                    "--sat-bg": `hsl(${28 - satAmount * 0.28},${68 + satAmount * 0.32}%,${88 - satAmount * 0.44}%)`
                  }
                : undefined
            }
          >
            <SatIcon mode={satMode} />
          </button>
          {satMode > 0 && (
            <input
              type="range"
              className="eq-sat-fader"
              min={0}
              max={100}
              value={satAmount}
              onChange={(e) => setSatAmount(Number(e.target.value))}
              style={{ "--sat-fill": `${satAmount}%` }}
            />
          )}
        </div>
      </div>
      <div
        className={cls("eq-range-select", rangeMenuOpen && "open")}
        onPointerDown={(event) => event.stopPropagation()}
        onMouseDown={(event) => event.stopPropagation()}
      >
        <button
          type="button"
          className="eq-range-trigger"
          aria-haspopup="listbox"
          aria-expanded={rangeMenuOpen}
          onClick={(event) => {
            event.stopPropagation();
            setRangeMenuOpen((open) => !open);
          }}
        >
          <span aria-hidden="true">{dbRange} dB</span>
        </button>
        {rangeMenuOpen && (
          <div className="eq-range-menu" role="listbox" aria-label="EQ display range">
            {[3, 6, 12, 30].map((range) => (
              <button
                type="button"
                key={range}
                className={range === rangeBase ? "active" : ""}
                role="option"
                aria-selected={range === rangeBase}
                onClick={() => selectRange(range)}
              >
                {range} dB
              </button>
            ))}
          </div>
        )}
      </div>
      {hoverPos && dragRef.current === null && (
        <div className="eq-hover-label" style={{ left: hoverPos.x }}>
          {formatFreq(freqFromX(hoverPos.x, canvasWidth))}
        </div>
      )}
      {panelRenderBand !== null && bands[panelRenderBand] && (
        <BandPanel
          band={bands[panelRenderBand]}
          left={getBandPanelLeft(bands[panelRenderBand], panelRenderBand === selectedBand)}
          onChange={updateBand}
          onDelete={deleteBand}
          isVisible={panelVisible}
          isEmphasis={panelHover || isBandDragging || selectedBand !== null}
          onHoverChange={setPanelHover}
          onRequestClose={closeBandPanel}
        />
      )}
      {contextMenu && contextBand && (
        <>
          <div
            className="eq-context-backdrop"
            onClick={() => setContextMenu(null)}
            onMouseMove={() => setContextMenu(null)}
          />
          <div
            className="eq-context-menu"
            style={{ left: contextMenu.x, top: contextMenu.y }}
            onMouseLeave={() => setContextMenu(null)}
          >
            <details className="eq-context-section" open>
              <summary>
                <b>Band</b>
                <em>{contextBand.on ? "On" : "Off"}</em>
              </summary>
              <div className="eq-context-section-body">
                <button
                  className="eq-context-command"
                  onClick={() => {
                    updateBandAt(contextMenu.index, "on", !contextBand.on);
                    setContextMenu(null);
                  }}
                >
                  {contextBand.on ? "Disable Band" : "Enable Band"}
                </button>
                <button
                  className="eq-context-command"
                  onClick={() => {
                    updateBandAt(contextMenu.index, "gain", -(contextBand.gain || 0));
                    setContextMenu(null);
                  }}
                >
                  Invert Gain
                </button>
                <button onClick={deleteBand} className="danger">
                  Delete
                </button>
              </div>
            </details>
            <details className="eq-context-section">
              <summary>
                <b>Shape</b>
                <em>{contextBand.type}</em>
              </summary>
              <div className="eq-context-grid eq-context-grid-shape">
                {FILTER_TYPES.map((type) => {
                  const isBlocked =
                    (type === "Low Cut" || type === "High Cut") &&
                    bands.some((band, index) => index !== contextMenu.index && band.type === type);
                  return (
                    <button
                      key={type}
                      className={cls(contextBand.type === type && "active")}
                      disabled={isBlocked}
                      onClick={() => {
                        updateBandAt(contextMenu.index, "type", type);
                        if (!type.includes("Cut")) {
                          updateBandAt(contextMenu.index, "slope", 12);
                        }
                      }}
                    >
                      {type}
                    </button>
                  );
                })}
              </div>
            </details>
            <details className="eq-context-section">
              <summary>
                <b>Slope</b>
                <em>{contextBand.type.includes("Cut") ? `${contextBand.slope || 12} dB` : "Cuts only"}</em>
              </summary>
              <div className="eq-context-grid">
                {EQ_SLOPE_OPTIONS.map((slope) => (
                  <button
                    key={slope}
                    className={cls((contextBand.slope || 12) === slope && "active")}
                    disabled={!contextBand.type.includes("Cut")}
                    onClick={() => updateBandAt(contextMenu.index, "slope", slope)}
                  >
                    {slope}
                  </button>
                ))}
              </div>
            </details>
            <details className="eq-context-section">
              <summary>
                <b>Stereo</b>
                <em>{EQ_PLACEMENT_OPTIONS.find(([value]) => value === (contextBand.placement || "stereo"))?.[1]}</em>
              </summary>
              <div className="eq-context-grid">
                {EQ_PLACEMENT_OPTIONS.map(([value, label]) => (
                  <button
                    key={value}
                    className={cls((contextBand.placement || "stereo") === value && "active")}
                    onClick={() => updateBandAt(contextMenu.index, "placement", value)}
                  >
                    {label}
                  </button>
                ))}
              </div>
            </details>
          </div>
        </>
      )}
    </section>
  );
}
