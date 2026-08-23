#include "modes/clock_mode.h"

CompanionMode ClockModeHandler::mode() const {
  return CompanionMode::Clock;
}

const char *ClockModeHandler::name() const {
  return "Clock";
}