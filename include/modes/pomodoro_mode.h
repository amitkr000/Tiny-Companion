#pragma once

#include "companion_mode_handler.h"

class PomodoroModeHandler : public CompanionModeHandler {
 public:
  CompanionMode mode() const override;
  const char *name() const override;
  void onEnter(ModeContext &context, uint32_t now) override;
  bool isStarted(const ModeContext &context) const override;
  ModeActionResult onSinglePress(ModeContext &context, uint32_t now) override;
  ModeActionResult onDoublePress(ModeContext &context, uint32_t now) override;
  ModeActionResult onTriplePress(ModeContext &context, uint32_t now) override;
};