#pragma once

#include <Arduino.h>

#include "config.h"

enum class TouchGesture : uint8_t {
  None,
  SingleTap,
  DoubleTap,
  TripleTap,
  LongPress,
};

class TouchInput {
 public:
  TouchInput(int pin, uint32_t tapWindowMs = TOUCH_TAP_WINDOW_MS, uint32_t longPressMs = TOUCH_LONG_PRESS_MS);

  void begin();
  void setTimings(uint32_t tapWindowMs, uint32_t longPressMs);
  TouchGesture update(uint32_t now);
  bool isEnabled() const;

 private:
  bool readPressed() const;

  int pin_;
  uint32_t tapWindowMs_;
  uint32_t longPressMs_;
  bool rawDown_ = false;
  bool stableDown_ = false;
  uint8_t tapCount_ = 0;
  uint32_t lastRawChangeAt_ = 0;
  uint32_t pressedAt_ = 0;
  uint32_t lastReleaseAt_ = 0;
};

