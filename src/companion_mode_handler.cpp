#include "companion_mode_handler.h"

bool ModeContext::ready() const {
  return state != nullptr;
}

void ModeContext::save() const {
  if (saveState) {
    saveState();
  }
}

void ModeContext::saveRuntimeSettings() const {
  if (saveRuntime) {
    saveRuntime();
  }
}

void CompanionModeHandler::onEnter(ModeContext &context, uint32_t now) {
  if (!context.ready()) return;
  context.state->companionMode = mode();
  context.state->hasReactionFace = false;
  context.state->lastAction = name();
  context.state->lastInteractionAt = now;
  context.save();
}

void CompanionModeHandler::onExit(ModeContext &, uint32_t) {}

void CompanionModeHandler::update(ModeContext &, uint32_t) {}

bool CompanionModeHandler::isStarted(const ModeContext &) const {
  return false;
}

ModeActionResult CompanionModeHandler::onSinglePress(ModeContext &, uint32_t) {
  return ModeActionResult::NextMode;
}

ModeActionResult CompanionModeHandler::onDoublePress(ModeContext &context, uint32_t now) {
  return recordAction(context, now, String(name()) + " action");
}

ModeActionResult CompanionModeHandler::onTriplePress(ModeContext &context, uint32_t now) {
  return recordAction(context, now, String(name()) + " action");
}

ModeActionResult CompanionModeHandler::onLongPress(ModeContext &context, uint32_t now) {
  return recordAction(context, now, String(name()) + " action");
}

ModeActionResult CompanionModeHandler::recordAction(ModeContext &context, uint32_t now, const String &label) const {
  if (!context.ready()) return ModeActionResult::None;
  context.state->lastAction = label;
  context.state->lastInteractionAt = now;
  context.save();
  return ModeActionResult::StateChanged;
}