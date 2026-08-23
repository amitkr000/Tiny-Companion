#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_system.h>

#include "companion_state.h"
#include "faces.h"
#include "mode_manager.h"
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
ModeManager modeManager;
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

static String generateDashboardToken() {
  char token[9];
  uint32_t value = esp_random();
  snprintf(token, sizeof(token), "%08lX", static_cast<unsigned long>(value));
  return String(token);
}

static String cleanStoredUserName(String value) {
  value.trim();
  String cleaned;
  cleaned.reserve(16);
  for (size_t i = 0; i < value.length() && cleaned.length() < 16; i++) {
    char c = value[i];
    bool alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    bool digit = c >= '0' && c <= '9';
    if (alpha || digit || c == ' ' || c == '-' || c == '_') {
      cleaned += c;
    }
  }
  cleaned.trim();
  return cleaned.length() > 0 ? cleaned : String(DEFAULT_USER_NAME);
}

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
  preferences.putString("apiToken", app.dashboardToken);
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
  app.userName = cleanStoredUserName(preferences.isKey("userName") ? preferences.getString("userName", DEFAULT_USER_NAME) : DEFAULT_USER_NAME);
  app.dashboardToken = preferences.isKey("apiToken") ? preferences.getString("apiToken", "") : "";
  if (app.dashboardToken.length() < 8) {
    app.dashboardToken = generateDashboardToken();
    preferences.putString("apiToken", app.dashboardToken);
  }
  app.sleepRequested = preferences.getBool("sleep", false);
  app.lastGreetingDay = preferences.getULong("greetDay", 0);

  app.displaySettings.brightness = constrain(preferences.getUChar("bright", 100), 1, 100);
  app.displaySettings.inverted = preferences.getBool("invert", false);
  app.displaySettings.idleAnimationEnabled = preferences.getBool("idleAnim", true);
  runtimeTouchEnabled = preferences.getBool("touch", ENABLE_TOUCH_NEXT);
  runtimeSoundEnabled = preferences.getBool("sound", ENABLE_SOUND);

  app.pomodoroSettings.focusMinutes = constrain(preferences.getUShort("focus", DEFAULT_FOCUS_MINUTES), 1, 120);
  if (!preferences.getBool("hydOffV1", false)) {
    app.reminderSettings.hydrationEnabled = false;
    preferences.putBool("hydOn", false);
    preferences.putBool("hydOffV1", true);
  } else {
    app.reminderSettings.hydrationEnabled = preferences.getBool("hydOn", false);
  }
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

static const char *gestureName(TouchGesture gesture) {
  switch (gesture) {
    case TouchGesture::SingleTap: return "single";
    case TouchGesture::DoubleTap: return "double";
    case TouchGesture::TripleTap: return "triple";
    case TouchGesture::LongPress: return "long";
    case TouchGesture::None:
    default: return "none";
  }
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

static bool timerActive(uint32_t until, uint32_t now) {
  return until != 0 && static_cast<int32_t>(until - now) > 0;
}

static void updateCompletionNotices(uint32_t now) {
  if (app.pomodoroCompleteUntil != 0 && !timerActive(app.pomodoroCompleteUntil, now)) {
    app.pomodoroCompleteUntil = 0;
  }
  if (app.hydrationCompleteUntil != 0 && !timerActive(app.hydrationCompleteUntil, now)) {
    app.hydrationCompleteUntil = 0;
    if (app.activeReminder == ReminderKind::Hydration) {
      app.activeReminder = ReminderKind::None;
    }
  }
}

static void updateHomePanel(uint32_t now) {
  app.homePanel = HomePanel::Face;
  if (app.companionMode != CompanionMode::Idle || (app.hasReactionFace && timerActive(app.reactionUntil, now))) {
    return;
  }

  bool pomodoroActive = pomodoro.isRunning();
  bool hydrationActive = app.reminderSettings.hydrationEnabled;
  if (!pomodoroActive && !hydrationActive) {
    return;
  }

  uint32_t loopPosition = now % IDLE_LOOP_MS;
  if (loopPosition < IDLE_CHEERFUL_MS) {
    return;
  }

  if (pomodoroActive && hydrationActive) {
    uint32_t activePosition = loopPosition - IDLE_CHEERFUL_MS;
    app.homePanel = activePosition < (ACTIVE_STATUS_FACE_MS / 2) ? HomePanel::Pomodoro : HomePanel::Hydration;
  } else if (pomodoroActive) {
    app.homePanel = HomePanel::Pomodoro;
  } else {
    app.homePanel = HomePanel::Hydration;
  }
}

static void renderNow() {
  uint32_t now = millis();
  if (now - lastFrameAt < FACE_FRAME_MS) {
    return;
  }
  lastFrameAt = now;
  updateCompletionNotices(now);
  FaceId idleFace = app.companionMode == CompanionMode::Idle && (pomodoro.isRunning() || app.reminderSettings.hydrationEnabled)
    ? FaceId::CheerfulIdle
    : weather.idleFaceFor(now);
  app.currentFace = selectFace(app, pomodoro, reminders, idleFace, now);
  updateHomePanel(now);
  renderDisplay(display, app, pomodoro, reminders, WiFi.isConnected(), WiFi.isConnected() ? WiFi.RSSI() : 0, savedSsid.length() ? savedSsid : WiFi.SSID(), WiFi.localIP());
}

static void startSetupPortal(bool showSetupScreen = true);

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
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.setAutoReconnect(true);
  WiFi.begin(savedSsid.c_str(), preferences.getString("password", "").c_str());

  while (millis() - connectingStartedAt < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      app.deviceMode = DeviceMode::Online;
      app.companionMode = CompanionMode::Idle;
      app.sleepRequested = false;
      app.hasReactionFace = false;
      app.showIpUntil = millis() + ONLINE_IP_SCREEN_MS;
      app.wifiSetupFailedUntil = 0;
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

  Serial.print("[wifi] connect failed status ");
  Serial.println(WiFi.status());
  WiFi.disconnect(false);
  return false;
}

static void saveWifiAndConnect(const String &ssid, const String &password) {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  savedSsid = ssid;

  WiFi.softAPdisconnect(false);
  if (!connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS)) {
    startSetupPortal(false);
    app.wifiSetupFailedUntil = millis() + WIFI_SETUP_FAILED_SCREEN_MS;
    app.currentFace = FaceId::Error;
    app.currentMessage = "WiFi failed";
    app.companionMode = CompanionMode::Idle;
    app.hasReactionFace = false;
    renderDisplay(display, app, pomodoro, reminders, false, 0, SETUP_AP_SSID, SETUP_AP_IP);
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

static void startSetupPortal(bool showSetupScreen) {
  Serial.println(showSetupScreen ? "[setup-ap] starting visible setup" : "[setup-ap] starting optional background setup");
  if (showSetupScreen) {
    app.deviceMode = DeviceMode::SetupPortal;
    app.currentFace = FaceId::Neutral;
    app.currentMessage = "Setup needed";
  } else {
    app.deviceMode = DeviceMode::Online;
    app.companionMode = CompanionMode::Idle;
    app.currentFace = FaceId::CheerfulIdle;
    app.currentMessage = "WiFi optional";
    app.showIpUntil = 0;
  }
  app.wifiSetupFailedUntil = 0;
  app.weather.wifiConnected = false;
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
    app.currentMessage = showSetupScreen ? "Setup AP ready" : "Face mode";
    startWebServer();
  } else if (showSetupScreen) {
    app.currentFace = FaceId::Error;
    app.currentMessage = "AP failed";
  } else {
    app.currentMessage = "Face mode";
  }
  if (showSetupScreen) {
    renderDisplay(display, app, pomodoro, reminders, false, 0, SETUP_AP_SSID, SETUP_AP_IP);
  }
  toneOnce(988, 45);
}


static void handleFaceGesture(TouchGesture gesture, uint32_t now) {
  if (gesture == TouchGesture::None) return;

  Serial.print("[touch] face ");
  Serial.println(gestureName(gesture));
  app.showIpUntil = 0;
  app.companionMode = CompanionMode::Idle;

  bool greetedToday = triggerDailyTouchGreeting(now);

  if (gesture == TouchGesture::SingleTap) {
    if (now - pokeWindowStartedAt > POKE_WINDOW_MS) {
      pokeWindowStartedAt = now;
      pokeCount = 0;
    }
    pokeCount++;
    if (pokeCount >= app.touchSettings.angryPokeCount) {
      app.stats.happiness = clampStat(app.stats.happiness - 10);
      if (!greetedToday) triggerReaction(app, FaceId::Angry, "Too many pokes", now, 5000);
    } else if (pokeCount >= app.touchSettings.annoyedPokeCount) {
      app.stats.happiness = clampStat(app.stats.happiness - 4);
      if (!greetedToday) triggerReaction(app, FaceId::Annoyed, "Annoyed", now, 4200);
    } else {
      app.stats.happiness = clampStat(app.stats.happiness + 2);
      if (!greetedToday) triggerReaction(app, FaceId::Poke, "Poked", now);
    }
    toneOnce(988, 35);
  } else if (gesture == TouchGesture::DoubleTap) {
    pokeCount = 0;
    app.stats.fullness = clampStat(app.stats.fullness + 24);
    app.stats.happiness = clampStat(app.stats.happiness + 6);
    app.stats.energy = clampStat(app.stats.energy + 3);
    if (!greetedToday) triggerReaction(app, app.stats.fullness > 92 ? FaceId::Full : FaceId::Feed, "Fed", now);
    toneOnce(1175, 40);
  } else if (gesture == TouchGesture::TripleTap) {
    pokeCount = 0;
    app.stats.happiness = clampStat(app.stats.happiness + 15);
    app.stats.energy = clampStat(app.stats.energy + 4);
    if (!greetedToday) triggerReaction(app, FaceId::Playful, "Played", now);
    toneOnce(1319, 45);
  } else if (gesture == TouchGesture::LongPress) {
    pokeCount = 0;
    app.stats.happiness = clampStat(app.stats.happiness + 18);
    app.stats.energy = clampStat(app.stats.energy + 4);
    if (!greetedToday) triggerReaction(app, FaceId::Love, "Petted", now, PETTING_FACE_MS);
    toneOnce(1568, 50);
  }

  saveCompanionState();
}

static void handleActionGesture(TouchGesture gesture, uint32_t now) {
  modeManager.handleActionGesture(gesture, now);
}

static bool setupResetGestureHeld() {
  if (FACE_TOUCH_PIN < 0 || ACTION_TOUCH_PIN < 0) {
    return false;
  }

  uint32_t startedAt = millis();
  while (millis() - startedAt < SETUP_RESET_HOLD_MS) {
    if (digitalRead(FACE_TOUCH_PIN) != HIGH || digitalRead(ACTION_TOUCH_PIN) != HIGH) {
      return false;
    }
    delay(25);
  }
  return true;
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
  forceSetupOnBoot = setupResetGestureHeld();
  if (forceSetupOnBoot) {
    Serial.println("[boot] setup reset gesture held");
  }

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
  ModeContext modeContext;
  modeContext.state = &app;
  modeContext.pomodoro = &pomodoro;
  modeContext.reminders = &reminders;
  modeContext.saveState = saveCompanionState;
  modeContext.saveRuntime = saveRuntimeSettings;
  modeManager.begin(modeContext);

  Serial.println("[boot] drawing startup screen");
  display.clearDisplay();
  app.currentMessage = "Starting";
  renderDisplay(display, app, pomodoro, reminders, false, 0, "", IPAddress());

  Serial.println("[boot] starting dashboard routes");
  dashboard.begin(server, app, weather, pomodoro, reminders);
  dashboard.setCallbacks(saveCompanionState, saveRuntimeSettings, applyDisplayCallback, saveWifiAndConnect, forgetWifi, forceWeatherSync);

  savedSsid = preferences.getString("ssid", "");
  if (START_SETUP_AP_ON_BOOT) {
    Serial.println("[boot] setup AP requested by config");
    startSetupPortal();
  } else if (forceSetupOnBoot) {
    Serial.println("[boot] clearing saved WiFi from reset gesture");
    preferences.remove("ssid");
    preferences.remove("password");
    savedSsid = "";
    startSetupPortal();
  } else if (savedSsid.length() > 0) {
    Serial.print("[boot] saved WiFi found: ");
    Serial.println(savedSsid);
    if (!connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS)) {
      Serial.println("[boot] saved WiFi failed, starting optional setup AP");
      startSetupPortal(false);
    }
  } else {
    Serial.println("[boot] no saved WiFi, starting optional setup AP");
    startSetupPortal(false);
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

  if (app.wifiSetupFailedUntil != 0 && static_cast<int32_t>(app.wifiSetupFailedUntil - now) <= 0) {
    app.wifiSetupFailedUntil = 0;
    app.currentMessage = "Face mode";
    app.currentFace = FaceId::CheerfulIdle;
    app.companionMode = CompanionMode::Idle;
  }

  server.handleClient();

  if (savedSsid.length() > 0 && !WiFi.isConnected() && app.deviceMode != DeviceMode::SetupPortal && !portalRunning) {
    if (now - lastConnectAttemptAt > WIFI_RECONNECT_INTERVAL_MS) {
      lastConnectAttemptAt = now;
      if (!connectToSavedWifi(WIFI_CONNECT_TIMEOUT_MS)) {
        startSetupPortal(false);
      }
    }
  } else if (WiFi.isConnected() && app.deviceMode != DeviceMode::Online) {
    app.deviceMode = DeviceMode::Online;
    app.companionMode = CompanionMode::Idle;
    app.showIpUntil = now + ONLINE_IP_SCREEN_MS;
  }

  app.weather.wifiConnected = WiFi.isConnected();
  weather.update(now, WiFi.isConnected());
  uint8_t completedBefore = pomodoro.completedSessions();
  pomodoro.update(now);
  if (pomodoro.completedSessions() != completedBefore) {
    app.pomodoroCompleteUntil = now + COMPLETION_SCREEN_MS;
    app.companionMode = CompanionMode::Idle;
    app.homePanel = HomePanel::Face;
    app.hasReactionFace = false;
    app.currentMessage = "Pomodoro complete";
    app.lastAction = "Pomodoro complete";
    pomodoro.reset(now);
  }

  ReminderKind reminder = reminders.update(now);
  if (reminder == ReminderKind::Hydration && !timerActive(app.hydrationCompleteUntil, now)) {
    app.activeReminder = ReminderKind::Hydration;
    app.hydrationCompleteUntil = now + COMPLETION_SCREEN_MS;
    app.companionMode = CompanionMode::Idle;
    app.homePanel = HomePanel::Face;
    app.hasReactionFace = false;
    app.currentMessage = "Hydration reminder";
    app.lastAction = "Hydration reminder";
    reminders.markDone(ReminderKind::Hydration, now);
  } else if (reminder == ReminderKind::Stretch && app.activeReminder == ReminderKind::None) {
    app.activeReminder = ReminderKind::Stretch;
    triggerReaction(app, FaceId::BreakTime, "Stretch break", now, 8000);
  }

  if (runtimeTouchEnabled) {
    handleFaceGesture(faceTouch.update(now), now);
    handleActionGesture(actionTouch.update(now), now);
  }

  modeManager.update(now);
  decayStats(now);
  renderNow();
}
