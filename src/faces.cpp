#include "faces.h"

static String shortText(const String &text, size_t maxLen) {
  if (text.length() <= maxLen) return text;
  return text.substring(0, maxLen - 3) + "...";
}

static void drawCentered(Adafruit_SSD1306 &display, const String &text, int16_t y, uint8_t size = 1) {
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, y);
  display.print(text);
}

static void drawCenteredText(Adafruit_SSD1306 &display, const char *text, int16_t y, uint8_t size = 1) {
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((OLED_WIDTH - w) / 2, y);
  display.print(text);
}

static void drawWrappedText(Adafruit_SSD1306 &display, String text, int16_t x, int16_t y, uint8_t maxCharsPerLine, uint8_t maxLines) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (uint8_t line = 0; line < maxLines && text.length() > 0; line++) {
    String chunk = shortText(text, maxCharsPerLine);
    int splitAt = chunk.lastIndexOf(' ');
    if (splitAt > 6 && text.length() > maxCharsPerLine) {
      chunk = chunk.substring(0, splitAt);
    }
    display.setCursor(x, y + line * 10);
    display.print(chunk);
    text = text.substring(min(text.length(), chunk.length()));
    text.trim();
  }
}

static void drawStatusBar(Adafruit_SSD1306 &display, const AppState &state, bool wifiConnected, int rssi) {
  display.drawFastHLine(0, 0, OLED_WIDTH, SSD1306_WHITE);
  display.setCursor(2, 2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (state.deviceMode == DeviceMode::Online) {
    display.print(companionModeName(state.companionMode));
  } else {
    display.print(deviceModeName(state.deviceMode));
  }

  int bars = wifiConnected ? constrain(rssi + 95, 0, 60) / 15 : 0;
  for (int i = 0; i < 4; i++) {
    int h = 2 + i;
    int x = OLED_WIDTH - 18 + i * 4;
    if (i < bars) {
      display.fillRect(x, 7 - h, 3, h, SSD1306_WHITE);
    } else {
      display.drawRect(x, 7 - h, 3, h, SSD1306_WHITE);
    }
  }
}

static void drawEyes(Adafruit_SSD1306 &display, int16_t y, int16_t leftHeight, int16_t rightHeight, int16_t mouthY) {
  uint8_t frame = (millis() / 180) % 24;
  int8_t bob = frame < 6 ? -1 : (frame < 12 ? 0 : (frame < 18 ? 1 : 0));
  bool blink = frame == 22;
  y += bob;
  mouthY += bob;

  if (blink) {
    display.drawFastHLine(32, y + 9, 22, SSD1306_WHITE);
    display.drawFastHLine(74, y + 9, 22, SSD1306_WHITE);
  } else {
    display.fillRoundRect(32, y + (18 - leftHeight) / 2, 22, leftHeight, 6, SSD1306_WHITE);
    display.fillRoundRect(74, y + (18 - rightHeight) / 2, 22, rightHeight, 6, SSD1306_WHITE);
  }
  display.drawFastHLine(46, mouthY + (frame % 10 == 0 ? 1 : 0), 36, SSD1306_WHITE);
}

static void drawTinyHeart(Adafruit_SSD1306 &display, int16_t x, int16_t y) {
  display.drawPixel(x, y, SSD1306_WHITE);
  display.drawPixel(x + 2, y, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 2, SSD1306_WHITE);
}

static void drawHeartEyes(Adafruit_SSD1306 &display) {
  uint8_t frame = (millis() / 150) % 18;
  int8_t bob = frame < 6 ? -1 : (frame < 12 ? 1 : 0);
  display.fillCircle(42, 27 + bob, 4, SSD1306_WHITE);
  display.fillCircle(50, 27 + bob, 4, SSD1306_WHITE);
  display.fillTriangle(38, 30 + bob, 54, 30 + bob, 46, 40 + bob, SSD1306_WHITE);
  display.fillCircle(78, 27 - bob, 4, SSD1306_WHITE);
  display.fillCircle(86, 27 - bob, 4, SSD1306_WHITE);
  display.fillTriangle(74, 30 - bob, 90, 30 - bob, 82, 40 - bob, SSD1306_WHITE);
  display.drawFastHLine(48, 51, 32, SSD1306_WHITE);
  drawTinyHeart(display, 16, 20 + (frame % 5));
  drawTinyHeart(display, 108, 38 - (frame % 6));
}

static void drawCheerfulIdle(Adafruit_SSD1306 &display) {
  uint8_t frame = (millis() / 150) % 24;
  int8_t bob = frame < 6 ? -1 : (frame < 12 ? 0 : (frame < 18 ? 1 : 0));
  int8_t pupilShift = frame < 8 ? -1 : (frame < 16 ? 1 : 0);
  bool blink = frame == 20 || frame == 21;
  bool happyMouth = frame % 12 < 6;

  int16_t headX = 22;
  int16_t headY = 14 + bob;
  display.drawRect(headX, headY, 84, 42, SSD1306_WHITE);
  display.drawFastHLine(headX + 4, headY - 1, 76, SSD1306_WHITE);
  display.drawFastHLine(headX + 4, headY + 42, 76, SSD1306_WHITE);
  display.drawPixel(headX + 1, headY + 1, SSD1306_BLACK);
  display.drawPixel(headX + 82, headY + 1, SSD1306_BLACK);
  display.drawPixel(headX + 1, headY + 40, SSD1306_BLACK);
  display.drawPixel(headX + 82, headY + 40, SSD1306_BLACK);

  display.drawFastVLine(20, headY + 25, 8, SSD1306_WHITE);
  display.drawFastVLine(107, headY + 25, 8, SSD1306_WHITE);
  display.drawRect(16, headY + 28, 4, 8, SSD1306_WHITE);
  display.drawRect(108, headY + 28, 4, 8, SSD1306_WHITE);

  display.drawLine(64, headY, 64, headY - 8, SSD1306_WHITE);
  display.drawPixel(64, headY - 10, SSD1306_WHITE);
  if (frame % 8 < 4) {
    display.drawPixel(62, headY - 9, SSD1306_WHITE);
    display.drawPixel(66, headY - 9, SSD1306_WHITE);
    display.drawPixel(64, headY - 12, SSD1306_WHITE);
  }

  if (blink) {
    display.drawFastHLine(35, headY + 19, 22, SSD1306_WHITE);
    display.drawFastHLine(71, headY + 19, 22, SSD1306_WHITE);
  } else {
    display.drawRect(34, headY + 12, 24, 18, SSD1306_WHITE);
    display.drawRect(70, headY + 12, 24, 18, SSD1306_WHITE);
    display.fillRect(41 + pupilShift, headY + 18, 8, 7, SSD1306_WHITE);
    display.fillRect(77 + pupilShift, headY + 18, 8, 7, SSD1306_WHITE);
    display.drawPixel(45 + pupilShift, headY + 19, SSD1306_BLACK);
    display.drawPixel(81 + pupilShift, headY + 19, SSD1306_BLACK);
  }

  display.drawPixel(31, headY + 32, SSD1306_WHITE);
  display.drawPixel(96, headY + 32, SSD1306_WHITE);
  display.drawPixel(33, headY + 33, SSD1306_WHITE);
  display.drawPixel(94, headY + 33, SSD1306_WHITE);

  display.drawLine(54, headY + 34, 60, headY + 38, SSD1306_WHITE);
  display.drawLine(60, headY + 38, 68, headY + 38, SSD1306_WHITE);
  display.drawLine(68, headY + 38, 74, headY + 34, SSD1306_WHITE);
  if (happyMouth) {
    display.drawPixel(58, headY + 36, SSD1306_WHITE);
    display.drawPixel(70, headY + 36, SSD1306_WHITE);
  }

  display.drawPixel(10, 18 + (frame % 4), SSD1306_WHITE);
  display.drawPixel(14, 18 + (frame % 4), SSD1306_WHITE);
  display.drawPixel(12, 16 + (frame % 4), SSD1306_WHITE);
  display.drawPixel(12, 20 + (frame % 4), SSD1306_WHITE);
  display.drawPixel(114, 42 - (frame % 5), SSD1306_WHITE);
  display.drawPixel(118, 42 - (frame % 5), SSD1306_WHITE);
  display.drawPixel(116, 40 - (frame % 5), SSD1306_WHITE);
  display.drawPixel(116, 44 - (frame % 5), SSD1306_WHITE);
}

static void drawGreeting(Adafruit_SSD1306 &display, const String &userName) {
  uint8_t frame = (millis() / 160) % 18;
  int8_t bob = frame < 6 ? -1 : (frame < 12 ? 1 : 0);
  bool blink = frame == 15;
  int16_t eyeY = 16 + bob;

  if (blink) {
    display.drawFastHLine(32, eyeY + 8, 22, SSD1306_WHITE);
    display.drawFastHLine(74, eyeY + 8, 22, SSD1306_WHITE);
  } else {
    display.fillRoundRect(32, eyeY, 22, 15, 7, SSD1306_WHITE);
    display.fillRoundRect(74, eyeY, 22, 15, 7, SSD1306_WHITE);
  }
  display.drawLine(48, 42 + bob, 56, 47 + bob, SSD1306_WHITE);
  display.drawLine(56, 47 + bob, 72, 47 + bob, SSD1306_WHITE);
  display.drawLine(72, 47 + bob, 80, 42 + bob, SSD1306_WHITE);
  drawTinyHeart(display, 15, 17 + (frame % 5));
  drawTinyHeart(display, 109, 21 + ((frame + 2) % 5));
  char greeting[20];
  snprintf(greeting, sizeof(greeting), "Hi %.12s", userName.c_str());
  drawCenteredText(display, greeting, 55);
}

static void drawSun(Adafruit_SSD1306 &display, int x, int y) {
  display.drawCircle(x, y, 8, SSD1306_WHITE);
  for (int i = -14; i <= 14; i += 7) {
    display.drawPixel(x + i, y - 13, SSD1306_WHITE);
    display.drawPixel(x + i, y + 13, SSD1306_WHITE);
    display.drawPixel(x - 13, y + i, SSD1306_WHITE);
    display.drawPixel(x + 13, y + i, SSD1306_WHITE);
  }
}

static void drawCloud(Adafruit_SSD1306 &display, int x, int y) {
  display.drawCircle(x, y, 8, SSD1306_WHITE);
  display.drawCircle(x + 10, y - 3, 9, SSD1306_WHITE);
  display.drawCircle(x + 22, y, 8, SSD1306_WHITE);
  display.drawFastHLine(x - 7, y + 8, 36, SSD1306_WHITE);
}

static void drawRain(Adafruit_SSD1306 &display, bool heavy) {
  uint8_t drift = (millis() / 140) % 10;
  const int lightDrops[] = {12, 24, 104, 116};
  const int heavyDrops[] = {8, 18, 28, 100, 110, 120, 48, 80};
  const int *drops = heavy ? heavyDrops : lightDrops;
  uint8_t count = heavy ? 8 : 4;

  for (uint8_t i = 0; i < count; i++) {
    int x = drops[i];
    int y = 10 + ((i * 13 + drift) % 42);
    display.drawLine(x, y, x - 2, y + 6, SSD1306_WHITE);
  }
}

static void drawLightning(Adafruit_SSD1306 &display, int x, int y) {
  display.drawLine(x, y, x - 6, y + 12, SSD1306_WHITE);
  display.drawLine(x - 6, y + 12, x + 1, y + 12, SSD1306_WHITE);
  display.drawLine(x + 1, y + 12, x - 8, y + 30, SSD1306_WHITE);
}

static void drawMoon(Adafruit_SSD1306 &display, MoonPhase phase) {
  display.drawCircle(64, 31, 17, SSD1306_WHITE);
  if (phase == MoonPhase::NewMoon) {
    display.fillCircle(64, 31, 15, SSD1306_BLACK);
    display.drawCircle(64, 31, 17, SSD1306_WHITE);
  } else if (phase == MoonPhase::Crescent) {
    display.fillCircle(70, 28, 16, SSD1306_BLACK);
  } else if (phase == MoonPhase::Half) {
    display.fillRect(64, 14, 19, 35, SSD1306_BLACK);
    display.drawCircle(64, 31, 17, SSD1306_WHITE);
  } else {
    display.fillCircle(64, 31, 14, SSD1306_WHITE);
    display.fillCircle(58, 25, 2, SSD1306_BLACK);
    display.fillCircle(68, 35, 2, SSD1306_BLACK);
  }
}

static void drawPomodoro(Adafruit_SSD1306 &display, const PomodoroTimer &pomodoro) {
  uint32_t remaining = pomodoro.remainingSeconds(millis());
  uint8_t minutes = remaining / 60;
  uint8_t seconds = remaining % 60;
  drawCentered(display, "Pomodoro", 13);
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", minutes, seconds);
  drawCentered(display, buffer, 28, 2);
  drawCentered(display, pomodoro.isRunning() ? "running" : "paused", 54);
}

static void drawStats(Adafruit_SSD1306 &display, const AppState &state) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 15);
  display.print("Full ");
  display.print(state.stats.fullness);
  display.setCursor(4, 30);
  display.print("Happy ");
  display.print(state.stats.happiness);
  display.setCursor(4, 45);
  display.print("Energy ");
  display.print(state.stats.energy);
  display.drawRoundRect(82, 20, 30, 22, 5, SSD1306_WHITE);
  display.fillRect(89, 42, 16, 4, SSD1306_WHITE);
}

static void drawSettingInfo(Adafruit_SSD1306 &display, const AppState &state, const IPAddress &ip) {
  drawCentered(display, "Web Setting", 13);
  if (ip == IPAddress()) {
    drawCentered(display, "IP not ready", 28);
  } else {
    drawCentered(display, String("IP ") + ip.toString(), 28);
  }
  drawCentered(display, "Token", 43);
  drawCentered(display, state.dashboardToken.length() ? state.dashboardToken : String("missing"), 54);
}

static void drawAmbientMotion(Adafruit_SSD1306 &display, FaceId face) {
  if (face == FaceId::RainyIdle || face == FaceId::MonsoonIdle || face == FaceId::StormyIdle) {
    return;
  }
  uint8_t frame = (millis() / 180) % 16;
  if (face == FaceId::Sleepy || face == FaceId::NightIdle) {
    display.setCursor(96, 20 - (frame % 5));
    display.print("z");
    display.setCursor(108, 12 - ((frame + 2) % 5));
    display.print("z");
    return;
  }
  display.drawPixel(14 + (frame % 5), 16, SSD1306_WHITE);
  display.drawPixel(16, 14 + (frame % 5), SSD1306_WHITE);
  display.drawPixel(110 - (frame % 5), 42, SSD1306_WHITE);
  display.drawPixel(112, 40 - (frame % 5), SSD1306_WHITE);
}

static void drawFace(Adafruit_SSD1306 &display, FaceId face, const AppState &state) {
  uint8_t frame = (millis() / 160) % 18;
  int8_t bob = frame < 6 ? -1 : (frame < 12 ? 1 : 0);
  int8_t shake = (frame % 2 == 0) ? -1 : 1;

  switch (face) {
    case FaceId::CheerfulIdle:
      drawCheerfulIdle(display);
      break;
    case FaceId::Greeting:
      drawGreeting(display, state.userName);
      break;
    case FaceId::Happy:
    case FaceId::Poke:
      drawEyes(display, 22, 16, 16, 50);
      display.drawPixel(44, 49, SSD1306_BLACK);
      display.drawPixel(83, 49, SSD1306_BLACK);
      break;
    case FaceId::Playful:
      display.fillRoundRect(30, 25, 25, 12, 6, SSD1306_WHITE);
      display.drawFastHLine(73, 30, 26, SSD1306_WHITE);
      display.drawFastHLine(47, 51, 34, SSD1306_WHITE);
      break;
    case FaceId::Hungry:
    case FaceId::Feed:
      drawEyes(display, 24, 10, 10, 49);
      display.setCursor(53, 52);
      display.print("yum");
      break;
    case FaceId::Full:
      drawEyes(display, 24, 12, 12, 51);
      display.drawCircle(64, 51, 5, SSD1306_WHITE);
      break;
    case FaceId::Sleepy:
      display.drawFastHLine(32, 30 + bob, 23, SSD1306_WHITE);
      display.drawFastHLine(74, 30 + bob, 23, SSD1306_WHITE);
      display.drawFastHLine(48, 50 + bob, 31, SSD1306_WHITE);
      break;
    case FaceId::Wake:
    case FaceId::Excited:
      drawEyes(display, 18, 18, 18, 51);
      display.drawCircle(43, 30, 8, SSD1306_BLACK);
      display.drawCircle(85, 30, 8, SSD1306_BLACK);
      break;
    case FaceId::Sad:
    case FaceId::Lonely:
      drawEyes(display, 25, 9, 9, 53);
      display.drawLine(47, 53, 63, 48, SSD1306_WHITE);
      display.drawLine(64, 48, 80, 53, SSD1306_WHITE);
      break;
    case FaceId::Love:
      drawHeartEyes(display);
      break;
    case FaceId::Proud:
      drawEyes(display, 21, 14, 14, 50);
      display.drawLine(30, 19, 53, 16, SSD1306_WHITE);
      display.drawLine(75, 16, 98, 19, SSD1306_WHITE);
      break;
    case FaceId::Pomodoro:
      display.fillRect(34, 28 + bob, 22, 8, SSD1306_WHITE);
      display.fillRect(72, 28 + bob, 22, 8, SSD1306_WHITE);
      display.drawFastHLine(50, 51 + bob, 28, SSD1306_WHITE);
      display.drawRect(20, 15, 88, 42, SSD1306_WHITE);
      break;
    case FaceId::BreakTime:
      drawEyes(display, 23, 12, 12, 50);
      display.drawCircle(98, 21, 7, SSD1306_WHITE);
      display.drawFastHLine(94, 21, 8, SSD1306_WHITE);
      break;
    case FaceId::Hydration:
      drawEyes(display, 24, 13, 13, 50);
      display.drawTriangle(101, 18, 93, 36, 109, 36, SSD1306_WHITE);
      display.drawCircle(101, 37, 7, SSD1306_WHITE);
      break;
    case FaceId::SunnyIdle:
      drawSun(display, 108, 18);
      drawEyes(display, 25, 13, 13, 51);
      break;
    case FaceId::RainyIdle:
      drawRain(display, false);
      drawEyes(display, 24, 10, 10, 51);
      break;
    case FaceId::MonsoonIdle:
      drawRain(display, true);
      drawEyes(display, 24, 9, 9, 52);
      break;
    case FaceId::CloudyIdle:
      drawCloud(display, 92, 15);
      drawEyes(display, 26, 11, 11, 52);
      break;
    case FaceId::StormyIdle:
      drawRain(display, true);
      if (frame % 4 < 2) drawLightning(display, 112, 9);
      drawEyes(display, 26, 8, 8, 53);
      break;
    case FaceId::FoggyIdle:
      drawEyes(display, 24, 10, 10, 50);
      for (int y = 17; y <= 49; y += 11) {
        display.drawFastHLine(4, y, 25, SSD1306_WHITE);
        display.drawFastHLine(99, y + 4, 25, SSD1306_WHITE);
      }
      break;
    case FaceId::WindyIdle:
      drawEyes(display, 24, 14, 7, 50);
      display.drawFastHLine(4, 16, 28, SSD1306_WHITE);
      display.drawFastHLine(92, 24, 30, SSD1306_WHITE);
      display.drawFastHLine(6, 38, 22, SSD1306_WHITE);
      break;
    case FaceId::HotIdle:
      drawSun(display, 101, 19);
      drawEyes(display, 28, 8, 8, 53);
      display.drawLine(49, 51, 80, 51, SSD1306_WHITE);
      break;
    case FaceId::ColdIdle:
    case FaceId::WinterIdle:
      drawEyes(display, 27, 10, 10, 51);
      display.drawPixel(96, 19, SSD1306_WHITE);
      display.drawPixel(103, 26, SSD1306_WHITE);
      display.drawPixel(91, 33, SSD1306_WHITE);
      display.drawFastHLine(48, 52, 28, SSD1306_WHITE);
      break;
    case FaceId::MorningIdle:
      drawSun(display, 108, 16);
      display.fillRoundRect(31, 29, 23, 9, 5, SSD1306_WHITE);
      display.drawFastHLine(74, 32, 23, SSD1306_WHITE);
      display.drawFastHLine(49, 51, 30, SSD1306_WHITE);
      display.drawPixel(24, 22, SSD1306_WHITE);
      display.drawPixel(20, 27, SSD1306_WHITE);
      break;
    case FaceId::AfternoonIdle:
      display.fillRoundRect(32, 31, 22, 7, 4, SSD1306_WHITE);
      display.fillRoundRect(74, 31, 22, 7, 4, SSD1306_WHITE);
      display.drawFastHLine(49, 53, 30, SSD1306_WHITE);
      display.drawLine(101, 24, 96, 34, SSD1306_WHITE);
      display.drawCircle(96, 37, 3, SSD1306_WHITE);
      break;
    case FaceId::EveningIdle:
      drawEyes(display, 25, 12, 12, 51);
      display.drawRoundRect(98, 35, 18, 13, 3, SSD1306_WHITE);
      display.drawCircle(118, 40, 4, SSD1306_WHITE);
      display.drawFastHLine(96, 49, 23, SSD1306_WHITE);
      display.drawPixel(103, 27, SSD1306_WHITE);
      display.drawPixel(108, 24, SSD1306_WHITE);
      display.drawPixel(112, 28, SSD1306_WHITE);
      break;
    case FaceId::NightIdle:
      display.drawFastHLine(32, 31 + bob, 23, SSD1306_WHITE);
      display.drawFastHLine(74, 31 + bob, 23, SSD1306_WHITE);
      display.drawFastHLine(49, 51 + bob, 30, SSD1306_WHITE);
      display.drawCircle(17, 18, 6, SSD1306_WHITE);
      display.fillCircle(20, 16, 6, SSD1306_BLACK);
      break;
    case FaceId::NewMoonIdle:
      drawMoon(display, MoonPhase::NewMoon);
      break;
    case FaceId::CrescentMoonIdle:
      drawMoon(display, MoonPhase::Crescent);
      break;
    case FaceId::HalfMoonIdle:
      drawMoon(display, MoonPhase::Half);
      break;
    case FaceId::FullMoonIdle:
      drawMoon(display, MoonPhase::Full);
      break;
    case FaceId::SpringIdle:
      drawEyes(display, 25, 14, 14, 50);
      display.drawCircle(101, 24, 3, SSD1306_WHITE);
      display.drawCircle(96, 29, 3, SSD1306_WHITE);
      display.drawCircle(106, 29, 3, SSD1306_WHITE);
      display.drawCircle(101, 34, 3, SSD1306_WHITE);
      break;
    case FaceId::SummerIdle:
      drawSun(display, 101, 20);
      drawEyes(display, 23, 16, 16, 50);
      break;
    case FaceId::AutumnIdle:
      drawEyes(display, 26, 12, 12, 51);
      display.drawLine(99, 19, 108, 30, SSD1306_WHITE);
      display.drawLine(108, 30, 96, 36, SSD1306_WHITE);
      display.drawLine(96, 36, 99, 19, SSD1306_WHITE);
      break;
    case FaceId::Annoyed:
      display.fillRoundRect(34 + shake, 30, 20, 8, 3, SSD1306_WHITE);
      display.fillRoundRect(74 + shake, 30, 20, 8, 3, SSD1306_WHITE);
      display.drawLine(32 + shake, 22, 56 + shake, 28, SSD1306_WHITE);
      display.drawLine(96 + shake, 22, 72 + shake, 28, SSD1306_WHITE);
      display.drawFastHLine(50, 52, 28, SSD1306_WHITE);
      break;
    case FaceId::Angry:
      display.fillRoundRect(34 + shake, 30, 20, 8, 3, SSD1306_WHITE);
      display.fillRoundRect(74 + shake, 30, 20, 8, 3, SSD1306_WHITE);
      display.drawLine(31 + shake, 19, 56 + shake, 29, SSD1306_WHITE);
      display.drawLine(97 + shake, 19, 72 + shake, 29, SSD1306_WHITE);
      display.drawLine(48, 54, 64, 49 + bob, SSD1306_WHITE);
      display.drawLine(64, 49 + bob, 80, 54, SSD1306_WHITE);
      break;
    case FaceId::Dizzy:
      display.drawCircle(43, 31, 9, SSD1306_WHITE);
      display.drawCircle(85, 31, 9, SSD1306_WHITE);
      display.drawLine(36, 24, 50, 38, SSD1306_WHITE);
      display.drawLine(50, 24, 36, 38, SSD1306_WHITE);
      display.drawLine(78, 24, 92, 38, SSD1306_WHITE);
      display.drawLine(92, 24, 78, 38, SSD1306_WHITE);
      display.drawFastHLine(50, 52, 28, SSD1306_WHITE);
      break;
    case FaceId::Ignored:
    case FaceId::Bored:
      display.drawFastHLine(32, 32, 23, SSD1306_WHITE);
      display.drawFastHLine(74, 32, 23, SSD1306_WHITE);
      display.drawFastHLine(48, 52, 31, SSD1306_WHITE);
      break;
    case FaceId::LowBattery:
      drawEyes(display, 25, 8, 8, 53);
      display.drawRect(92, 18, 24, 12, SSD1306_WHITE);
      display.drawRect(116, 22, 2, 4, SSD1306_WHITE);
      display.fillRect(95, 21, 4, 6, SSD1306_WHITE);
      break;
    case FaceId::Error:
      drawEyes(display, 25, 7, 7, 52);
      display.drawTriangle(99, 17, 88, 40, 110, 40, SSD1306_WHITE);
      display.drawFastVLine(99, 24, 8, SSD1306_WHITE);
      display.drawPixel(99, 35, SSD1306_WHITE);
      break;
    case FaceId::Neutral:
    default:
      drawEyes(display, 24, 16, 16, 50);
      break;
  }
  drawAmbientMotion(display, face);
}

void applyDisplaySettings(Adafruit_SSD1306 &display, const DisplaySettings &settings) {
  display.dim(settings.brightness < 45);
  display.invertDisplay(settings.inverted);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(map(settings.brightness, 1, 100, 4, 255));
}

void triggerReaction(AppState &state, FaceId face, const String &action, uint32_t now, uint32_t durationMs) {
  state.reactionFace = face;
  state.hasReactionFace = true;
  state.reactionUntil = now + durationMs;
  state.currentFace = face;
  state.lastAction = action;
  state.lastInteractionAt = now;
}

FaceId selectFace(AppState &state, const PomodoroTimer &pomodoro, const ReminderService &, FaceId idleFace, uint32_t now) {
  if (state.lowBattery) return FaceId::LowBattery;
  if (state.deviceMode == DeviceMode::SetupPortal || state.deviceMode == DeviceMode::Connecting || state.deviceMode == DeviceMode::Booting) {
    return state.currentFace;
  }
  if (state.hasReactionFace && static_cast<int32_t>(state.reactionUntil - now) > 0) {
    return state.reactionFace;
  }
  state.hasReactionFace = false;
  if (state.sleepRequested) return FaceId::Sleepy;
  if (state.companionMode == CompanionMode::Pomodoro) {
    return FaceId::Pomodoro;
  }
  if (state.activeReminder == ReminderKind::Hydration) return FaceId::Hydration;
  if (state.activeReminder == ReminderKind::Stretch) return FaceId::BreakTime;
  if (state.stats.energy < 18) return FaceId::Sleepy;
  if (state.stats.fullness < 18) return FaceId::Hungry;
  if (state.stats.happiness < 20) return FaceId::Lonely;
  if (state.stats.happiness < 36) return FaceId::Bored;
  return idleFace;
}

void renderDisplay(Adafruit_SSD1306 &display, const AppState &state, const PomodoroTimer &pomodoro, const ReminderService &reminders, bool wifiConnected, int rssi, const String &ssid, const IPAddress &ip) {
  display.clearDisplay();
  bool faceOnlyScreen = state.deviceMode == DeviceMode::Online
    && state.displaySettings.idleAnimationEnabled
    && (state.showIpUntil == 0 || static_cast<int32_t>(state.showIpUntil - millis()) <= 0)
    && (state.hasReactionFace || state.companionMode == CompanionMode::Idle)
    && state.companionMode != CompanionMode::Pomodoro
    && state.companionMode != CompanionMode::Status
    && state.companionMode != CompanionMode::Reminders
    && state.companionMode != CompanionMode::Settings;
  if (!faceOnlyScreen) {
    drawStatusBar(display, state, wifiConnected, rssi);
  }

  switch (state.deviceMode) {
    case DeviceMode::SetupPortal:
      drawCentered(display, "Setup WiFi", 12);
      drawWrappedText(display, "Connect to " + String(SETUP_AP_SSID), 0, 27, 21, 2);
      drawCentered(display, "Use WiFi setup", 54);
      break;
    case DeviceMode::Connecting:
      drawCentered(display, "Joining WiFi", 16);
      drawWrappedText(display, shortText(ssid, 32), 0, 32, 21, 2);
      break;
    case DeviceMode::Online:
      if (state.showIpUntil != 0 && static_cast<int32_t>(state.showIpUntil - millis()) > 0) {
        drawCentered(display, "Dashboard", 12);
        drawCentered(display, String(ip[0]) + "." + String(ip[1]) + ".", 28);
        drawCentered(display, String(ip[2]) + "." + String(ip[3]), 40);
        drawCentered(display, "faces soon", 55);
      } else if (!state.displaySettings.idleAnimationEnabled) {
        drawCentered(display, DEVICE_NAME, 12);
        drawCentered(display, String(ip[0]) + "." + String(ip[1]) + ".", 28);
        drawCentered(display, String(ip[2]) + "." + String(ip[3]), 40);
        drawCentered(display, shortText(ssid, 18), 55);
      } else if (state.hasReactionFace && static_cast<int32_t>(state.reactionUntil - millis()) > 0) {
        drawFace(display, state.currentFace, state);
      } else if (state.companionMode == CompanionMode::Pomodoro) {
        drawPomodoro(display, pomodoro);
      } else if (state.companionMode == CompanionMode::Status) {
        drawStats(display, state);
      } else if (state.companionMode == CompanionMode::Settings) {
        drawSettingInfo(display, state, ip);
      } else if (state.companionMode == CompanionMode::Reminders) {
        drawCentered(display, "Hydrate in", 14);
        drawCentered(display, String(reminders.minutesUntilHydration(millis())) + " min", 28, 2);
        drawCentered(display, "Stretch " + String(reminders.minutesUntilStretch(millis())) + "m", 54);
      } else {
        drawFace(display, state.currentFace, state);
      }
      break;
    case DeviceMode::Booting:
    default:
      if (state.currentFace == FaceId::Greeting || state.reactionFace == FaceId::Greeting) {
        drawFace(display, FaceId::Greeting, state);
      } else {
        drawCentered(display, DEVICE_NAME, 18);
        drawCentered(display, state.currentMessage, 36);
      }
      break;
  }

  display.display();
}
