#include "faces.h"

#include <time.h>

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

static bool timerActive(uint32_t until, uint32_t now) {
  return until != 0 && static_cast<int32_t>(until - now) > 0;
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

static int8_t faceBob(uint8_t frame) {
  return frame < 6 ? -1 : (frame < 12 ? 0 : (frame < 18 ? 1 : 0));
}

static int16_t styledEyeHeight(int16_t requestedHeight) {
  return constrain(requestedHeight + 12, 10, 32);
}

static void drawSmallMouth(Adafruit_SSD1306 &display, int16_t y) {
  display.fillRoundRect(56, y, 16, 5, 2, SSD1306_WHITE);
  display.drawPixel(55, y - 1, SSD1306_WHITE);
  display.drawPixel(72, y - 1, SSD1306_WHITE);
}

static void drawSmallOpenMouth(Adafruit_SSD1306 &display, int16_t y) {
  display.fillRoundRect(57, y - 1, 14, 9, 3, SSD1306_WHITE);
  display.fillRoundRect(60, y + 1, 8, 5, 2, SSD1306_BLACK);
  display.drawFastHLine(61, y + 6, 6, SSD1306_WHITE);
}

static void drawSmallSadMouth(Adafruit_SSD1306 &display, int16_t y) {
  display.drawLine(55, y + 5, 61, y + 1, SSD1306_WHITE);
  display.drawLine(55, y + 6, 61, y + 2, SSD1306_WHITE);
  display.drawFastHLine(61, y + 1, 7, SSD1306_WHITE);
  display.drawFastHLine(61, y + 2, 7, SSD1306_WHITE);
  display.drawLine(68, y + 1, 74, y + 5, SSD1306_WHITE);
  display.drawLine(68, y + 2, 74, y + 6, SSD1306_WHITE);
}

static void drawFriendlySmile(Adafruit_SSD1306 &display, int16_t mouthY) {
  uint8_t frame = (millis() / 180) % 24;
  int16_t y = mouthY + faceBob(frame);
  display.drawLine(54, y, 61, y + 4, SSD1306_WHITE);
  display.drawLine(54, y + 1, 61, y + 5, SSD1306_WHITE);
  display.drawFastHLine(61, y + 4, 8, SSD1306_WHITE);
  display.drawFastHLine(61, y + 5, 8, SSD1306_WHITE);
  display.drawLine(69, y + 4, 76, y, SSD1306_WHITE);
  display.drawLine(69, y + 5, 76, y + 1, SSD1306_WHITE);
}

static void drawBigEyesOnly(Adafruit_SSD1306 &display, int16_t y, int16_t leftHeight, int16_t rightHeight) {
  uint8_t frame = (millis() / 180) % 24;
  int8_t bob = faceBob(frame);
  bool blink = frame == 22;
  int16_t leftH = styledEyeHeight(leftHeight);
  int16_t rightH = styledEyeHeight(rightHeight);
  int16_t leftY = y + bob + (32 - leftH) / 2;
  int16_t rightY = y + bob + (32 - rightH) / 2;

  if (blink) {
    display.drawFastHLine(20, y + bob + 16, 24, SSD1306_WHITE);
    display.drawFastHLine(84, y + bob + 16, 24, SSD1306_WHITE);
    display.drawFastHLine(22, y + bob + 17, 20, SSD1306_WHITE);
    display.drawFastHLine(86, y + bob + 17, 20, SSD1306_WHITE);
  } else {
    display.fillRoundRect(20, leftY, 24, leftH, 8, SSD1306_WHITE);
    display.fillRoundRect(84, rightY, 24, rightH, 8, SSD1306_WHITE);
  }
}

static void drawEyes(Adafruit_SSD1306 &display, int16_t y, int16_t leftHeight, int16_t rightHeight, int16_t mouthY) {
  uint8_t frame = (millis() / 180) % 24;
  drawBigEyesOnly(display, y, leftHeight, rightHeight);
  drawSmallMouth(display, mouthY + faceBob(frame));
}

static void drawSmilingEyes(Adafruit_SSD1306 &display, int16_t y, int16_t leftHeight, int16_t rightHeight, int16_t mouthY) {
  drawBigEyesOnly(display, y, leftHeight, rightHeight);
  drawFriendlySmile(display, mouthY);
}

static void drawTinyHeart(Adafruit_SSD1306 &display, int16_t x, int16_t y) {
  display.drawPixel(x, y, SSD1306_WHITE);
  display.drawPixel(x + 2, y, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 2, SSD1306_WHITE);
}

static void drawHeartEyes(Adafruit_SSD1306 &display) {
  uint8_t frame = (millis() / 120) % 32;
  uint8_t gaze = (millis() / 520) % 6;
  int8_t bob = frame < 8 ? -1 : (frame < 16 ? 0 : (frame < 24 ? 1 : 0));
  bool blink = frame == 26 || frame == 27;
  int8_t lookX = 0;
  int8_t lookY = 0;

  switch (gaze) {
    case 0: lookX = -3; break;
    case 1: lookX = 3; break;
    case 2: lookY = -2; break;
    case 3: lookY = 2; break;
    case 4: lookX = -2; lookY = 1; break;
    case 5: lookX = 2; lookY = -1; break;
  }

  int16_t eyeY = 14 + bob + lookY;
  int16_t leftX = 20 + lookX;
  int16_t rightX = 84 + lookX;
  if (blink) {
    display.drawFastHLine(20, 30 + bob, 24, SSD1306_WHITE);
    display.drawFastHLine(22, 31 + bob, 20, SSD1306_WHITE);
    display.drawFastHLine(84, 30 + bob, 24, SSD1306_WHITE);
    display.drawFastHLine(86, 31 + bob, 20, SSD1306_WHITE);
  } else {
    display.fillRoundRect(leftX, eyeY, 24, 30, 8, SSD1306_WHITE);
    display.fillRoundRect(rightX, eyeY, 24, 30, 8, SSD1306_WHITE);
    display.drawPixel(leftX + 8 + (frame % 3), eyeY + 8, SSD1306_BLACK);
    display.drawPixel(leftX + 16, eyeY + 17 + (frame % 2), SSD1306_BLACK);
    display.drawPixel(rightX + 8 + ((frame + 1) % 3), eyeY + 8, SSD1306_BLACK);
    display.drawPixel(rightX + 16, eyeY + 17 + ((frame + 1) % 2), SSD1306_BLACK);
  }

  drawFriendlySmile(display, 49);

  uint8_t floatA = frame % 7;
  uint8_t floatB = (frame + 3) % 7;
  drawTinyHeart(display, 5, 9 + floatA);
  drawTinyHeart(display, 15, 31 - floatB);
  drawTinyHeart(display, 8, 52 - floatA);
  drawTinyHeart(display, 116, 10 + floatB);
  drawTinyHeart(display, 121, 33 - floatA);
  drawTinyHeart(display, 112, 53 - floatB);
  if (frame % 8 < 4) {
    drawTinyHeart(display, 58, 6);
    drawTinyHeart(display, 67, 7);
  }
}

static void drawCheerfulIdle(Adafruit_SSD1306 &display) {
  uint32_t now = millis();
  uint8_t frame = (now / 140) % 32;
  uint8_t gaze = (now / 1550) % 7;
  int8_t bob = frame < 8 ? -1 : (frame < 16 ? 0 : (frame < 24 ? 1 : 0));
  bool blink = frame == 24 || frame == 25 || gaze == 6;
  int8_t lookX = 0;
  int8_t lookY = 0;

  switch (gaze) {
    case 0: lookX = -6; break;
    case 1: lookX = 6; break;
    case 2: lookY = -3; break;
    case 3: lookY = 3; break;
    case 4: lookX = -4; lookY = -2; break;
    case 5: lookX = 4; lookY = 2; break;
    default: break;
  }

  int16_t eyeY = 12 + bob + lookY;
  int16_t leftEyeX = 16 + lookX;
  int16_t rightEyeX = 91 + lookX;

  if (blink) {
    display.drawFastHLine(15, 29 + bob, 28, SSD1306_WHITE);
    display.drawFastHLine(17, 30 + bob, 24, SSD1306_WHITE);
    display.drawFastHLine(90, 29 + bob, 28, SSD1306_WHITE);
    display.drawFastHLine(92, 30 + bob, 24, SSD1306_WHITE);
  } else {
    display.fillRoundRect(leftEyeX, eyeY, 22, 31, 8, SSD1306_WHITE);
    display.fillRoundRect(rightEyeX, eyeY, 22, 31, 8, SSD1306_WHITE);
    display.drawPixel(leftEyeX + 14, eyeY + 7, SSD1306_BLACK);
    display.drawPixel(leftEyeX + 15, eyeY + 8, SSD1306_BLACK);
    display.drawPixel(rightEyeX + 14, eyeY + 7, SSD1306_BLACK);
    display.drawPixel(rightEyeX + 15, eyeY + 8, SSD1306_BLACK);
  }

  drawSmallMouth(display, 43 + bob);
}

static void drawGreeting(Adafruit_SSD1306 &display, const String &userName) {
  uint8_t frame = (millis() / 160) % 18;
  drawBigEyesOnly(display, 11, 18, 18);
  drawFriendlySmile(display, 45);
  drawTinyHeart(display, 8, 16 + (frame % 5));
  drawTinyHeart(display, 116, 21 + ((frame + 2) % 5));
  char greeting[20];
  snprintf(greeting, sizeof(greeting), "Hi %.12s", userName.c_str());
  drawCenteredText(display, greeting, 55);
}

static void drawSun(Adafruit_SSD1306 &display, int x, int y) {
  display.drawCircle(x, y, 5, SSD1306_WHITE);
  display.drawPixel(x, y - 9, SSD1306_WHITE);
  display.drawPixel(x, y + 9, SSD1306_WHITE);
  display.drawPixel(x - 9, y, SSD1306_WHITE);
  display.drawPixel(x + 9, y, SSD1306_WHITE);
  display.drawPixel(x - 6, y - 6, SSD1306_WHITE);
  display.drawPixel(x + 6, y - 6, SSD1306_WHITE);
  display.drawPixel(x - 6, y + 6, SSD1306_WHITE);
  display.drawPixel(x + 6, y + 6, SSD1306_WHITE);
}

static void drawCloud(Adafruit_SSD1306 &display, int x, int y) {
  display.drawCircle(x, y, 4, SSD1306_WHITE);
  display.drawCircle(x + 6, y - 2, 5, SSD1306_WHITE);
  display.drawCircle(x + 13, y, 4, SSD1306_WHITE);
  display.drawFastHLine(x - 3, y + 4, 20, SSD1306_WHITE);
}

static void drawRain(Adafruit_SSD1306 &display, bool heavy) {
  uint8_t drift = (millis() / 140) % 10;
  const int lightDrops[] = {6, 15, 113, 122};
  const int heavyDrops[] = {4, 12, 20, 108, 116, 124};
  const int *drops = heavy ? heavyDrops : lightDrops;
  uint8_t count = heavy ? 6 : 4;

  for (uint8_t i = 0; i < count; i++) {
    int x = drops[i];
    int y = 9 + ((i * 13 + drift) % 45);
    display.drawLine(x, y, x - 2, y + 6, SSD1306_WHITE);
  }
}

static void drawLightning(Adafruit_SSD1306 &display, int x, int y) {
  display.drawLine(x, y, x - 4, y + 8, SSD1306_WHITE);
  display.drawLine(x - 4, y + 8, x + 1, y + 8, SSD1306_WHITE);
  display.drawLine(x + 1, y + 8, x - 5, y + 20, SSD1306_WHITE);
}

static void drawMoon(Adafruit_SSD1306 &display, MoonPhase phase) {
  const int x = 114;
  const int y = 12;
  display.drawCircle(x, y, 7, SSD1306_WHITE);
  if (phase == MoonPhase::NewMoon) {
    display.fillCircle(x, y, 5, SSD1306_BLACK);
    display.drawCircle(x, y, 7, SSD1306_WHITE);
  } else if (phase == MoonPhase::Crescent) {
    display.fillCircle(x + 3, y - 1, 7, SSD1306_BLACK);
  } else if (phase == MoonPhase::Half) {
    display.fillRect(x, y - 7, 8, 15, SSD1306_BLACK);
    display.drawCircle(x, y, 7, SSD1306_WHITE);
  } else {
    display.fillCircle(x, y, 6, SSD1306_WHITE);
    display.drawPixel(x - 2, y - 2, SSD1306_BLACK);
    display.drawPixel(x + 2, y + 2, SSD1306_BLACK);
  }
}

static void drawPomodoro(Adafruit_SSD1306 &display, const PomodoroTimer &pomodoro) {
  uint32_t remaining = pomodoro.remainingSeconds(millis());
  uint8_t minutes = remaining / 60;
  uint8_t seconds = remaining % 60;
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", minutes, seconds);
  drawCentered(display, buffer, 28, 2);
  drawCentered(display, pomodoro.isRunning() ? "running" : "paused", 54);
}

static void drawAttentionFlash(Adafruit_SSD1306 &display) {
  uint8_t phase = (millis() / 180) % 4;
  if (phase < 2) {
    display.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
    display.drawRect(2, 2, OLED_WIDTH - 4, OLED_HEIGHT - 4, SSD1306_WHITE);
    display.fillRect(0, 0, 12, 5, SSD1306_WHITE);
    display.fillRect(OLED_WIDTH - 12, 0, 12, 5, SSD1306_WHITE);
    display.fillRect(0, OLED_HEIGHT - 5, 12, 5, SSD1306_WHITE);
    display.fillRect(OLED_WIDTH - 12, OLED_HEIGHT - 5, 12, 5, SSD1306_WHITE);
  } else {
    display.drawRect(1, 1, OLED_WIDTH - 2, OLED_HEIGHT - 2, SSD1306_WHITE);
    display.fillRect(0, 10, 4, 12, SSD1306_WHITE);
    display.fillRect(0, 42, 4, 12, SSD1306_WHITE);
    display.fillRect(OLED_WIDTH - 4, 10, 4, 12, SSD1306_WHITE);
    display.fillRect(OLED_WIDTH - 4, 42, 4, 12, SSD1306_WHITE);
  }
}

static void drawPomodoroComplete(Adafruit_SSD1306 &display) {
  drawAttentionFlash(display);
  drawSmilingEyes(display, 12, 18, 18, 45);
  drawCentered(display, "Pomodoro", 2);
  drawCentered(display, "complete", 54);
}

static void drawHydrationStatus(Adafruit_SSD1306 &display, const ReminderService &reminders) {
  drawCentered(display, "Hydration", 8);
  drawCentered(display, String(reminders.minutesUntilHydration(millis())) + " min", 25, 2);
  drawCentered(display, "next reminder", 54);
}

static void drawHydrationComplete(Adafruit_SSD1306 &display) {
  drawAttentionFlash(display);
  drawSmilingEyes(display, 12, 18, 18, 45);
  display.drawTriangle(116, 7, 111, 19, 121, 19, SSD1306_WHITE);
  display.drawCircle(116, 21, 4, SSD1306_WHITE);
  drawCentered(display, "Hydration", 2);
  drawCentered(display, "complete", 54);
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

static void drawSettingInfo(Adafruit_SSD1306 &display, const AppState &state, bool wifiConnected, const String &ssid, const IPAddress &ip) {
  if (wifiConnected) {
    drawCentered(display, "WiFi connected", 18);
    drawCentered(display, shortText(ssid, 20), 29);
    drawCentered(display, String("IP ") + ip.toString(), 40);
  } else {
    drawCentered(display, "WiFi not connected", 18);
    drawCentered(display, "Setup AP TinyBotSetup", 31);
    drawCentered(display, "AP IP 192.168.4.1", 42);
  }
  drawCentered(display, state.dashboardToken.length() ? state.dashboardToken : String("Token missing"), 54);
}

static void drawWifiSetupFailed(Adafruit_SSD1306 &display) {
  drawCentered(display, "WiFi failed", 10);
  drawCentered(display, "Check name/pass", 25);
  drawCentered(display, "2.4GHz only", 38);
  drawCentered(display, "Face in 30s", 53);
}

static uint32_t displayEpochFromState(const AppState &state) {
  if (state.weather.epochAtSync == 0 || state.weather.lastSyncAt == 0) {
    return 0;
  }
  return state.weather.epochAtSync + ((millis() - state.weather.lastSyncAt) / 1000UL);
}

static String localTimeText(const AppState &state) {
  uint32_t epoch = displayEpochFromState(state);
  if (epoch == 0) {
    return "--:--";
  }
  int64_t adjustedEpoch = static_cast<int64_t>(epoch) + static_cast<int64_t>(state.weather.timezoneOffsetMinutes) * 60LL;
  time_t localEpoch = static_cast<time_t>(adjustedEpoch);
  struct tm *timeInfo = gmtime(&localEpoch);
  if (!timeInfo) {
    return "--:--";
  }
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);
  return String(buffer);
}

static void drawTimeWeatherInfo(Adafruit_SSD1306 &display, const AppState &state) {
  bool weatherAvailable = state.weather.wifiConnected && state.weather.hasData;
  WeatherTheme theme = state.weather.manualWeather ? state.weather.overrideTheme : state.weather.theme;
  String weatherText = weatherAvailable && theme != WeatherTheme::Unknown ? String("Weather ") + weatherThemeName(theme) : String("Weather unavailable");
  String tempText = weatherAvailable ? String("Temp ") + String(state.weather.temperatureC, 1) + " C" : String("WiFi not connected");
  String seasonText = weatherAvailable ? String("Season ") + seasonThemeName(state.weather.season) : String("Connect WiFi for weather");

  drawCentered(display, String("Time ") + localTimeText(state), 6, 1);
  drawCentered(display, weatherText, 21, 1);
  drawCentered(display, tempText, 36, 1);
  drawCentered(display, seasonText, 51, 1);
}

static void drawAmbientMotion(Adafruit_SSD1306 &display, FaceId face) {
  if (face == FaceId::RainyIdle || face == FaceId::MonsoonIdle || face == FaceId::StormyIdle) {
    return;
  }
  uint8_t frame = (millis() / 180) % 16;
  if (face == FaceId::Sleepy || face == FaceId::NightIdle) {
    display.setCursor(6, 21 - (frame % 5));
    display.print("z");
    display.setCursor(115, 10 - ((frame + 2) % 5));
    display.print("z");
    return;
  }
  display.drawPixel(7 + (frame % 5), 13, SSD1306_WHITE);
  display.drawPixel(9, 11 + (frame % 5), SSD1306_WHITE);
  display.drawPixel(118 - (frame % 5), 51, SSD1306_WHITE);
  display.drawPixel(120, 49 - (frame % 5), SSD1306_WHITE);
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
      drawSmilingEyes(display, 14, 18, 18, 49);
      break;
    case FaceId::Playful:
      display.fillRoundRect(20, 15 + bob, 24, 28, 8, SSD1306_WHITE);
      display.drawFastHLine(84, 29 + bob, 24, SSD1306_WHITE);
      display.drawFastHLine(86, 30 + bob, 20, SSD1306_WHITE);
      drawFriendlySmile(display, 49);
      break;
    case FaceId::Hungry:
    case FaceId::Feed:
      drawBigEyesOnly(display, 17, 12, 12);
      drawSmallOpenMouth(display, 48 + bob);
      display.drawCircle(10, 49, 3, SSD1306_WHITE);
      display.drawCircle(118, 49, 3, SSD1306_WHITE);
      break;
    case FaceId::Full:
      drawBigEyesOnly(display, 16, 11, 11);
      drawSmallOpenMouth(display, 49 + bob);
      break;
    case FaceId::Sleepy:
      display.drawFastHLine(20, 31 + bob, 24, SSD1306_WHITE);
      display.drawFastHLine(84, 31 + bob, 24, SSD1306_WHITE);
      drawSmallMouth(display, 50 + bob);
      break;
    case FaceId::Wake:
    case FaceId::Excited:
      drawBigEyesOnly(display, 12, 20, 20);
      drawSmallOpenMouth(display, 50 + bob);
      break;
    case FaceId::Sad:
    case FaceId::Lonely:
      drawBigEyesOnly(display, 18, 10, 10);
      drawSmallSadMouth(display, 50 + bob);
      break;
    case FaceId::Love:
      drawHeartEyes(display);
      break;
    case FaceId::Proud:
      drawSmilingEyes(display, 15, 16, 16, 49);
      display.drawLine(19, 10, 44, 8, SSD1306_WHITE);
      display.drawLine(84, 8, 109, 10, SSD1306_WHITE);
      break;
    case FaceId::Pomodoro:
      drawEyes(display, 16, 8, 8, 50);
      display.drawCircle(116, 13, 6, SSD1306_WHITE);
      display.drawFastVLine(116, 7, 3, SSD1306_WHITE);
      break;
    case FaceId::BreakTime:
      drawSmilingEyes(display, 17, 13, 13, 49);
      display.drawCircle(116, 13, 5, SSD1306_WHITE);
      display.drawFastHLine(113, 13, 6, SSD1306_WHITE);
      break;
    case FaceId::Hydration:
      drawSmilingEyes(display, 17, 14, 14, 49);
      display.drawTriangle(116, 7, 111, 19, 121, 19, SSD1306_WHITE);
      display.drawCircle(116, 21, 4, SSD1306_WHITE);
      break;
    case FaceId::SunnyIdle:
      drawSmilingEyes(display, 17, 13, 13, 49);
      drawSun(display, 116, 11);
      break;
    case FaceId::RainyIdle:
      drawRain(display, false);
      drawSmilingEyes(display, 18, 10, 10, 50);
      break;
    case FaceId::MonsoonIdle:
      drawRain(display, true);
      drawSmilingEyes(display, 18, 9, 9, 51);
      break;
    case FaceId::CloudyIdle:
      drawSmilingEyes(display, 18, 11, 11, 50);
      drawCloud(display, 105, 10);
      break;
    case FaceId::StormyIdle:
      drawRain(display, true);
      drawSmilingEyes(display, 18, 8, 8, 51);
      if (frame % 4 < 2) drawLightning(display, 120, 6);
      break;
    case FaceId::FoggyIdle:
      drawSmilingEyes(display, 18, 10, 10, 49);
      display.drawFastHLine(2, 12, 18, SSD1306_WHITE);
      display.drawFastHLine(109, 18, 18, SSD1306_WHITE);
      display.drawFastHLine(3, 53, 20, SSD1306_WHITE);
      break;
    case FaceId::WindyIdle:
      drawSmilingEyes(display, 17, 14, 7, 49);
      display.drawFastHLine(2, 14, 18, SSD1306_WHITE);
      display.drawFastHLine(109, 22, 18, SSD1306_WHITE);
      display.drawFastHLine(3, 53, 20, SSD1306_WHITE);
      break;
    case FaceId::HotIdle:
      drawSmilingEyes(display, 18, 8, 8, 51);
      drawSun(display, 116, 11);
      break;
    case FaceId::ColdIdle:
    case FaceId::WinterIdle:
      drawSmilingEyes(display, 18, 10, 10, 50);
      display.drawPixel(116, 10, SSD1306_WHITE);
      display.drawPixel(122, 18, SSD1306_WHITE);
      display.drawPixel(112, 25, SSD1306_WHITE);
      break;
    case FaceId::TimeWeatherInfo:
      drawTimeWeatherInfo(display, state);
      break;
    case FaceId::MorningIdle:
      drawSmilingEyes(display, 16, 12, 12, 49);
      drawSun(display, 116, 11);
      display.drawPixel(7, 18, SSD1306_WHITE);
      display.drawPixel(12, 22, SSD1306_WHITE);
      break;
    case FaceId::AfternoonIdle:
      drawBigEyesOnly(display, 19, 7, 7);
      drawFriendlySmile(display, 51);
      display.drawLine(116, 12, 111, 22, SSD1306_WHITE);
      display.drawCircle(111, 25, 3, SSD1306_WHITE);
      break;
    case FaceId::EveningIdle:
      drawSmilingEyes(display, 18, 12, 12, 49);
      display.drawRoundRect(111, 45, 12, 8, 2, SSD1306_WHITE);
      display.drawCircle(124, 49, 3, SSD1306_WHITE);
      display.drawFastHLine(109, 54, 17, SSD1306_WHITE);
      display.drawPixel(115, 35, SSD1306_WHITE);
      display.drawPixel(119, 32, SSD1306_WHITE);
      break;
    case FaceId::NightIdle:
      display.drawFastHLine(20, 31 + bob, 24, SSD1306_WHITE);
      display.drawFastHLine(84, 31 + bob, 24, SSD1306_WHITE);
      drawFriendlySmile(display, 50);
      display.drawCircle(12, 13, 5, SSD1306_WHITE);
      display.fillCircle(15, 12, 5, SSD1306_BLACK);
      break;
    case FaceId::NewMoonIdle:
      drawSmilingEyes(display, 18, 10, 10, 50);
      drawMoon(display, MoonPhase::NewMoon);
      break;
    case FaceId::CrescentMoonIdle:
      drawSmilingEyes(display, 18, 10, 10, 50);
      drawMoon(display, MoonPhase::Crescent);
      break;
    case FaceId::HalfMoonIdle:
      drawSmilingEyes(display, 18, 10, 10, 50);
      drawMoon(display, MoonPhase::Half);
      break;
    case FaceId::FullMoonIdle:
      drawSmilingEyes(display, 18, 10, 10, 50);
      drawMoon(display, MoonPhase::Full);
      break;
    case FaceId::SpringIdle:
      drawSmilingEyes(display, 17, 14, 14, 49);
      display.drawCircle(115, 14, 2, SSD1306_WHITE);
      display.drawCircle(111, 18, 2, SSD1306_WHITE);
      display.drawCircle(119, 18, 2, SSD1306_WHITE);
      display.drawCircle(115, 22, 2, SSD1306_WHITE);
      break;
    case FaceId::SummerIdle:
      drawSmilingEyes(display, 16, 16, 16, 49);
      drawSun(display, 116, 11);
      break;
    case FaceId::AutumnIdle:
      drawSmilingEyes(display, 18, 12, 12, 50);
      display.drawLine(115, 12, 122, 20, SSD1306_WHITE);
      display.drawLine(122, 20, 112, 25, SSD1306_WHITE);
      display.drawLine(112, 25, 115, 12, SSD1306_WHITE);
      break;
    case FaceId::Annoyed:
      drawBigEyesOnly(display, 19, 6, 6);
      display.drawLine(18 + shake, 12, 45 + shake, 20, SSD1306_WHITE);
      display.drawLine(110 + shake, 12, 83 + shake, 20, SSD1306_WHITE);
      drawSmallMouth(display, 51);
      break;
    case FaceId::Angry:
      drawBigEyesOnly(display, 19, 5, 5);
      display.drawLine(17 + shake, 10, 46 + shake, 21, SSD1306_WHITE);
      display.drawLine(111 + shake, 10, 82 + shake, 21, SSD1306_WHITE);
      drawSmallSadMouth(display, 49 + bob);
      break;
    case FaceId::Dizzy:
      drawBigEyesOnly(display, 16, 16, 16);
      display.drawLine(24, 21, 40, 37, SSD1306_BLACK);
      display.drawLine(40, 21, 24, 37, SSD1306_BLACK);
      display.drawLine(88, 21, 104, 37, SSD1306_BLACK);
      display.drawLine(104, 21, 88, 37, SSD1306_BLACK);
      drawSmallMouth(display, 51);
      break;
    case FaceId::Ignored:
    case FaceId::Bored:
      display.drawFastHLine(20, 31, 24, SSD1306_WHITE);
      display.drawFastHLine(84, 31, 24, SSD1306_WHITE);
      drawSmallMouth(display, 51);
      break;
    case FaceId::LowBattery:
      drawBigEyesOnly(display, 18, 8, 8);
      drawSmallSadMouth(display, 50);
      display.drawRect(111, 8, 13, 7, SSD1306_WHITE);
      display.drawRect(124, 10, 2, 3, SSD1306_WHITE);
      display.fillRect(113, 10, 3, 3, SSD1306_WHITE);
      break;
    case FaceId::Error:
      drawBigEyesOnly(display, 18, 7, 7);
      drawSmallOpenMouth(display, 50);
      display.drawTriangle(116, 8, 110, 22, 122, 22, SSD1306_WHITE);
      display.drawFastVLine(116, 12, 5, SSD1306_BLACK);
      display.drawPixel(116, 19, SSD1306_BLACK);
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
  if (state.companionMode != CompanionMode::Idle && state.activeReminder == ReminderKind::Hydration) return FaceId::Hydration;
  if (state.companionMode != CompanionMode::Idle && state.activeReminder == ReminderKind::Stretch) return FaceId::BreakTime;
  if (state.stats.energy < 18) return FaceId::Sleepy;
  if (state.stats.fullness < 18) return FaceId::Hungry;
  if (state.stats.happiness < 20) return FaceId::Lonely;
  if (state.stats.happiness < 36) return FaceId::Bored;
  return idleFace;
}

void renderDisplay(Adafruit_SSD1306 &display, const AppState &state, const PomodoroTimer &pomodoro, const ReminderService &reminders, bool wifiConnected, int rssi, const String &ssid, const IPAddress &ip) {
  display.clearDisplay();
  uint32_t now = millis();
  bool ipVisible = state.showIpUntil != 0 && static_cast<int32_t>(state.showIpUntil - now) > 0;
  bool homeModeScreen = state.deviceMode == DeviceMode::Online
    && state.displaySettings.idleAnimationEnabled
    && !ipVisible
    && state.companionMode == CompanionMode::Idle;
  bool faceOnlyScreen = homeModeScreen
    || (state.deviceMode == DeviceMode::Online
      && state.displaySettings.idleAnimationEnabled
      && !ipVisible
      && state.hasReactionFace);
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
      if (timerActive(state.wifiSetupFailedUntil, now)) {
        drawWifiSetupFailed(display);
      } else if (ipVisible) {
        drawCentered(display, "Dashboard", 12);
        drawCentered(display, String(ip[0]) + "." + String(ip[1]) + ".", 28);
        drawCentered(display, String(ip[2]) + "." + String(ip[3]), 40);
        drawCentered(display, "faces soon", 55);
      } else if (!state.displaySettings.idleAnimationEnabled) {
        drawCentered(display, DEVICE_NAME, 12);
        drawCentered(display, String(ip[0]) + "." + String(ip[1]) + ".", 28);
        drawCentered(display, String(ip[2]) + "." + String(ip[3]), 40);
        drawCentered(display, shortText(ssid, 18), 55);
      } else if (timerActive(state.pomodoroCompleteUntil, now)) {
        drawPomodoroComplete(display);
      } else if (timerActive(state.hydrationCompleteUntil, now)) {
        drawHydrationComplete(display);
      } else if (state.hasReactionFace && static_cast<int32_t>(state.reactionUntil - now) > 0) {
        drawFace(display, state.currentFace, state);
      } else if (state.companionMode == CompanionMode::Idle && state.homePanel == HomePanel::Pomodoro) {
        drawPomodoro(display, pomodoro);
      } else if (state.companionMode == CompanionMode::Idle && state.homePanel == HomePanel::Hydration) {
        drawHydrationStatus(display, reminders);
      } else if (state.companionMode == CompanionMode::Pomodoro) {
        drawPomodoro(display, pomodoro);
      } else if (state.companionMode == CompanionMode::Status) {
        drawStats(display, state);
      } else if (state.companionMode == CompanionMode::Settings) {
        drawSettingInfo(display, state, wifiConnected, ssid, ip);
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
