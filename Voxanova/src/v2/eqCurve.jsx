import React from "react";

// Interactive EQ curve — independent pre/post, draggable points with Q + comp control

const EQ_SATURATION_LABELS = ['OFF', '1073', 'TAPE', 'TUBE'];
const DEFAULT_EQ_SATURATION_AMOUNT = 20;
const DEFAULT_EQ_Q = 5;
const DEFAULT_EQ_SHELF_Q = 1.3;
const DEFAULT_EQ_LOW_CUT_SLOPE = 30;

function EQCurve({ postPoints, setPostPoints, prePoints, setPrePoints, mode, setMode, showWaveform = true, scale = 12, scaleOpen, setScaleOpen, scaleOptions, setScale, scaleRef, saturation = { mode: 0, amount: 0 }, onSaturationChange, detectedFrequency = 0, spectrumData = [], detectorData = [], graphHeight = 320 }) {
  const W = 1296;
  const targetGraphHeight = Math.max(280, Math.min(430, Number(graphHeight) || 320));
  const H = targetGraphHeight;
  const padL = 0, padR = 52, padT = 28, padB = 46;
  const innerW = W - padL - padR;
  const innerH = H - padT - padB;
  const freqLabelY = H - 10;
  const svgRef = React.useRef(null);
  const gradientUid = React.useId().replace(/:/g, '');
  const [activeIdx, setActiveIdx] = React.useState(null); // selected (focus)
  const [dragMode, setDragMode] = React.useState(null);   // 'node' | 'comp' | null
  const [dragIdx, setDragIdx] = React.useState(null);
  const dragRef = React.useRef({ startX: 0, startY: 0, q: DEFAULT_EQ_Q, comp: 0 });
  const [hoverPoint, setHoverPoint] = React.useState(null);
  const [typeOpen, setTypeOpen] = React.useState(false);
  const [displayScale, setDisplayScale] = React.useState(scale);
  const displayScaleRef = React.useRef(scale);
  const latestPointsRef = React.useRef([]);
  const nodeInfoRef = React.useRef(null);
  const setPointsRef = React.useRef(null);
  const dragCommitRef = React.useRef({ raf: 0, points: null });
  const surferVisualStateRef = React.useRef({ pre: [], post: [] });
  const [surferVisualFreqs, setSurferVisualFreqs] = React.useState({ pre: [], post: [] });

  const points = mode === 'pre' ? prePoints : postPoints;
  const setPoints = mode === 'pre' ? setPrePoints : setPostPoints;
  const satMode = saturation?.mode || 0;
  const satAmount = saturation?.amount || 0;

  React.useEffect(() => {
    setPointsRef.current = setPoints;
  }, [setPoints]);

  const commitDragPoints = React.useCallback((nextPts) => {
    latestPointsRef.current = nextPts;
    dragCommitRef.current.points = nextPts;

    if (dragCommitRef.current.raf) return;

    dragCommitRef.current.raf = requestAnimationFrame(() => {
      const pending = dragCommitRef.current.points;
      dragCommitRef.current.raf = 0;
      dragCommitRef.current.points = null;
      if (pending && setPointsRef.current) setPointsRef.current(pending);
    });
  }, []);

  const flushDragPoints = React.useCallback(() => {
    const pending = dragCommitRef.current.points;
    if (dragCommitRef.current.raf) cancelAnimationFrame(dragCommitRef.current.raf);
    dragCommitRef.current.raf = 0;
    dragCommitRef.current.points = null;
    if (pending && setPointsRef.current) setPointsRef.current(pending);
  }, []);

  React.useEffect(() => () => {
    if (dragCommitRef.current.raf) cancelAnimationFrame(dragCommitRef.current.raf);
  }, []);

  React.useEffect(() => {
    latestPointsRef.current = points;
  }, [points]);

  React.useEffect(() => {
    let raf = 0;
    const from = displayScaleRef.current;
    const to = scale;
    const startedAt = performance.now();
    const duration = 220;

    const tick = (now) => {
      const t = Math.min(1, (now - startedAt) / duration);
      const eased = 1 - Math.pow(1 - t, 3);
      const nextScale = from + (to - from) * eased;
      displayScaleRef.current = nextScale;
      setDisplayScale(nextScale);
      if (t < 1) raf = requestAnimationFrame(tick);
    };

    if (Math.abs(from - to) < 0.01) {
      displayScaleRef.current = to;
      setDisplayScale(to);
      return undefined;
    }

    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [scale]);

  const clamp = (value, min, max) => Math.max(min, Math.min(max, value));
  const DESSER_TYPE = 'Desser';
  const DESSER_FREQ_MIN = 2500;
  const DESSER_FREQ_MAX = 16000;
  const DESSER_THRESHOLD_MIN = -60;
  const DESSER_THRESHOLD_MAX = 0;
  const EQ_COMP_THRESHOLD_MIN = -60;
  const EQ_COMP_THRESHOLD_MAX = 0;
  const EQ_COMP_ATTACK_MIN = 0.1;
  const EQ_COMP_ATTACK_MAX = 200;
  const EQ_COMP_RELEASE_MIN = 5;
  const EQ_COMP_RELEASE_MAX = 1000;
  const EQ_COMP_RATIO_MIN = 1;
  const EQ_COMP_RATIO_MAX = 20;
  const FILTER_TYPES = ['Bell', 'Surfer Bell', 'Low Cut', 'High Cut', 'Low Shelf', 'High Shelf', 'Notch', 'Band Pass'];
  const CUT_TYPES = new Set(['Low Cut', 'High Cut']);
  const DYNAMIC_EQ_TYPES = new Set(['Bell', 'Surfer Bell', 'Low Shelf', 'High Shelf', 'Band Pass']);
  const SLOPE_TYPES = new Set(['Low Cut', 'High Cut', 'Low Shelf', 'High Shelf']);
  const WALL_SLOPE = 'wall';
  const SLOPE_OPTIONS = [6, 12, 18, 24, 36, 48];
  const CUT_SLOPE_OPTIONS = [6, 12, 18, 24, 30, 36, 48, 72, 96, WALL_SLOPE];
  const getSurfRatio = (freq) => {
    const detected = Number(detectedFrequency);
    return Number.isFinite(detected) && detected >= 55
      ? clamp(freq / detected, 0.125, 128)
      : undefined;
  };

  const getCreatedFilterType = (freq) =>
    freq < 50
      ? 'Low Cut'
      : freq <= 100
        ? 'Low Shelf'
        : freq >= 15000
          ? 'High Cut'
          : freq > 5000
            ? 'High Shelf'
            : 'Bell';

  const getBandType = (point) => point.type === DESSER_TYPE ? 'Bell' : (point.type || getCreatedFilterType(point.freq));
  const canUseFilterType = (type, targetIdx = null, pts = points) => (
    !CUT_TYPES.has(type) ||
    !pts.some((point, index) => index !== targetIdx && getBandType(point) === type)
  );
  const getSelectableFilterTypes = (targetIdx) => (
    FILTER_TYPES.filter((type) => canUseFilterType(type, targetIdx))
  );
  const getQRange = (point) => SLOPE_TYPES.has(getBandType(point)) ? [0.1, 10] : [0.1, 50];
  const getDefaultQForType = (type) => (
    type === 'Low Shelf' || type === 'High Shelf' ? DEFAULT_EQ_SHELF_Q : DEFAULT_EQ_Q
  );
  const getDefaultSlopeForType = (type) => (
    type === 'Low Cut' ? DEFAULT_EQ_LOW_CUT_SLOPE : 12
  );
  const getPointQ = (value, type) => {
    const numeric = Number(value);
    return Number.isFinite(numeric) && numeric > 0 ? numeric : getDefaultQForType(type);
  };
  const isWallSlope = (slope) => `${slope}`.toLowerCase() === WALL_SLOPE;
  const getSlopeOptions = (type) => CUT_TYPES.has(type) ? CUT_SLOPE_OPTIONS : SLOPE_OPTIONS;
  const normalizeSlope = (slope, type) => {
    if (isWallSlope(slope)) return CUT_TYPES.has(type) ? WALL_SLOPE : SLOPE_OPTIONS[SLOPE_OPTIONS.length - 1];
    const numeric = Number(slope);
    const options = getSlopeOptions(type).filter(option => option !== WALL_SLOPE);
    if (!Number.isFinite(numeric)) return getDefaultSlopeForType(type);
    return options.reduce((closest, option) => (
      Math.abs(option - numeric) < Math.abs(closest - numeric) ? option : closest
    ), options[0]);
  };
  const getSlopeIndex = (slope, type) => {
    const normalized = normalizeSlope(slope, type);
    return getSlopeOptions(type).findIndex(option => option === normalized);
  };
  const getNeededEqRange = (pts) => (
    pts
      .filter((p) => !CUT_TYPES.has(getBandType(p)))
      .reduce((max, p) => Math.max(max, Math.abs(p.gain || 0), Math.abs(p.comp || 0)), 0) > 12
      ? 30
      : 12
  );
  const withBandDefaults = (point) => {
    const type = getBandType(point);
    const isDesser = type === DESSER_TYPE;
    const gain = isDesser ? 0 : clamp(Number(point.gain) || 0, -30, 30);
    const comp = clamp(Number(point.comp) || 0, -30, 30);
    const hasExplicitCompEnabled = Object.prototype.hasOwnProperty.call(point, 'compEnabled');
    const explicitCompEnabled = point.compEnabled === true || point.compEnabled === 'true' || Number(point.compEnabled) >= 0.5;
    const legacyCompEnabled = Math.abs(comp) > 0.05 && Math.abs(comp - gain) > 0.05;
    return {
      freq: clamp(point.freq || (isDesser ? 5600 : 1000), isDesser ? DESSER_FREQ_MIN : 20, isDesser ? DESSER_FREQ_MAX : 20000),
      gain,
      q: getPointQ(point.q, type),
      comp,
      compEnabled: DYNAMIC_EQ_TYPES.has(type) && (hasExplicitCompEnabled ? explicitCompEnabled : legacyCompEnabled),
      compThreshold: Number.isFinite(point.compThreshold) ? clamp(point.compThreshold, EQ_COMP_THRESHOLD_MIN, EQ_COMP_THRESHOLD_MAX) : -18,
      compAttack: Number.isFinite(point.compAttack) ? clamp(point.compAttack, EQ_COMP_ATTACK_MIN, EQ_COMP_ATTACK_MAX) : 12,
      compRelease: Number.isFinite(point.compRelease) ? clamp(point.compRelease, EQ_COMP_RELEASE_MIN, EQ_COMP_RELEASE_MAX) : 140,
      compRatio: Number.isFinite(point.compRatio) ? clamp(point.compRatio, EQ_COMP_RATIO_MIN, EQ_COMP_RATIO_MAX) : 4,
      slope: normalizeSlope(point.slope, type),
      type,
      on: point.on !== false,
      solo: point.solo === true || point.solo === 'true' || Number(point.solo) >= 0.5,
      placement: point.placement || 'stereo',
      deessMode: point.deessMode === 'wider' ? 'wider' : 'split',
      threshold: Number.isFinite(point.threshold) ? clamp(point.threshold, DESSER_THRESHOLD_MIN, DESSER_THRESHOLD_MAX) : -24,
      intensity: Number.isFinite(point.intensity) ? clamp(point.intensity, 0, 100) : 50,
      ...(type === 'Surfer Bell' && Number.isFinite(point.surfRatio) && point.surfRatio > 0
        ? { surfRatio: clamp(point.surfRatio, 0.125, 128) }
        : {})
    };
  };
  const supportsBandDynamics = (point) => DYNAMIC_EQ_TYPES.has(getBandType(point));
  const bandHasDynamics = (point) => {
    const band = withBandDefaults(point);
    return supportsBandDynamics(band) && band.compEnabled && Math.abs((Number(band.comp) || 0) - (Number(band.gain) || 0)) > 0.05;
  };

  React.useEffect(() => {
    let raf = 0;
    let cancelled = false;
    const detected = Number(detectedFrequency);
    const hasDetectedPitch = Number.isFinite(detected) && detected >= 55;

    const smoothFrequency = (current, target, snap) => {
      if (snap || !Number.isFinite(current) || current <= 0) return target;
      if (!Number.isFinite(target) || target <= 0) return current;

      const cents = Math.abs(1200 * Math.log2(target / current));
      if (cents < 1.2 || Math.abs(target - current) < 0.15) return target;

      return current * Math.pow(target / current, 0.18);
    };

    const buildEntries = (pts, modeKey) => {
      const previous = surferVisualStateRef.current[modeKey] || [];

      return pts.map((point, index) => {
        const band = withBandDefaults(point);
        const previousEntry = previous[index] || {};
        const anchorChanged = Math.abs((Number(previousEntry.anchor) || 0) - band.freq) > 0.5 ||
          previousEntry.type !== band.type;
        const isDraggedSurfBand = dragMode === 'node' && dragIdx === index && mode === modeKey;
        const shouldSnap = anchorChanged || isDraggedSurfBand;

        if (band.type !== 'Surfer Bell' || band.on === false) {
          return {
            type: band.type,
            anchor: band.freq,
            ratio: 0,
            freq: band.freq,
            target: band.freq,
            moving: false
          };
        }

        const explicitRatio = Number.isFinite(band.surfRatio) && band.surfRatio > 0;
        let ratio = explicitRatio ? band.surfRatio : Number(previousEntry.ratio);

        if (!explicitRatio && (!Number.isFinite(ratio) || ratio <= 0 || anchorChanged) && hasDetectedPitch) {
          ratio = clamp(band.freq / detected, 0.125, 128);
        }

        const target = isDraggedSurfBand
          ? band.freq
          : hasDetectedPitch && Number.isFinite(ratio) && ratio > 0
          ? clamp(detected * ratio, 20, 20000)
          : band.freq;
        const current = Number.isFinite(previousEntry.freq) ? previousEntry.freq : target;
        const freq = smoothFrequency(current, target, shouldSnap);
        const centsAway = Math.abs(1200 * Math.log2(target / clamp(freq, 20, 20000)));

        return {
          type: band.type,
          anchor: band.freq,
          ratio: Number.isFinite(ratio) ? ratio : 0,
          freq,
          target,
          moving: centsAway > 1.2
        };
      });
    };

    const tick = () => {
      const nextState = {
        pre: buildEntries(prePoints, 'pre'),
        post: buildEntries(postPoints, 'post')
      };
      const previous = surferVisualStateRef.current;
      let changed = previous.pre.length !== nextState.pre.length || previous.post.length !== nextState.post.length;
      let keepAnimating = false;

      for (const modeKey of ['pre', 'post']) {
        nextState[modeKey].forEach((entry, index) => {
          const oldFreq = previous[modeKey]?.[index]?.freq;
          if (!Number.isFinite(oldFreq) || Math.abs(oldFreq - entry.freq) > 0.05) changed = true;
          if (entry.moving) keepAnimating = true;
        });
      }

      surferVisualStateRef.current = nextState;

      if (changed) {
        setSurferVisualFreqs({
          pre: nextState.pre.map((entry) => entry.freq),
          post: nextState.post.map((entry) => entry.freq)
        });
      }

      if (!cancelled && keepAnimating) raf = requestAnimationFrame(tick);
    };

    tick();

    return () => {
      cancelled = true;
      if (raf) cancelAnimationFrame(raf);
    };
  // withBandDefaults is deterministic here; including it would restart the glide on every render.
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [detectedFrequency, dragIdx, dragMode, mode, postPoints, prePoints]);

  React.useEffect(() => {
    if (!typeOpen) return undefined;
    const onDocPointer = (event) => {
      if (nodeInfoRef.current && !nodeInfoRef.current.contains(event.target)) setTypeOpen(false);
    };
    const onKey = (event) => {
      if (event.key === 'Escape') setTypeOpen(false);
    };
    document.addEventListener('pointerdown', onDocPointer);
    document.addEventListener('keydown', onKey);
    return () => {
      document.removeEventListener('pointerdown', onDocPointer);
      document.removeEventListener('keydown', onKey);
    };
  }, [typeOpen]);

  React.useEffect(() => {
    setTypeOpen(false);
  }, [activeIdx, mode]);

  const formatFreq = (freq) => (
    freq < 1000
      ? `${Math.round(freq)} Hz`
      : `${(freq / 1000).toFixed(freq >= 10000 ? 1 : 2)} kHz`
  );
  const formatGain = (gain) => `${gain >= 0 ? '+' : ''}${gain.toFixed(1)} dB`;
  const formatSlope = (slope) => isWallSlope(slope) ? 'WALL' : `${slope} dB/OCT`;
  const formatThreshold = (threshold) => `${Math.round(threshold)} dB`;
  const formatIntensity = (intensity) => `${Math.round(intensity)}%`;
  const formatMs = (value) => value < 10 ? `${value.toFixed(1)} ms` : `${Math.round(value)} ms`;
  const formatRatio = (value) => `${Number(value).toFixed(value < 10 ? 1 : 0)}:1`;

  const applyPointPatch = (idx, patch, options = {}) => {
    const { resort = true, updateRange = true } = options;
    if (idx === null || idx === undefined || idx < 0) return idx;
    const currentPoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    if (!currentPoints[idx]) return idx;
    if (patch.type && !canUseFilterType(patch.type, idx, currentPoints)) return idx;

    const updated = withBandDefaults({ ...currentPoints[idx], ...patch });
    if (updated.type === 'Surfer Bell' && (Object.prototype.hasOwnProperty.call(patch, 'freq') || Object.prototype.hasOwnProperty.call(patch, 'type'))) {
      const ratio = getSurfRatio(updated.freq);
      if (ratio) updated.surfRatio = ratio;
    }
    const nextPts = [...currentPoints];
    nextPts[idx] = updated;
    if (resort) nextPts.sort((a, b) => a.freq - b.freq);
    const nextIdx = nextPts.indexOf(updated);
    const normalizedIdx = nextIdx === -1 ? idx : nextIdx;

    latestPointsRef.current = nextPts;
    setPoints(nextPts);
    setActiveIdx(normalizedIdx);
    if (updateRange) setScale?.(getNeededEqRange(nextPts));
    return normalizedIdx;
  };

  const deleteActivePoint = () => {
    if (activeIdx === null || activeIdx === undefined || points.length <= 0) return;
    const np = points.filter((_, i) => i !== activeIdx);
    latestPointsRef.current = np;
    setPoints(np);
    setActiveIdx(null);
    setDragIdx(null);
    setDragMode(null);
    setTypeOpen(false);
    setScale?.(getNeededEqRange(np));
  };

  const onInfoDragStart = (field) => (e) => {
    if (activeIdx === null || activeIdx === undefined || !points[activeIdx]) return;
    e.preventDefault();
    e.stopPropagation();
    setTypeOpen(false);

    const startPoint = withBandDefaults(points[activeIdx]);
    const startX = e.clientX;
    const startY = e.clientY;
    let targetIdx = activeIdx;
    e.currentTarget.setPointerCapture?.(e.pointerId);

    const onPointerMove = (event) => {
      event.preventDefault();
      const dx = event.clientX - startX;
      const dy = event.clientY - startY;
      if (field === 'freq') {
        const freq = Math.round(clamp(startPoint.freq * Math.pow(2, dx / 130), 20, 20000));
        targetIdx = applyPointPatch(targetIdx, { freq }, { resort: true, updateRange: false });
      } else if (field === 'gain') {
        const gain = CUT_TYPES.has(startPoint.type)
          ? 0
          : Number(clamp(startPoint.gain - dy / 5, -30, 30).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { gain }, { resort: false, updateRange: true });
      } else if (field === 'q') {
        const [minQ, maxQ] = getQRange(startPoint);
        const q = Number(clamp(startPoint.q * Math.exp((dx - dy) / 120), minQ, maxQ).toFixed(2));
        targetIdx = applyPointPatch(targetIdx, { q }, { resort: false, updateRange: false });
      } else if (field === 'slope') {
        const options = getSlopeOptions(startPoint.type);
        const startIndex = getSlopeIndex(startPoint.slope, startPoint.type);
        const stepDelta = Math.round((dx - dy) / 28);
        const slope = options[clamp(startIndex + stepDelta, 0, options.length - 1)];
        targetIdx = applyPointPatch(targetIdx, { slope }, { resort: false, updateRange: false });
      } else if (field === 'threshold') {
        const threshold = Number(clamp(startPoint.threshold - dy / 2, DESSER_THRESHOLD_MIN, DESSER_THRESHOLD_MAX).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { threshold }, { resort: false, updateRange: false });
      } else if (field === 'intensity') {
        const intensity = Math.round(clamp(startPoint.intensity - dy / 1.8, 0, 100));
        targetIdx = applyPointPatch(targetIdx, { intensity }, { resort: false, updateRange: true });
      } else if (field === 'compThreshold') {
        const compThreshold = Number(clamp(startPoint.compThreshold - dy / 2, EQ_COMP_THRESHOLD_MIN, EQ_COMP_THRESHOLD_MAX).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { compThreshold }, { resort: false, updateRange: false });
      } else if (field === 'compAttack') {
        const compAttack = Number(clamp(startPoint.compAttack * Math.exp((dx - dy) / 150), EQ_COMP_ATTACK_MIN, EQ_COMP_ATTACK_MAX).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { compAttack }, { resort: false, updateRange: false });
      } else if (field === 'compRelease') {
        const compRelease = Number(clamp(startPoint.compRelease * Math.exp((dx - dy) / 150), EQ_COMP_RELEASE_MIN, EQ_COMP_RELEASE_MAX).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { compRelease }, { resort: false, updateRange: false });
      } else if (field === 'compRatio') {
        const compRatio = Number(clamp(startPoint.compRatio * Math.exp((dx - dy) / 180), EQ_COMP_RATIO_MIN, EQ_COMP_RATIO_MAX).toFixed(1));
        targetIdx = applyPointPatch(targetIdx, { compRatio }, { resort: false, updateRange: false });
      }
    };

    const onPointerUp = () => {
      setScale?.(getNeededEqRange(latestPointsRef.current));
      window.removeEventListener('pointermove', onPointerMove);
    };

    window.addEventListener('pointermove', onPointerMove);
    window.addEventListener('pointerup', onPointerUp, { once: true });
  };

  const FILTER_TYPE_PATHS = {
    'Bell': 'M2 11.5 C5 11.5 5.8 4 9 4 C12.2 4 13 11.5 16 11.5',
    'Surfer Bell': 'M2 11.2 C4.4 11.2 5.5 4.8 8.2 4.8 C10.3 4.8 11.2 8.1 12.8 8.1 C14.2 8.1 14.8 6.7 16 6.7',
    'Low Cut': 'M2 12.5 C5 12.5 5.8 5 8.9 5 L16 5',
    'High Cut': 'M2 5 L9.1 5 C12.2 5 13 12.5 16 12.5',
    'Low Shelf': 'M2 11.5 C5 11.5 5.8 6.6 8.8 6.6 L16 6.6',
    'High Shelf': 'M2 6.6 L9.2 6.6 C12.2 6.6 13 11.5 16 11.5',
    'Notch': 'M2 5.2 C5.2 5.2 5.7 12.3 9 12.3 C12.3 12.3 12.8 5.2 16 5.2',
    'Band Pass': 'M2 12.2 C5.1 12.2 5.5 5.3 9 5.3 C12.5 5.3 12.9 12.2 16 12.2',
    'Desser': 'M2 11.7 C4.6 11.7 5.2 7.2 7.1 7.2 C8.7 7.2 9.1 11.6 10.8 11.6 C12.7 11.6 13.1 5.4 16 5.4'
  };
  const getFilterTypePath = (type) => FILTER_TYPE_PATHS[type] || FILTER_TYPE_PATHS.Bell;

  const FilterTypeIcon = ({ type }) => {
    return (
      <svg className="eq-type-icon" width="20" height="16" viewBox="0 0 18 16" fill="none" aria-hidden="true">
        <path d="M2 12.8H16" stroke="currentColor" strokeWidth="0.7" strokeLinecap="round" opacity="0.22" />
        <path d={getFilterTypePath(type)} stroke="currentColor" strokeWidth="1.35" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    );
  };

  const freqToX = (f) => {
    return padL + (Math.log10(clamp(f, 20, 20000) / 20) / Math.log10(1000)) * innerW;
  };
  const xToFreq = (x) => {
    return 20 * 1000 ** clamp((x - padL) / innerW, 0, 1);
  };
  const gainToYAtScale = (g, range) => padT + ((range - Math.max(-range, Math.min(range, g))) / (range * 2)) * innerH;
  const gainToCurveYAtScale = (g, range) => padT + ((range - g) / (range * 2)) * innerH;
  const gainToY = (g) => gainToYAtScale(g, displayScale);
  const gainToCurveY = (g) => gainToCurveYAtScale(g, displayScale);
  const gainMarkToY = (g) => gainToYAtScale(g, scale);
  const yToGain = (y) => scale - ((y - padT) / innerH) * (scale * 2);
  const yToGainAtScale = (y, range) => range - ((y - padT) / innerH) * (range * 2);
  const thresholdToDisplayGain = (threshold, range = displayScale) => {
    const normalized = (clamp(threshold, DESSER_THRESHOLD_MIN, DESSER_THRESHOLD_MAX) - DESSER_THRESHOLD_MIN) /
      (DESSER_THRESHOLD_MAX - DESSER_THRESHOLD_MIN);
    return normalized * range * 2 - range;
  };
  const thresholdFromY = (y, range = displayScale) => {
    const normalized = clamp((yToGainAtScale(y, range) + range) / (range * 2), 0, 1);
    return Number((DESSER_THRESHOLD_MIN + normalized * (DESSER_THRESHOLD_MAX - DESSER_THRESHOLD_MIN)).toFixed(1));
  };

  const bandShapeGainAt = (point, freq) => {
    const p = withBandDefaults(point);
    const logD = Math.log10(clamp(freq, 20, 20000) / p.freq);
    const q = clamp(p.q || DEFAULT_EQ_Q, ...getQRange(p));

    switch (p.type) {
      case 'Low Shelf':
        return p.gain / (1 + (freq / p.freq) ** (q * 2));
      case 'High Shelf':
        return p.gain / (1 + (p.freq / freq) ** (q * 2));
      case 'Notch':
        return -Math.abs(p.gain) * Math.exp(-logD * logD * q * 10);
      case 'Band Pass':
        return p.gain * Math.exp(-logD * logD * q * 2);
      case 'Low Cut': {
        if (isWallSlope(p.slope)) {
          return -160 / (1 + Math.exp(logD * 420));
        }
        const n = p.slope / 6;
        const cut = -10 * Math.log10(1 + (p.freq / freq) ** (2 * n));
        return cut + (p.gain || 0) * Math.exp(-logD * logD * q * 3);
      }
      case 'High Cut': {
        if (isWallSlope(p.slope)) {
          return -160 / (1 + Math.exp(-logD * 420));
        }
        const n = p.slope / 6;
        const cut = -10 * Math.log10(1 + (freq / p.freq) ** (2 * n));
        return cut + (p.gain || 0) * Math.exp(-logD * logD * q * 3);
      }
      case DESSER_TYPE: {
        const slopeFactor = p.deessMode === 'wider' ? 1.15 : 4.7;
        const highBand = 1 / (1 + (p.freq / clamp(freq, 20, 20000)) ** (slopeFactor * 2));
        const thresholdDrive = 0.5 + Math.abs(p.threshold) / 120;
        const amountDb = 1.2 + (p.intensity / 100) * 16;
        return -amountDb * thresholdDrive * highBand;
      }
      case 'Bell':
      case 'Surfer Bell':
      default:
        return p.gain * Math.exp(-logD * logD * q * 5);
    }
  };
  const bandGainAt = (point, freq) => point.on === false ? 0 : bandShapeGainAt(point, freq);
  const nodeGainAt = (point) => {
    const p = withBandDefaults(point);
    if (p.type === DESSER_TYPE) return thresholdToDisplayGain(p.threshold);
    return CUT_TYPES.has(p.type) ? -3 : bandShapeGainAt(p, p.freq);
  };
  const gainFromNodeGain = (point, nodeGain) => {
    const p = withBandDefaults(point);
    if (p.type === DESSER_TYPE) return 0;
    if (CUT_TYPES.has(p.type)) return 0;
    if (p.type === 'Low Shelf' || p.type === 'High Shelf') return nodeGain * 2;
    if (p.type === 'Notch') return Math.abs(nodeGain);
    return nodeGain;
  };

  const getDisplayPoints = (pts, modeKey) => (
    pts.map((point, index) => {
      const band = withBandDefaults(point);
      const visualFreq = surferVisualFreqs[modeKey]?.[index];

      if (band.type !== 'Surfer Bell' || !Number.isFinite(visualFreq)) return band;

      return {
        ...band,
        anchorFreq: band.freq,
        freq: clamp(visualFreq, 20, 20000),
        isSurfing: Math.abs(clamp(visualFreq, 20, 20000) - band.freq) > 0.5
      };
    })
  );

  const preDisplayPoints = getDisplayPoints(prePoints, 'pre');
  const postDisplayPoints = getDisplayPoints(postPoints, 'post');
  const displayPoints = mode === 'pre' ? preDisplayPoints : postDisplayPoints;

  // Total curve at any freq = sum of all band contributions.
  const totalGain = (pts, f) => pts.reduce((acc, p) => acc + bandGainAt(p, f), 0);
  const cutIsolationWeight = (cutGain) => {
    const attenuation = Math.abs(Math.min(0, cutGain));
    if (attenuation <= 0.65) return 1;
    if (attenuation >= 5.0) return 0;
    const t = clamp((attenuation - 0.65) / 4.35, 0, 1);
    return 1 - t * t * (3 - 2 * t);
  };
  const totalDisplayGain = (pts, f) => {
    let cutGain = 0;
    let tonalGain = 0;
    let strongestCutGain = 0;

    pts.forEach((point) => {
      if (point.on === false) return;
      const band = withBandDefaults(point);
      const gain = bandShapeGainAt(band, f);

      if (CUT_TYPES.has(band.type)) {
        cutGain += gain;
        strongestCutGain = Math.min(strongestCutGain, gain);
        return;
      }

      tonalGain += gain;
    });

    return cutGain + tonalGain * cutIsolationWeight(strongestCutGain);
  };
  const buildFrequencySamples = (pts, samples = 420) => {
    const frequencies = [];
    const addFrequency = (freq) => {
      const clamped = clamp(Number(freq) || 20, 20, 20000);
      frequencies.push(clamped);
    };

    for (let i = 0; i <= samples; i++) {
      const t = i / samples;
      addFrequency(20 * 1000 ** t);
    }

    pts.forEach((point) => {
      const band = withBandDefaults(point);
      if (band.on === false || !CUT_TYPES.has(band.type)) return;

      [0.45, 0.62, 0.78, 0.90, 0.96, 0.985, 0.997, 1, 1.003, 1.015, 1.04, 1.11, 1.28, 1.62]
        .forEach((ratio) => addFrequency(band.freq * ratio));
    });

    return frequencies
      .sort((a, b) => a - b)
      .filter((freq, index, list) => index === 0 || Math.abs(freqToX(freq) - freqToX(list[index - 1])) > 0.12);
  };

  // Build smooth curve sampled across freq range
  const buildSampledCurve = (pts) => {
    const hasCut = pts.some((p) => p.on !== false && CUT_TYPES.has(getBandType(p)));
    const frequencies = buildFrequencySamples(pts, hasCut ? 520 : 260);
    let d = '';
    frequencies.forEach((f, i) => {
      const g = hasCut ? totalDisplayGain(pts, f) : totalGain(pts, f);
      const x = freqToX(f);
      const y = hasCut ? gainToCurveY(g) : gainToY(g);
      d += i === 0 ? `M ${x} ${y}` : ` L ${x} ${y}`;
    });
    return d;
  };

  // Build per-band curve (filled)
  const buildBandCurve = (pts, idx) => {
    if (!pts[idx]) return { line: '', fill: '', isCut: false };
    const p = withBandDefaults(pts[idx]);
    const isCut = CUT_TYPES.has(p.type);
    const frequencies = buildFrequencySamples([p], isCut ? 440 : 180);
    let line = '';
    frequencies.forEach((f, i) => {
      const g = bandShapeGainAt(p, f);
      const x = freqToX(f);
      const y = isCut ? gainToCurveY(g) : gainToY(g);
      line += i === 0 ? `M ${x} ${y}` : ` L ${x} ${y}`;
    });
    const baseY = gainToY(0);
    const fill = isCut ? '' : `${line} L ${padL + innerW} ${baseY} L ${padL} ${baseY} Z`;
    return { line, fill, isCut };
  };

  const buildCompMirrorCurve = (point, includeMuted = false) => {
    const source = withBandDefaults(point);
    const mirrorGain = clamp(Number(source.comp) || 0, -scale, scale);
    const hasTarget = Math.abs(mirrorGain - (Number(source.gain) || 0)) > 0.05;
    if ((!includeMuted && !bandHasDynamics(source)) || !hasTarget) {
      return { line: '', fill: '', active: false };
    }

    const mirrorPoint = { ...source, gain: mirrorGain };
    const samples = 200;
    const sourcePts = [];
    const mirrorPts = [];
    for (let i = 0; i <= samples; i++) {
      const t = i / samples;
      const lf = Math.log10(20) + t * (Math.log10(20000) - Math.log10(20));
      const f = Math.pow(10, lf);
      const x = padL + t * innerW;
      sourcePts.push({ x, y: gainToY(bandShapeGainAt(source, f)) });
      mirrorPts.push({ x, y: gainToY(bandShapeGainAt(mirrorPoint, f)) });
    }

    const line = mirrorPts.map((pt, i) => `${i === 0 ? 'M' : 'L'} ${pt.x} ${pt.y}`).join(' ');
    const fill = `${line} ${sourcePts.slice().reverse().map((pt) => `L ${pt.x} ${pt.y}`).join(' ')} Z`;
    return { line, fill, active: true };
  };

  const buildCompMotionCurve = (point, engagement) => {
    const source = withBandDefaults(point);
    const amount = clamp(Number(engagement) || 0, 0, 1);
    if (!bandHasDynamics(source) || amount <= 0.003) {
      return { line: '', fill: '', active: false, cx: 0, cy: 0 };
    }

    const liveGain = source.gain + (source.comp - source.gain) * amount;
    const livePoint = { ...source, gain: liveGain };
    const samples = 200;
    const sourcePts = [];
    const livePts = [];
    for (let i = 0; i <= samples; i++) {
      const t = i / samples;
      const lf = Math.log10(20) + t * (Math.log10(20000) - Math.log10(20));
      const f = Math.pow(10, lf);
      const x = padL + t * innerW;
      sourcePts.push({ x, y: gainToY(bandShapeGainAt(source, f)) });
      livePts.push({ x, y: gainToY(bandShapeGainAt(livePoint, f)) });
    }

    const line = livePts.map((pt, i) => `${i === 0 ? 'M' : 'L'} ${pt.x} ${pt.y}`).join(' ');
    const fill = `${line} ${sourcePts.slice().reverse().map((pt) => `L ${pt.x} ${pt.y}`).join(' ')} Z`;
    return {
      line,
      fill,
      active: true,
      cx: freqToX(livePoint.freq),
      cy: gainToY(nodeGainAt(livePoint))
    };
  };

  const getGraphPointer = (e) => {
    if (!svgRef.current) return null;
    const rect = svgRef.current.getBoundingClientRect();
    const sx = (e.clientX - rect.left) * (W / rect.width);
    const sy = (e.clientY - rect.top) * (H / rect.height);
    if (sx < padL || sx > padL + innerW || sy < padT || sy > padT + innerH) return null;
    return { sx, sy };
  };
  const getNodeIndexAtGraphPoint = (sx, sy, radius = 20) => {
    const hitRadius = radius;
    return points.findIndex((point, index) => {
      const sourceBand = withBandDefaults(point);
      const visualBand = displayPoints[index] ? withBandDefaults(displayPoints[index]) : sourceBand;
      const liveX = freqToX(visualBand.freq);
      const liveY = gainToY(nodeGainAt(visualBand));

      if (Math.hypot(sx - liveX, sy - liveY) <= hitRadius) return true;

      const isSurfing = sourceBand.type === 'Surfer Bell' && Math.abs(visualBand.freq - sourceBand.freq) > 0.5;
      if (!isSurfing) return false;

      const anchorX = freqToX(sourceBand.freq);
      const anchorY = gainToY(nodeGainAt(sourceBand));
      return Math.hypot(sx - anchorX, sy - anchorY) <= hitRadius;
    });
  };
  const blocksBackgroundCreate = (target) => Boolean(
    target.closest?.('.eq-scale-wrap, .eq-node-hit, .eq-surf-ghost-hit, .eq-comp-arrow, .eq-band-fill, .eq-band-line')
  );
  const createBandAtGraphPoint = (sx, sy, { startDrag = false, event = null } = {}) => {
    const f = clamp(xToFreq(sx), 20, 20000);
    const type = getCreatedFilterType(f);
    if (!canUseFilterType(type, null, points)) {
      const existingIdx = points.findIndex((point) => getBandType(point) === type);
      if (existingIdx >= 0) setActiveIdx(existingIdx);
      return;
    }
    const q = getDefaultQForType(type);
    const slope = getDefaultSlopeForType(type);
    const g = gainFromNodeGain({ freq: f, type, q, slope }, yToGain(sy));
    const created = withBandDefaults({
      freq: Math.round(f),
      gain: Number(clamp(g, -30, 30).toFixed(1)),
      q,
      comp: 0,
      compEnabled: false,
      type,
      slope,
      on: true,
      placement: 'stereo',
      ...(type === 'Surfer Bell' && getSurfRatio(f) ? { surfRatio: getSurfRatio(f) } : {})
    });
    const newPts = [...points, created];
    newPts.sort((a, b) => a.freq - b.freq);
    latestPointsRef.current = newPts;
    setPoints(newPts);
    const newIdx = newPts.findIndex(p => p === created);
    setActiveIdx(newIdx);
    setScale?.(getNeededEqRange(newPts));
    if (startDrag) {
      setDragIdx(newIdx);
      setDragMode('node');
      if (Number.isFinite(event?.pointerId)) event.currentTarget.setPointerCapture?.(event.pointerId);
    }
  };

  const onSvgPointerDown = (e) => {
    if (e.target.closest?.('.eq-scale-wrap')) return;
    const pointer = getGraphPointer(e);
    if (!pointer) return;

    const nearNodeIdx = getNodeIndexAtGraphPoint(pointer.sx, pointer.sy);
    if (nearNodeIdx >= 0) {
      setActiveIdx(nearNodeIdx);
      return;
    }

    if (blocksBackgroundCreate(e.target)) return;
    if (Math.abs(pointer.sy - baseY) > 8) return;

    createBandAtGraphPoint(pointer.sx, baseY, { startDrag: true, event: e });
  };
  const onSvgDoubleClick = (e) => {
    if (blocksBackgroundCreate(e.target)) return;
    const pointer = getGraphPointer(e);
    if (!pointer) return;

    const nearNodeIdx = getNodeIndexAtGraphPoint(pointer.sx, pointer.sy);
    if (nearNodeIdx >= 0) {
      setActiveIdx(nearNodeIdx);
      return;
    }

    e.preventDefault();
    createBandAtGraphPoint(pointer.sx, pointer.sy);
  };
  const onBandSelectDown = (idx) => (e) => {
    e.stopPropagation();
    e.preventDefault();
    setActiveIdx(idx);
    setTypeOpen(false);
  };

  const onNodeDown = (idx) => (e) => {
    e.stopPropagation();
    setActiveIdx(idx);
    setDragIdx(idx);
    setDragMode('node');
    e.currentTarget.setPointerCapture?.(e.pointerId);
  };
  const onSurferGhostDown = (idx, liveFrequency) => (e) => {
    e.stopPropagation();
    e.preventDefault();

    const currentPoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    const sourcePoint = currentPoints[idx];
    if (!sourcePoint) return;

    const frozenFrequency = clamp(liveFrequency, 20, 20000);
    const frozen = withBandDefaults({
      ...sourcePoint,
      freq: frozenFrequency,
      type: 'Surfer Bell'
    });
    const ratio = getSurfRatio(frozenFrequency);
    if (ratio) frozen.surfRatio = ratio;

    const nextPts = [...currentPoints];
    nextPts[idx] = frozen;
    nextPts.sort((a, b) => a.freq - b.freq);
    const nextIdx = nextPts.indexOf(frozen);
    const normalizedIdx = nextIdx === -1 ? idx : nextIdx;

    latestPointsRef.current = nextPts;
    setPoints(nextPts);
    setActiveIdx(normalizedIdx);
    setDragIdx(normalizedIdx);
    setDragMode('node');
    setTypeOpen(false);
    e.currentTarget.setPointerCapture?.(e.pointerId);
  };
  const onCompDown = (idx, direction) => (e) => {
    e.stopPropagation();
    e.preventDefault();
    const currentPoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    const current = withBandDefaults(currentPoints[idx]);
    if (!current || !supportsBandDynamics(current)) return;

    const arrowDirection = direction >= 0 ? 1 : -1;
    let dragComp = Number(current.comp) || 0;
    if (!bandHasDynamics(current)) {
      const next = [...currentPoints];
      const baseGain = Number(current.gain) || 0;
      const initialStep = Math.max(1.2, Math.min(6, Math.abs(baseGain) * 0.45 || 2.5));
      dragComp = Number(clamp(baseGain + arrowDirection * initialStep, -30, 30).toFixed(1));
      next[idx] = withBandDefaults({
        ...currentPoints[idx],
        comp: dragComp,
        compEnabled: Math.abs(dragComp - baseGain) > 0.05
      });
      latestPointsRef.current = next;
      setPoints(next);
      if (Math.abs(dragComp) > scale) setScale?.(30);
    }

    setActiveIdx(idx);
    setDragIdx(idx);
    setDragMode('comp');
    dragRef.current = {
      startX: e.clientX, startY: e.clientY,
      q: current.q || DEFAULT_EQ_Q, comp: dragComp,
      direction: arrowDirection,
    };
    e.currentTarget.setPointerCapture?.(e.pointerId);
  };
  const onNodeWheel = (idx, e) => {
    e.preventDefault();
    const np = [...points];
    const p = withBandDefaults(np[idx]);
    if (p.type === DESSER_TYPE) {
      p.intensity = Math.round(clamp(p.intensity + (e.deltaY < 0 ? 4 : -4), 0, 100));
    } else if (CUT_TYPES.has(p.type)) {
      const options = getSlopeOptions(p.type);
      const currentIndex = getSlopeIndex(p.slope, p.type);
      const nextIndex = clamp(currentIndex + (e.deltaY < 0 ? 1 : -1), 0, options.length - 1);
      p.slope = options[nextIndex];
    } else {
      p.q = clamp((p.q || DEFAULT_EQ_Q) * Math.exp(-e.deltaY / 200), ...getQRange(p));
    }
    np[idx] = p;
    setPoints(np);
    setActiveIdx(idx);
  };
  const onNodeContext = (idx) => (e) => {
    e.preventDefault();
    if (points.length <= 0) return;
    const np = points.filter((_, i) => i !== idx);
    setPoints(np);
    if (activeIdx === idx) setActiveIdx(null);
  };

  const onMove = (e) => {
    if (!svgRef.current) return;
    const rect = svgRef.current.getBoundingClientRect();
    const sx = (e.clientX - rect.left) * (W / rect.width);
    const sy = (e.clientY - rect.top) * (H / rect.height);
    const inGraph = sx >= padL && sx <= padL + innerW && sy >= padT && sy <= padT + innerH;
    const overExistingNode = Boolean(e.target.closest?.('.eq-node-hit, .eq-surf-ghost-hit, .eq-comp-arrow'));
    setHoverPoint(inGraph && !overExistingNode ? { x: clamp(sx, padL, padL + innerW), y: clamp(sy, padT, padT + innerH) } : null);
    if (dragIdx === null || dragMode === null) return;

    const sourcePoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    const newPts = [...sourcePoints];
    if (!newPts[dragIdx]) return;
    const p = { ...newPts[dragIdx] };

    if (dragMode === 'node') {
      const current = withBandDefaults(p);
      if (current.type === DESSER_TYPE) {
        p.freq = Math.round(clamp(xToFreq(sx), DESSER_FREQ_MIN, DESSER_FREQ_MAX));
        p.type = current.type;
        p.slope = current.slope;
        p.on = current.on;
        p.placement = current.placement;
        p.gain = 0;
        p.q = current.q;
        p.comp = current.comp;
        p.deessMode = current.deessMode;
        p.threshold = thresholdFromY(sy);
        p.intensity = current.intensity;
        newPts[dragIdx] = p;
        newPts.sort((a, b) => a.freq - b.freq);
        const newIdx = newPts.findIndex(pp => pp === p);
        commitDragPoints(newPts);
        if (newIdx !== -1) {
          setDragIdx(newIdx);
          setActiveIdx(newIdx);
        }
        return;
      }
      const rawNodeGainAtTwelve = yToGainAtScale(sy, 12);
      const shouldExpand = !CUT_TYPES.has(current.type) && (scale > 12 || Math.abs(rawNodeGainAtTwelve) > 12);
      const nextRange = shouldExpand ? 30 : scale;
      const nodeGain = shouldExpand ? yToGainAtScale(sy, 30) : yToGain(sy);
      p.freq = clamp(xToFreq(sx), 20, 20000);
      p.type = current.type;
      p.slope = current.slope;
      p.on = current.on;
      p.placement = current.placement;
      p.gain = CUT_TYPES.has(current.type)
        ? 0
        : Number(clamp(gainFromNodeGain(current, nodeGain), -30, 30).toFixed(1));
      if (current.type === 'Surfer Bell') {
        const ratio = getSurfRatio(p.freq);
        if (ratio) p.surfRatio = ratio;
      }
      newPts[dragIdx] = p;
      newPts.sort((a, b) => a.freq - b.freq);
      const newIdx = newPts.findIndex(pp => pp === p);
      commitDragPoints(newPts);
      if (nextRange !== scale) setScale?.(nextRange);
      if (newIdx !== -1) {
        setDragIdx(newIdx);
        setActiveIdx(newIdx);
      }
    } else if (dragMode === 'comp') {
      const current = withBandDefaults(p);
      if (!supportsBandDynamics(current)) return;
      const signedComp = clamp(yToGain(sy), -scale, scale);
      p.comp = Number(signedComp.toFixed(1));
      p.compEnabled = Math.abs(p.comp - (Number(current.gain) || 0)) > 0.05;
      newPts[dragIdx] = p;
      commitDragPoints(newPts);
    }
  };

  const onUp = () => {
    flushDragPoints();
    if (dragIdx !== null && dragMode === 'node') {
      setScale?.(getNeededEqRange(latestPointsRef.current));
    }
    setDragIdx(null);
    setDragMode(null);
  };

  const baseY = gainToY(0);
  const activeFillId = mode === 'pre' ? 'preFill' : 'postFill';
  const activeColor = mode === 'pre' ? 'var(--curve-pre)' : 'var(--curve-post)';
  const activeHasCut = displayPoints.some((p) => p.on !== false && CUT_TYPES.has(getBandType(p)));
  const activeFillFocusY = (() => {
    const selectedBand = activeIdx !== null && activeIdx !== undefined && displayPoints[activeIdx]
      ? withBandDefaults(displayPoints[activeIdx])
      : null;
    if (selectedBand && !CUT_TYPES.has(selectedBand.type)) return gainToY(nodeGainAt(selectedBand));

    const strongestBandY = displayPoints
      .map((point) => withBandDefaults(point))
      .filter((point) => !CUT_TYPES.has(point.type))
      .map((point) => gainToY(nodeGainAt(point)))
      .reduce((strongest, y) => (
        Math.abs(y - baseY) > Math.abs(strongest - baseY) ? y : strongest
      ), baseY - 1);

    return Math.abs(strongestBandY - baseY) < 1 ? baseY - 1 : strongestBandY;
  })();

  const bandColor = () => activeColor;
  const bandFillId = (idx) => `${gradientUid}-eq-band-fill-${mode}-${idx}`;
  const compMirrorFillId = (idx) => `${gradientUid}-eq-comp-fill-${mode}-${idx}`;

  const gainMarks = React.useMemo(() => {
    const step = scale <= 3 ? 1 : scale <= 6 ? 2 : scale <= 12 ? 3 : 6;
    const marks = [];
    for (let g = -scale; g <= scale; g += step) marks.push(g);
    return marks;
  }, [scale]);

  const freqMarks = [50, 100, 200, 500, 1000, 2000, 5000, 10000];
  const metricFreqMarks = [
    20, 30, 40, 50, 60, 70, 80, 90, 100,
    200, 300, 400, 500, 600, 700, 800, 900, 1000,
    2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000
  ];
  const dbAxisX = padL + innerW + 10;
  const formatDbLabel = (value) => {
    const rounded = Math.round(value * 10) / 10;
    return Number.isInteger(rounded) ? `${rounded}` : rounded.toFixed(1);
  };
  const isDarkMode = document.body?.dataset?.mode === 'dark';
  const spectrumRgb = isDarkMode ? '214,218,228' : '38,49,64';
  const spectrumStroke = `rgba(${spectrumRgb},${isDarkMode ? 0.42 : 0.34})`;
  const eqGridStroke = (alpha, tone = 'minor') => {
    if (!isDarkMode) {
      const rgb = tone === 'zero' ? '31,48,70' : '68,86,108';
      return `rgba(${rgb},${alpha})`;
    }

    const lift =
      tone === 'zero'
        ? alpha * 1.62 + 0.03
        : tone === 'major'
          ? alpha * 1.95 + 0.022
          : alpha * 1.85 + 0.018;

    return `rgba(242,247,255,${Math.min(lift, 0.42)})`;
  };

  const inputSpectrum = React.useMemo(() => (
    Array.isArray(spectrumData)
      ? spectrumData.map((value) => clamp(Number(value) || 0, 0, 1))
      : []
  ), [spectrumData]);
  const inputDetectorDbs = React.useMemo(() => (
    Array.isArray(detectorData)
      ? detectorData.map((value) => {
        const numeric = Number(value);
        return Number.isFinite(numeric) ? clamp(numeric, -120, 24) : -120;
      })
      : []
  ), [detectorData]);

  const spectrumShape = React.useMemo(() => {
    if (inputSpectrum.length < 2 || inputSpectrum.every((value) => value <= 0.002)) {
      return { line: '', fill: '' };
    }

    const baselineY = padT + innerH * 0.92;
    const topY = padT + innerH * 0.16;
    const format = (value) => (Math.round(value * 100) / 100).toString();
    const pointAt = (points, index) => points[clamp(index, 0, points.length - 1)];
    const smoothPath = (points) => {
      if (points.length < 2) return '';
      let path = `M ${format(points[0].x)} ${format(points[0].y)}`;

      for (let index = 0; index < points.length - 1; index += 1) {
        const p0 = pointAt(points, index - 1);
        const p1 = pointAt(points, index);
        const p2 = pointAt(points, index + 1);
        const p3 = pointAt(points, index + 2);
        const minY = Math.min(p1.y, p2.y);
        const maxY = Math.max(p1.y, p2.y);
        const cp1x = p1.x + (p2.x - p0.x) / 6;
        const cp1y = clamp(p1.y + (p2.y - p0.y) / 6, minY, maxY);
        const cp2x = p2.x - (p3.x - p1.x) / 6;
        const cp2y = clamp(p2.y - (p3.y - p1.y) / 6, minY, maxY);
        path += ` C ${format(cp1x)} ${format(cp1y)} ${format(cp2x)} ${format(cp2y)} ${format(p2.x)} ${format(p2.y)}`;
      }

      return path;
    };

    const points = inputSpectrum.map((value, index) => {
      const weightAt = (offset, weight) => {
        const sample = inputSpectrum[index + offset];
        return sample === undefined ? { value: 0, weight: 0 } : { value: sample * weight, weight };
      };
      const weighted = [
        weightAt(-1, 0.16),
        weightAt(0, 0.68),
        weightAt(1, 0.16)
      ];
      const weightTotal = weighted.reduce((sum, item) => sum + item.weight, 0) || 1;
      const smoothed = weighted.reduce((sum, item) => sum + item.value, 0) / weightTotal;
      const t = inputSpectrum.length <= 1 ? 0 : index / (inputSpectrum.length - 1);
      const shaped = clamp((smoothed - 0.025) / 0.975, 0, 1) * 0.88;
      return {
        x: padL + t * innerW,
        y: baselineY - shaped * (baselineY - topY)
      };
    });

    const line = smoothPath(points);
    return {
      line,
      fill: `${line} L ${padL + innerW} ${baselineY} L ${padL} ${baselineY} Z`
    };
  }, [inputSpectrum, innerH, innerW]);

  const cycleSaturation = () => {
    const nextMode = (satMode + 1) % EQ_SATURATION_LABELS.length;
    onSaturationChange?.({
      mode: nextMode,
      amount: nextMode === 0 ? 0 : (satAmount || DEFAULT_EQ_SATURATION_AMOUNT)
    });
  };
  const setSaturationAmount = (event) => {
    onSaturationChange?.({
      mode: satMode || 1,
      amount: Number(event.target.value)
    });
  };
  const activeBand = activeIdx !== null && activeIdx !== undefined && points[activeIdx]
    ? withBandDefaults(points[activeIdx])
    : null;
  const activeDisplayBand = activeIdx !== null && activeIdx !== undefined && displayPoints[activeIdx]
    ? withBandDefaults(displayPoints[activeIdx])
    : activeBand;
  const activeBandIsDesser = activeBand?.type === DESSER_TYPE;
  const activeBandSupportsComp = Boolean(activeBand && supportsBandDynamics(activeBand));
  const activeBandHasCompTarget = Boolean(
    activeBandSupportsComp &&
    Math.abs((Number(activeBand.comp) || 0) - (Number(activeBand.gain) || 0)) > 0.05
  );
  const activeBandHasComp = Boolean(activeBand && bandHasDynamics(activeBand));
  const activeBandCompPanelVisible = activeBandSupportsComp && activeBandHasCompTarget;
  const activeBandDynamicsMode = activeBandCompPanelVisible && (Number(activeBand.comp) || 0) > (Number(activeBand.gain) || 0) ? 'EXP' : 'COMP';
  const spectrumValueToDb = (value) => clamp(Number(value) || 0, 0, 1) * 60 - 66;
  const detectorDbAt = (index) => {
    if (!Number.isInteger(index) || index < 0 || index >= inputDetectorDbs.length) return undefined;
    const value = inputDetectorDbs[index];
    return Number.isFinite(value) ? value : undefined;
  };
  const thresholdKneeEngagement = (levelDb, thresholdDb, kneeDb = 6) => {
    if (levelDb <= thresholdDb - kneeDb * 0.5) return 0;
    if (levelDb >= thresholdDb + kneeDb * 0.5) return 1;
    const t = clamp((levelDb - thresholdDb + kneeDb * 0.5) / kneeDb, 0, 1);
    return t * t * (3 - 2 * t);
  };
  const getBandSpectrumPeak = (band, visualBand = band) => {
    if (!band || inputSpectrum.length < 2) return 0;

    const frequency = clamp(visualBand?.freq || band.freq, 20, 20000);
    const centerT = (Math.log10(frequency) - Math.log10(20)) / (Math.log10(20000) - Math.log10(20));
    const q = clamp(band.q || DEFAULT_EQ_Q, 0.1, 50);
    const windowWidth = band.type === 'Low Shelf' || band.type === 'High Shelf'
      ? 0.22
      : clamp(0.018 + 0.11 / Math.sqrt(q), 0.018, 0.16);
    let peak = 0;

    inputSpectrum.forEach((value, index) => {
      const t = inputSpectrum.length <= 1 ? 0 : index / (inputSpectrum.length - 1);
      let weight = 0;
      if (band.type === 'Low Shelf') {
        weight = t <= centerT ? 1 : Math.max(0, 1 - (t - centerT) / windowWidth) ** 2;
      } else if (band.type === 'High Shelf') {
        weight = t >= centerT ? 1 : Math.max(0, 1 - (centerT - t) / windowWidth) ** 2;
      } else {
        const distance = Math.abs(t - centerT);
        if (distance > windowWidth) return;
        weight = (1 - distance / windowWidth) ** 2;
      }
      peak = Math.max(peak, value * weight);
    });

    return peak;
  };
  const getBandDetectorDb = (band, visualBand = band, index = undefined) => {
    const detectorDb = detectorDbAt(index);
    return Number.isFinite(detectorDb) ? detectorDb : spectrumValueToDb(getBandSpectrumPeak(band, visualBand));
  };
  const getBandDynamicEngagement = (band, visualBand = band, detectorDbOverride = undefined) => {
    if (!bandHasDynamics(band)) return 0;

    const dynamicRangeDb = Math.abs((Number(band.comp) || 0) - (Number(band.gain) || 0));
    if (dynamicRangeDb <= 0.05) return 0;

    const detectorDb = Number.isFinite(detectorDbOverride)
      ? detectorDbOverride
      : getBandDetectorDb(band, visualBand);
    const overDb = Math.max(0, detectorDb - (Number(band.compThreshold) || -18));
    const ratio = clamp(Number(band.compRatio) || 4, EQ_COMP_RATIO_MIN, EQ_COMP_RATIO_MAX);
    const ratioMoveDb = overDb * (1 - 1 / ratio);
    const ratioEngagement = clamp(ratioMoveDb / dynamicRangeDb, 0, 1);
    return clamp(thresholdKneeEngagement(detectorDb, Number(band.compThreshold) || -18) * ratioEngagement, 0, 1);
  };
  const activeBandDetectorDb = activeBandCompPanelVisible
    ? getBandDetectorDb(activeBand, activeDisplayBand, activeIdx)
    : -120;
  const withVisualDynamicGain = (visualPoint, sourcePoint = visualPoint, detectorDbOverride = undefined) => {
    const visualBand = withBandDefaults(visualPoint);
    const sourceBand = withBandDefaults(sourcePoint);
    const amount = getBandDynamicEngagement(sourceBand, visualBand, detectorDbOverride);
    if (amount <= 0.003) return visualBand;

    return {
      ...visualBand,
      gain: sourceBand.gain + (sourceBand.comp - sourceBand.gain) * amount
    };
  };
  const preDynamicDisplayPoints = preDisplayPoints.map((point, index) => (
    withVisualDynamicGain(point, prePoints[index] || point, mode === 'pre' ? detectorDbAt(index) : undefined)
  ));
  const postDynamicDisplayPoints = postDisplayPoints.map((point, index) => (
    withVisualDynamicGain(point, postPoints[index] || point, mode === 'post' ? detectorDbAt(index) : undefined)
  ));
  const dynamicDisplayPoints = mode === 'pre' ? preDynamicDisplayPoints : postDynamicDisplayPoints;
  const postCurve = buildSampledCurve(postDynamicDisplayPoints);
  const preCurve = buildSampledCurve(preDynamicDisplayPoints);
  const activeCurve = mode === 'pre' ? preCurve : postCurve;
  const activeCurveFill = `${activeCurve} L ${padL + innerW} ${baseY} L ${padL} ${baseY} Z`;
  const compInputFill = activeBandCompPanelVisible
    ? `${Math.round(clamp((activeBandDetectorDb - EQ_COMP_THRESHOLD_MIN) / (EQ_COMP_THRESHOLD_MAX - EQ_COMP_THRESHOLD_MIN), 0, 1) * 100)}%`
    : '0%';
  const setActiveCompThreshold = (event) => {
    if (!activeBandCompPanelVisible) return;
    const compThreshold = Number(clamp(Number(event.target.value), EQ_COMP_THRESHOLD_MIN, EQ_COMP_THRESHOLD_MAX).toFixed(1));
    applyPointPatch(activeIdx, { compThreshold }, { resort: false, updateRange: false });
  };
  const toggleActiveComp = () => {
    if (!activeBandCompPanelVisible) return;
    applyPointPatch(activeIdx, { compEnabled: !activeBand.compEnabled }, { resort: false, updateRange: false });
  };
  const toggleActiveMute = () => {
    if (activeIdx === null || activeIdx === undefined) return;
    const currentPoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    if (!currentPoints[activeIdx]) return;

    const current = withBandDefaults(currentPoints[activeIdx]);
    const nextOn = current.on === false;
    applyPointPatch(activeIdx, {
      on: nextOn,
      solo: nextOn ? current.solo : false
    }, { resort: false, updateRange: true });
  };
  const toggleActiveSolo = () => {
    if (activeIdx === null || activeIdx === undefined) return;
    const currentPoints = latestPointsRef.current.length ? latestPointsRef.current : points;
    if (!currentPoints[activeIdx]) return;

    const nextSolo = !withBandDefaults(currentPoints[activeIdx]).solo;
    const nextPts = currentPoints.map((point, index) => withBandDefaults({
      ...point,
      solo: index === activeIdx ? nextSolo : false
    }));

    latestPointsRef.current = nextPts;
    setPoints(nextPts);
    if (nextSolo) {
      const clearSolo = (list) => list.map((point) => withBandDefaults({ ...point, solo: false }));
      if (mode === 'pre') setPostPoints(clearSolo(postPoints));
      else setPrePoints(clearSolo(prePoints));
    }
    setActiveIdx(activeIdx);
  };
  const eqGridCenterX = padL + innerW * 0.5;
  const eqGridCenterY = padT + innerH * 0.5;
  const eqGridOvalRadius = innerH * 0.64;
  const eqGridOvalScale = Math.max(1, (innerW / innerH) * 0.70);
  const hoverPreview = (() => {
    if (!hoverPoint || dragIdx !== null || dragMode !== null || typeOpen) return null;

    const isNearExistingNode = points.some((point, index) => {
      const sourceBand = withBandDefaults(point);
      const visualBand = displayPoints[index] ? withBandDefaults(displayPoints[index]) : sourceBand;
      const liveX = freqToX(visualBand.freq);
      const liveY = gainToY(nodeGainAt(visualBand));
      const liveDistance = Math.hypot(hoverPoint.x - liveX, hoverPoint.y - liveY);

      if (liveDistance <= 20) return true;

      const isSurfing = sourceBand.type === 'Surfer Bell' && Math.abs(visualBand.freq - sourceBand.freq) > 0.5;
      if (!isSurfing) return false;

      const anchorX = freqToX(sourceBand.freq);
      const anchorY = gainToY(nodeGainAt(sourceBand));
      return Math.hypot(hoverPoint.x - anchorX, hoverPoint.y - anchorY) <= 20;
    });
    if (isNearExistingNode) return null;

    const freq = clamp(xToFreq(hoverPoint.x), 20, 20000);
    const type = getCreatedFilterType(freq);
    const totalY = gainToY(totalGain(displayPoints, freq));
    const nearTotalCurve = Math.abs(hoverPoint.y - totalY) < 20;

    if (!nearTotalCurve) return null;

    const hasAnyBand = prePoints.length > 0 || postPoints.length > 0;
    const showIcon = !hasAnyBand && canUseFilterType(type, null, points);

    return {
      freq,
      type,
      showIcon,
      x: freqToX(freq),
      y: totalY,
      iconX: clamp(freqToX(freq) - 18, padL + 6, padL + innerW - 42),
      iconY: Math.max(padT + 4, totalY - 42),
    };
  })();

  return (
    <div className="eq-container">
      <div className="eq-toolbar">
        <div className="seg">
          <button className={`seg-btn${mode === 'pre' ? ' active pre' : ''}`} onClick={() => { setMode('pre'); setActiveIdx(null); }}>PRE COMP</button>
          <button className={`seg-btn${mode === 'post' ? ' active post' : ''}`} onClick={() => { setMode('post'); setActiveIdx(null); }}>POST COMP</button>
        </div>
        <div className={`eq-sat${satMode ? ` active mode-${satMode}` : ''}`}>
          <button
            type="button"
            className="eq-sat-btn"
            aria-pressed={satMode > 0}
            title={satMode ? `Saturation ${EQ_SATURATION_LABELS[satMode]}` : 'Saturation off'}
            onClick={cycleSaturation}
          >
            <svg className="eq-sat-icon" width="12" height="12" viewBox="0 0 14 14" fill="none" aria-hidden="true">
              <path d="M2.2 10.1h2.2c1.1 0 1.7-.9 2.2-3.1.5-2.1 1.1-3.1 2.2-3.1H11.8" stroke="currentColor" strokeWidth="1.35" strokeLinecap="round" strokeLinejoin="round" />
              <path d="M2.4 4.2h2.1M9.5 9.8h2.1" stroke="currentColor" strokeWidth="1" strokeLinecap="round" opacity="0.55" />
            </svg>
            <span>{EQ_SATURATION_LABELS[satMode]}</span>
          </button>
          {satMode > 0 && (
            <input
              className="eq-sat-slider"
              type="range"
              min="0"
              max="100"
              value={satAmount}
              onChange={setSaturationAmount}
              aria-label="Saturation amount"
              style={{ '--sat-fill': `${satAmount}%` }}
            />
          )}
        </div>
      </div>

      <svg
        ref={svgRef}
        viewBox={`0 0 ${W} ${H}`}
        preserveAspectRatio="none"
        className="eq-svg"
        style={{ height: `${H}px` }}
        onPointerDown={onSvgPointerDown}
        onDoubleClick={onSvgDoubleClick}
        onPointerMove={onMove}
        onPointerUp={onUp}
        onPointerLeave={() => { onUp(); setHoverPoint(null); }}
      >
        <defs>
          <linearGradient id="postFill" gradientUnits="userSpaceOnUse" x1="0" x2="0" y1={activeFillFocusY} y2={baseY}>
            <stop offset="0%" stopColor="var(--curve-post)" stopOpacity="0.20" />
            <stop offset="58%" stopColor="var(--curve-post)" stopOpacity="0.055" />
            <stop offset="100%" stopColor="var(--curve-post)" stopOpacity="0" />
          </linearGradient>
          <linearGradient id="preFill" gradientUnits="userSpaceOnUse" x1="0" x2="0" y1={activeFillFocusY} y2={baseY}>
            <stop offset="0%" stopColor="var(--curve-pre)" stopOpacity="0.17" />
            <stop offset="58%" stopColor="var(--curve-pre)" stopOpacity="0.045" />
            <stop offset="100%" stopColor="var(--curve-pre)" stopOpacity="0" />
          </linearGradient>
          {displayPoints.map((point, i) => {
            const band = withBandDefaults(point);
            if (CUT_TYPES.has(band.type)) return null;

            const color = bandColor(points, i);
            const isActive = activeIdx === i;
            const isMuted = band.on === false;
            const fillColor = isMuted ? 'var(--ink-4)' : color;
            const x = freqToX(band.freq);
            const y = gainToY(nodeGainAt(band));

            return (
              <linearGradient key={`band-fill-${i}`} id={bandFillId(i)} gradientUnits="userSpaceOnUse" x1={x} x2={x} y1={y} y2={baseY}>
                <stop offset="0%" stopColor={fillColor} stopOpacity={isMuted ? (isActive ? 0.09 : 0.052) : isActive ? 0.255 : 0.145} />
                <stop offset="52%" stopColor={fillColor} stopOpacity={isMuted ? (isActive ? 0.032 : 0.020) : isActive ? 0.095 : 0.052} />
                <stop offset="100%" stopColor={fillColor} stopOpacity={isMuted ? 0.006 : isActive ? 0.018 : 0.010} />
              </linearGradient>
            );
          })}
          {displayPoints.map((point, i) => {
            const source = withBandDefaults(point);
            const mirrorGain = clamp(Number(source.comp) || 0, -scale, scale);
            if (!bandHasDynamics(source)) return null;

            const mirrorPoint = { ...source, gain: mirrorGain };
            const x = freqToX(mirrorPoint.freq);
            const y = gainToY(nodeGainAt(mirrorPoint));
            const isActive = activeIdx === i;

            return (
              <linearGradient key={`comp-fill-${i}`} id={compMirrorFillId(i)} gradientUnits="userSpaceOnUse" x1={x} x2={x} y1={y} y2={baseY}>
                <stop offset="0%" stopColor="var(--warm)" stopOpacity={isActive ? 0.230 : 0.125} />
                <stop offset="52%" stopColor="var(--warm)" stopOpacity={isActive ? 0.085 : 0.045} />
                <stop offset="100%" stopColor="var(--warm)" stopOpacity={isActive ? 0.016 : 0.009} />
              </linearGradient>
            );
          })}
          <linearGradient id="spectrumFill" x1="0" x2="0" y1="0" y2="1">
            <stop offset="0%" stopColor={`rgb(${spectrumRgb})`} stopOpacity={isDarkMode ? 0.16 : 0.11} />
            <stop offset="68%" stopColor={`rgb(${spectrumRgb})`} stopOpacity={isDarkMode ? 0.055 : 0.04} />
            <stop offset="100%" stopColor={`rgb(${spectrumRgb})`} stopOpacity="0" />
          </linearGradient>
          <filter id="curveGlow" x="-5%" y="-50%" width="110%" height="200%">
            <feGaussianBlur stdDeviation="2" result="blur" />
            <feMerge>
              <feMergeNode in="blur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <clipPath id="eqGraphClip">
            <rect x={padL} y={padT} width={innerW} height={innerH} />
          </clipPath>
          <radialGradient
            id="eqGridOvalGradient"
            gradientUnits="userSpaceOnUse"
            cx={eqGridCenterX}
            cy={eqGridCenterY}
            r={eqGridOvalRadius}
            gradientTransform={`translate(${eqGridCenterX} ${eqGridCenterY}) scale(${eqGridOvalScale} 1) translate(${-eqGridCenterX} ${-eqGridCenterY})`}
          >
            <stop offset="0%" stopColor="white" stopOpacity="1" />
            <stop offset="38%" stopColor="white" stopOpacity="0.96" />
            <stop offset="68%" stopColor="white" stopOpacity="0.48" />
            <stop offset="100%" stopColor="white" stopOpacity="0" />
          </radialGradient>
          <mask id="eqGridOvalMask" maskUnits="userSpaceOnUse" x={padL} y={padT} width={innerW} height={innerH}>
            <rect x={padL} y={padT} width={innerW} height={innerH} fill="url(#eqGridOvalGradient)" />
          </mask>
        </defs>

        {/* Background hit-zone for adding nodes */}
        <rect x={padL} y={padT} width={innerW} height={innerH} fill="transparent" className="eq-bg-hit" />

        {/* Grid */}
        <g mask="url(#eqGridOvalMask)">
          {metricFreqMarks.map(f => {
            const x = freqToX(f);
            const isMajor = f === 100 || f === 1000 || f === 10000;
            const isLabeled = freqMarks.includes(f);
            const hover = hoverPoint ? Math.max(0, 1 - Math.abs(x - hoverPoint.x) / (innerW * 0.16)) ** 1.8 : 0;
            const lift = hover * 1.2;
            const alpha = (isMajor ? 0.150 : isLabeled ? 0.105 : 0.068) + hover * (isMajor ? 0.050 : 0.034);
            const strokeWidth = (isMajor ? 0.82 : isLabeled ? 0.62 : 0.44) + hover * 0.10;

            return (
              <line
                key={f}
                x1={x}
                y1={padT - lift}
                x2={x}
                y2={padT + innerH + lift}
                className="eq-bg-hit"
                stroke={eqGridStroke(alpha, isMajor ? 'major' : 'minor')}
                strokeWidth={strokeWidth}
              />
            );
          })}
          {gainMarks.map(g => {
            const y = gainMarkToY(g);
            const isZero = g === 0;
            const hover = hoverPoint ? Math.max(0, 1 - Math.abs(y - hoverPoint.y) / (innerH * 0.18)) ** 1.8 : 0;
            const push = hover * 1.4;
            const alpha = (isZero ? 0.210 : 0.074) + hover * (isZero ? 0.052 : 0.034);

            return (
              <line
                key={g}
                x1={padL - push}
                y1={y}
                x2={padL + innerW + push}
                y2={y}
                className="eq-bg-hit"
                stroke={eqGridStroke(alpha, isZero ? 'zero' : 'minor')}
                strokeWidth={(isZero ? 0.92 : 0.50) + hover * 0.09}
              />
            );
          })}
        </g>

        <line
          x1={dbAxisX}
          y1={padT}
          x2={dbAxisX}
          y2={padT + innerH}
          className="eq-bg-hit"
          stroke={eqGridStroke(0.180, 'zero')}
          strokeWidth="0.95"
        />
        {gainMarks.filter(g => g !== scale).map(g => {
          const y = gainMarkToY(g);
          const isZero = g === 0;

          return (
            <g key={`db-${g}`} className="eq-bg-hit">
              <line
                x1={dbAxisX - 3}
                y1={y}
                x2={dbAxisX + (isZero ? 6 : 4)}
                y2={y}
                stroke={eqGridStroke(isZero ? 0.260 : 0.165, isZero ? 'zero' : 'minor')}
                strokeWidth={isZero ? 0.95 : 0.62}
              />
              <text
                x={dbAxisX + 7}
                y={y + 3}
                textAnchor="start"
                className={`eq-db-label${isZero ? ' zero' : ''}`}
              >
                {formatDbLabel(g)}
              </text>
            </g>
          );
        })}

        <g clipPath="url(#eqGraphClip)">
          {showWaveform && spectrumShape.fill && (
            <>
              <path d={spectrumShape.fill} className="eq-bg-hit" fill="url(#spectrumFill)" />
              <path d={spectrumShape.line} fill="none" className="eq-bg-hit" stroke={spectrumStroke}
                    strokeWidth="1.05" strokeLinejoin="round" strokeLinecap="round"
                    shapeRendering="geometricPrecision" />
            </>
          )}

          {/* Per-band individual curves with fills (active mode only) */}
          {displayPoints.map((p, i) => {
            const { line, fill, isCut } = buildBandCurve(dynamicDisplayPoints, i);
            const color = bandColor(points, i);
            const isActive = activeIdx === i;
            const source = withBandDefaults(points[i] || p);
            const isMuted = source.on === false;
            const curveColor = isMuted ? 'var(--ink-4)' : color;
            const detectorDb = detectorDbAt(i);
            const isDynamic = bandHasDynamics(source) && getBandDynamicEngagement(source, withBandDefaults(p), detectorDb) > 0.003;
            return (
              <g key={`band-${i}`}>
                {!isCut && fill && (
                  <path
                    d={fill}
                    className="eq-band-fill"
                    fill={`url(#${bandFillId(i)})`}
                    pointerEvents="visiblePainted"
                    style={{ cursor: 'pointer' }}
                    onPointerDown={onBandSelectDown(i)}
                    onDoubleClick={(e) => { e.stopPropagation(); e.preventDefault(); }}
                  />
                )}
                <path d={line} fill="none" stroke={curveColor}
                      className="eq-band-line"
                      strokeWidth={isDynamic ? (isActive ? 1.65 : 1.15) : (isActive ? 1.1 : 0.8)}
                      strokeDasharray={isActive ? '0' : '2 3'}
                      opacity={isMuted ? (isActive ? 0.32 : 0.20) : isDynamic ? (isActive ? 0.95 : 0.58) : (isActive ? 0.7 : 0.35)}
                      pointerEvents="visibleStroke"
                      style={{ cursor: 'pointer' }}
                      onPointerDown={onBandSelectDown(i)}
                      onDoubleClick={(e) => { e.stopPropagation(); e.preventDefault(); }} />
              </g>
            );
          })}
          {displayPoints.map((p, i) => {
            const source = withBandDefaults(points[i] || p);
            const visualBand = withBandDefaults(p);
            const isActive = activeIdx === i;
            const includeMuted = isActive && activeBandCompPanelVisible;
            const pointHasDynamics = bandHasDynamics(source);
            const mirror = buildCompMirrorCurve(visualBand, includeMuted);
            if (!mirror.active) return null;
            const detectorDb = detectorDbAt(i);
            const motion = buildCompMotionCurve(visualBand, getBandDynamicEngagement(source, visualBand, detectorDb));
            return (
              <g key={`comp-mirror-${i}`} pointerEvents="none">
                <path d={mirror.fill} fill={`url(#${compMirrorFillId(i)})`} opacity={pointHasDynamics ? (isActive ? 0.44 : 0.26) : 0.20} />
                <path
                  d={mirror.line}
                  fill="none"
                  stroke="var(--warm)"
                  strokeWidth={isActive ? 1.1 : 0.85}
                  strokeDasharray={pointHasDynamics ? '3.5 4.5' : '2.5 5'}
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  opacity={pointHasDynamics ? (isActive ? 0.70 : 0.42) : 0.34}
                />
                {motion.active && (
                  <>
                    <path d={motion.fill} fill={activeColor} opacity={isActive ? 0.30 : 0.16} />
                    <path
                      d={motion.line}
                      fill="none"
                      stroke={activeColor}
                      strokeWidth={isActive ? 1.7 : 1.15}
                      strokeLinecap="round"
                      strokeLinejoin="round"
                      opacity={isActive ? 0.96 : 0.56}
                    />
                    {isActive && (
                      <circle cx={motion.cx} cy={motion.cy} r="3.1" fill="var(--warm)" stroke="var(--panel)" strokeWidth="0.8" opacity="0.98" />
                    )}
                  </>
                )}
              </g>
            );
          })}

          {/* Active total curve */}
          {!activeHasCut && <path d={activeCurveFill} fill={`url(#${activeFillId})`} pointerEvents="none" />}
          <path d={activeCurve} fill="none" stroke={activeColor}
                strokeWidth="2.35"
                strokeLinejoin="round" strokeLinecap="round"
                opacity="0.96"
                pointerEvents="none" />
        </g>

        {hoverPreview && (
          <g className="eq-hover-preview" pointerEvents="none">
            <circle cx={hoverPreview.x} cy={hoverPreview.y} r="13" fill={activeColor} opacity="0.08" />
            <circle cx={hoverPreview.x} cy={hoverPreview.y} r="6" fill="var(--panel)" stroke={activeColor}
                    strokeWidth="1.35" opacity="0.94"
                    style={{ filter: 'drop-shadow(0 1px 2px rgba(0,0,0,0.12))' }} />
            <circle cx={hoverPreview.x} cy={hoverPreview.y} r="2" fill={activeColor} opacity="0.82" />
            {hoverPreview.showIcon && (
              <g transform={`translate(${hoverPreview.iconX} ${hoverPreview.iconY})`}
                 style={{ filter: 'drop-shadow(0 4px 10px rgba(20,30,55,0.08))' }}>
                <rect x="0" y="0" width="36" height="26" rx="13"
                      fill="var(--panel)" stroke={activeColor} strokeWidth="0.65" opacity="0.96" />
                <path d="M11 18.3H25" stroke={activeColor} strokeWidth="0.7" strokeLinecap="round" opacity="0.24" />
                <path d={getFilterTypePath(hoverPreview.type)}
                      transform="translate(9 5)"
                      fill="none" stroke={activeColor} strokeWidth="1.35"
                      strokeLinecap="round" strokeLinejoin="round" />
              </g>
            )}
          </g>
        )}

        {/* Freq labels */}
        {freqMarks.map(f => (
          <text key={f} x={freqToX(f)} y={freqLabelY} textAnchor="middle" className="eq-label-text">
            {f < 1000 ? f : (f / 1000) + 'k'}
          </text>
        ))}

        {/* Scale selector replaces the topmost dB label at the top of the axis. */}
        <foreignObject x={dbAxisX - 24} y={padT - 11} width="60" height="22"
                       style={{ overflow: 'visible' }}>
          <div xmlns="http://www.w3.org/1999/xhtml" className="eq-scale-wrap" ref={scaleRef}
               style={{ position: 'relative' }}>
            <button className="eq-scale-btn" onClick={() => setScaleOpen(o => !o)}>
              <span>{scale} dB</span>
              <svg width="8" height="8" viewBox="0 0 10 10"><path d="M2 4l3 3 3-3" stroke="currentColor" strokeWidth="1.3" fill="none" strokeLinecap="round" /></svg>
            </button>
            {scaleOpen && (
              <div className="eq-scale-menu">
                {scaleOptions.map(s => (
                  <button key={s} className={`eq-scale-item${s === scale ? ' active' : ''}`}
                    onClick={() => { setScale(s); setScaleOpen(false); }}>{s} dB</button>
                ))}
              </div>
            )}
          </div>
        </foreignObject>


        {/* Nodes */}
        {points.map((p, i) => {
          const sourceBand = withBandDefaults(p);
          const visualBand = displayPoints[i] || sourceBand;
          const isActive = activeIdx === i;
          const isDragging = dragIdx === i;
          const color = bandColor(points, i);
          const isMuted = sourceBand.on === false;
          const nodeColor = isMuted ? 'var(--ink-4)' : color;
          const isSurfing = sourceBand.type === 'Surfer Bell' && Math.abs(visualBand.freq - sourceBand.freq) > 0.5;
          const isSurferBell = sourceBand.type === 'Surfer Bell';
          const editSurfAnchor = isSurferBell && isDragging;
          const band = withBandDefaults(editSurfAnchor ? sourceBand : visualBand);
          const liveX = freqToX(visualBand.freq);
          const liveY = gainToY(nodeGainAt(visualBand));
          const anchorX = freqToX(sourceBand.freq);
          const anchorY = gainToY(nodeGainAt(sourceBand));
          const x = editSurfAnchor ? anchorX : liveX;
          const y = editSurfAnchor ? anchorY : liveY;
          const isDesser = band.type === DESSER_TYPE;
          const showCompArrow = isActive && supportsBandDynamics(band);
          const compGain = clamp(Number(band.comp) || 0, -scale, scale);
          const isCompDragging = isDragging && dragMode === 'comp';
          const hasCompControl = showCompArrow && (bandHasDynamics(band) || isCompDragging);
          const compPoint = hasCompControl ? { ...band, gain: compGain } : null;
          const compY = compPoint ? gainToY(nodeGainAt(compPoint)) : y;
          const compDistanceFromNode = Math.abs(compY - y);
          const compArrowBlend = clamp((compDistanceFromNode - 12) / 34, 0, 1);
          const compArrowGap = 29 + (9 - 29) * compArrowBlend;
          const keepClearOfNode = (value, fallbackSign) => {
            const minClearance = 23;
            const delta = value - y;
            if (Math.abs(delta) >= minClearance) return value;
            return y + (delta === 0 ? fallbackSign : Math.sign(delta)) * minClearance;
          };
          const arrowTopY = keepClearOfNode(compY - compArrowGap, -1);
          const arrowBottomY = keepClearOfNode(compY + compArrowGap, 1);
          const topArrowPath = 'M 0 -3.6 L 3.6 3 L -3.6 3 Z';
          const bottomArrowPath = 'M 0 3.6 L 3.6 -3 L -3.6 -3 Z';

          return (
            <g key={i}>
              {isSurfing && !isDesser && (
                <g className="eq-surf-link">
                  <line x1={anchorX} y1={anchorY} x2={liveX} y2={liveY}
                        pointerEvents="none"
                        stroke={nodeColor} strokeWidth="1.05" strokeDasharray="4 5" opacity={isMuted ? 0.18 : editSurfAnchor ? 0.46 : 0.34} />
                  <circle cx={anchorX} cy={anchorY} r={editSurfAnchor ? 9.2 : 7.4}
                          pointerEvents="none"
                          fill={nodeColor} opacity={isMuted ? 0.10 : editSurfAnchor ? 0.22 : 0.16} />
                  <circle cx={anchorX} cy={anchorY} r={editSurfAnchor ? 5.8 : 5.1}
                          pointerEvents="none"
                          fill="var(--panel)" stroke={nodeColor}
                          strokeWidth={editSurfAnchor ? 1.7 : 1.45}
                          strokeDasharray="2.2 2.6" opacity={isMuted ? 0.54 : 0.96}
                          style={{ color: nodeColor, filter: 'drop-shadow(0 0 5px currentColor)' }} />
                  <circle cx={anchorX} cy={anchorY} r="1.8" fill={nodeColor} opacity={isMuted ? 0.46 : 0.95} pointerEvents="none" />
                  <circle className="eq-surf-ghost-hit" cx={anchorX} cy={anchorY} r="15"
                          fill="transparent" style={{ cursor: 'grab' }}
                          onPointerDown={onSurferGhostDown(i, visualBand.freq)}
                          onContextMenu={onNodeContext(i)} />
                  {editSurfAnchor && (
                    <circle cx={liveX} cy={liveY} r="3.4" fill={nodeColor} opacity={isMuted ? 0.28 : 0.64} pointerEvents="none" />
                  )}
                </g>
              )}

              {/* Subtle glow when active */}
              {(isActive || isDragging) && <circle cx={x} cy={y} r="14" fill={nodeColor} opacity={isMuted ? 0.06 : 0.10} />}
              {isSurfing && !isDesser && <circle cx={x} cy={y} r={isActive ? 10 : 8}
                                                fill="none" stroke={nodeColor} strokeWidth="0.75"
                                                opacity={isMuted ? 0.16 : isActive ? 0.42 : 0.25} />}

              {/* Main node */}
              <circle cx={x} cy={y} r={isActive ? 6 : 5} fill="white" stroke={nodeColor} strokeWidth={isActive ? 1.8 : 1.3}
                      style={{ filter: 'drop-shadow(0 1px 2px rgba(0,0,0,0.14))' }} />
              <circle cx={x} cy={y} r="2" fill={nodeColor} opacity={isMuted ? 0.42 : isActive ? 1 : 0.6} />
              {!isSurferBell && (
                <circle className="eq-node-hit" cx={x} cy={y} r="16" fill="transparent" style={{ cursor: 'grab' }}
                        onPointerDown={onNodeDown(i)} onContextMenu={onNodeContext(i)}
                        onWheel={(e) => onNodeWheel(i, e)} />
              )}
              {isSurferBell && !isSurfing && (
                <circle className="eq-node-hit" cx={x} cy={y} r="16" fill="transparent" style={{ cursor: 'grab' }}
                        onPointerDown={onSurferGhostDown(i, visualBand.freq)} onContextMenu={onNodeContext(i)}
                        onWheel={(e) => onNodeWheel(i, e)} />
              )}
              {showCompArrow && (
                <>
                  <g
                    className="eq-comp-arrow top"
                    transform={`translate(${x} ${arrowTopY})`}
                    style={{ cursor: 'ns-resize' }}
                    onPointerDown={onCompDown(i, 1)}
                    onContextMenu={onNodeContext(i)}
                  >
                    <path d={topArrowPath} fill={nodeColor} opacity={isMuted ? 0.18 : 0.52} />
                    <circle r="8.5" fill="transparent" />
                  </g>
                  <g
                    className="eq-comp-arrow bottom"
                    transform={`translate(${x} ${arrowBottomY})`}
                    style={{ cursor: 'ns-resize' }}
                    onPointerDown={onCompDown(i, -1)}
                    onContextMenu={onNodeContext(i)}
                  >
                    <path d={bottomArrowPath} fill={nodeColor} opacity={isMuted ? 0.18 : 0.52} />
                    <circle r="8.5" fill="transparent" />
                  </g>
                </>
              )}

            </g>
          );
        })}
      </svg>

      {activeBand && (
        <div className={`eq-node-info${activeBandIsDesser ? ' has-desser' : ''}`} ref={nodeInfoRef}>
          <button
            type="button"
            className={`eq-solo-btn${activeBand.solo ? ' active' : ''}`}
            aria-pressed={activeBand.solo}
            aria-label="Solo band"
            title="Solo band"
            onClick={toggleActiveSolo}
          >
            S
          </button>
          <button
            type="button"
            className={`eq-mute-btn${activeBand.on === false ? ' muted' : ''}`}
            aria-pressed={activeBand.on === false}
            aria-label={activeBand.on === false ? 'Unmute band' : 'Mute band'}
            title={activeBand.on === false ? 'Unmute band' : 'Mute band'}
            onClick={toggleActiveMute}
          >
            M
          </button>
          <button type="button" className="eq-info-pair eq-info-drag freq" onPointerDown={onInfoDragStart('freq')} aria-label="Frequency">
            <b>F</b>{formatFreq(activeBand.type === 'Surfer Bell' ? activeDisplayBand.freq : activeBand.freq)}
          </button>
          {activeBandIsDesser ? (
            <>
              <button type="button" className="eq-info-pair eq-info-drag threshold" onPointerDown={onInfoDragStart('threshold')} aria-label="Desser threshold">
                <b>THR</b>{formatThreshold(activeBand.threshold)}
              </button>
              <button type="button" className="eq-info-pair eq-info-drag intensity" onPointerDown={onInfoDragStart('intensity')} aria-label="Desser intensity">
                <b>INT</b>{formatIntensity(activeBand.intensity)}
              </button>
              <div className="eq-desser-mode" aria-label="Desser mode">
                {[
                  ['split', 'Split'],
                  ['wider', 'Wider']
                ].map(([value, label]) => (
                  <button
                    key={value}
                    type="button"
                    className={activeBand.deessMode === value ? 'active' : ''}
                    onClick={() => applyPointPatch(activeIdx, { deessMode: value }, { resort: false, updateRange: true })}
                  >
                    {label}
                  </button>
                ))}
              </div>
            </>
          ) : (
            <>
              <button type="button" className="eq-info-pair eq-info-drag gain" onPointerDown={onInfoDragStart('gain')} aria-label="Gain">
                <b>G</b>{formatGain(activeBand.gain)}
              </button>
              {CUT_TYPES.has(activeBand.type) ? (
                <button type="button" className="eq-info-pair eq-info-drag slope" onPointerDown={onInfoDragStart('slope')} aria-label="Cut slope">
                  <b>S</b>{formatSlope(activeBand.slope)}
                </button>
              ) : (
                <button type="button" className="eq-info-pair eq-info-drag q" onPointerDown={onInfoDragStart('q')} aria-label="Q">
                  <b>Q</b>{activeBand.q.toFixed(2)}
                </button>
              )}
            </>
          )}
          <div className={`eq-type-control${typeOpen ? ' open' : ''}`}>
            <button
              type="button"
              className="eq-type-btn"
              aria-label="Band type"
              aria-expanded={typeOpen}
              onClick={(e) => {
                e.stopPropagation();
                setTypeOpen(open => !open);
              }}
            >
              <FilterTypeIcon type={activeBand.type} />
              <span className="eq-type-value">{activeBand.type}</span>
            </button>
            {typeOpen && (
              <div className="eq-type-menu">
                {getSelectableFilterTypes(activeIdx).map((type) => (
                  <button
                    key={type}
                    type="button"
                    className={`eq-type-item${activeBand.type === type ? ' active' : ''}`}
                    onClick={() => {
                      const [minQ, maxQ] = getQRange({ ...activeBand, type });
                      const nextIsDesser = type === DESSER_TYPE;
                      const typeChanged = activeBand.type !== type;
                      const nextSupportsDynamics = supportsBandDynamics({ ...activeBand, type });
                      applyPointPatch(activeIdx, {
                        type,
                        gain: nextIsDesser || CUT_TYPES.has(type) ? 0 : activeBand.gain,
                        q: clamp(typeChanged ? getDefaultQForType(type) : activeBand.q, minQ, maxQ),
                        slope: typeChanged ? getDefaultSlopeForType(type) : normalizeSlope(activeBand.slope, type),
                        threshold: activeBand.threshold,
                        intensity: activeBand.intensity,
                        deessMode: activeBand.deessMode,
                        comp: nextSupportsDynamics ? activeBand.comp : 0,
                        compEnabled: nextSupportsDynamics ? activeBand.compEnabled : false,
                        compThreshold: activeBand.compThreshold,
                        compAttack: activeBand.compAttack,
                        compRelease: activeBand.compRelease,
                        compRatio: activeBand.compRatio
                      }, { resort: false, updateRange: true });
                      setTypeOpen(false);
                    }}
                  >
                    <FilterTypeIcon type={type} />
                    <span className="eq-type-value">{type}</span>
                  </button>
                ))}
              </div>
            )}
          </div>
          {activeBandCompPanelVisible && (
            <div
              className={`eq-comp-panel ${activeBandDynamicsMode.toLowerCase()}${activeBandHasComp ? '' : ' muted'}`}
              style={{ '--eq-comp-audio-fill': compInputFill }}
            >
              <button
                type="button"
                className="eq-comp-tab"
                aria-pressed={activeBandHasComp}
                title={activeBandHasComp ? 'Mute band compression' : 'Enable band compression'}
                onClick={toggleActiveComp}
              >
                {activeBandHasComp ? activeBandDynamicsMode : 'OFF'}
              </button>
              <label className="eq-comp-fader" aria-label="EQ dynamics threshold">
                <span>THR</span>
                <span className="eq-comp-meter">
                  <span className="eq-comp-meter-fill" />
                  <input
                    type="range"
                    min={EQ_COMP_THRESHOLD_MIN}
                    max={EQ_COMP_THRESHOLD_MAX}
                    step="0.1"
                    value={activeBand.compThreshold}
                    onChange={setActiveCompThreshold}
                  />
                </span>
                <b>{formatThreshold(activeBand.compThreshold)}</b>
              </label>
              <button type="button" className="eq-info-pair eq-info-drag comp-attack" onPointerDown={onInfoDragStart('compAttack')} aria-label="EQ dynamics attack">
                <b>ATK</b>{formatMs(activeBand.compAttack)}
              </button>
              <button type="button" className="eq-info-pair eq-info-drag comp-release" onPointerDown={onInfoDragStart('compRelease')} aria-label="EQ dynamics release">
                <b>REL</b>{formatMs(activeBand.compRelease)}
              </button>
              <button type="button" className="eq-info-pair eq-info-drag comp-ratio" onPointerDown={onInfoDragStart('compRatio')} aria-label="EQ dynamics ratio">
                <b>RTO</b>{formatRatio(activeBand.compRatio)}
              </button>
            </div>
          )}
          <button type="button" className="eq-node-delete" onClick={deleteActivePoint} disabled={points.length <= 0} aria-label="Eliminar banda" title="Eliminar banda">
            <span aria-hidden="true" style={{ display: 'inline-block', transform: 'translateY(-0.5px)' }}>
              ×
            </span>
          </button>
        </div>
      )}
    </div>
  );
}

export { EQCurve };
