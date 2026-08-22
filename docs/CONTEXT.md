# Tiny Companion Context

Tiny Companion is a small ESP32-C3 desk companion built around expressive OLED faces. The device is meant to feel like a tiny digital pet and daily desk helper, not a moving robot or voice assistant.

## Supported Hardware

| Part | Purpose | Default Connection |
| --- | --- | --- |
| ESP32-C3 | Main controller, Wi-Fi, web dashboard | ESP32-C3 Super Mini profile |
| 0.96 inch SSD1306 OLED | Face and status display | SDA GPIO 3, SCL GPIO 4 |
| TTP223 touch sensor 1 | Face interaction input | GPIO 1 |
| TTP223 touch sensor 2 | Mode/action input | GPIO 2 |
| 400 mAh LiPo battery | Portable power | Through charger/power path |
| TP4056 charger | Battery charging | Battery to B+/B- |
| Slide switch | Main power switch | Between battery output and ESP32 power |

Use a protected LiPo or TP4056 protection module. A basic TP4056 is a charger, not a complete load-sharing power system.

## Supported Features

- OLED face system with weather, time, moon, season, action, reaction, reminder, and system faces.
- Wi-Fi setup portal at `TinyBotSetup`.
- Local dashboard after Wi-Fi connection.
- Open-Meteo weather sync using latitude, longitude, and timezone.
- NTP time sync with last-known-time fallback.
- Local moon phase calculation.
- Automatic season selection from date and hemisphere inferred from latitude.
- Manual weather and season overrides from the dashboard.
- Companion stats: fullness, happiness, and energy.
- Face touch gestures: poke, feed, love, sleep/wake.
- Action touch gestures: mode cycling and Pomodoro control.
- Pomodoro timer with configurable focus, short break, long break, and long-break rounds.
- Hydration and stretch reminders.
- OLED brightness, invert, and face-animation settings.
- JSON status endpoint at `/api/state`.

## Face Categories

System faces have highest priority: boot, setup, connecting, low battery, and error.

Mode faces appear while a mode is active: focused Pomodoro, break time, reminders, sleep.

Reaction faces appear briefly after touch or dashboard actions: poke, feed, full, love, angry, annoyed, proud, and wake.

Idle faces appear when nothing urgent is happening:

- Weather: sunny, rainy, cloudy, stormy, foggy, windy, hot, cold.
- Time: morning, afternoon, evening, night.
- Moon: new moon, crescent, half moon, full moon.
- Season: spring, summer, monsoon, autumn, winter.

## Touch Behavior

| Sensor | Gesture | Behavior |
| --- | --- | --- |
| Face touch | Single tap | Poke/pet reaction |
| Face touch | Double tap | Feed |
| Face touch | Triple tap | Love/play reaction |
| Face touch | Long press | Sleep or wake |
| Face touch | Repeated taps | Annoyed, then angry |
| Action touch | Single tap outside Pomodoro | Cycle mode screen |
| Action touch | Long press outside Pomodoro | Enter Pomodoro and start |
| Action touch | Single tap in Pomodoro | Start/pause |
| Action touch | Double tap in Pomodoro | Reset timer |
| Action touch | Long press in Pomodoro | Switch focus/break phase |
| Action touch | Triple tap in Pomodoro | Leave Pomodoro screen |

## Dashboard/API Summary

The dashboard supports Wi-Fi setup, face previews, touch-like actions, Pomodoro controls, reminder settings, weather settings, display settings, and touch timing.

The `/api/state` endpoint returns device mode, companion mode, IP/SSID, face, stats, Pomodoro state, weather theme, temperature, season, and moon phase.

## Current Limitations

- No speaker or microphone is included.
- No movement is possible without motors or servos.
- Battery percentage is not accurate unless a safe ADC voltage divider is added and configured.
- Weather requires Wi-Fi. Offline behavior uses saved weather and approximate time.
- The Flutter `mobile_app` folder is not part of this firmware pass and remains optional future work.

## Future Expansion Ideas

- Add a buzzer for tiny sound reactions.
- Add a battery voltage divider for real battery status.
- Add a light sensor for automatic day/night or desk presence.
- Add a tiny vibration motor for haptic feedback.
- Turn the Flutter app into a polished companion controller.
- Add OTA updates after the firmware is stable.
