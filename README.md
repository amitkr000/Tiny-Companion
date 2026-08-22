# Tiny Companion ESP32 Firmware

Tiny Companion is an ESP32-C3 desk companion with expressive OLED faces, two touch sensors, Pomodoro mode, reminders, Wi-Fi setup, a GitHub Pages dashboard, and weather/time/season/moon-based idle faces.

The polished dashboard is hosted with GitHub Pages. The root Pages URL redirects to the dashboard:

```text
https://amitkr000.github.io/Tiny-Companion/
```

The ESP32 hosts the first-boot Wi-Fi setup page plus a small local API for the hosted dashboard.

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

On first boot, or when saved Wi-Fi fails, the ESP32 starts:

```text
SSID: TinyBotSetup
Password: none
Setup page: http://192.168.4.1
```

After it joins Wi-Fi, open the hosted dashboard and enter the IP shown on the OLED.
The IP address is shown for about 30 seconds after connection, then the companion returns to face mode.

To force setup mode, hold either touch sensor while powering on. This clears saved Wi-Fi credentials and starts the setup network again.

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
