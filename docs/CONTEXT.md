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

- OLED face system with animated faces, a cheerful default idle face, personalized greetings, plus weather, time, moon, season, action, reaction, reminder, and system faces.
- Optional Wi-Fi setup portal at `TinyBotSetup`; face mode works without home Wi-Fi.
- GitHub Pages hosted dashboard after Wi-Fi connection.
- ESP32 fallback page plus local JSON API.
- Per-device dashboard access token for state-changing API routes.
- Open-Meteo weather sync using latitude, longitude, and timezone.
- NTP time sync with last-known-time fallback.
- Local moon phase calculation.
- Automatic season selection from date and hemisphere inferred from latitude.
- Manual weather and season overrides from the dashboard.
- Companion stats: fullness, happiness, and energy.
- User name setting for first-boot greetings and first-touch-of-day greetings.
- Face touch gestures: poke, feed, love/play, and long-press pet/rub affection.
- Action touch gestures: long-press mode cycling with automatic return to face mode.
- Face mode shows only the face and visual effects, without labels, status icons, or extra text.
- Pomodoro timer with configurable duration.
- Hydration and stretch reminders.
- OLED brightness, invert, and face-animation settings.
- JSON status endpoint at `/api/state`.

## Face Categories

System faces have highest priority: boot, setup, connecting, low battery, and error.

Mode faces appear while a mode is active: Pomodoro timer, reminders, stretch break, sleep.

Reaction faces appear briefly after touch or dashboard actions: greeting, poke, feed, full, love/petting, angry, annoyed, proud, sleep, and wake. These reactions immediately replace the idle face.

Face mode uses a repeating 60-second idle loop whenever nothing urgent is happening: 40 seconds of the cheerful robot idle face, 5 seconds of the current weather face when Wi-Fi/weather data is available, 5 seconds of the current time-of-day face, then 10 seconds of time/weather information. Offline units skip weather faces and show weather unavailable in the info screen:

- Weather: sunny, rainy, cloudy, stormy, foggy, windy, hot, cold.
- Time: morning waking/nap energy, afternoon low energy, evening tea, night sleep/zzz.
- Moon: new moon, crescent, half moon, full moon.
- Season: spring, summer, monsoon, autumn, winter.
- Weather effects are available as preview/override faces. Rain uses falling drops, and storm/heavy rain can add lightning without hiding the expression.

## Touch Behavior

| Sensor | Gesture | Behavior |
| --- | --- | --- |
| Face touch | Single tap | Poke/pet reaction |
| Face touch | Double tap | Feed |
| Face touch | Triple tap | Love/play reaction |
| Face touch | Long press | Pet/rub affection with animated love face |
| Face touch | Repeated taps | Annoyed, then angry |
| Face touch | First touch of a new local day | Greets the saved user name |
| Face mode | No recent touch | Loops 40s cheerful face, 5s weather face, 5s time face, 10s time/weather info |
| Face mode | Any touch/dashboard action | Shows the reaction face immediately |
| Action touch | Single/double/triple tap | Reserved for current/future mode actions |
| Action touch | Long press | Cycle Face, Pomodoro, Clock, Reminders, Status, and Setting modes |
| Action touch | Paused/preview mode timeout | Returns automatically to face mode after about 20 seconds |
| Website | Pomodoro controls | Start, pause, and reset |
| Website | Mode test controls | Cycle mode, Face mode, Pomodoro mode, and Setting mode |

## Dashboard/API Summary

The hosted dashboard supports device discovery, face previews, touch-like actions, mode testing, website-only Pomodoro controls, reminder settings, weather settings, user name, display settings, and touch timing. Actions, face previews, settings writes, and Wi-Fi reset require the per-device access token shown on the ESP32 local dashboard.

The ESP32 API exposes `/api/state`, `/api/discover`, `/api/action`, `/api/face`, and `/api/settings` with CORS headers for the hosted dashboard. Settings reads/writes and all state-changing routes require `X-Tiny-Token` or a matching `token` form/query parameter.

## Current Limitations

- No speaker or microphone is included.
- No movement is possible without motors or servos.
- Battery percentage is not accurate unless a safe ADC voltage divider is added and configured.
- Weather faces require connected Wi-Fi and synced weather data. Offline behavior keeps idle/time faces and shows weather unavailable.
- Daily first-touch greetings need synced or saved time context to know the local day.
- Browser local-network protections may require manual IP entry or local-network permission for the hosted dashboard.
- The Flutter `mobile_app` folder is not part of this firmware pass and remains optional future work.

## Future Expansion Ideas

- Add a buzzer for tiny sound reactions.
- Add a battery voltage divider for real battery status.
- Add a light sensor for automatic day/night or desk presence.
- Add a tiny vibration motor for haptic feedback.
- Turn the Flutter app into a polished companion controller.
- Add OTA updates after the firmware is stable.
