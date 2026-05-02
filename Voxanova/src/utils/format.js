// Utilidades puras de formato y helpers genéricos.
// No contienen estado React. Si necesitas formatear un valor o juntar
// clases CSS, importa desde aquí en lugar de duplicar.

export function cls(...parts) {
  return parts.filter(Boolean).join(" ");
}

export function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

export function formatGain(value) {
  return `${value >= 0 ? "+" : ""}${value.toFixed(1)} dB`;
}

export function formatFreq(freq) {
  return freq >= 1000 ? `${(freq / 1000).toFixed(freq >= 10000 ? 0 : 1)}k` : `${Math.round(freq)}`;
}

export function formatDbFromPercent(value, min = -40, max = 10) {
  const db = min + (clamp(value, 0, 100) / 100) * (max - min);
  return `${db >= 0 ? "+" : ""}${db.toFixed(1)}`;
}
