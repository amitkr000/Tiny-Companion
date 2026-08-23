# Tiny Companion User Manual

## What It Does

Tiny Companion is a small desk pet with expressive animated OLED faces. It reacts to touch, greets you by name, runs a one-minute cheerful/time/weather idle loop, runs a Pomodoro timer, and reminds you to drink water or stretch.

In Face mode, the OLED shows only the face and its visual effects. Labels, status icons, and extra text are hidden so touch reactions like poke/feed/love can take over cleanly.

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

After connection, the OLED shows online status. Open the ESP32 local dashboard at the IP shown on the OLED and copy the access token. Then open the hosted dashboard and enter the ESP32 IP address plus that token:

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
| Long press | Pet/rub affection |
| Too many fast taps | Annoyed, then angry |

The first face touch of each local day shows a greeting using the saved user name. Set the name from the dashboard Device tab. The daily check uses synced or saved time, so connect Wi-Fi/NTP at least once for reliable daily greetings.

Action touch sensor:

| Gesture | Normal Mode |
| --- | --- |
| Single/double/triple tap | Reserved for the current mode or future modes |
| Long press | Cycle modes: Face, Pomodoro, Clock, Reminders, Status, Setting |

In Pomodoro mode:

| Gesture | Pomodoro Action |
| --- | --- |
| Short tap | Ignored |
| Long press | Cycle to the next mode |

If a selected mode is only being previewed and no timer/reminder is active, Tiny Companion returns to face mode automatically after about 20 seconds.

## Pomodoro

Use the hosted dashboard to start and control Pomodoro mode. Pomodoro is website-only so accidental physical touches do not interrupt the timer.

Default timing:

- Pomodoro: 25 minutes

You can change these from the dashboard settings.

## Reminders

Tiny Companion supports:

- Hydration reminders
- Stretch reminders

When a reminder appears, use the dashboard action to mark it done. The face will briefly show a proud reaction.

## Weather And Idle Faces

Weather faces use Open-Meteo after Wi-Fi setup. Set your latitude, longitude, timezone, and UTC offset in the dashboard.

Face mode keeps the display clean: no face labels, no status bar, and no extra text while a face is showing. It loops every minute: 40 seconds cheerful robot idle face, 10 seconds current time-of-day face, then 10 seconds current weather face:

- Morning: waking up / nap energy.
- Afternoon: sleepy and lower energy.
- Evening: tea-time mood.
- Night: sleeping with zzz.
- Weather: sunny, rainy, cloudy, stormy, foggy, windy, hot, or cold.

Weather faces are available as preview/override faces in the dashboard:

- Sunny, rainy, cloudy, stormy, foggy, windy, hot, or cold weather.
- Spring, summer, monsoon, autumn, or winter.

Rainy faces use falling drops around the expression. Storm/heavy rain faces can also show lightning.

Manual weather and season overrides are available in the dashboard.

## Dashboard

The dashboard lets you:

- Connect to the ESP32 by saved IP, manual IP, `tinycompanion.local`, or best-effort local scan.
- Use the access token from the ESP32 local dashboard for actions and settings changes.
- Preview faces.
- Trigger poke/feed/play/pet/love/sleep/wake.
- Test mode cycling with Face, Pomodoro, Setting, and Cycle Mode buttons.
- Start, pause, and reset Pomodoro.
- Configure reminders.
- Configure weather location and timezone.
- Override weather or season.
- Adjust OLED brightness and inverted colors.
- Set the user name used in greetings.
- Tune touch timing and poke anger threshold.
- View JSON status at `/api/state`.
- Forget Wi-Fi and return to setup.

If actions or settings show an access-token error, open `http://ESP32_IP/`, copy the token from the Companion card, and paste it into the hosted dashboard. If your browser blocks local-network access from the hosted page, open `http://ESP32_IP/api/state` once directly or use another browser/device on the same Wi-Fi.

## Common Faces

| Face | Meaning |
| --- | --- |
| Cheerful | Default animated face mode expression |
| Greeting | First boot or first touch of the local day |
| Morning/afternoon/evening/night | Short time-of-day glance |
| Spring/summer/monsoon/autumn/winter | Short season glance |
| Sunny/rainy/cloudy | Weather preview or override face |
| Moon faces | Moon phase preview face |
| Hungry | Fullness is low |
| Sleepy | Energy is low or sleep mode is active |
| Lonely/bored | Happiness is low |
| Annoyed/angry | Too many pokes too quickly |
| Pomodoro | Pomodoro timer |
| Break | Stretch reminder |
| Hydration | Drink water reminder |

## Setting Mode

Long-press the Action touch sensor until Setting mode appears. It shows the ESP32 IP address and access token for connecting the hosted web dashboard. If left alone, it returns to Face mode after the normal preview timeout.

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
- Open `http://ESP32_IP/api/state` and check `weather.hasData` and `weather.lastSyncAgeSeconds`.
