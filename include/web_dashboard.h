#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "companion_state.h"
#include "pomodoro.h"
#include "reminders.h"
#include "weather_service.h"

using DashboardCallback = void (*)();
using WifiSaveCallback = void (*)(const String &ssid, const String &password);

class WebDashboard {
 public:
  void begin(WebServer &server, AppState &state, WeatherService &weather, PomodoroTimer &pomodoro, ReminderService &reminders);
  void setCallbacks(DashboardCallback saveState, DashboardCallback saveRuntime, DashboardCallback applyDisplay, WifiSaveCallback saveWifi, DashboardCallback forgetWifi, DashboardCallback forceWeatherSync);
  void registerRoutes();
  void sendHomePage();
  void sendSetupPage();
  void sendStatusPage();
  void sendStateJson();
  void sendSettingsJson();
  void sendDiscoverJson();
  void sendPlainOk();
  void handleOptions();

 private:
  WebServer *server_ = nullptr;
  AppState *state_ = nullptr;
  WeatherService *weather_ = nullptr;
  PomodoroTimer *pomodoro_ = nullptr;
  ReminderService *reminders_ = nullptr;
  DashboardCallback saveState_ = nullptr;
  DashboardCallback saveRuntime_ = nullptr;
  DashboardCallback applyDisplay_ = nullptr;
  WifiSaveCallback saveWifi_ = nullptr;
  DashboardCallback forgetWifi_ = nullptr;
  DashboardCallback forceWeatherSync_ = nullptr;

  String pageShell(const String &title, const String &body);
  String scanOptions();
  String metricBlock(const String &label, uint8_t value);
  String checkboxInput(const String &name, bool checked);
  void addCorsHeaders();
  void sendJson(int code, const String &json);
  void sendJsonError(int code, const String &message);
  bool applyAction(const String &action);
  bool applyFacePreview(const String &id);
  bool applySettingsFromRequest();
  bool applySettingsFromJson();
  void redirectHome();
  void sendDashboardPage();
  void handleAction();
  void handleFacePreview();
  void handleSettings();
  void handleApiAction();
  void handleApiFace();
  void handleApiSettings();
  void handleWifiSave();
  void handleForgetWifi();
  void handleCaptivePortal();
};
