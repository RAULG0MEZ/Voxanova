import React, { useMemo, useState } from "react";
import { createRoot } from "react-dom/client";
import pluginPanelUrl from "../assets/real-theme-bg.png";
import "./styles.css";

const REPOSITORY = "RAULG0MEZ/Voxanova";
const RELEASES_URL = `https://github.com/${REPOSITORY}/releases/latest`;

type Platform = "macos" | "windows" | "linux";

type PlatformDownload = {
  assetName: string;
  formats: string;
  label: string;
};

const platformCopy: Record<Platform, PlatformDownload> = {
  macos: {
    assetName: "Voxanova-macOS-Installer.zip",
    label: "macOS",
    formats: "AU, VST3 y Standalone"
  },
  windows: {
    assetName: "Voxanova-Windows-Installer.zip",
    label: "Windows",
    formats: "VST3 y Standalone"
  },
  linux: {
    assetName: "Voxanova-Linux-Installer.zip",
    label: "Linux",
    formats: "VST3 y Standalone"
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

function getInstallerUrl(platform: Platform) {
  return `https://github.com/${REPOSITORY}/releases/latest/download/${platformCopy[platform].assetName}`;
}

function App() {
  const detectedPlatform = useMemo(detectPlatform, []);
  const [selectedPlatform, setSelectedPlatform] = useState<Platform>(detectedPlatform);
  const selectedDownload = platformCopy[selectedPlatform];
  const installerUrl = getInstallerUrl(selectedPlatform);

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
            <a
              className="download-button"
              href={installerUrl}
              download={selectedDownload.assetName}
              rel="noreferrer"
            >
              Descargar instalador para {selectedDownload.label}
            </a>
            <a className="secondary-button" href={RELEASES_URL} rel="noreferrer">
              Ver descargas
            </a>
          </div>
          <p className="download-note" aria-live="polite">
            Baja directo a tu carpeta de Descargas: {selectedDownload.assetName}
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
          <h2>Un instalador para cada computadora</h2>
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
          <span>El boton descarga el instalador publicado en la ultima release.</span>
          <a href={installerUrl} download={selectedDownload.assetName} rel="noreferrer">
            Descargar {selectedDownload.label}
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
