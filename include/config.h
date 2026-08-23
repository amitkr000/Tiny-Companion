#pragma once

#include <Arduino.h>

// Device identity shown on the OLED and setup pages.
static constexpr const char *DEVICE_NAME = "Tiny Companion";
static constexpr const char *HOSTED_DASHBOARD_URL = "https://amitkr000.github.io/Tiny-Companion/";
static constexpr const char *MDNS_HOSTNAME = "tinycompanion";
static constexpr const char *DEFAULT_USER_NAME = "Friend";

// ESP32-hosted Wi-Fi setup portal. Connect a phone/computer to this access
// point, then use the captive setup page to send home Wi-Fi details.
static constexpr const char *SETUP_AP_SSID = "TinyBotSetup";
static constexpr const char *SETUP_AP_PASSWORD = "";
static const IPAddress SETUP_AP_IP(192, 168, 4, 1);
static constexpr uint8_t SETUP_AP_CHANNEL = 6;
static constexpr bool START_SETUP_AP_ON_BOOT = false;
static constexpr uint32_t ONLINE_IP_SCREEN_MS = 30000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
static constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 30000;
static constexpr uint32_t WIFI_SETUP_FAILED_SCREEN_MS = 30000;

// ESP32-C3 Super Mini + SSD1306 OLED defaults from The Mochi wiring guide.
static constexpr int I2C_SDA_PIN = 3;
static constexpr int I2C_SCL_PIN = 4;
static constexpr int OLED_WIDTH = 128;
static constexpr int OLED_HEIGHT = 64;
static constexpr uint8_t OLED_ADDRESS = 0x3C;
static constexpr bool OLED_FLIP_180 = false;

// Optional inputs/outputs. Set to -1 to disable.
// FACE_TOUCH_PIN is the "pet" sensor: poke, feed, love/play, and pet/rub.
// ACTION_TOUCH_PIN is the mode/action sensor: long-press mode cycle and future mode input.
static constexpr int FACE_TOUCH_PIN = 1;
static constexpr int ACTION_TOUCH_PIN = 2;
static constexpr int BUZZER_PIN = -1;
static constexpr int BATTERY_ADC_PIN = -1;

// Display behavior.
static constexpr uint32_t FACE_FRAME_MS = 60;
static constexpr uint32_t IDLE_FACE_MS = 7000;
static constexpr uint32_t WEATHER_FACE_MS = 4500;
static constexpr uint32_t NOTIFICATION_FACE_MS = 6500;
static constexpr uint32_t NAVIGATION_REFRESH_MS = 350;
static constexpr uint32_t MODE_PREVIEW_TIMEOUT_MS = 20000;
static constexpr uint32_t IDLE_LOOP_MS = 60000;
static constexpr uint32_t IDLE_CHEERFUL_MS = 40000;
static constexpr uint32_t IDLE_TIME_FACE_MS = 5000;
static constexpr uint32_t IDLE_INFO_FACE_MS = 5000;
static constexpr uint32_t ACTIVE_STATUS_FACE_MS = 10000;
static constexpr uint32_t COMPLETION_SCREEN_MS = 60000;
static constexpr uint32_t GREETING_FACE_MS = 5200;
static constexpr uint32_t PETTING_FACE_MS = 5200;
static constexpr uint32_t WEATHER_CHANGE_FACE_MS = 18000;
static constexpr uint32_t WEATHER_GLANCE_INTERVAL_MS = 90000;
static constexpr uint32_t WEATHER_SYNC_INTERVAL_MS = 30UL * 60UL * 1000UL;
static constexpr uint32_t STATS_DECAY_INTERVAL_MS = 5UL * 60UL * 1000UL;

// Personalization knobs.
static constexpr bool ENABLE_IDLE_FACE = true;
static constexpr bool ENABLE_WEATHER_FACE = true;
static constexpr bool ENABLE_NOTIFICATIONS = true;
static constexpr bool ENABLE_NAVIGATION = true;
static constexpr bool ENABLE_TOUCH_NEXT = true;
static constexpr bool ENABLE_SOUND = false;
static constexpr bool ENABLE_WEATHER_SYNC = true;

// Default weather/time settings. Change in the dashboard after first boot.
static constexpr float DEFAULT_LATITUDE = 12.9716f;
static constexpr float DEFAULT_LONGITUDE = 77.5946f;
static constexpr const char *DEFAULT_TIMEZONE = "Asia/Kolkata";
static constexpr int DEFAULT_TZ_OFFSET_MINUTES = 330;
static constexpr int DEFAULT_FOCUS_MINUTES = 25;
static constexpr int DEFAULT_SHORT_BREAK_MINUTES = 5;
static constexpr int DEFAULT_LONG_BREAK_MINUTES = 15;
static constexpr int DEFAULT_POMODORO_ROUNDS = 4;
static constexpr int DEFAULT_HYDRATION_MINUTES = 60;
static constexpr int DEFAULT_STRETCH_MINUTES = 90;
static constexpr int DEFAULT_QUIET_START_HOUR = 23;
static constexpr int DEFAULT_QUIET_END_HOUR = 7;

// Touch timing. TTP223 modules normally idle LOW and go HIGH while touched.
static constexpr bool TOUCH_ACTIVE_HIGH = true;
static constexpr uint32_t TOUCH_DEBOUNCE_MS = 35;
static constexpr uint32_t TOUCH_TAP_WINDOW_MS = 420;
static constexpr uint32_t TOUCH_LONG_PRESS_MS = 1200;
static constexpr uint32_t SETUP_RESET_HOLD_MS = 2500;
static constexpr uint32_t POKE_WINDOW_MS = 10000;
static constexpr uint8_t ANNOYED_POKE_COUNT = 5;
static constexpr uint8_t ANGRY_POKE_COUNT = 8;

// Buzzer tuning. Works with a passive buzzer if BUZZER_PIN is configured.
static constexpr int BUZZER_CHANNEL = 0;
static constexpr int BUZZER_RESOLUTION_BITS = 8;
static constexpr int BUZZER_DUTY = 120;
