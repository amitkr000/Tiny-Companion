#include "modes/pomodoro_mode.h"

CompanionMode PomodoroModeHandler::mode() const {
  return CompanionMode::Pomodoro;
}

const char *PomodoroModeHandler::name() const {
  return "Pomodoro";
}

void PomodoroModeHandler::onEnter(ModeContext &context, uint32_t now) {
  if (context.pomodoro && !context.pomodoro->isRunning()) {
    context.pomodoro->reset(now);
  }
  CompanionModeHandler::onEnter(context, now);
}

bool PomodoroModeHandler::isStarted(const ModeContext &context) const {
  return context.pomodoro && context.pomodoro->isRunning();
}

ModeActionResult PomodoroModeHandler::onSinglePress(ModeContext &, uint32_t) {
  return ModeActionResult::None;
}

ModeActionResult PomodoroModeHandler::onDoublePress(ModeContext &, uint32_t) {
  return ModeActionResult::None;
}

ModeActionResult PomodoroModeHandler::onTriplePress(ModeContext &, uint32_t) {
  return ModeActionResult::None;
}