#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "companion_state.h"

class WeatherService {
 public:
  void begin(Preferences &preferences, WeatherContext &context);
  void loadSettings();
  void saveSettings();
  void syncTime();
  void update(uint32_t now, bool wifiConnected);
  bool fetchNow();
  uint32_t currentEpoch(uint32_t now) const;
  int localHour(uint32_t now) const;
  SeasonTheme resolvedSeason(uint32_t now) const;
  FaceId idleFaceFor(uint32_t now) const;

 private:
  Preferences *preferences_ = nullptr;
  WeatherContext *context_ = nullptr;
  uint32_t lastAttemptAt_ = 0;

  WeatherTheme themeFromWeatherCode(int code, float temperatureC, float windKmh) const;
  MoonPhase moonFromEpoch(uint32_t epoch) const;
  SeasonTheme seasonFromMonth(int month) const;
  FaceId faceForWeather(WeatherTheme theme) const;
  FaceId faceForSeason(SeasonTheme season) const;
  FaceId faceForMoon(MoonPhase phase) const;
};

