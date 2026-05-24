import React, { useEffect, useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import pluginPanelUrl from "../assets/real-theme-bg.png";
import "./styles.css";

const REPOSITORY = "RAULG0MEZ/Voxanova";
const RELEASES_URL = `https://github.com/${REPOSITORY}/releases/latest`;
const API_RELEASE_URL = `https://api.github.com/repos/${REPOSITORY}/releases/latest`;

type Platform = "macos" | "windows" | "linux";

type ReleaseAsset = {
  name: string;
  browser_download_url: string;
};

type ReleaseResponse = {
  tag_name?: string;
  assets?: ReleaseAsset[];
};

type DownloadState =
  | { status: "loading"; label: string; href: string; assetName?: string }
  | { status: "ready"; label: string; href: string; assetName: string; version?: string }
  | { status: "fallback"; label: string; href: string; assetName?: string };

const platformCopy: Record<Platform, { label: string; formats: string; matcher: RegExp }> = {
  macos: {
    label: "macOS",
    formats: "AU, VST3 y Standalone",
    matcher: /macos|darwin|apple/i
  },
  windows: {
    label: "Windows",
    formats: "VST3 y Standalone",
    matcher: /windows|win64|win/i
  },
  linux: {
    label: "Linux",
    formats: "VST3 y Standalone",
    matcher: /linux/i
  }
};

const platforms: Platform[] = ["macos", "windows", "linux"];

function detectPlatform(): Platform {
  const signature = `${navigator.platform} ${navigator.userAgent}`.toLowerCase();

  if (signature.includes("mac")) return "macos";
  if (signature.includes("win")) return "windows";
  if (signature.includes("linux")) return "linux";

  return "macos";
}

async function resolveReleaseAsset(platform: Platform): Promise<DownloadState> {
  const response = await fetch(API_RELEASE_URL, {
    headers: {
      Accept: "application/vnd.github+json"
    }
  });

  if (!response.ok) {
    throw new Error(`GitHub release lookup failed: ${response.status}`);
  }

  const release = (await response.json()) as ReleaseResponse;
  const asset = release.assets?.find((candidate) =>
    platformCopy[platform].matcher.test(candidate.name)
  );

  if (!asset) {
    throw new Error(`No release asset found for ${platform}`);
  }

  return {
    status: "ready",
    label: `Descargar para ${platformCopy[platform].label}`,
    href: asset.browser_download_url,
    assetName: asset.name,
    version: release.tag_name
  };
}

function App() {
  const detectedPlatform = useMemo(detectPlatform, []);
  const [selectedPlatform, setSelectedPlatform] = useState<Platform>(detectedPlatform);
  const [download, setDownload] = useState<DownloadState>({
    status: "loading",
    label: `Buscando descarga para ${platformCopy[detectedPlatform].label}`,
    href: RELEASES_URL
  });

  useEffect(() => {
    let cancelled = false;

    setDownload({
      status: "loading",
      label: `Buscando descarga para ${platformCopy[selectedPlatform].label}`,
      href: RELEASES_URL
    });

    resolveReleaseAsset(selectedPlatform)
      .then((asset) => {
        if (!cancelled) setDownload(asset);
      })
      .catch(() => {
        if (!cancelled) {
          setDownload({
            status: "fallback",
            label: "Abrir descargas en GitHub",
            href: RELEASES_URL
          });
        }
      });

    return () => {
      cancelled = true;
    };
  }, [selectedPlatform]);

  return (
    <main>
      <section className="hero" style={{ backgroundImage: `url(${pluginPanelUrl})` }}>
        <nav className="topbar" aria-label="Principal">
          <a className="brand" href="#top" aria-label="Voxanova">
            Voxanova
          </a>
          <div className="nav-actions">
            <a href={`https://github.com/${REPOSITORY}`} rel="noreferrer">
              GitHub
            </a>
            <a href="#download">Descargar</a>
          </div>
        </nav>

        <div className="hero-copy" id="top">
          <p className="eyebrow">Vocal chain plugin</p>
          <h1>Voxanova</h1>
          <p className="intro">
            Afinación, dinámica, EQ, delay y reverb en una sola cadena vocal para pasar de idea
            cruda a voz lista sin perder el flow.
          </p>
          <div className="hero-actions" aria-label="Descargas principales">
            <a className="download-button" href={download.href}>
              {download.label}
            </a>
            <a
              className="secondary-button"
              href={`https://github.com/${REPOSITORY}`}
              rel="noreferrer"
            >
              Ver repo
            </a>
          </div>
          <p className="download-note" aria-live="polite">
            {download.status === "ready"
              ? `${download.assetName}${download.version ? ` - ${download.version}` : ""}`
              : "Si todavia no hay release publicada, el boton abre la pagina de descargas."}
          </p>
        </div>

        <div className="plugin-preview" aria-label="Vista previa de Voxanova">
          <div className="preview-screen">
            <span>EQ curve</span>
            <div className="curve" />
          </div>
          <div className="preview-modules">
            {["Tune", "Gate", "Comp", "Width", "Verb"].map((label, index) => (
              <div className="module" key={label}>
                <span>{label}</span>
                <strong>{index === 0 ? "ON" : `${72 - index * 8}%`}</strong>
              </div>
            ))}
          </div>
        </div>
      </section>

      <section className="download-section" id="download">
        <div className="section-heading">
          <p className="eyebrow">Descarga directa</p>
          <h2>Un ZIP para cada computadora</h2>
        </div>

        <div className="platform-grid">
          {platforms.map((platform) => {
            const copy = platformCopy[platform];
            const active = selectedPlatform === platform;

            return (
              <button
                className={`platform-card${active ? " is-active" : ""}`}
                key={platform}
                type="button"
                onClick={() => setSelectedPlatform(platform)}
              >
                <span>{copy.label}</span>
                <strong>{copy.formats}</strong>
              </button>
            );
          })}
        </div>

        <div className="release-strip">
          <span>GitHub Actions compila los paquetes de release.</span>
          <a href={download.href}>
            {download.status === "ready" ? "Bajar paquete seleccionado" : "Ver releases"}
          </a>
        </div>
      </section>
    </main>
  );
}

createRoot(document.getElementById("landing-root") as HTMLElement).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
