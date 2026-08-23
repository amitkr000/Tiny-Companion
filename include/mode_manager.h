#pragma once

#include "companion_mode_handler.h"
#include "modes/face_mode.h"
#include "modes/pomodoro_mode.h"
#include "modes/reminders_mode.h"
#include "modes/settings_mode.h"
#include "modes/status_mode.h"

class ModeManager {
 public:
  void begin(const ModeContext &context);
  void select(CompanionMode mode, uint32_t now);
  void selectNext(uint32_t now);
  void update(uint32_t now);
  void handleActionGesture(TouchGesture gesture, uint32_t now);
  bool currentModeIsStarted() const;

 private:
  ModeContext context_;
  FaceModeHandler faceMode_;
  PomodoroModeHandler pomodoroMode_;
  RemindersModeHandler remindersMode_;
  StatusModeHandler statusMode_;
  SettingsModeHandler settingsMode_;

  CompanionModeHandler *handlerFor(CompanionMode mode);
  const CompanionModeHandler *handlerFor(CompanionMode mode) const;
  CompanionMode nextMode(CompanionMode mode) const;
  CompanionMode currentMode() const;
};