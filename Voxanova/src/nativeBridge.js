function makeNativeUrl(path, params) {
  const root = window.__JUCE__?.backend ? window.location.origin : "";
  const query = new URLSearchParams({ ...params, t: String(Date.now()) });
  return `${root}${path}?${query.toString()}`;
}

function requestNativeResource(path, params) {
  if (typeof fetch !== "function") return false;

  fetch(makeNativeUrl(path, params), { cache: "no-store" }).catch(() => {
    // Normal browser preview does not expose the JUCE resource provider.
  });

  return true;
}

export function hasNativeBackend() {
  return Boolean(window.__JUCE__?.backend?.emitEvent);
}

export function sendNativeParameter(id, value) {
  try {
    const backend = window.__JUCE__?.backend;
    if (backend?.emitEvent) {
      backend.emitEvent("voxanovaSetParameter", { id, value });
      return true;
    }
  } catch {
    // Fall through to the resource-provider path below.
  }

  return requestNativeResource("/api/setParameter", { id, value: String(value) });
}

export function sendNativeEqBands(eqBands) {
  const payload = {
    pre: Array.isArray(eqBands?.pre) ? eqBands.pre : [],
    post: Array.isArray(eqBands?.post) ? eqBands.post : []
  };

  try {
    const backend = window.__JUCE__?.backend;
    if (backend?.emitEvent) {
      backend.emitEvent("voxanovaSetEqBands", payload);
      return true;
    }
  } catch {
    // Fall through to the resource-provider path below.
  }

  return requestNativeResource("/api/setEqBands", { json: JSON.stringify(payload) });
}

export function sendNativeEditorSize(scale, width, height) {
  try {
    const backend = window.__JUCE__?.backend;
    if (backend?.emitEvent) {
      backend.emitEvent("voxanovaSetEditorSize", { scale, width, height });
      return true;
    }
  } catch {
    // Fall through to the resource-provider path below.
  }

  return requestNativeResource("/api/setEditorSize", {
    scale: String(scale),
    width: String(width),
    height: String(height)
  });
}
