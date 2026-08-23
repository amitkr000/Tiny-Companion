#pragma once

#include "companion_mode_handler.h"

class RemindersModeHandler : public CompanionModeHandler {
 public:
  CompanionMode mode() const override;
  const char *name() const override;
  ModeActionResult onLongPress(ModeContext &context, uint32_t now) override;
};