#include "reminders.h"

void ReminderService::begin(const ReminderSettings &settings, uint32_t now) {
  settings_ = settings;
  lastHydrationAt_ = now;
  lastStretchAt_ = now;
}

void ReminderService::applySettings(const ReminderSettings &settings, uint32_t now) {
  settings_ = settings;
  if (!settings_.hydrationEnabled) {
    lastHydrationAt_ = now;
  }
  if (!settings_.stretchEnabled) {
    lastStretchAt_ = now;
  }
}

ReminderKind ReminderService::update(uint32_t now) {
  if (settings_.hydrationEnabled && now - lastHydrationAt_ >= settings_.hydrationMinutes * 60UL * 1000UL) {
    return ReminderKind::Hydration;
  }
  if (settings_.stretchEnabled && now - lastStretchAt_ >= settings_.stretchMinutes * 60UL * 1000UL) {
    return ReminderKind::Stretch;
  }
  return ReminderKind::None;
}

void ReminderService::markDone(ReminderKind kind, uint32_t now) {
  if (kind == ReminderKind::Hydration) {
    lastHydrationAt_ = now;
  } else if (kind == ReminderKind::Stretch) {
    lastStretchAt_ = now;
  }
}

uint32_t ReminderService::minutesUntilHydration(uint32_t now) const {
  uint32_t interval = settings_.hydrationMinutes * 60UL * 1000UL;
  uint32_t elapsed = now - lastHydrationAt_;
  if (!settings_.hydrationEnabled || elapsed >= interval) return 0;
  return (interval - elapsed + 59999UL) / 60000UL;
}

uint32_t ReminderService::minutesUntilStretch(uint32_t now) const {
  uint32_t interval = settings_.stretchMinutes * 60UL * 1000UL;
  uint32_t elapsed = now - lastStretchAt_;
  if (!settings_.stretchEnabled || elapsed >= interval) return 0;
  return (interval - elapsed + 59999UL) / 60000UL;
}

