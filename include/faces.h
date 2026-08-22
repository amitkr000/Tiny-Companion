#pragma once

#include <Adafruit_SSD1306.h>
#include <IPAddress.h>

#include "companion_state.h"
#include "pomodoro.h"
#include "reminders.h"

void applyDisplaySettings(Adafruit_SSD1306 &display, const DisplaySettings &settings);
void triggerReaction(AppState &state, FaceId face, const String &action, uint32_t now, uint32_t durationMs = 3500);
FaceId selectFace(AppState &state, const PomodoroTimer &pomodoro, const ReminderService &reminders, FaceId idleFace, uint32_t now);
void renderDisplay(Adafruit_SSD1306 &display, const AppState &state, const PomodoroTimer &pomodoro, const ReminderService &reminders, bool wifiConnected, int rssi, const String &ssid, const IPAddress &ip);

