# Tiny Companion ESP32 Firmware

Tiny Companion is an ESP32-C3 desk companion with expressive animated OLED faces, two touch sensors, personalized greetings, Pomodoro mode, reminders, optional Wi-Fi setup, a GitHub Pages dashboard, and a cheerful idle loop with timed time/weather faces when Wi-Fi is available.

The polished dashboard is hosted with GitHub Pages. The root Pages URL redirects to the dashboard:

```text
https://amitkr000.github.io/Tiny-Companion/
```

The ESP32 hosts an optional Wi-Fi setup page plus a small local API for the hosted dashboard. Without home Wi-Fi, it still boots into face mode and runs as a desk companion.

## Hardware

| Part | Default GPIO |
| --- | --- |
| SSD1306 OLED SDA | GPIO 3 |
| SSD1306 OLED SCL | GPIO 4 |
| Face touch TTP223 | GPIO 1 |
| Action touch TTP223 | GPIO 2 |
| Buzzer | Disabled |
| Battery ADC | Disabled |

Power is designed around a 400 mAh LiPo, TP4056 charger, and slide switch. Use a protected battery or charger protection module.

## First Boot

On first boot, or when saved Wi-Fi fails, Tiny Companion still starts face mode and also runs this optional setup network in the background:

```text
SSID: TinyBotSetup
Password: none
Setup page: Wi-Fi captive portal
```

You can ignore Wi-Fi completely. If you later connect it to Wi-Fi, open the ESP32 local dashboard at the IP shown in Setting mode and copy the access token. Then open the hosted dashboard and enter both the IP address and token.
The IP address is shown for about 30 seconds after connection, then the companion returns to face mode.

After Wi-Fi is saved, normal power cycling reconnects automatically. If setup fails, Tiny Companion shows a Wi-Fi failed screen for about 30 seconds, keeps setup available for retry, then returns to face mode. To intentionally clear saved Wi-Fi and force setup mode, hold both touch sensors while powering on for about 2.5 seconds.

## Build And Flash

Install VS Code + PlatformIO, open this folder, then build:

```powershell
pio run
```

Flash:

```powershell
pio run -t upload
```

If the board does not enter flashing mode automatically, hold `BOOT`, plug in USB, start upload, then release `BOOT` after upload begins.

## Documentation

- [Project context](docs/CONTEXT.md)
- [Project structure](docs/PROJECT_STRUCTURE.md)
- [User manual](docs/USER_MANUAL.md)

The existing `mobile_app` folder is optional future work. The GitHub Pages web dashboard is the main control surface.
