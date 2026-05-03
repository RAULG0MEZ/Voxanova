import React from "react";
import { BypassBtn, Select } from "./controls.jsx";
import { Knob } from "./knob.jsx";
import { autoTuneNotes, autoTuneScales } from "../pluginContract.js";

// AutoTune module — minimal compact

const AT_NOTES = autoTuneNotes;
const AT_SCALE_OPTIONS = autoTuneScales;

function maskToNotes(mask) {
  const rawMask = Number(mask);
  const numericMask = Number.isFinite(rawMask) ? Math.max(0, Math.min(4095, Math.round(rawMask))) : 4095;
  return Object.fromEntries(AT_NOTES.map((note, index) => [note, (numericMask & (1 << index)) !== 0]));
}

function notesToMask(notes) {
  return AT_NOTES.reduce((mask, note, index) => (
    notes?.[note] !== false ? mask | (1 << index) : mask
  ), 0);
}

function formatRetunePitch(value) {
  const amount = Math.max(0, Math.min(1, Number(value || 0) / 100));
  const retuneMs = Math.pow(1 - amount, 2.65) * 400;
  return `${retuneMs < 10 ? retuneMs.toFixed(1) : Math.round(retuneMs)}ms`;
}

function retuneVisualTiming(value) {
  const amount = Math.max(0, Math.min(1, Number(value || 0) / 100));
  const slowProgress = Math.pow(1 - amount, 1.35);
  const markerMs = Math.round(145 + slowProgress * 655);

  return {
    markerMs,
  };
}

function AutoTunePitchViz({ active, amount, key_, livePitch }) {
  const livePitchActive = active && Number(livePitch?.confidence) > 1;
  const retuneTiming = React.useMemo(() => retuneVisualTiming(amount), [amount]);

  const displayPitch = React.useMemo(() => {
    if (!livePitchActive) {
      return {
        note: '--',
        octave: '',
        cents: 0,
        confidence: 0,
      };
    }

    const targetMidi = Number(livePitch?.targetMidi) || (
      Number(livePitch?.frequency) > 0
        ? 69 + 12 * Math.log2(Number(livePitch.frequency) / 440)
        : 60
    );
    const roundedMidi = Math.round(targetMidi);
    const noteIndex = ((roundedMidi % 12) + 12) % 12;

    return {
      note: AT_NOTES[noteIndex] || key_,
      octave: Math.floor(roundedMidi / 12) - 1,
      cents: Math.max(-100, Math.min(100, Number(livePitch?.cents) || 0)),
      confidence: Math.max(0, Math.min(100, Math.round(Number(livePitch?.confidence) || 0))),
    };
  }, [key_, livePitch, livePitchActive]);

  const signalVisible = active && livePitchActive;
  const cents = signalVisible ? Math.round(displayPitch.cents) : 0;
  const marker = signalVisible ? Math.max(-42, Math.min(42, cents * 1.4)) : 0;

  return (
    <div className={`at-pitch-viz${!active || !livePitchActive ? ' idle' : ''}`}>
      <div className="at-pitch-grid" />
      <div className="at-pitch-shift" aria-hidden="true">
        <span>PITCH</span>
        <b>+0</b>
      </div>
      <div className="at-pitch-main">
        <span className="at-pitch-note">
          {signalVisible ? displayPitch.note : '--'}<small>{signalVisible ? displayPitch.octave : ''}</small>
        </span>
      </div>
      <div className="at-tuner-row">
        <span className="at-pitch-readout at-cents">{signalVisible ? `${cents > 0 ? '+' : ''}${cents}c` : 'IDLE'}</span>
        <div className="at-tuner">
          <span className="at-tuner-center" />
          <span className="at-tuner-band" />
          <span
            className="at-tuner-marker"
            style={{
              '--pitch-marker': `${marker}px`,
              '--pitch-marker-speed': `${retuneTiming.markerMs}ms`,
            }}
          />
        </div>
        <span className="at-pitch-readout at-confidence">{signalVisible ? `${displayPitch.confidence}%` : 'NO SIGNAL'}</span>
      </div>
    </div>
  );
}

function CustomNoteGrid({ notes, onToggle, disabled }) {
  return (
    <div className={`at-custom-notes${disabled ? ' disabled' : ''}`}>
      {AT_NOTES.map(note => {
        const active = notes?.[note] !== false;
        return (
          <button
            key={note}
            type="button"
            className={`at-note-tile${active ? ' active' : ' muted'}`}
            onClick={() => onToggle(note)}
            disabled={disabled}
            aria-pressed={active}
          >
            {note}
          </button>
        );
      })}
    </div>
  );
}

// AutoTune: pitch display + knob + key/scale selectors, no waveform
function AutoTuneModule({
  amount,
  setAmount,
  key_,
  setKey,
  scale_,
  setScale,
  customMask,
  setCustomMask,
  on,
  setOn,
  signalActive,
  livePitch,
}) {
  const [customNotes, setCustomNotes] = React.useState(() => (
    maskToNotes(customMask)
  ));
  const active = signalActive && on;

  React.useEffect(() => {
    setCustomNotes(maskToNotes(customMask));
  }, [customMask]);

  const toggleCustomNote = (note) => {
    setCustomNotes(current => {
      const next = { ...current, [note]: current[note] === false };
      setCustomMask?.(notesToMask(next));
      return next;
    });
  };

  return (
    <div className={`io-unit at-unit${!on ? ' bypassed' : ''}`}>
      <div className="io-unit-header">
        <BypassBtn on={on} onChange={setOn} />
        <span className="mod-name">FAIRY DUST TUNE</span>
        <span className="io-unit-val">{formatRetunePitch(amount)}</span>
      </div>
      <AutoTunePitchViz
        active={active}
        amount={amount}
        key_={key_}
        livePitch={livePitch}
      />
      <div className="at-body">
        <Knob value={amount} onChange={setAmount} min={0} max={100} size={44}
              defaultValue={82} color="var(--accent)" disabled={!on}
              label="RETUNE"
              format={formatRetunePitch} />
        <div className="at-selects">
          <Select value={key_} onChange={setKey} options={AT_NOTES} floating />
          <Select value={scale_} onChange={setScale} options={AT_SCALE_OPTIONS} floating />
        </div>
      </div>
      {scale_ === 'CUSTOM' && (
        <CustomNoteGrid notes={customNotes} onToggle={toggleCustomNote} disabled={!on} />
      )}
    </div>
  );
}

export { AutoTuneModule, AutoTunePitchViz };
