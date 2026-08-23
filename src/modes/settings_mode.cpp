#include "modes/settings_mode.h"

CompanionMode SettingsModeHandler::mode() const {
  return CompanionMode::Settings;
}

const char *SettingsModeHandler::name() const {
  return "Setting";
}