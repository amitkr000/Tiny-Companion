#include "mode_manager.h"

void ModeManager::begin(const ModeContext &context) {
  context_ = context;
}

void ModeManager::select(CompanionMode mode, uint32_t now) {
  CompanionModeHandler *next = handlerFor(mode);
  if (!next) {
    next = &faceMode_;
  }

  CompanionModeHandler *current = handlerFor(currentMode());
  if (current && current != next) {
    current->onExit(context_, now);
  }
  next->onEnter(context_, now);
}

void ModeManager::selectNext(uint32_t now) {
  select(nextMode(currentMode()), now);
}

void ModeManager::update(uint32_t now) {
  if (!context_.ready()) return;

  CompanionModeHandler *handler = handlerFor(context_.state->companionMode);
  if (!handler) {
    select(CompanionMode::Idle, now);
    return;
  }

  handler->update(context_, now);
  if (handler->mode() == CompanionMode::Idle) {
    return;
  }

  if (handler->isStarted(context_)) {
    context_.state->lastInteractionAt = now;
    return;
  }

  if (context_.state->lastInteractionAt == 0) {
    context_.state->lastInteractionAt = now;
    return;
  }

  if (now - context_.state->lastInteractionAt >= MODE_PREVIEW_TIMEOUT_MS) {
    select(CompanionMode::Idle, now);
  }
}

void ModeManager::handleActionGesture(TouchGesture gesture, uint32_t now) {
  if (gesture == TouchGesture::None || !context_.ready()) return;

  CompanionModeHandler *handler = handlerFor(context_.state->companionMode);
  if (!handler) {
    handler = &faceMode_;
  }

  ModeActionResult result = ModeActionResult::None;
  switch (gesture) {
    case TouchGesture::SingleTap:
      result = handler->onSinglePress(context_, now);
      break;
    case TouchGesture::DoubleTap:
      result = handler->onDoublePress(context_, now);
      break;
    case TouchGesture::TripleTap:
      result = handler->onTriplePress(context_, now);
      break;
    case TouchGesture::LongPress:
      result = handler->onLongPress(context_, now);
      break;
    case TouchGesture::None:
    default:
      return;
  }

  if (result == ModeActionResult::NextMode) {
    selectNext(now);
  }
}

bool ModeManager::currentModeIsStarted() const {
  const CompanionModeHandler *handler = handlerFor(currentMode());
  return handler && handler->isStarted(context_);
}

CompanionModeHandler *ModeManager::handlerFor(CompanionMode mode) {
  switch (mode) {
    case CompanionMode::Idle: return &faceMode_;
    case CompanionMode::Pomodoro: return &pomodoroMode_;
    case CompanionMode::Reminders: return &remindersMode_;
    case CompanionMode::Status: return &statusMode_;
    case CompanionMode::Settings: return &settingsMode_;
    default: return nullptr;
  }
}

const CompanionModeHandler *ModeManager::handlerFor(CompanionMode mode) const {
  switch (mode) {
    case CompanionMode::Idle: return &faceMode_;
    case CompanionMode::Pomodoro: return &pomodoroMode_;
    case CompanionMode::Reminders: return &remindersMode_;
    case CompanionMode::Status: return &statusMode_;
    case CompanionMode::Settings: return &settingsMode_;
    default: return nullptr;
  }
}

CompanionMode ModeManager::nextMode(CompanionMode mode) const {
  switch (mode) {
    case CompanionMode::Idle: return CompanionMode::Pomodoro;
    case CompanionMode::Pomodoro: return CompanionMode::Reminders;
    case CompanionMode::Reminders: return CompanionMode::Status;
    case CompanionMode::Status: return CompanionMode::Settings;
    case CompanionMode::Settings:
    default: return CompanionMode::Idle;
  }
}

CompanionMode ModeManager::currentMode() const {
  return context_.state ? context_.state->companionMode : CompanionMode::Idle;
}