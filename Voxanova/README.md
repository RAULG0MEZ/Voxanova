# Voxanova Vocal Chain

Plugin de procesamiento vocal. Por ahora estamos en **fase de diseño visual**:
estamos construyendo cómo se va a ver y sentir el plugin en el navegador con
React, y cuando el diseño esté aprobado lo conectamos al motor de audio real
(C++ con JUCE en `plugin-shell/`).

## Cómo arrancarlo

```bash
npm install        # solo la primera vez
npm run dev        # abre el diseño en http://localhost:5173
```

## Comandos disponibles

```bash
npm run dev        # servidor de desarrollo en localhost:5173
npm run build      # genera la versión de producción en dist/
npm run preview    # previsualiza el build
npm run lint       # revisa el código en busca de errores
npm run lint:fix   # arregla automáticamente lo que pueda
npm run format     # formatea todo el código con Prettier
```

> Recomendación: corre `npm run lint` y `npm run format` antes de cualquier
> commit. Te ahorran problemas y mantienen el código uniforme.

## Estructura del proyecto

```
Voxanova/
├── src/                  # Diseño en React (la "verdad" visual hoy)
│   ├── main.jsx          # Punto de entrada
│   ├── App.jsx           # Componente raíz (estado global + composición)
│   ├── components/       # Piezas de UI (un componente por archivo)
│   ├── utils/            # Funciones puras (cálculos, formatos, valores iniciales)
│   └── styles.css        # Estilos (CSS plano, ~5,731 líneas — frágil)
├── plugin-shell/         # Plugin nativo C++/JUCE (se conecta DESPUÉS)
├── designs/              # Referencias visuales (mockups, knobs, etc.)
├── docs/                 # Documentación auxiliar (limpieza pendiente, etc.)
├── eslint.config.js      # Reglas del linter
├── .prettierrc.json      # Reglas del formateador
├── index.html            # HTML base que carga React
├── vite.config.js        # Configuración del servidor de desarrollo
└── package.json
```

## Estado actual

| Carpeta         | Estado                                             |
| --------------- | -------------------------------------------------- |
| `src/`          | ✅ Activa — aquí estamos diseñando                 |
| `plugin-shell/` | 🟡 Esqueleto — vacío de lógica, se llenará después |
| `designs/`      | 🟡 Referencias estáticas (PNGs)                    |
| `docs/`         | 📝 Notas y pendientes (ej. limpieza CSS)           |

## Reglas de oro

1. La **web React es la fuente de verdad** mientras dure el diseño.
2. Cuando algo se modifique, se modifica **en su lugar**: nada de archivos
   nuevos dejando huérfanos viejos.
3. Los **valores iniciales** del plugin (gains, thresholds, defaults de
   reverb/delay, etc.) viven en `src/utils/initialState.js`. Si quieres
   cambiar un valor por defecto, ese es el único lugar.
4. Cada cambio que se haga debe quedar reflejado en este README o en el
   `CLAUDE.md` si afecta la arquitectura.
