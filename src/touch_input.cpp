#include "touch_input.h"

TouchInput::TouchInput(int pin, uint32_t tapWindowMs, uint32_t longPressMs)
    : pin_(pin), tapWindowMs_(tapWindowMs), longPressMs_(longPressMs) {}

void TouchInput::begin() {
  if (pin_ >= 0) {
    pinMode(pin_, INPUT_PULLDOWN);
    rawDown_ = readPressed();
    stableDown_ = rawDown_;
    lastRawChangeAt_ = millis();
    pressedAt_ = rawDown_ ? lastRawChangeAt_ : 0;
    tapCount_ = 0;
  }
}

void TouchInput::setTimings(uint32_t tapWindowMs, uint32_t longPressMs) {
  tapWindowMs_ = tapWindowMs;
  longPressMs_ = longPressMs;
}

bool TouchInput::isEnabled() const {
  return pin_ >= 0;
}

bool TouchInput::readPressed() const {
  return digitalRead(pin_) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW);
}

TouchGesture TouchInput::update(uint32_t now) {
  if (!isEnabled()) {
    return TouchGesture::None;
  }

  bool rawDown = readPressed();
  if (rawDown != rawDown_) {
    rawDown_ = rawDown;
    lastRawChangeAt_ = now;
  }

  if (now - lastRawChangeAt_ >= TOUCH_DEBOUNCE_MS && rawDown_ != stableDown_) {
    stableDown_ = rawDown_;
    if (stableDown_) {
      pressedAt_ = now;
    } else {
      uint32_t pressDuration = now - pressedAt_;
      if (pressedAt_ != 0 && pressDuration >= longPressMs_) {
        tapCount_ = 0;
        pressedAt_ = 0;
        return TouchGesture::LongPress;
      }
      tapCount_ = min<uint8_t>(3, tapCount_ + 1);
      lastReleaseAt_ = now;
      pressedAt_ = 0;
    }
  }

  if (!stableDown_ && tapCount_ > 0 && now - lastReleaseAt_ >= tapWindowMs_) {
    uint8_t count = tapCount_;
    tapCount_ = 0;
    if (count == 1) return TouchGesture::SingleTap;
    if (count == 2) return TouchGesture::DoubleTap;
    return TouchGesture::TripleTap;
  }

  return TouchGesture::None;
}
