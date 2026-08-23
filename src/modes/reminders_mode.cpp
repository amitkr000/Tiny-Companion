#include "modes/reminders_mode.h"

CompanionMode RemindersModeHandler::mode() const {
  return CompanionMode::Reminders;
}

const char *RemindersModeHandler::name() const {
  return "Reminders";
}