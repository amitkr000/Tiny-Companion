#include "weather_service.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>

static String encodeTimezone(String value) {
  value.replace("/", "%2F");
  value.replace(" ", "%20");
  return value;
}

void WeatherService::begin(Preferences &preferences, WeatherContext &context) {
  preferences_ = &preferences;
  context_ = &context;
  loadSettings();
}

void WeatherService::loadSettings() {
  if (!preferences_ || !context_) return;
  context_->enabled = preferences_->getBool("weatherOn", ENABLE_WEATHER_SYNC);
  context_->latitude = preferences_->getFloat("lat", DEFAULT_LATITUDE);
  context_->longitude = preferences_->getFloat("lon", DEFAULT_LONGITUDE);
  context_->timezone = preferences_->getString("tz", DEFAULT_TIMEZONE);
  context_->timezoneOffsetMinutes = preferences_->getInt("tzOff", DEFAULT_TZ_OFFSET_MINUTES);
  context_->manualWeather = preferences_->getBool("manWeather", false);
  context_->manualSeason = preferences_->getBool("manSeason", false);
  context_->overrideTheme = static_cast<WeatherTheme>(preferences_->getUChar("wOverride", static_cast<uint8_t>(WeatherTheme::Unknown)));
  context_->overrideSeason = static_cast<SeasonTheme>(preferences_->getUChar("sOverride", static_cast<uint8_t>(SeasonTheme::Auto)));
  context_->theme = static_cast<WeatherTheme>(preferences_->getUChar("wTheme", static_cast<uint8_t>(WeatherTheme::Unknown)));
  context_->season = static_cast<SeasonTheme>(preferences_->getUChar("season", static_cast<uint8_t>(SeasonTheme::Auto)));
  context_->moon = static_cast<MoonPhase>(preferences_->getUChar("moon", static_cast<uint8_t>(MoonPhase::NewMoon)));
  context_->temperatureC = preferences_->getFloat("tempC", 0.0f);
  context_->weatherCode = preferences_->getInt("wCode", -1);
  context_->isDay = preferences_->getBool("isDay", true);
  context_->hasData = preferences_->getBool("hasWeather", false);
  context_->lastSyncAt = 0;
  context_->lastWeatherSyncAt = preferences_->getULong("weatherMs", context_->hasData ? preferences_->getULong("wSyncMs", 0) : 0);
  context_->lastWeatherChangeAt = 0;
  context_->epochAtSync = preferences_->getULong("epochSync", 0);
  if (context_->epochAtSync != 0) {
    context_->lastSyncAt = millis();
  }
}

void WeatherService::saveSettings() {
  if (!preferences_ || !context_) return;
  preferences_->putBool("weatherOn", context_->enabled);
  preferences_->putFloat("lat", context_->latitude);
  preferences_->putFloat("lon", context_->longitude);
  preferences_->putString("tz", context_->timezone);
  preferences_->putInt("tzOff", context_->timezoneOffsetMinutes);
  preferences_->putBool("manWeather", context_->manualWeather);
  preferences_->putBool("manSeason", context_->manualSeason);
  preferences_->putUChar("wOverride", static_cast<uint8_t>(context_->overrideTheme));
  preferences_->putUChar("sOverride", static_cast<uint8_t>(context_->overrideSeason));
  preferences_->putUChar("wTheme", static_cast<uint8_t>(context_->theme));
  preferences_->putUChar("season", static_cast<uint8_t>(context_->season));
  preferences_->putUChar("moon", static_cast<uint8_t>(context_->moon));
  preferences_->putFloat("tempC", context_->temperatureC);
  preferences_->putInt("wCode", context_->weatherCode);
  preferences_->putBool("isDay", context_->isDay);
  preferences_->putULong("timeSyncMs", context_->lastSyncAt);
  preferences_->putULong("weatherMs", context_->lastWeatherSyncAt);
  preferences_->putULong("wSyncMs", context_->lastWeatherSyncAt);
  preferences_->putULong("epochSync", context_->epochAtSync);
  preferences_->putBool("hasWeather", context_->hasData);
}

void WeatherService::syncTime() {
  if (!context_) return;
  configTime(context_->timezoneOffsetMinutes * 60L, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 3500)) {
    time_t nowEpoch;
    time(&nowEpoch);
    context_->epochAtSync = static_cast<uint32_t>(nowEpoch);
    context_->lastSyncAt = millis();
    context_->moon = moonFromEpoch(currentEpoch(millis()));
    saveSettings();
  }
}

void WeatherService::update(uint32_t now, bool wifiConnected) {
  if (!context_) return;
  context_->wifiConnected = wifiConnected;
  context_->moon = moonFromEpoch(currentEpoch(now));
  context_->season = resolvedSeason(now);

  if (!wifiConnected || !context_->enabled) {
    return;
  }

  bool neverSynced = !context_->hasData || context_->lastWeatherSyncAt == 0;
  if (neverSynced || now - context_->lastWeatherSyncAt >= WEATHER_SYNC_INTERVAL_MS) {
    if (now - lastAttemptAt_ >= 60000UL || neverSynced) {
      lastAttemptAt_ = now;
      fetchNow();
    }
  }
}

bool WeatherService::fetchNow() {
  if (!context_ || WiFi.status() != WL_CONNECTED || !context_->enabled) {
    return false;
  }

  syncTime();

  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += String(context_->latitude, 4);
  url += "&longitude=";
  url += String(context_->longitude, 4);
  url += "&current=temperature_2m,is_day,weather_code,wind_speed_10m";
  url += "&timezone=";
  url += encodeTimezone(context_->timezone);
  url += "&forecast_days=1";

  HTTPClient http;
  http.setTimeout(5000);
  http.begin(url);
  int status = http.GET();
  if (status != 200) {
    Serial.print("[weather] fetch failed status ");
    Serial.println(status);
    http.end();
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, http.getString());
  http.end();
  if (err) {
    Serial.println("[weather] invalid json");
    return false;
  }

  JsonObject current = doc["current"];
  WeatherTheme previousTheme = context_->manualWeather ? context_->overrideTheme : context_->theme;
  int previousCode = context_->weatherCode;
  bool firstWeather = !context_->hasData;

  context_->temperatureC = current["temperature_2m"] | context_->temperatureC;
  context_->weatherCode = current["weather_code"] | context_->weatherCode;
  context_->isDay = (current["is_day"] | 1) == 1;
  float windKmh = current["wind_speed_10m"] | 0.0f;
  context_->theme = themeFromWeatherCode(context_->weatherCode, context_->temperatureC, windKmh);
  WeatherTheme resolvedTheme = context_->manualWeather ? context_->overrideTheme : context_->theme;
  if (firstWeather || previousTheme != resolvedTheme || previousCode != context_->weatherCode) {
    context_->lastWeatherChangeAt = millis();
  }
  context_->hasData = true;
  context_->lastWeatherSyncAt = millis();
  time_t nowEpoch;
  time(&nowEpoch);
  context_->epochAtSync = static_cast<uint32_t>(nowEpoch);
  context_->moon = moonFromEpoch(context_->epochAtSync);
  context_->season = resolvedSeason(millis());
  Serial.print("[weather] synced ");
  Serial.print(weatherThemeName(context_->theme));
  Serial.print(" code ");
  Serial.print(context_->weatherCode);
  Serial.print(" temp ");
  Serial.println(context_->temperatureC, 1);
  saveSettings();
  return true;
}

uint32_t WeatherService::currentEpoch(uint32_t now) const {
  if (!context_ || context_->epochAtSync == 0 || context_->lastSyncAt == 0) {
    return 0;
  }
  return context_->epochAtSync + ((now - context_->lastSyncAt) / 1000UL);
}

int WeatherService::localHour(uint32_t now) const {
  uint32_t epoch = currentEpoch(now);
  if (epoch == 0) {
    uint32_t dayMs = now % 86400000UL;
    return static_cast<int>((dayMs / 3600000UL + DEFAULT_QUIET_END_HOUR) % 24);
  }
  time_t localEpoch = epoch + (context_ ? context_->timezoneOffsetMinutes * 60L : 0);
  struct tm *timeInfo = gmtime(&localEpoch);
  return timeInfo ? timeInfo->tm_hour : 12;
}

SeasonTheme WeatherService::resolvedSeason(uint32_t now) const {
  if (!context_) return SeasonTheme::Summer;
  if (context_->manualSeason && context_->overrideSeason != SeasonTheme::Auto) {
    return context_->overrideSeason;
  }

  uint32_t epoch = currentEpoch(now);
  if (epoch == 0) {
    return SeasonTheme::Summer;
  }

  time_t localEpoch = epoch + context_->timezoneOffsetMinutes * 60L;
  struct tm *timeInfo = gmtime(&localEpoch);
  if (!timeInfo) return SeasonTheme::Summer;
  return seasonFromMonth(timeInfo->tm_mon + 1);
}

FaceId WeatherService::glanceFaceFor(uint32_t now) const {
  if (!context_) return FaceId::CheerfulIdle;
  bool weatherMinute = ((now / IDLE_LOOP_MS) % 2UL) == 1UL;
  if (weatherMinute) {
    if (!context_->wifiConnected || !context_->hasData) {
      return FaceId::CheerfulIdle;
    }
    WeatherTheme theme = context_->manualWeather ? context_->overrideTheme : context_->theme;
    FaceId weatherFace = faceForWeather(theme);
    return weatherFace != FaceId::Neutral ? weatherFace : FaceId::CloudyIdle;
  }
  FaceId timeFace = faceForTime(localHour(now));
  return timeFace != FaceId::Neutral ? timeFace : FaceId::CheerfulIdle;
}

FaceId WeatherService::idleFaceFor(uint32_t now) const {
  if (!context_) return FaceId::CheerfulIdle;

  uint32_t loopPosition = now % IDLE_LOOP_MS;
  if (loopPosition < IDLE_CHEERFUL_MS) {
    return FaceId::CheerfulIdle;
  }

  uint32_t infoStart = IDLE_CHEERFUL_MS + IDLE_TIME_FACE_MS;
  uint32_t infoEnd = infoStart + IDLE_INFO_FACE_MS;
  if (loopPosition < infoStart) {
    return glanceFaceFor(now);
  }
  if (loopPosition < infoEnd) {
    return FaceId::TimeWeatherInfo;
  }

  return FaceId::CheerfulIdle;
}

WeatherTheme WeatherService::themeFromWeatherCode(int code, float temperatureC, float windKmh) const {
  if (temperatureC >= 34.0f) return WeatherTheme::Hot;
  if (temperatureC <= 10.0f) return WeatherTheme::Cold;
  if (windKmh >= 35.0f) return WeatherTheme::Windy;
  if (code == 0 || code == 1) return WeatherTheme::Sunny;
  if (code == 2 || code == 3) return WeatherTheme::Cloudy;
  if (code == 45 || code == 48) return WeatherTheme::Foggy;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return WeatherTheme::Rainy;
  if (code >= 95) return WeatherTheme::Stormy;
  if (code >= 71 && code <= 77) return WeatherTheme::Cold;
  return WeatherTheme::Unknown;
}

MoonPhase WeatherService::moonFromEpoch(uint32_t epoch) const {
  if (epoch == 0) return MoonPhase::NewMoon;
  const double synodicMonth = 29.53058867;
  const uint32_t knownNewMoon = 947182440UL;
  double days = (static_cast<double>(epoch) - knownNewMoon) / 86400.0;
  double age = fmod(days, synodicMonth);
  if (age < 0) age += synodicMonth;
  if (age < 3.5 || age > 26.0) return MoonPhase::NewMoon;
  if (age < 10.5) return MoonPhase::Crescent;
  if (age < 18.5) return MoonPhase::Full;
  return MoonPhase::Half;
}

SeasonTheme WeatherService::seasonFromMonth(int month) const {
  bool southern = context_ && context_->latitude < 0;
  if (southern) {
    month += 6;
    if (month > 12) month -= 12;
  }
  if (month >= 3 && month <= 4) return SeasonTheme::Spring;
  if (month >= 5 && month <= 6) return SeasonTheme::Summer;
  if (month >= 7 && month <= 9) return SeasonTheme::Monsoon;
  if (month >= 10 && month <= 11) return SeasonTheme::Autumn;
  return SeasonTheme::Winter;
}

FaceId WeatherService::faceForTime(int hour) const {
  if (hour < DEFAULT_QUIET_END_HOUR || hour >= DEFAULT_QUIET_START_HOUR) return FaceId::NightIdle;
  if (hour < 11) return FaceId::MorningIdle;
  if (hour < 17) return FaceId::AfternoonIdle;
  if (hour < 21) return FaceId::EveningIdle;
  return FaceId::NightIdle;
}

FaceId WeatherService::faceForWeather(WeatherTheme theme) const {
  switch (theme) {
    case WeatherTheme::Sunny: return FaceId::SunnyIdle;
    case WeatherTheme::Rainy: return FaceId::RainyIdle;
    case WeatherTheme::Cloudy: return FaceId::CloudyIdle;
    case WeatherTheme::Stormy: return FaceId::StormyIdle;
    case WeatherTheme::Foggy: return FaceId::FoggyIdle;
    case WeatherTheme::Windy: return FaceId::WindyIdle;
    case WeatherTheme::Hot: return FaceId::HotIdle;
    case WeatherTheme::Cold: return FaceId::ColdIdle;
    case WeatherTheme::Unknown:
    default: return FaceId::Neutral;
  }
}

FaceId WeatherService::faceForMoon(MoonPhase phase) const {
  switch (phase) {
    case MoonPhase::Crescent: return FaceId::CrescentMoonIdle;
    case MoonPhase::Half: return FaceId::HalfMoonIdle;
    case MoonPhase::Full: return FaceId::FullMoonIdle;
    case MoonPhase::NewMoon:
    default: return FaceId::NewMoonIdle;
  }
}

FaceId WeatherService::faceForSeason(SeasonTheme season) const {
  switch (season) {
    case SeasonTheme::Spring: return FaceId::SpringIdle;
    case SeasonTheme::Summer: return FaceId::SummerIdle;
    case SeasonTheme::Monsoon: return FaceId::MonsoonIdle;
    case SeasonTheme::Autumn: return FaceId::AutumnIdle;
    case SeasonTheme::Winter: return FaceId::WinterIdle;
    case SeasonTheme::Auto:
    default: return FaceId::Neutral;
  }
}
