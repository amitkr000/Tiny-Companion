# Tiny Companion Project Structure

The firmware is split into small modules so each feature has one clear home.

## Folder Map

```text
include/
  config.h              Hardware pins, defaults, timing constants
  companion_state.h     Shared enums, settings, stats, and app state
  faces.h               OLED face rendering interface
  touch_input.h         TTP223 gesture detection
  weather_service.h     Weather, NTP, season, and moon context
  pomodoro.h            Pomodoro state machine
  reminders.h           Hydration and stretch reminders
  web_dashboard.h       Web UI and API routes

src/
  main.cpp              Setup, loop, Wi-Fi orchestration, service ticks
  companion_state.cpp   String/id lookup tables for faces and themes
  faces.cpp             OLED drawing and face priority
  touch_input.cpp       Debounced single/double/triple/long press logic
  weather_service.cpp   Open-Meteo, NTP, idle face context
  pomodoro.cpp          Single Pomodoro countdown behavior
  reminders.cpp         Reminder scheduling
  web_dashboard.cpp     Dashboard pages, settings, actions, JSON API

docs/
  CONTEXT.md
  PROJECT_STRUCTURE.md
  USER_MANUAL.md

web_dashboard/
  index.html            GitHub Pages dashboard shell
  styles.css            Hosted dashboard visual design
  app.js                Device discovery, API calls, UI state

.github/workflows/
  pages.yml             GitHub Pages deployment workflow
```

## Data Flow

`main.cpp` owns the global `AppState` and calls each service from `loop()`.

Touch input produces gestures. Face-touch gestures update stats and reaction faces. Action-touch long-press cycles modes; short taps are reserved for the active mode or future mode actions.

Weather service updates `WeatherContext` from Open-Meteo and NTP. It also calculates moon phase and season. The face renderer asks it for the best idle face.

Pomodoro and reminders update non-blockingly. They never delay the loop, so touch and dashboard requests stay responsive.

The face renderer selects the final face using priority:

1. System state.
2. Sleep/low-battery state.
3. Active Pomodoro or reminder.
4. Recent reaction.
5. Stat-driven needs.
6. Weather/time/season/moon idle face.

In Face mode, `renderDisplay()` hides the status bar and draws only the selected face plus visual effects. Reaction faces still win first, so touch actions replace the idle face immediately.

The hosted GitHub Pages dashboard talks to the ESP32 through JSON API routes. Settings are saved to ESP32 Preferences/NVS.

## Where To Add Things

Add a new OLED face in:

- `include/companion_state.h`: add a `FaceId`.
- `src/companion_state.cpp`: add the face ID, label, and description.
- `src/faces.cpp`: add drawing behavior in `drawFace()`.

Add a new touch gesture behavior in `src/main.cpp`:

- `handleFaceGesture()` for companion/pet actions.
- `handleActionGesture()` for mode cycling and mode-specific actions.
- `autoReturnToFaceMode()` for returning paused/preview modes to idle faces.

Add a new mode by updating:

- `CompanionMode` in `include/companion_state.h`.
- `nextCompanionMode()` in `src/main.cpp`.
- `renderDisplay()` in `src/faces.cpp`.
- Dashboard controls in `src/web_dashboard.cpp`.

Add a new setting by updating:

- The relevant settings struct in `include/companion_state.h`.
- `loadRuntimeSettings()` and `saveRuntimeSettings()` in `src/main.cpp`, or `WeatherService` for weather settings.
- The settings form and parser in `src/web_dashboard.cpp`.
- The hosted dashboard form and API payload in `web_dashboard/app.js`.
