#pragma once

#include "companion_mode_handler.h"

class SettingsModeHandler : public CompanionModeHandler {
 public:
  CompanionMode mode() const override;
  const char *name() const override;
};