#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "companion_state.h"
#include "faces.h"
#include "pomodoro.h"
#include "reminders.h"
#include "touch_input.h"
#include "weather_service.h"
#include "web_dashboard.h"

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

AppState app;
WeatherService weather;
PomodoroTimer pomodoro;
ReminderService reminders;
WebDashboard dashboard;
TouchInput faceTouch(FACE_TOUCH_PIN);
TouchInput actionTouch(ACTION_TOUCH_PIN);

String savedSsid;
bool portalRunning = false;
bool runtimeTouchEnabled = ENABLE_TOUCH_NEXT;
bool runtimeSoundEnabled = ENABLE_SOUND;
bool forceSetupOnBoot = false;
bool webServerStarted = false;
bool webRoutesRegistered = false;
bool mdnsStarted = false;
uint32_t lastFrameAt = 0;
uint32_t lastConnectAttemptAt = 0;
uint32_t connectingStartedAt = 0;
uint32_t pokeWindowStartedAt = 0;
uint8_t pokeCount = 0;

static void toneOnce(int frequency, int durationMs) {
  if (!runtimeSoundEnabled || BUZZER_PIN < 0) {
    return;
  }

  ledcWriteTone(BUZZER_CHANNEL, frequency);
  ledcWrite(BUZZER_CHANNEL, BUZZER_DUTY);
  delay(durationMs);
  ledcWrite(BUZZER_CHANNEL, 0);
}

static void saveCompanionState() {
  preferences.putUChar("face", static_cast<uint8_t>(app.currentFace));
  preferences.putUChar("fullness", app.stats.fullness);
  preferences.putUChar("happy", app.stats.happiness);
  preferences.putUChar("energy", app.stats.energy);
  preferences.putString("lastAction", app.lastAction);
  preferences.putBool("sleep", app.sleepRequested);
  preferences.putULong("greetDay", app.lastGreetingDay);
}

static void saveRuntimeSettings() {
  preferences.putString("userName", app.userName);
  preferences.putUChar("bright", app.displaySettings.brightness);
  preferences.putBool("invert", app.displaySettings.inverted);
  preferences.putBool("idleAnim", app.displaySettings.idleAnimationEnabled);
  preferences.putBool("touch", runtimeTouchEnabled);
  preferences.putBool("sound", runtimeSoundEnabled);
  preferences.putUShort("focus", app.pomodoroSettings.focusMinutes);
  preferences.putBool("hydOn", app.reminderSettings.hydrationEnabled);
  preferences.putBool("stretchOn", app.reminderSettings.stretchEnabled);
  preferences.putUShort("hydMin", app.reminderSettings.hydrationMinutes);
  preferences.putUShort("stretchMin", app.reminderSettings.stretchMinutes);
  preferences.putULong("tapWin", app.touchSettings.tapWindowMs);
  preferences.putULong("longPress", app.touchSettings.longPressMs);
  preferences.putUChar("annoyPoke", app.touchSettings.annoyedPokeCount);
  preferences.putUChar("angryPoke", app.touchSettings.angryPokeCount);
}

static void loadRuntimeSettings() {
  app.currentFace = static_cast<FaceId>(preferences.getUChar("face", static_cast<uint8_t>(FaceId::CheerfulIdle)));
  app.stats.fullness = constrain(preferences.getUChar("fullness", 72), 0, 100);
  app.stats.happiness = constrain(preferences.getUChar("happy", 78), 0, 100);
  app.stats.energy = constrain(preferences.getUChar("energy", 66), 0, 100);
  app.lastAction = preferences.isKey("lastAction") ? preferences.getString("lastAction", "Ready") : "Ready";
  app.userName = preferences.isKey("userName") ? preferences.getString("userName", DEFAULT_USER_NAME) : DEFAULT_USER_NAME;
  app.userName.trim();
  if (app.userName.length() == 0) app.userName = DEFAULT_USER_NAME;
  app.sleepRequested = preferences.getBool("sleep", false);
  app.lastGreetingDay = preferences.getULong("greetDay", 0);

  app.displaySettings.brightness = constrain(preferences.getUChar("bright", 100), 1, 100);
  app.displaySettings.inverted = preferences.getBool("invert", false);
  app.displaySettings.idleAnimationEnabled = preferences.getBool("idleAnim", true);
  runtimeTouchEnabled = preferences.getBool("touch", ENABLE_TOUCH_NEXT);
  runtimeSoundEnabled = preferences.getBool("sound", ENABLE_SOUND);

  app.pomodoroSettings.focusMinutes = constrain(preferences.getUShort("focus", DEFAULT_FOCUS_MINUTES), 1, 120);
  app.reminderSettings.hydrationEnabled = preferences.getBool("hydOn", true);
  app.reminderSettings.stretchEnabled = preferences.getBool("stretchOn", true);
  app.reminderSettings.hydrationMinutes = constrain(preferences.getUShort("hydMin", DEFAULT_HYDRATION_MINUTES), 5, 240);
  app.reminderSettings.stretchMinutes = constrain(preferences.getUShort("stretchMin", DEFAULT_STRETCH_MINUTES), 5, 240);
  app.touchSettings.tapWindowMs = constrain(preferences.getULong("tapWin", TOUCH_TAP_WINDOW_MS), 180UL, 900UL);
  app.touchSettings.longPressMs = constrain(preferences.getULong("longPress", TOUCH_LONG_PRESS_MS), 500UL, 2500UL);
  app.touchSettings.annoyedPokeCount = constrain(preferences.getUChar("annoyPoke", ANNOYED_POKE_COUNT), 2, 12);
  app.touchSettings.angryPokeCount = constrain(preferences.getUChar("angryPoke", ANGRY_POKE_COUNT), 3, 20);
}

static uint32_t currentLocalDay(uint32_t now) {
  uint32_t epoch = weather.currentEpoch(now);
  if (epoch == 0) {
    return 0;
  }
  int64_t localEpoch = static_cast<int64_t>(epoch) + static_cast<int64_t>(app.weather.timezoneOffsetMinutes) * 60LL;
  if (localEpoch < 0) {
    return 0;
  }
  return static_cast<uint32_t>(localEpoch / 86400LL);
}

static String greetingActionText(const __FlashStringHelper *prefix) {
  String text(prefix);
  text += app.userName;
  return text;
}

static bool triggerDailyTouchGreeting(uint32_t now) {
  uint32_t localDay = currentLocalDay(now);
  if (localDay == 0 || localDay == app.lastGreetingDay) {
    return false;
  }
  app.lastGreetingDay = localDay;
  app.stats.happiness = clampStat(app.stats.happiness + 4);
  triggerReaction(app, FaceId::Greeting, greetingActionText(F("Hi ")), now, GREETING_FACE_MS);
  toneOnce(1568, 45);
  saveCompanionState();
  return true;
}

static void showFirstBootGreeting() {
  if (preferences.getBool("bootHello", false)) {
    return;
  }
  uint32_t now = millis();
  app.currentMessage = greetingActionText(F("Hi "));
  triggerReaction(app, FaceId::Greeting, greetingActionText(F("Welcome ")), now, GREETING_FACE_MS);
  preferences.putBool("bootHello", true);
  saveCompanionState();
  renderDisplay(display, app, pomodoro, reminders, false, 0, "", IPAddress());
  delay(2200);
}

static void applyDisplayCallback() {
  applyDisplaySettings(display, app.displaySettings);
  faceTouch.setTimings(app.touchSettings.tapWindowMs, app.touchSettings.longPressMs);
  actionTouch.setTimings(app.touchSettings.tapWindowMs, app.touchSettings.longPressMs);
}

static void renderNow() {
  uint32_t now = millis();
  if (now - lastFrameAt < FACE_FRAME_MS) {
    return;
  }
  lastFrameAt = now;
  FaceId idleFace = weather.idleFaceFor(now);
  app.currentFace = selectFace(app, pomodoro, reminders, idleFace, now);
  renderDisplay(display, app, pomodoro, reminders, WiFi.isConnected(), WiFi.isConnected() ? WiFi.RSSI() : 0, savedSsid.length() ? savedSsid : WiFi.SSID(), WiFi.localIP());
}

static void startSetupPortal();

static void startWebServer() {
  if (webServerStarted) {
    return;
  }
  if (!webRoutesRegistered) {
    dashboard.registerRoutes();
    webRoutesRegistered = true;
  }
  server.begin();
  webServerStarted = true;
  Serial.println("[web] server started");
}

static void startMdns() {
  if (mdnsStarted || !WiFi.isConnected()) {
    return;
  }
  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
    Serial.print("[mdns] http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
  } else {
    Serial.println("[mdns] failed");
  }
}

static bool connectToSavedWifi(uint32_t timeoutMs) {
  if (savedSsid.length() == 0) {
    return false;
  }

  if (portalRunning) {
    dnsServer.stop();
    portalRunning = false;
  }

  app.deviceMode = DeviceMode::Connecting;
  connectingStartedAt = millis();
  app.currentMessage = "Joining WiFi";
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(savedSsid.c_str(), preferences.getString("password", "").c_str());

  while (millis() - connectingStartedAt < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      app.deviceMode = DeviceMode::Online;
      app.companionMode = CompanionMode::Idle;
      app.sleepRequested = false;
      app.hasReactionFace = false;
      app.showIpUntil = millis() + ONLINE_IP_SCREEN_MS;
      app.currentMessage = "Connected";
      startWebServer();
      startMdns();
      weather.syncTime();
      weather.update(millis(), true);
      toneOnce(1568, 55);
      return true;
    }
    server.handleClient();
    renderNow();
    delay(60);
  }

  WiFi.disconnect();
  return false;
}

static void saveWifiAndConnect(const String &ssid, const String &password) {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  savedSsid = ssid;

  WiFi.softAPdisconnect(false);
  if (!connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS)) {
    startSetupPortal();
  }
}

static void forgetWifi() {
  preferences.remove("ssid");
  preferences.remove("password");
  savedSsid = "";
  WiFi.disconnect(true);
  delay(250);
  ESP.restart();
}

static void forceWeatherSync() {
  weather.fetchNow();
}

static void startSetupPortal() {
  Serial.println("[setup-ap] starting");
  app.deviceMode = DeviceMode::SetupPortal;
  app.currentFace = FaceId::Neutral;
  app.currentMessage = "Setup needed";
  portalRunning = true;

  WiFi.persistent(false);
  WiFi.disconnect(false);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(150);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.softAPConfig(SETUP_AP_IP, SETUP_AP_IP, IPAddress(255, 255, 255, 0));
  bool apStarted = strlen(SETUP_AP_PASSWORD) == 0
    ? WiFi.softAP(SETUP_AP_SSID, nullptr, SETUP_AP_CHANNEL, false, 4)
    : WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASSWORD, SETUP_AP_CHANNEL, false, 4);
  Serial.print("Setup AP ");
  Serial.println(apStarted ? "started" : "failed");
  Serial.print("Setup AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (apStarted) {
    dnsServer.start(53, "*", SETUP_AP_IP);
    app.currentMessage = "Setup AP ready";
    startWebServer();
  } else {
    app.currentFace = FaceId::Error;
    app.currentMessage = "AP failed";
  }
  renderDisplay(display, app, pomodoro, reminders, false, 0, SETUP_AP_SSID, SETUP_AP_IP);
  toneOnce(988, 45);
}

static CompanionMode nextCompanionMode(CompanionMode mode) {
  switch (mode) {
    case CompanionMode::Idle: return CompanionMode::Pomodoro;
    case CompanionMode::Pomodoro: return CompanionMode::Clock;
    case CompanionMode::Clock: return CompanionMode::Reminders;
    case CompanionMode::Reminders: return CompanionMode::Status;
    case CompanionMode::Status:
    default: return CompanionMode::Idle;
  }
}

static void prepareModePreview(CompanionMode mode, uint32_t now) {
  if (mode == CompanionMode::Pomodoro && !pomodoro.isRunning()) {
    pomodoro.reset(now);
  }
}

static void selectCompanionMode(CompanionMode mode, uint32_t now) {
  prepareModePreview(mode, now);
  app.companionMode = mode;
  app.hasReactionFace = false;
  app.lastAction = companionModeName(mode);
  app.lastInteractionAt = now;
  saveCompanionState();
}

static bool currentModeIsStarted() {
  if (app.companionMode == CompanionMode::Pomodoro) {
    return pomodoro.isRunning();
  }
  return false;
}

static void autoReturnToFaceMode(uint32_t now) {
  if (app.companionMode == CompanionMode::Idle) {
    return;
  }
  if (currentModeIsStarted()) {
    app.lastInteractionAt = now;
    return;
  }
  if (app.lastInteractionAt == 0) {
    app.lastInteractionAt = now;
    return;
  }
  if (now - app.lastInteractionAt >= MODE_PREVIEW_TIMEOUT_MS) {
    selectCompanionMode(CompanionMode::Idle, now);
  }
}

static void handleFaceGesture(TouchGesture gesture, uint32_t now) {
  if (gesture == TouchGesture::None) return;

  if (triggerDailyTouchGreeting(now)) {
    return;
  }

  if (gesture == TouchGesture::SingleTap) {
    if (now - pokeWindowStartedAt > POKE_WINDOW_MS) {
      pokeWindowStartedAt = now;
      pokeCount = 0;
    }
    pokeCount++;
    if (pokeCount >= app.touchSettings.angryPokeCount) {
      app.stats.happiness = clampStat(app.stats.happiness - 10);
      triggerReaction(app, FaceId::Angry, "Too many pokes", now, 5000);
    } else if (pokeCount >= app.touchSettings.annoyedPokeCount) {
      app.stats.happiness = clampStat(app.stats.happiness - 4);
      triggerReaction(app, FaceId::Annoyed, "Annoyed", now, 4200);
    } else {
      app.stats.happiness = clampStat(app.stats.happiness + 2);
      triggerReaction(app, FaceId::Poke, "Poked", now);
    }
    toneOnce(988, 35);
  } else if (gesture == TouchGesture::DoubleTap) {
    pokeCount = 0;
    app.stats.fullness = clampStat(app.stats.fullness + 24);
    app.stats.happiness = clampStat(app.stats.happiness + 6);
    app.stats.energy = clampStat(app.stats.energy + 3);
    triggerReaction(app, app.stats.fullness > 92 ? FaceId::Full : FaceId::Feed, "Fed", now);
    toneOnce(1175, 40);
  } else if (gesture == TouchGesture::TripleTap) {
    pokeCount = 0;
    app.stats.happiness = clampStat(app.stats.happiness + 15);
    app.stats.energy = clampStat(app.stats.energy + 4);
    triggerReaction(app, FaceId::Love, "Loved", now);
    toneOnce(1319, 45);
  } else if (gesture == TouchGesture::LongPress) {
    pokeCount = 0;
    app.stats.happiness = clampStat(app.stats.happiness + 18);
    app.stats.energy = clampStat(app.stats.energy + 4);
    triggerReaction(app, FaceId::Love, "Petted", now, PETTING_FACE_MS);
    toneOnce(1568, 50);
  }

  saveCompanionState();
}

static void handleActionGesture(TouchGesture gesture, uint32_t now) {
  if (gesture == TouchGesture::None) return;

  if (gesture == TouchGesture::LongPress) {
    selectCompanionMode(nextCompanionMode(app.companionMode), now);
    return;
  }

  if (app.companionMode == CompanionMode::Pomodoro) {
    return;
  }

  app.lastAction = String(companionModeName(app.companionMode)) + " action";
  app.lastInteractionAt = now;
  saveCompanionState();
}

static void decayStats(uint32_t now) {
  if (app.lastStatsDecayAt == 0) {
    app.lastStatsDecayAt = now;
    return;
  }
  if (now - app.lastStatsDecayAt < STATS_DECAY_INTERVAL_MS) {
    return;
  }
  app.lastStatsDecayAt = now;
  app.stats.fullness = clampStat(app.stats.fullness - 2);
  app.stats.happiness = clampStat(app.stats.happiness - 1);
  if (!app.sleepRequested) {
    app.stats.energy = clampStat(app.stats.energy - 1);
  }
  saveCompanionState();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[boot] Tiny Companion starting");

  Serial.println("[boot] starting i2c/display");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("SSD1306 init failed");
    while (true) {
      delay(1000);
    }
  }
  if (OLED_FLIP_180) {
    display.setRotation(2);
  }

  Serial.println("[boot] starting touch inputs");
  faceTouch.begin();
  actionTouch.begin();
  delay(80);
  forceSetupOnBoot = (FACE_TOUCH_PIN >= 0 && digitalRead(FACE_TOUCH_PIN) == HIGH) || (ACTION_TOUCH_PIN >= 0 && digitalRead(ACTION_TOUCH_PIN) == HIGH);

  if (ENABLE_SOUND && BUZZER_PIN >= 0) {
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RESOLUTION_BITS);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  }

  Serial.println("[boot] opening preferences");
  preferences.begin("wifi", false);
  loadRuntimeSettings();
  showFirstBootGreeting();
  Serial.println("[boot] loading weather/settings");
  weather.begin(preferences, app.weather);
  applyDisplayCallback();
  pomodoro.begin(app.pomodoroSettings);
  reminders.begin(app.reminderSettings, millis());

  Serial.println("[boot] drawing startup screen");
  display.clearDisplay();
  app.currentMessage = "Starting WiFi";
  renderDisplay(display, app, pomodoro, reminders, false, 0, "", IPAddress());

  Serial.println("[boot] starting dashboard routes");
  dashboard.begin(server, app, weather, pomodoro, reminders);
  dashboard.setCallbacks(saveCompanionState, saveRuntimeSettings, applyDisplayCallback, saveWifiAndConnect, forgetWifi, forceWeatherSync);

  savedSsid = preferences.getString("ssid", "");
  if (START_SETUP_AP_ON_BOOT || forceSetupOnBoot) {
    Serial.println("[boot] setup AP requested");
    if (forceSetupOnBoot) {
      preferences.remove("ssid");
      preferences.remove("password");
      savedSsid = "";
    }
    startSetupPortal();
  } else if (!connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS)) {
    startSetupPortal();
  }
}

void loop() {
  uint32_t now = millis();

  if (portalRunning) {
    dnsServer.processNextRequest();
  }

  if (WiFi.isConnected()) {
    startMdns();
  }

  server.handleClient();

  if (savedSsid.length() > 0 && !WiFi.isConnected() && app.deviceMode != DeviceMode::SetupPortal) {
    if (now - lastConnectAttemptAt > WIFI_RECONNECT_INTERVAL_MS) {
      lastConnectAttemptAt = now;
      connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS);
    }
  } else if (WiFi.isConnected() && app.deviceMode != DeviceMode::Online) {
    app.deviceMode = DeviceMode::Online;
    app.companionMode = CompanionMode::Idle;
    app.showIpUntil = now + ONLINE_IP_SCREEN_MS;
  }

  weather.update(now, WiFi.isConnected());
  pomodoro.update(now);
  ReminderKind reminder = reminders.update(now);
  if (reminder != ReminderKind::None && app.activeReminder == ReminderKind::None) {
    app.activeReminder = reminder;
    triggerReaction(app, reminder == ReminderKind::Hydration ? FaceId::Hydration : FaceId::BreakTime, reminder == ReminderKind::Hydration ? "Drink water" : "Stretch break", now, 8000);
  }

  if (runtimeTouchEnabled) {
    handleFaceGesture(faceTouch.update(now), now);
    handleActionGesture(actionTouch.update(now), now);
  }

  autoReturnToFaceMode(now);
  decayStats(now);
  renderNow();
}
