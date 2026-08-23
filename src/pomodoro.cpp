#include "pomodoro.h"

void PomodoroTimer::begin(const PomodoroSettings &settings) {
  settings_ = settings;
  reset(millis());
}

void PomodoroTimer::applySettings(const PomodoroSettings &settings, uint32_t now) {
  settings_ = settings;
  reset(now);
}

void PomodoroTimer::update(uint32_t now) {
  if (!running_ || remainingSeconds(now) > 0) {
    return;
  }

  remainingAtPause_ = 0;
  running_ = false;
  completedSessions_++;
}

void PomodoroTimer::start(uint32_t now) {
  if (!running_) {
    startPause(now);
  }
}

void PomodoroTimer::pause(uint32_t now) {
  if (running_) {
    startPause(now);
  }
}

void PomodoroTimer::startPause(uint32_t now) {
  if (running_) {
    remainingAtPause_ = remainingSeconds(now);
    running_ = false;
    return;
  }

  if (remainingAtPause_ == 0) {
    remainingAtPause_ = configuredDurationSeconds();
  }
  durationSeconds_ = remainingAtPause_;
  startedAt_ = now;
  running_ = true;
}

void PomodoroTimer::reset(uint32_t now) {
  running_ = false;
  durationSeconds_ = configuredDurationSeconds();
  remainingAtPause_ = durationSeconds_;
  startedAt_ = now;
}

bool PomodoroTimer::isRunning() const {
  return running_;
}

bool PomodoroTimer::isActive(uint32_t now) const {
  return running_ || remainingSeconds(now) < configuredDurationSeconds();
}

uint8_t PomodoroTimer::completedSessions() const {
  return completedSessions_;
}

uint32_t PomodoroTimer::remainingSeconds(uint32_t now) const {
  if (!running_) {
    return remainingAtPause_;
  }

  uint32_t elapsed = (now - startedAt_) / 1000UL;
  if (elapsed >= durationSeconds_) {
    return 0;
  }
  return durationSeconds_ - elapsed;
}

uint32_t PomodoroTimer::durationSeconds() const {
  return running_ ? durationSeconds_ : configuredDurationSeconds();
}

uint32_t PomodoroTimer::configuredDurationSeconds() const {
  return max<uint16_t>(1, settings_.focusMinutes) * 60UL;
}
