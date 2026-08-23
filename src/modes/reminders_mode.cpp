#include "modes/reminders_mode.h"

CompanionMode RemindersModeHandler::mode() const {
  return CompanionMode::Reminders;
}

const char *RemindersModeHandler::name() const {
  return "Reminders";
}

ModeActionResult RemindersModeHandler::onLongPress(ModeContext &context, uint32_t now) {
  if (!context.ready()) {
    return ModeActionResult::None;
  }

  context.state->reminderSettings.hydrationEnabled = !context.state->reminderSettings.hydrationEnabled;
  if (context.reminders) {
    context.reminders->applySettings(context.state->reminderSettings, now);
  }
  if (!context.state->reminderSettings.hydrationEnabled && context.state->activeReminder == ReminderKind::Hydration) {
    context.state->activeReminder = ReminderKind::None;
  }
  context.state->lastAction = context.state->reminderSettings.hydrationEnabled ? "Hydration reminders on" : "Hydration reminders off";
  context.state->lastInteractionAt = now;
  context.saveRuntimeSettings();
  context.save();
  return ModeActionResult::StateChanged;
}
