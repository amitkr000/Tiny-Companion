#include "modes/face_mode.h"

CompanionMode FaceModeHandler::mode() const {
  return CompanionMode::Idle;
}

const char *FaceModeHandler::name() const {
  return "Face";
}