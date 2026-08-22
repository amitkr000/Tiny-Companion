#include "touch_input.h"

TouchInput::TouchInput(int pin, uint32_t tapWindowMs, uint32_t longPressMs)
    : pin_(pin), tapWindowMs_(tapWindowMs), longPressMs_(longPressMs) {}

void TouchInput::begin() {
  if (pin_ >= 0) {
    pinMode(pin_, INPUT_PULLDOWN);
    rawDown_ = digitalRead(pin_) == HIGH;
    stableDown_ = rawDown_;
    lastRawChangeAt_ = millis();
    pressedAt_ = rawDown_ ? lastRawChangeAt_ : 0;
    longPressSent_ = rawDown_;
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

TouchGesture TouchInput::update(uint32_t now) {
  if (!isEnabled()) {
    return TouchGesture::None;
  }

  bool rawDown = digitalRead(pin_) == HIGH;
  if (rawDown != rawDown_) {
    rawDown_ = rawDown;
    lastRawChangeAt_ = now;
  }

  if (now - lastRawChangeAt_ >= TOUCH_DEBOUNCE_MS && rawDown_ != stableDown_) {
    stableDown_ = rawDown_;
    if (stableDown_) {
      pressedAt_ = now;
      longPressSent_ = false;
    } else if (!longPressSent_) {
      tapCount_ = min<uint8_t>(3, tapCount_ + 1);
      lastReleaseAt_ = now;
    }
  }

  if (stableDown_ && !longPressSent_ && now - pressedAt_ >= longPressMs_) {
    longPressSent_ = true;
    tapCount_ = 0;
    return TouchGesture::LongPress;
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
