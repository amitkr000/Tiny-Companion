# Tiny Companion User Manual

## What It Does

Tiny Companion is a small desk pet with expressive OLED faces. It reacts to touch, shows weather/time/season/moon idle faces, runs Pomodoro focus sessions, and reminds you to drink water or stretch.

In Face mode, the OLED automatically alternates between the current idle face and a compact time/weather information screen. Any touch input interrupts that cycle and shows the correct reaction face immediately.

## Wiring Overview

| Part | ESP32-C3 Pin |
| --- | --- |
| OLED SDA | GPIO 3 |
| OLED SCL | GPIO 4 |
| OLED VCC | 3.3V |
| OLED GND | GND |
| Face touch sensor OUT | GPIO 1 |
| Action touch sensor OUT | GPIO 2 |

Battery wiring:

1. Connect the LiPo battery to TP4056 `B+` and `B-`.
2. Route the charger/load output through the slide switch.
3. Power the ESP32 from the switched output.

Use the correct input pin for your exact ESP32-C3 board. Some boards have `5V`, `VBUS`, `3V3`, and battery pads with different behavior.

## First Boot

On first boot, Tiny Companion starts a setup Wi-Fi network:

```text
SSID: TinyBotSetup
Password: none
Setup page: http://192.168.4.1
```

Connect your phone or computer to that network, open `http://192.168.4.1`, choose your home Wi-Fi, enter the password, and save.

After connection, the OLED shows online status. Open the hosted dashboard and enter the ESP32 IP address:

```text
https://amitkr000.github.io/Tiny-Companion/
```

The IP address stays on screen for about 30 seconds, then Tiny Companion returns to face mode.

To force setup mode, hold either touch sensor while powering on the ESP32. This clears saved Wi-Fi credentials and starts the setup network again.

## Daily Touch Controls

Face touch sensor:

| Gesture | Action |
| --- | --- |
| Single tap | Poke/pet |
| Double tap | Feed |
| Triple tap | Love/play |
| Long press | Sleep or wake |
| Too many fast taps | Annoyed, then angry |

Action touch sensor:

| Gesture | Normal Mode |
| --- | --- |
| Single/double/triple tap | Reserved for the current mode or future modes |
| Long press | Cycle modes: Face, Pomodoro, Break, Clock, Reminders, Status |

In Pomodoro mode:

| Gesture | Pomodoro Action |
| --- | --- |
| Short tap | Ignored |
| Long press | Cycle to the next mode |

If a selected mode is only being previewed and no timer/reminder is active, Tiny Companion returns to face mode automatically after about 20 seconds.

## Pomodoro

Use the hosted dashboard to start and control Pomodoro mode. Pomodoro is website-only so accidental physical touches do not break a focus session.

Default timing:

- Focus: 25 minutes
- Short break: 5 minutes
- Long break: 15 minutes
- Long break after: 4 focus rounds

You can change these from the dashboard settings.

## Reminders

Tiny Companion supports:

- Hydration reminders
- Stretch reminders

When a reminder appears, use the dashboard action to mark it done. The face will briefly show a proud reaction.

## Weather And Idle Faces

Weather faces use Open-Meteo after Wi-Fi setup. Set your latitude, longitude, timezone, and UTC offset in the dashboard.

Face mode cycles between idle faces and an info screen with time, date, weather, temperature, season, and moon phase. Idle faces can reflect:

- Sunny, rainy, cloudy, stormy, foggy, windy, hot, or cold weather.
- Morning, afternoon, evening, or night.
- New moon, crescent, half moon, or full moon.
- Spring, summer, monsoon, autumn, or winter.

Manual weather and season overrides are available in the dashboard.

## Dashboard

The dashboard lets you:

- Connect to the ESP32 by saved IP, manual IP, `tinycompanion.local`, or best-effort local scan.
- Preview faces.
- Trigger poke/feed/play/love/sleep/wake.
- Test mode cycling with Face, Pomodoro, Break, and Cycle Mode buttons.
- Start, pause, reset, and switch Pomodoro.
- Configure reminders.
- Configure weather location and timezone.
- Override weather or season.
- Adjust OLED brightness and inverted colors.
- Tune touch timing and poke anger threshold.
- View JSON status at `/api/state`.
- Forget Wi-Fi and return to setup.

If your browser blocks local-network access from the hosted page, open `http://ESP32_IP/api/state` once directly or use another browser/device on the same Wi-Fi.

## Common Faces

| Face | Meaning |
| --- | --- |
| Sunny/rainy/cloudy | Current weather idle |
| Moon faces | Night idle, based on moon phase |
| Hungry | Fullness is low |
| Sleepy | Energy is low or sleep mode is active |
| Lonely/bored | Happiness is low |
| Annoyed/angry | Too many pokes too quickly |
| Focused | Pomodoro focus session |
| Break | Pomodoro break or stretch reminder |
| Hydration | Drink water reminder |

## Charging And Power

A 400 mAh battery is enough for a small desk companion, but runtime depends heavily on Wi-Fi and OLED brightness.

Expected rough runtime:

- Wi-Fi always active: a few hours.
- Balanced use with dim OLED and periodic Wi-Fi: longer.
- Sleep mode: much longer, but less interactive.

For best results, keep brightness moderate and avoid constant dashboard polling.

## Troubleshooting

If the OLED is blank:

- Check SDA/SCL pins.
- Check OLED address, usually `0x3C`.
- Check 3.3V and GND.

If touch does not work:

- Confirm the sensor OUT pins match `FACE_TOUCH_PIN` and `ACTION_TOUCH_PIN` in `include/config.h`.
- Check that the TTP223 output goes HIGH when touched.

If Wi-Fi setup does not appear:

- Restart the device.
- Hold either touch sensor while powering on to force setup mode.
- Forget the setup network on your phone and reconnect.
- Use `http://192.168.4.1` directly.

If weather faces do not update:

- Confirm the ESP32 is online.
- Check latitude, longitude, timezone, and UTC offset.
- Use the dashboard `Sync weather` button.
