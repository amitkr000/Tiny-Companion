#pragma once

#include <Arduino.h>

#include "companion_state.h"

class ReminderService {
 public:
  void begin(const ReminderSettings &settings, uint32_t now);
  void applySettings(const ReminderSettings &settings, uint32_t now);
  ReminderKind update(uint32_t now);
  void markDone(ReminderKind kind, uint32_t now);
  uint32_t minutesUntilHydration(uint32_t now) const;
  uint32_t minutesUntilStretch(uint32_t now) const;

 private:
  ReminderSettings settings_;
  uint32_t lastHydrationAt_ = 0;
  uint32_t lastStretchAt_ = 0;
};

