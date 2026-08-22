#include "pomodoro.h"

void PomodoroTimer::begin(const PomodoroSettings &settings) {
  settings_ = settings;
  setPhase(PomodoroPhase::Focus, millis(), false);
}

void PomodoroTimer::applySettings(const PomodoroSettings &settings, uint32_t now) {
  settings_ = settings;
  setPhase(phase_, now, false);
}

void PomodoroTimer::update(uint32_t now) {
  if (!running_ || remainingSeconds(now) > 0) {
    return;
  }

  if (phase_ == PomodoroPhase::Focus) {
    completedRounds_++;
    bool longBreak = settings_.roundsBeforeLongBreak > 0 && completedRounds_ % settings_.roundsBeforeLongBreak == 0;
    setPhase(longBreak ? PomodoroPhase::LongBreak : PomodoroPhase::ShortBreak, now, false);
  } else {
    setPhase(PomodoroPhase::Focus, now, false);
  }
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
  } else {
    durationSeconds_ = max<uint32_t>(1, remainingAtPause_);
    startedAt_ = now;
    running_ = true;
  }
}

void PomodoroTimer::reset(uint32_t now) {
  setPhase(phase_, now, false);
}

void PomodoroTimer::switchPhase(uint32_t now) {
  if (phase_ == PomodoroPhase::Focus) {
    setPhase(PomodoroPhase::ShortBreak, now, false);
  } else {
    setPhase(PomodoroPhase::Focus, now, false);
  }
}

void PomodoroTimer::selectPhase(PomodoroPhase phase, uint32_t now) {
  setPhase(phase, now, false);
}

bool PomodoroTimer::isRunning() const {
  return running_;
}

PomodoroPhase PomodoroTimer::phase() const {
  return phase_;
}

uint8_t PomodoroTimer::completedRounds() const {
  return completedRounds_;
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
  return running_ ? durationSeconds_ : durationFor(phase_);
}

String PomodoroTimer::phaseName() const {
  switch (phase_) {
    case PomodoroPhase::ShortBreak: return "Short break";
    case PomodoroPhase::LongBreak: return "Long break";
    case PomodoroPhase::Focus:
    default: return "Focus";
  }
}

uint32_t PomodoroTimer::durationFor(PomodoroPhase phase) const {
  switch (phase) {
    case PomodoroPhase::ShortBreak: return max<uint16_t>(1, settings_.shortBreakMinutes) * 60UL;
    case PomodoroPhase::LongBreak: return max<uint16_t>(1, settings_.longBreakMinutes) * 60UL;
    case PomodoroPhase::Focus:
    default: return max<uint16_t>(1, settings_.focusMinutes) * 60UL;
  }
}

void PomodoroTimer::setPhase(PomodoroPhase phase, uint32_t now, bool run) {
  phase_ = phase;
  durationSeconds_ = durationFor(phase);
  remainingAtPause_ = durationSeconds_;
  startedAt_ = now;
  running_ = run;
}
