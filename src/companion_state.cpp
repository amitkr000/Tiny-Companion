#include "companion_state.h"

static const FaceSpec FACE_SPECS[] = {
  {FaceId::Neutral, "neutral", "Calm and ready"},
  {FaceId::Happy, "happy", "Soft smile"},
  {FaceId::Playful, "playful", "Ready to play"},
  {FaceId::Hungry, "hungry", "Wants a snack"},
  {FaceId::Sleepy, "sleepy", "Low-energy eyes"},
  {FaceId::Excited, "excited", "Wide awake"},
  {FaceId::Sad, "sad", "Needs attention"},
  {FaceId::Love, "love", "Feeling attached"},
  {FaceId::Poke, "poke", "Poked"},
  {FaceId::Feed, "feed", "Snack time"},
  {FaceId::Full, "full", "Too full"},
  {FaceId::Wake, "wake", "Waking up"},
  {FaceId::Proud, "proud", "Proud helper"},
  {FaceId::Focused, "focused", "Focus mode"},
  {FaceId::BreakTime, "break", "Break time"},
  {FaceId::Hydration, "hydration", "Drink water"},
  {FaceId::SunnyIdle, "sunny", "Sunny"},
  {FaceId::RainyIdle, "rainy", "Rainy"},
  {FaceId::CloudyIdle, "cloudy", "Cloudy"},
  {FaceId::StormyIdle, "stormy", "Stormy"},
  {FaceId::FoggyIdle, "foggy", "Foggy"},
  {FaceId::WindyIdle, "windy", "Windy"},
  {FaceId::HotIdle, "hot", "Hot day"},
  {FaceId::ColdIdle, "cold", "Cold day"},
  {FaceId::MorningIdle, "morning", "Morning"},
  {FaceId::AfternoonIdle, "afternoon", "Afternoon"},
  {FaceId::EveningIdle, "evening", "Evening"},
  {FaceId::NightIdle, "night", "Night"},
  {FaceId::NewMoonIdle, "new-moon", "New moon"},
  {FaceId::CrescentMoonIdle, "crescent-moon", "Crescent moon"},
  {FaceId::HalfMoonIdle, "half-moon", "Half moon"},
  {FaceId::FullMoonIdle, "full-moon", "Full moon"},
  {FaceId::SpringIdle, "spring", "Spring"},
  {FaceId::SummerIdle, "summer", "Summer"},
  {FaceId::MonsoonIdle, "monsoon", "Monsoon"},
  {FaceId::AutumnIdle, "autumn", "Autumn"},
  {FaceId::WinterIdle, "winter", "Winter"},
  {FaceId::Annoyed, "annoyed", "Too many pokes"},
  {FaceId::Angry, "angry", "Really too many pokes"},
  {FaceId::Dizzy, "dizzy", "Overstimulated"},
  {FaceId::Ignored, "ignored", "Ignored for a while"},
  {FaceId::Bored, "bored", "Needs something to do"},
  {FaceId::Lonely, "lonely", "Misses you"},
  {FaceId::LowBattery, "low-battery", "Battery is low"},
  {FaceId::Error, "error", "Needs attention"},
};

const FaceSpec *faceSpecs(size_t &count) {
  count = sizeof(FACE_SPECS) / sizeof(FACE_SPECS[0]);
  return FACE_SPECS;
}

const char *faceId(FaceId face) {
  size_t count;
  const FaceSpec *specs = faceSpecs(count);
  for (size_t i = 0; i < count; i++) {
    if (specs[i].id == face) {
      return specs[i].name;
    }
  }
  return "neutral";
}

const char *faceName(FaceId face) {
  size_t count;
  const FaceSpec *specs = faceSpecs(count);
  for (size_t i = 0; i < count; i++) {
    if (specs[i].id == face) {
      return specs[i].description;
    }
  }
  return "Calm and ready";
}

FaceId faceFromId(const String &id, FaceId fallback) {
  size_t count;
  const FaceSpec *specs = faceSpecs(count);
  for (size_t i = 0; i < count; i++) {
    if (id == specs[i].name) {
      return specs[i].id;
    }
  }
  return fallback;
}

const char *weatherThemeId(WeatherTheme theme) {
  switch (theme) {
    case WeatherTheme::Sunny: return "sunny";
    case WeatherTheme::Rainy: return "rainy";
    case WeatherTheme::Cloudy: return "cloudy";
    case WeatherTheme::Stormy: return "stormy";
    case WeatherTheme::Foggy: return "foggy";
    case WeatherTheme::Windy: return "windy";
    case WeatherTheme::Hot: return "hot";
    case WeatherTheme::Cold: return "cold";
    case WeatherTheme::Unknown:
    default: return "auto";
  }
}

const char *weatherThemeName(WeatherTheme theme) {
  switch (theme) {
    case WeatherTheme::Sunny: return "Sunny";
    case WeatherTheme::Rainy: return "Rainy";
    case WeatherTheme::Cloudy: return "Cloudy";
    case WeatherTheme::Stormy: return "Stormy";
    case WeatherTheme::Foggy: return "Foggy";
    case WeatherTheme::Windy: return "Windy";
    case WeatherTheme::Hot: return "Hot";
    case WeatherTheme::Cold: return "Cold";
    case WeatherTheme::Unknown:
    default: return "Auto";
  }
}

WeatherTheme weatherThemeFromId(const String &id, WeatherTheme fallback) {
  if (id == "sunny") return WeatherTheme::Sunny;
  if (id == "rainy") return WeatherTheme::Rainy;
  if (id == "cloudy") return WeatherTheme::Cloudy;
  if (id == "stormy") return WeatherTheme::Stormy;
  if (id == "foggy") return WeatherTheme::Foggy;
  if (id == "windy") return WeatherTheme::Windy;
  if (id == "hot") return WeatherTheme::Hot;
  if (id == "cold") return WeatherTheme::Cold;
  if (id == "auto") return WeatherTheme::Unknown;
  return fallback;
}

const char *seasonThemeId(SeasonTheme season) {
  switch (season) {
    case SeasonTheme::Spring: return "spring";
    case SeasonTheme::Summer: return "summer";
    case SeasonTheme::Monsoon: return "monsoon";
    case SeasonTheme::Autumn: return "autumn";
    case SeasonTheme::Winter: return "winter";
    case SeasonTheme::Auto:
    default: return "auto";
  }
}

const char *seasonThemeName(SeasonTheme season) {
  switch (season) {
    case SeasonTheme::Spring: return "Spring";
    case SeasonTheme::Summer: return "Summer";
    case SeasonTheme::Monsoon: return "Monsoon";
    case SeasonTheme::Autumn: return "Autumn";
    case SeasonTheme::Winter: return "Winter";
    case SeasonTheme::Auto:
    default: return "Auto";
  }
}

SeasonTheme seasonThemeFromId(const String &id, SeasonTheme fallback) {
  if (id == "spring") return SeasonTheme::Spring;
  if (id == "summer") return SeasonTheme::Summer;
  if (id == "monsoon") return SeasonTheme::Monsoon;
  if (id == "autumn") return SeasonTheme::Autumn;
  if (id == "winter") return SeasonTheme::Winter;
  if (id == "auto") return SeasonTheme::Auto;
  return fallback;
}

const char *moonPhaseName(MoonPhase phase) {
  switch (phase) {
    case MoonPhase::NewMoon: return "New moon";
    case MoonPhase::Crescent: return "Crescent";
    case MoonPhase::Half: return "Half moon";
    case MoonPhase::Full: return "Full moon";
    default: return "New moon";
  }
}

const char *companionModeName(CompanionMode mode) {
  switch (mode) {
    case CompanionMode::Clock: return "Clock";
    case CompanionMode::Pomodoro: return "Pomodoro";
    case CompanionMode::Break: return "Break";
    case CompanionMode::Reminders: return "Reminders";
    case CompanionMode::Status: return "Status";
    case CompanionMode::Idle:
    default: return "Face";
  }
}

const char *deviceModeName(DeviceMode mode) {
  switch (mode) {
    case DeviceMode::SetupPortal: return "setup";
    case DeviceMode::Connecting: return "connecting";
    case DeviceMode::Online: return "online";
    case DeviceMode::Booting:
    default: return "booting";
  }
}
