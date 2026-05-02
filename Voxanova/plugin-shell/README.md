# Voxanova — Plugin nativo (C++/JUCE)

Esta carpeta contiene el **esqueleto** del plugin nativo en C++ con JUCE.
Es el "cuerpo" que un día va a vivir dentro de un DAW (Logic, Ableton, etc.).

## Estado actual: base nativa inicial

- El target nativo ya declara salida **AU, VST3 y Standalone** con el nombre
  público **Voxanova**.
- El plugin ya tiene parámetros JUCE automatizables mediante
  `AudioProcessorValueTreeState`.
- El estado del plugin ya se guarda y restaura desde el DAW.
- El DSP inicial procesa audio con input/output gain, gate, tres etapas de
  dinámica, width estéreo, delay y reverb simples.
- La UI React compilada en `../dist/` ya se embebe dentro del plugin mediante
  `juce::WebBrowserComponent`, así que el plugin usa el diseño visual existente.
- **VST2 clásico** no está habilitado por defecto porque JUCE moderno no incluye
  el SDK/licencia discontinuado de Steinberg. Si de verdad se necesita VST2, hay
  que agregar ese SDK legalmente y configurar JUCE aparte.

## Compilar

```bash
cmake -S plugin-shell -B plugin-shell/build -DJUCE_DIR=/ruta/absoluta/a/JUCE
cmake --build plugin-shell/build --config Release
```

Si no pasas `JUCE_DIR`, CMake se detiene a propósito porque necesita un checkout
local de JUCE.
