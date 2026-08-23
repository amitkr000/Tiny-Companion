#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include "config.h"

enum class DeviceMode : uint8_t {
  Booting,
  SetupPortal,
  Connecting,
  Online,
};

enum class CompanionMode : uint8_t {
  Idle,
  Clock,
  Pomodoro,
  Break,
  Reminders,
  Status,
};

enum class FaceId : uint8_t {
  Neutral,
  Happy,
  Playful,
  Hungry,
  Sleepy,
  Excited,
  Sad,
  Love,
  Poke,
  Feed,
  Full,
  Wake,
  Proud,
  Focused,
  BreakTime,
  Hydration,
  SunnyIdle,
  RainyIdle,
  CloudyIdle,
  StormyIdle,
  FoggyIdle,
  WindyIdle,
  HotIdle,
  ColdIdle,
  MorningIdle,
  AfternoonIdle,
  EveningIdle,
  NightIdle,
  NewMoonIdle,
  CrescentMoonIdle,
  HalfMoonIdle,
  FullMoonIdle,
  SpringIdle,
  SummerIdle,
  MonsoonIdle,
  AutumnIdle,
  WinterIdle,
  Annoyed,
  Angry,
  Dizzy,
  Ignored,
  Bored,
  Lonely,
  LowBattery,
  Error,
};

enum class WeatherTheme : uint8_t {
  Unknown,
  Sunny,
  Rainy,
  Cloudy,
  Stormy,
  Foggy,
  Windy,
  Hot,
  Cold,
};

enum class SeasonTheme : uint8_t {
  Auto,
  Spring,
  Summer,
  Monsoon,
  Autumn,
  Winter,
};

enum class MoonPhase : uint8_t {
  NewMoon,
  Crescent,
  Half,
  Full,
};

enum class ReminderKind : uint8_t {
  None,
  Hydration,
  Stretch,
};

enum class PomodoroPhase : uint8_t {
  Focus,
  ShortBreak,
  LongBreak,
};

struct FaceSpec {
  FaceId id;
  const char *name;
  const char *description;
};

struct CompanionStats {
  uint8_t fullness = 72;
  uint8_t happiness = 78;
  uint8_t energy = 66;
};

struct WeatherContext {
  bool enabled = ENABLE_WEATHER_SYNC;
  bool hasData = false;
  bool manualWeather = false;
  bool manualSeason = false;
  float latitude = DEFAULT_LATITUDE;
  float longitude = DEFAULT_LONGITUDE;
  int timezoneOffsetMinutes = DEFAULT_TZ_OFFSET_MINUTES;
  String timezone = DEFAULT_TIMEZONE;
  WeatherTheme theme = WeatherTheme::Unknown;
  WeatherTheme overrideTheme = WeatherTheme::Unknown;
  SeasonTheme season = SeasonTheme::Auto;
  SeasonTheme overrideSeason = SeasonTheme::Auto;
  MoonPhase moon = MoonPhase::NewMoon;
  float temperatureC = 0.0f;
  int weatherCode = -1;
  bool isDay = true;
  uint32_t lastSyncAt = 0;
  uint32_t lastWeatherSyncAt = 0;
  uint32_t lastWeatherChangeAt = 0;
  uint32_t epochAtSync = 0;
};

struct PomodoroSettings {
  uint16_t focusMinutes = DEFAULT_FOCUS_MINUTES;
  uint16_t shortBreakMinutes = DEFAULT_SHORT_BREAK_MINUTES;
  uint16_t longBreakMinutes = DEFAULT_LONG_BREAK_MINUTES;
  uint8_t roundsBeforeLongBreak = DEFAULT_POMODORO_ROUNDS;
};

struct ReminderSettings {
  bool hydrationEnabled = true;
  bool stretchEnabled = true;
  uint16_t hydrationMinutes = DEFAULT_HYDRATION_MINUTES;
  uint16_t stretchMinutes = DEFAULT_STRETCH_MINUTES;
};

struct TouchSettings {
  uint32_t tapWindowMs = TOUCH_TAP_WINDOW_MS;
  uint32_t longPressMs = TOUCH_LONG_PRESS_MS;
  uint8_t annoyedPokeCount = ANNOYED_POKE_COUNT;
  uint8_t angryPokeCount = ANGRY_POKE_COUNT;
};

struct DisplaySettings {
  bool idleAnimationEnabled = true;
  bool inverted = false;
  uint8_t brightness = 100;
};

struct AppState {
  DeviceMode deviceMode = DeviceMode::Booting;
  CompanionMode companionMode = CompanionMode::Idle;
  FaceId currentFace = FaceId::Neutral;
  FaceId reactionFace = FaceId::Neutral;
  bool hasReactionFace = false;
  uint32_t reactionUntil = 0;
  String currentMessage = "Starting";
  String lastAction = "Ready";
  CompanionStats stats;
  WeatherContext weather;
  PomodoroSettings pomodoroSettings;
  ReminderSettings reminderSettings;
  TouchSettings touchSettings;
  DisplaySettings displaySettings;
  ReminderKind activeReminder = ReminderKind::None;
  bool sleepRequested = false;
  bool lowBattery = false;
  uint32_t showIpUntil = 0;
  uint32_t lastInteractionAt = 0;
  uint32_t lastStatsDecayAt = 0;
};

static inline uint8_t clampStat(int value) {
  return static_cast<uint8_t>(constrain(value, 0, 100));
}

const FaceSpec *faceSpecs(size_t &count);
const char *faceId(FaceId face);
const char *faceName(FaceId face);
FaceId faceFromId(const String &id, FaceId fallback = FaceId::Neutral);
const char *weatherThemeId(WeatherTheme theme);
const char *weatherThemeName(WeatherTheme theme);
WeatherTheme weatherThemeFromId(const String &id, WeatherTheme fallback = WeatherTheme::Unknown);
const char *seasonThemeId(SeasonTheme season);
const char *seasonThemeName(SeasonTheme season);
SeasonTheme seasonThemeFromId(const String &id, SeasonTheme fallback = SeasonTheme::Auto);
const char *moonPhaseName(MoonPhase phase);
const char *companionModeName(CompanionMode mode);
const char *deviceModeName(DeviceMode mode);
