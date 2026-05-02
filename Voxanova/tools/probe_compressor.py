#!/usr/bin/env python3
import argparse
import json
import math
import struct
from pathlib import Path

import numpy as np


def db_to_gain(db):
    return 10.0 ** (db / 20.0)


def gain_to_db(gain):
    gain = max(float(gain), 1.0e-12)
    return 20.0 * math.log10(gain)


def clamp(value, low, high):
    return max(low, min(high, value))


def soft_clip(samples, drive):
    return np.tanh(samples * drive) / math.tanh(drive)


def read_float_wav(path):
    data = Path(path).read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("Expected a RIFF/WAVE file")

    offset = 12
    fmt = None
    pcm = None
    while offset + 8 <= len(data):
        chunk_id = data[offset:offset + 4]
        chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
        chunk_start = offset + 8
        chunk_end = chunk_start + chunk_size

        if chunk_id == b"fmt ":
            format_tag, channels, sample_rate, byte_rate, block_align, bits = struct.unpack_from(
                "<HHIIHH", data, chunk_start
            )
            fmt = {
                "format_tag": format_tag,
                "channels": channels,
                "sample_rate": sample_rate,
                "byte_rate": byte_rate,
                "block_align": block_align,
                "bits": bits,
            }
        elif chunk_id == b"data":
            pcm = data[chunk_start:chunk_end]

        offset = chunk_end + (chunk_size % 2)

    if fmt is None or pcm is None:
        raise ValueError("Missing fmt or data chunk")
    if fmt["format_tag"] != 3 or fmt["bits"] != 32:
        raise ValueError(f"Only 32-bit float WAV is supported here, got {fmt}")

    audio = np.frombuffer(pcm, dtype="<f4").astype(np.float32)
    audio = audio.reshape((-1, fmt["channels"]))
    return audio, fmt["sample_rate"], fmt


def write_float_wav(path, audio, sample_rate):
    audio = np.asarray(audio, dtype="<f4")
    frames, channels = audio.shape
    data = audio.tobytes()
    fmt_chunk_size = 16
    byte_rate = sample_rate * channels * 4
    block_align = channels * 4
    riff_size = 4 + (8 + fmt_chunk_size) + (8 + len(data))

    with Path(path).open("wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", riff_size))
        f.write(b"WAVE")
        f.write(b"fmt ")
        f.write(struct.pack("<IHHIIHH", fmt_chunk_size, 3, channels, sample_rate, byte_rate, block_align, 32))
        f.write(b"data")
        f.write(struct.pack("<I", len(data)))
        f.write(data)


class CompressorState:
    def __init__(self):
        self.envelope = 0.0
        self.gain = 1.0


def generic_compressor(stereo, sample_rate, threshold_db, ratio, amount_percent, attack_ms, release_ms, knee_db):
    state = CompressorState()
    out = np.empty_like(stereo)
    reduction = np.empty(stereo.shape[0], dtype=np.float32)
    amount = amount_percent / 100.0

    for i, (left, right) in enumerate(stereo):
        detector = max(abs(float(left)), abs(float(right)))
        if detector <= 1.0e-6 or amount <= 0.0:
            state.envelope = 0.0
            state.gain = 1.0
            out[i] = (left, right)
            reduction[i] = 0.0
            continue

        coeff = math.exp(-1.0 / (sample_rate * ((attack_ms if detector > state.envelope else release_ms) / 1000.0)))
        state.envelope = detector + coeff * (state.envelope - detector)

        target_gain_db = 0.0
        level_db = gain_to_db(state.envelope)
        half_knee = knee_db * 0.5
        over_db = level_db - threshold_db
        if knee_db > 0.0 and 0.0 < over_db < knee_db:
            target_gain_db = (1.0 / ratio - 1.0) * over_db * over_db / (2.0 * knee_db)
        elif over_db >= knee_db or (knee_db <= 0.0 and over_db > 0.0):
            target_gain_db = (
                (1.0 / ratio - 1.0) * (over_db - half_knee)
                if knee_db > 0.0
                else (1.0 / ratio - 1.0) * over_db
            )

        target_gain = db_to_gain(target_gain_db * amount)
        gain_coeff = math.exp(
            -1.0 / (sample_rate * ((attack_ms if target_gain < state.gain else release_ms) / 1000.0))
        )
        state.gain = target_gain + gain_coeff * (state.gain - target_gain)
        out[i] = (left * state.gain, right * state.gain)
        reduction[i] = max(0.0, -gain_to_db(state.gain))

    return out, reduction


def named_compressor(stereo, sample_rate, threshold_db, mode):
    configs = {
        "peak": dict(detector_attack=2.2, detector_release=48.0, gain_attack=5.0, min_release=70.0,
                     max_release=310.0, ratio=3.15, knee=10.0, max_reduction=8.5),
        "glue": dict(detector_attack=18.0, detector_release=180.0, gain_attack=32.0, min_release=170.0,
                     max_release=620.0, ratio=1.85, knee=12.0, max_reduction=5.5),
        "face": dict(detector_attack=2.8, detector_release=42.0, gain_attack=5.5, min_release=58.0,
                     max_release=185.0, ratio=7.0, knee=8.0, max_reduction=13.5),
    }
    cfg = configs[mode]
    state = CompressorState()
    out = np.empty_like(stereo)
    reduction = np.empty(stereo.shape[0], dtype=np.float32)

    for i, (left, right) in enumerate(stereo):
        left = float(left)
        right = float(right)
        peak_detector = max(abs(left), abs(right))
        if peak_detector <= 1.0e-6:
            state.envelope = 0.0
            state.gain = 1.0
            out[i] = (left, right)
            reduction[i] = 0.0
            continue

        if mode == "face":
            rms_detector = math.sqrt((left * left + right * right) * 0.5)
            detector = peak_detector * 0.68 + rms_detector * 0.32
        else:
            detector = peak_detector

        detect_ms = cfg["detector_attack"] if detector > state.envelope else cfg["detector_release"]
        detector_coeff = math.exp(-1.0 / (sample_rate * (detect_ms / 1000.0)))
        state.envelope = detector + detector_coeff * (state.envelope - detector)

        level_db = gain_to_db(state.envelope)
        over_db = level_db - threshold_db
        half_knee = cfg["knee"] * 0.5
        target_gain_db = 0.0
        if 0.0 < over_db < cfg["knee"]:
            target_gain_db = (1.0 / cfg["ratio"] - 1.0) * over_db * over_db / (2.0 * cfg["knee"])
        elif over_db >= cfg["knee"]:
            target_gain_db = (1.0 / cfg["ratio"] - 1.0) * (over_db - half_knee)

        target_gain_db = clamp(target_gain_db, -cfg["max_reduction"], 0.0)
        target_gain = db_to_gain(target_gain_db)
        depth = clamp(-target_gain_db / cfg["max_reduction"], 0.0, 1.0)
        release_ms = cfg["min_release"] + (cfg["max_release"] - cfg["min_release"]) * depth
        gain_ms = cfg["gain_attack"] if target_gain < state.gain else release_ms
        gain_coeff = math.exp(-1.0 / (sample_rate * (gain_ms / 1000.0)))
        state.gain = target_gain + gain_coeff * (state.gain - target_gain)

        if mode == "face":
            reduction_db = max(0.0, -gain_to_db(state.gain))
            makeup_gain = db_to_gain(clamp(reduction_db * 0.58, 0.0, 7.0))
            density_drive = 1.0 + depth * 0.65
            compressed = soft_clip(np.array([[left * state.gain, right * state.gain]], dtype=np.float32), density_drive)[0]
            parallel_blend = 0.72
            out[i] = ((np.array([left, right]) * (1.0 - parallel_blend)) + (compressed * parallel_blend)) * makeup_gain
            reduction[i] = reduction_db
        else:
            out[i] = (left * state.gain, right * state.gain)
            reduction[i] = max(0.0, -gain_to_db(state.gain))

    return out, reduction


def stats(audio):
    peak = float(np.max(np.abs(audio))) if audio.size else 0.0
    rms = float(np.sqrt(np.mean(audio * audio))) if audio.size else 0.0
    return {
        "peak_dbfs": gain_to_db(peak),
        "rms_dbfs": gain_to_db(rms),
        "crest_db": gain_to_db(peak) - gain_to_db(rms),
        "clipped_samples": int(np.count_nonzero(np.abs(audio) >= 0.999999)),
    }


def reduction_stats(values):
    values = np.asarray(values)
    active = values[values > 0.02]
    return {
        "max_db": float(np.max(values)) if values.size else 0.0,
        "mean_when_active_db": float(np.mean(active)) if active.size else 0.0,
        "active_percent": float(active.size / values.size * 100.0) if values.size else 0.0,
    }


def process(audio, sample_rate, args):
    if audio.shape[1] == 1:
        stereo = np.repeat(audio, 2, axis=1)
    else:
        stereo = audio[:, :2].copy()

    stage_reductions = {}
    stereo *= db_to_gain(args.input_gain)
    stereo, stage_reductions["pre"] = generic_compressor(stereo, sample_rate, -18.0, 1.8, 100.0, 8.0, 85.0, 5.0)
    stereo = soft_clip(stereo, 1.08).astype(np.float32)

    stereo, stage_reductions["peak_tamer"] = named_compressor(stereo, sample_rate, args.peak_threshold, "peak")
    stereo, stage_reductions["glue"] = named_compressor(stereo, sample_rate, args.glue_threshold, "glue")
    stereo, stage_reductions["in_your_face"] = named_compressor(stereo, sample_rate, args.face_threshold, "face")

    stereo, stage_reductions["post"] = generic_compressor(stereo, sample_rate, -14.0, 2.2, 100.0, 12.0, 120.0, 5.0)
    stereo = soft_clip(stereo, 2.8).astype(np.float32)
    stereo *= db_to_gain(args.output_gain)
    stereo = np.clip(stereo, -1.0, 1.0)

    return stereo.astype(np.float32), stage_reductions


def main():
    parser = argparse.ArgumentParser(description="Probe Voxanova compressor behavior on a real WAV file.")
    parser.add_argument("input")
    parser.add_argument("--out-dir", default="Voxanova/reports/compressor_probe")
    parser.add_argument("--input-gain", type=float, default=0.0)
    parser.add_argument("--output-gain", type=float, default=0.0)
    parser.add_argument("--peak-threshold", type=float, default=-18.0)
    parser.add_argument("--glue-threshold", type=float, default=-20.0)
    parser.add_argument("--face-threshold", type=float, default=-16.0)
    args = parser.parse_args()

    audio, sample_rate, fmt = read_float_wav(args.input)
    processed, reductions = process(audio, sample_rate, args)

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_wav = out_dir / "06_04_rosman_lead_voxanova_compressor_probe.wav"
    out_json = out_dir / "06_04_rosman_lead_voxanova_compressor_probe.json"
    write_float_wav(out_wav, processed, sample_rate)

    report = {
        "source": str(Path(args.input)),
        "sample_rate": sample_rate,
        "channels_in_file": fmt["channels"],
        "duration_sec": audio.shape[0] / sample_rate,
        "settings": {
            "input_gain_db": args.input_gain,
            "output_gain_db": args.output_gain,
            "peak_threshold_db": args.peak_threshold,
            "glue_threshold_db": args.glue_threshold,
            "face_threshold_db": args.face_threshold,
        },
        "input": stats(audio[:, : min(audio.shape[1], 2)]),
        "output": stats(processed),
        "gain_reduction": {name: reduction_stats(values) for name, values in reductions.items()},
    }
    out_json.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(json.dumps(report, indent=2))
    print(f"Wrote {out_wav}")
    print(f"Wrote {out_json}")


if __name__ == "__main__":
    main()
