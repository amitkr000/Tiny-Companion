#pragma once

#include <Arduino.h>

#include "companion_state.h"

class PomodoroTimer {
 public:
  void begin(const PomodoroSettings &settings);
  void update(uint32_t now);
  void start(uint32_t now);
  void pause(uint32_t now);
  void startPause(uint32_t now);
  void reset(uint32_t now);
  void switchPhase(uint32_t now);
  void applySettings(const PomodoroSettings &settings, uint32_t now);

  bool isRunning() const;
  PomodoroPhase phase() const;
  uint8_t completedRounds() const;
  uint32_t remainingSeconds(uint32_t now) const;
  uint32_t durationSeconds() const;
  String phaseName() const;

 private:
  PomodoroSettings settings_;
  PomodoroPhase phase_ = PomodoroPhase::Focus;
  bool running_ = false;
  uint8_t completedRounds_ = 0;
  uint32_t durationSeconds_ = DEFAULT_FOCUS_MINUTES * 60UL;
  uint32_t remainingAtPause_ = DEFAULT_FOCUS_MINUTES * 60UL;
  uint32_t startedAt_ = 0;

  uint32_t durationFor(PomodoroPhase phase) const;
  void setPhase(PomodoroPhase phase, uint32_t now, bool run);
};
