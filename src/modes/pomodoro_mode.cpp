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

ModeActionResult PomodoroModeHandler::onSinglePress(ModeContext &context, uint32_t now) {
  if (!context.pomodoro || !context.ready()) {
    return ModeActionResult::None;
  }
  context.pomodoro->startPause(now);
  context.state->lastAction = context.pomodoro->isRunning() ? "Pomodoro started" : "Pomodoro paused";
  context.state->lastInteractionAt = now;
  context.save();
  return ModeActionResult::StateChanged;
}

ModeActionResult PomodoroModeHandler::onDoublePress(ModeContext &context, uint32_t now) {
  if (!context.pomodoro || !context.ready()) {
    return ModeActionResult::None;
  }
  context.pomodoro->reset(now);
  context.state->lastAction = "Pomodoro reset";
  context.state->lastInteractionAt = now;
  context.save();
  return ModeActionResult::StateChanged;
}

ModeActionResult PomodoroModeHandler::onTriplePress(ModeContext &, uint32_t) {
  return ModeActionResult::None;
}