# Limpieza pendiente

---

## Cambio 2026-04-29 — Contenedor orgánico del panel EQ

**Archivos modificados:** `src/styles.css` (bloque añadido al final; la ubicación
actual se movió por crecimiento del CSS)

**Qué se hizo:**
Se rediseñó el contenedor visual del `eq-band-panel` para que tenga una
silueta orgánica tipo nube en lugar de un rectángulo con esquinas redondeadas.

**Cómo funciona:**

- El `div.eq-band-panel` ahora tiene `background: transparent` — sin fondo propio.
- Se usa un pseudo-elemento `::before` con `position: absolute` que cubre todo el div.
- Ese `::before` tiene `clip-path: path('...')` con una curva Bézier que sube
  y baja según la importancia visual de cada elemento:
  - Botón de encendido (extremo izq.) → forma baja (y ≈ 95 en canvas de 150px)
  - TYPE dropdown → sube moderado (y ≈ 70)
  - FREQ knob → sube más (y ≈ 26)
  - GAIN knob → pico máximo (y ≈ 6)
  - Q knob → simétrico a FREQ (y ≈ 26)
  - Botón cerrar (extremo der.) → forma baja (y ≈ 95)
- La sombra usa `filter: drop-shadow()` en el `::before`, lo que hace que
  la sombra siga el contorno orgánico (no el rectángulo).
- Se usa `isolation: isolate` en el padre para que el `z-index: -1` del
  `::before` funcione correctamente detrás del contenido.
- Se añadió variante `.has-slope::before` con un path más ancho (676px).

**Jerarquía de tamaño de knobs:**

- FREQ y Q: `54 × 54 px`
- GAIN: `76 × 76 px` (más grande, mayor importancia visual)

**Reglas visuales:**

- `align-items: end` en el grid para que todos los elementos queden pegados
  abajo, y la nube suba sobre ellos naturalmente.
- Énfasis (banda activa): fondo más opaco y sombra más profunda.
- Difuminado (otra banda activa): `opacity: 0.22` en el `::before`.

---

## Clases CSS huérfanas en `src/styles.css`

Después de la pasada final de limpieza no quedan clases huérfanas de alta
confianza detectadas desde `src/` en el análisis actual.

La limpieza removió selectores legacy de EQ, EFX, knobs antiguos, toggles viejos
y layouts previos de faders. También se quitaron comentarios históricos del CSS
que ya no describían el estado real. La app quedó verificada con `npm run lint`,
`npm run build`, `prettier --check` y revisión visual en navegador.

Si en el futuro vuelve a crecer esta deuda, no borres clases automáticamente.
Primero confirma que la clase no exista en JSX, que no sea una clase dinámica
(`text-${color}`, `module-${type}`, `level-fill-${color}`, etc.) y que no sea
parte de otro nombre válido (`fader-gr-meter` no es `.gr-meter`).

### Cómo se obtuvo esta lista

```bash
# 1. Extraer todas las clases definidas en styles.css
# 2. Extraer todas las strings tipo clase usadas en src/**/*.jsx
# 3. Expandir clases dinámicas (text-${color}, dial-${color}, etc.)
#    con los colores reales: cyan, blue, green, magenta, yellow, orange
#    y los module types: curve, dualFader, gateStereo, stereo, effects, compressorRack
# 4. Restar las usadas de las definidas
```

Si quieres regenerar la lista en el futuro, vuelve a hacer ese análisis y valida
visualmente los estados principales antes de borrar.
