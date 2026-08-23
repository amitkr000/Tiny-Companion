#include "modes/status_mode.h"

CompanionMode StatusModeHandler::mode() const {
  return CompanionMode::Status;
}

const char *StatusModeHandler::name() const {
  return "Status";
}