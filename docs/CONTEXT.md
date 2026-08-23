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
- Action touch gestures: delegated through per-mode handler classes, with single-press mode cycling and automatic return to face mode.
- Face mode shows only the face and visual effects, without labels, status icons, or extra text.
- Pomodoro timer with configurable duration.
- Hydration and stretch reminders.
- OLED brightness, invert, and face-animation settings.
- JSON status endpoint at `/api/state`.

## Face Categories

System faces have highest priority: boot, setup, connecting, low battery, and error.

Mode faces appear while a mode is active: Pomodoro timer, reminders, stretch break, sleep.

Reaction faces appear briefly after touch or dashboard actions: greeting, poke, feed, full, love/petting, angry, annoyed, proud, sleep, and wake. These reactions immediately replace the idle face.

Face mode uses a repeating 60-second home loop whenever nothing urgent is happening. With no running Pomodoro and hydration reminders OFF, each minute starts with 40 seconds of the cheerful robot face, shows 5 seconds of either a weather or time face, shows 5 seconds of time/weather information, then returns to the cheerful face for the rest of the minute. The contextual face alternates by minute so weather and time do not appear back to back in one minute. Offline units skip weather faces and show weather unavailable in the info screen:

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
| Face mode | Pomodoro not running and hydration OFF | 40s cheerful face, 5s alternating weather-or-time face, 5s time/weather info, then cheerful face |
| Face mode | Any touch/dashboard action | Shows the reaction face immediately |
| Action touch | Single tap | Cycle Face, Pomodoro, Reminders, Status, and Setting modes |
| Action touch | Double tap in Pomodoro mode | Reset Pomodoro |
| Action touch | Triple tap | Reserved for current/future mode actions |
| Face mode | Pomodoro running or hydration ON | 40s face, then 20s active status; if both are active, Pomodoro and hydration split 10s each |
| Completion screen | Pomodoro finished | Shows Pomodoro complete for 1 minute, resets Pomodoro, then returns to Face mode |
| Completion screen | Hydration due | Shows Hydration complete for 1 minute, then returns to Face mode and schedules the next interval |
| Action touch | Long press in Pomodoro mode | Start or pause Pomodoro |
| Action touch | Long press in Reminders/Hydration mode | Start or stop hydration reminders |
| Action touch | Paused/preview mode timeout | Returns automatically to face mode after about 20 seconds |
| Website | Pomodoro controls | Start, pause, and reset |
| Website | Mode test controls | Cycle mode, Face mode, Pomodoro mode, Reminders mode, and Setting mode |

## Dashboard/API Summary

The hosted dashboard supports device discovery, face previews, touch-like actions, mode testing, Pomodoro controls, reminder settings, weather settings, user name, display settings, and touch timing. Actions, face previews, settings writes, and Wi-Fi reset require the per-device access token shown on the ESP32 local dashboard.

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
