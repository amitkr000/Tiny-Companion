#pragma once

#include <Arduino.h>

#include "companion_state.h"
#include "pomodoro.h"
#include "reminders.h"
#include "touch_input.h"

enum class ModeActionResult : uint8_t {
  None,
  StateChanged,
  NextMode,
};

struct ModeContext {
  AppState *state = nullptr;
  PomodoroTimer *pomodoro = nullptr;
  ReminderService *reminders = nullptr;
  void (*saveState)() = nullptr;
  void (*saveRuntime)() = nullptr;

  bool ready() const;
  void save() const;
  void saveRuntimeSettings() const;
};

class CompanionModeHandler {
 public:
  virtual ~CompanionModeHandler() = default;

  virtual CompanionMode mode() const = 0;
  virtual const char *name() const = 0;

  virtual void onEnter(ModeContext &context, uint32_t now);
  virtual void onExit(ModeContext &context, uint32_t now);
  virtual void update(ModeContext &context, uint32_t now);
  virtual bool isStarted(const ModeContext &context) const;

  virtual ModeActionResult onSinglePress(ModeContext &context, uint32_t now);
  virtual ModeActionResult onDoublePress(ModeContext &context, uint32_t now);
  virtual ModeActionResult onTriplePress(ModeContext &context, uint32_t now);
  virtual ModeActionResult onLongPress(ModeContext &context, uint32_t now);

 protected:
  ModeActionResult recordAction(ModeContext &context, uint32_t now, const String &label) const;
};