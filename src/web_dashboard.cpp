#include "web_dashboard.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "faces.h"

static String htmlEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    switch (value[i]) {
      case '&': escaped += F("&amp;"); break;
      case '<': escaped += F("&lt;"); break;
      case '>': escaped += F("&gt;"); break;
      case '"': escaped += F("&quot;"); break;
      case '\'': escaped += F("&#39;"); break;
      default: escaped += value[i]; break;
    }
  }
  return escaped;
}

static String jsonEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '"' || c == '\\') {
      escaped += '\\';
      escaped += c;
    } else if (c == '\n') {
      escaped += F("\\n");
    } else if (c == '\r') {
      escaped += F("\\r");
    } else {
      escaped += c;
    }
  }
  return escaped;
}

static String cleanUserName(String value) {
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

static CompanionMode nextDashboardMode(CompanionMode mode) {
  switch (mode) {
    case CompanionMode::Idle: return CompanionMode::Pomodoro;
    case CompanionMode::Pomodoro: return CompanionMode::Clock;
    case CompanionMode::Clock: return CompanionMode::Reminders;
    case CompanionMode::Reminders: return CompanionMode::Status;
    case CompanionMode::Status: return CompanionMode::Settings;
    case CompanionMode::Settings:
    default: return CompanionMode::Idle;
  }
}

static void prepareDashboardMode(CompanionMode mode, PomodoroTimer &pomodoro, uint32_t now) {
  if (mode == CompanionMode::Pomodoro && !pomodoro.isRunning()) {
    pomodoro.reset(now);
  }
}

static void setDashboardMode(AppState &state, PomodoroTimer &pomodoro, CompanionMode mode, uint32_t now) {
  prepareDashboardMode(mode, pomodoro, now);
  state.companionMode = mode;
  state.hasReactionFace = false;
  state.lastAction = companionModeName(mode);
  state.lastInteractionAt = now;
}

void WebDashboard::addCorsHeaders() {
  server_->sendHeader(F("Access-Control-Allow-Origin"), F("https://amitkr000.github.io"));
  server_->sendHeader(F("Access-Control-Allow-Methods"), F("GET, POST, OPTIONS"));
  server_->sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
  server_->sendHeader(F("Access-Control-Allow-Private-Network"), F("true"));
  server_->sendHeader(F("Cache-Control"), F("no-store"));
}

void WebDashboard::sendJson(int code, const String &json) {
  addCorsHeaders();
  server_->send(code, F("application/json"), json);
}

void WebDashboard::sendJsonError(int code, const String &message) {
  String json;
  json.reserve(96);
  json += F("{\"ok\":false,\"error\":\"");
  json += jsonEscape(message);
  json += F("\"}");
  sendJson(code, json);
}

void WebDashboard::handleOptions() {
  addCorsHeaders();
  server_->send(204, F("text/plain"), "");
}

void WebDashboard::begin(WebServer &server, AppState &state, WeatherService &weather, PomodoroTimer &pomodoro, ReminderService &reminders) {
  server_ = &server;
  state_ = &state;
  weather_ = &weather;
  pomodoro_ = &pomodoro;
  reminders_ = &reminders;
}

void WebDashboard::setCallbacks(DashboardCallback saveState, DashboardCallback saveRuntime, DashboardCallback applyDisplay, WifiSaveCallback saveWifi, DashboardCallback forgetWifi, DashboardCallback forceWeatherSync) {
  saveState_ = saveState;
  saveRuntime_ = saveRuntime;
  applyDisplay_ = applyDisplay;
  saveWifi_ = saveWifi;
  forgetWifi_ = forgetWifi;
  forceWeatherSync_ = forceWeatherSync;
}

void WebDashboard::registerRoutes() {
  Serial.println("[web] registering routes");
  static const char *headerKeys[] = {"X-Tiny-Token"};
  server_->collectHeaders(headerKeys, 1);
  server_->on("/", HTTP_GET, [this]() { sendHomePage(); });
  server_->on("/test", HTTP_GET, [this]() { sendPlainOk(); });
  server_->on("/setup", HTTP_GET, [this]() { sendSetupPage(); });
  server_->on("/status", HTTP_GET, [this]() { sendStatusPage(); });
  server_->on("/generate_204", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/gen_204", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/hotspot-detect.html", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/library/test/success.html", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/connecttest.txt", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/ncsi.txt", HTTP_GET, [this]() { handleCaptivePortal(); });
  server_->on("/save", HTTP_POST, [this]() { handleWifiSave(); });
  server_->on("/action", HTTP_GET, [this]() { handleAction(); });
  server_->on("/action", HTTP_POST, [this]() { handleAction(); });
  server_->on("/face", HTTP_GET, [this]() { handleFacePreview(); });
  server_->on("/face", HTTP_POST, [this]() { handleFacePreview(); });
  server_->on("/settings", HTTP_POST, [this]() { handleSettings(); });
  server_->on("/api/state", HTTP_GET, [this]() { sendStateJson(); });
  server_->on("/api/state", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_->on("/api/discover", HTTP_GET, [this]() { sendDiscoverJson(); });
  server_->on("/api/discover", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_->on("/api/settings", HTTP_GET, [this]() {
    if (!requireToken()) return;
    sendSettingsJson();
  });
  server_->on("/api/settings", HTTP_POST, [this]() { handleApiSettings(); });
  server_->on("/api/settings", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_->on("/api/action", HTTP_POST, [this]() { handleApiAction(); });
  server_->on("/api/action", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_->on("/api/face", HTTP_POST, [this]() { handleApiFace(); });
  server_->on("/api/face", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_->on("/reset", HTTP_GET, [this]() { handleForgetWifi(); });
  server_->onNotFound([this]() {
    if (server_->method() == HTTP_OPTIONS) {
      handleOptions();
      return;
    }
    Serial.print("[web] not found fallback: ");
    Serial.println(server_->uri());
    sendHomePage();
  });
}

String WebDashboard::pageShell(const String &title, const String &body) {
  String html;
  html.reserve(18000);
  html += F("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">");
  html += F("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">");
  html += F("<title>");
  html += title;
  html += F("</title><style>");
  html += F(":root{font-family:Inter,system-ui,-apple-system,BlinkMacSystemFont,Segoe UI,sans-serif;color:#17202a;background:#eef2ef}");
  html += F("body{margin:0;min-height:100vh;padding:18px;box-sizing:border-box}");
  html += F("main{width:min(1040px,100%);margin:0 auto;background:#fff;border:1px solid #d7ded8;border-radius:8px;box-shadow:0 16px 42px #1b1b1b18;padding:22px;box-sizing:border-box}");
  html += F("header{display:flex;justify-content:space-between;gap:12px;align-items:flex-start;margin-bottom:18px}");
  html += F("h1{font-size:26px;line-height:1.1;margin:0 0 6px;color:#16251f;letter-spacing:0}");
  html += F("h2{font-size:16px;margin:0 0 12px;color:#26352d;letter-spacing:0}");
  html += F("p{line-height:1.45;color:#4c5a51;margin:0 0 16px}");
  html += F("label{display:block;font-size:13px;font-weight:700;color:#26352d;margin:14px 0 6px}");
  html += F("input,select{width:100%;box-sizing:border-box;border:1px solid #b9c0b8;border-radius:6px;font:inherit;padding:10px;background:#fff;color:#17202a}");
  html += F("input[type=range]{padding:0;accent-color:#1f6f55}.check{display:flex;align-items:center;gap:10px;margin:9px 0}.check input{width:auto}");
  html += F("button,a.button{display:inline-flex;align-items:center;justify-content:center;min-height:40px;border:0;border-radius:6px;background:#1f6f55;color:#fff;text-decoration:none;font-weight:800;font:inherit;padding:0 13px;cursor:pointer}");
  html += F(".secondary{background:#36454f}.danger{background:#9b2d20}.ghost{background:#edf2ee;color:#23382e}.row{display:flex;gap:10px;flex-wrap:wrap}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px}.card{border:1px solid #d9e0da;border-radius:8px;padding:14px;background:#fbfcfa}.status{background:#edf6f1;border:1px solid #c9e0d4;color:#244d3b;border-radius:6px;padding:12px;margin:0}.metric{margin:10px 0}.meter{height:9px;background:#e4e8e4;border-radius:999px;overflow:hidden}.meter span{display:block;height:100%;background:#1f6f55}.muted{color:#647067;font-size:13px}.pill{display:inline-flex;border-radius:999px;background:#edf2ee;padding:5px 10px;font-size:12px;font-weight:800;color:#26352d}.mood button{width:100%;height:100%;min-height:72px;display:block;text-align:left;background:#f8faf8;color:#17202a;border:1px solid #d7ded8}.mood strong{display:block;font-size:15px;margin-bottom:4px}.mood.active button{background:#e2f1e9;border-color:#1f6f55}.actions form,.mood{margin:0}");
  html += F("</style></head><body><main>");
  html += body;
  html += F("</main></body></html>");
  return html;
}

String WebDashboard::scanOptions() {
  String options;
  int networkCount = WiFi.scanNetworks(false, true);
  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;
    options += F("<option value=\"");
    options += htmlEscape(ssid);
    options += F("\">");
    options += htmlEscape(ssid);
    options += F(" (");
    options += WiFi.RSSI(i);
    options += F(" dBm)</option>");
  }
  WiFi.scanDelete();
  return options;
}

String WebDashboard::metricBlock(const String &label, uint8_t value) {
  String block;
  block += F("<div class=\"metric\"><div class=\"row\" style=\"justify-content:space-between\"><strong>");
  block += label;
  block += F("</strong><span>");
  block += value;
  block += F("%</span></div><div class=\"meter\"><span style=\"width:");
  block += value;
  block += F("%\"></span></div></div>");
  return block;
}

String WebDashboard::checkboxInput(const String &name, bool checked) {
  String input;
  input += F("<input type=\"checkbox\" name=\"");
  input += name;
  input += F("\" value=\"1\"");
  if (checked) input += F(" checked");
  input += F(">");
  return input;
}

bool WebDashboard::hasValidToken() const {
  if (!state_ || state_->dashboardToken.length() == 0) {
    return false;
  }
  if (server_->hasHeader("X-Tiny-Token") && server_->header("X-Tiny-Token") == state_->dashboardToken) {
    return true;
  }
  if (server_->hasArg("token") && server_->arg("token") == state_->dashboardToken) {
    return true;
  }
  return false;
}

bool WebDashboard::requireToken() {
  if (hasValidToken()) {
    return true;
  }
  addCorsHeaders();
  server_->send(401, F("text/plain"), F("Unauthorized"));
  return false;
}

String WebDashboard::tokenField() const {
  String field;
  field.reserve(72);
  field += F("<input type=\"hidden\" name=\"token\" value=\"");
  field += state_ ? htmlEscape(state_->dashboardToken) : String("");
  field += F("\">");
  return field;
}

String WebDashboard::tokenQuery() const {
  String query = F("?token=");
  query += state_ ? htmlEscape(state_->dashboardToken) : String("");
  return query;
}

void WebDashboard::redirectHome() {
  server_->sendHeader(F("Location"), F("/"));
  server_->send(303, F("text/plain"), F("See Other"));
}

void WebDashboard::sendSetupPage() {
  String body;
  body.reserve(4200);
  body += F("<h1>Tiny Companion Setup</h1>");
  body += F("<p>Enter your home Wi-Fi details. After saving, reconnect to your normal Wi-Fi. The connected dashboard address will be shown after Wi-Fi joins.</p>");
  body += F("<form method=\"post\" action=\"/save\">");
  body += F("<label for=\"ssid\">Wi-Fi name</label><input id=\"ssid\" name=\"ssid\" autocomplete=\"off\" required>");
  body += F("<label for=\"password\">Wi-Fi password</label><input id=\"password\" name=\"password\" type=\"password\" autocomplete=\"current-password\">");
  body += F("<button type=\"submit\">Save and connect</button></form>");
  body += F("<div class=\"row\"><a class=\"button secondary\" href=\"/test\">Test server</a><a class=\"button secondary\" href=\"/status\">Status</a></div>");
  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Setup"), body));
}

void WebDashboard::sendPlainOk() {
  addCorsHeaders();
  server_->send(200, F("text/plain"), F("Tiny Companion web server OK"));
}

void WebDashboard::handleCaptivePortal() {
  server_->sendHeader(F("Location"), F("http://192.168.4.1/"), true);
  server_->send(302, F("text/plain"), F("Tiny Companion setup"));
}

static bool setupApActive() {
  wifi_mode_t mode = WiFi.getMode();
  return mode == WIFI_AP || mode == WIFI_AP_STA;
}

void WebDashboard::sendHomePage() {
  if (!WiFi.isConnected() && (state_->deviceMode == DeviceMode::SetupPortal || setupApActive())) {
    sendSetupPage();
    return;
  }
  sendDashboardPage();
}

void WebDashboard::sendStatusPage() {
  String body;
  body.reserve(1800);
  body += F("<h1>Status</h1>");
  if (WiFi.isConnected()) {
    body += F("<p class=\"status\">Connected to ");
    body += htmlEscape(WiFi.SSID());
    body += F("<br>IP address: ");
    body += WiFi.localIP().toString();
    body += F("</p>");
  } else if (state_->deviceMode == DeviceMode::SetupPortal || setupApActive()) {
    body += F("<p class=\"status\">Optional setup network is running. Connect to ");
    body += SETUP_AP_SSID;
    body += F(" and use the Wi-Fi setup page when you want Wi-Fi features.</p>");
  } else {
    body += F("<p class=\"status\">Not connected yet.</p>");
  }
  body += F("<div class=\"row\"><a class=\"button\" href=\"/\">Dashboard</a><a class=\"button secondary\" href=\"/setup\">Configure Wi-Fi</a><a class=\"button secondary\" href=\"/reset\">Forget saved Wi-Fi</a></div>");
  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Status"), body));
}

void WebDashboard::sendDashboardPage() {
  String body;
  body.reserve(4200);
  body += F("<header><div><h1>Tiny Companion</h1><p>Use the hosted dashboard for the full control UI.</p></div><span class=\"pill\">");
  body += WiFi.isConnected() ? F("Online") : F("Offline");
  body += F("</span></header>");

  body += F("<section class=\"grid\">");
  body += F("<div class=\"card\"><h2>Connection</h2>");
  if (WiFi.isConnected()) {
    body += F("<p class=\"status\">");
    body += htmlEscape(WiFi.SSID());
    body += F("<br>");
    body += WiFi.localIP().toString();
    body += F("<br>RSSI ");
    body += WiFi.RSSI();
    body += F(" dBm</p>");
  } else {
    body += F("<p class=\"status\">Wi-Fi is optional. Connect to TinyBotSetup when you want dashboard, weather, and sync features.</p>");
  }
  body += F("<div class=\"row\" style=\"margin-top:12px\"><a class=\"button\" href=\"");
  body += HOSTED_DASHBOARD_URL;
  body += F("\">Open hosted dashboard</a><a class=\"button ghost\" href=\"/api/state\">State JSON</a></div></div>");

  body += F("<div class=\"card\"><h2>Companion</h2>");
  body += F("<p class=\"muted\">Hello, ");
  body += htmlEscape(state_->userName);
  body += F("<br>Access token: <strong>");
  body += htmlEscape(state_->dashboardToken);
  body += F("</strong>");
  body += F("</p>");
  body += metricBlock(F("Fullness"), state_->stats.fullness);
  body += metricBlock(F("Happiness"), state_->stats.happiness);
  body += metricBlock(F("Energy"), state_->stats.energy);
  body += F("<p class=\"muted\">Face: ");
  body += faceName(state_->currentFace);
  body += F("<br>Last action: ");
  body += htmlEscape(state_->lastAction);
  body += F("</p></div>");

  body += F("<div class=\"card\"><h2>Pomodoro</h2><p class=\"status\">");
  body += F("Pomodoro");
  body += pomodoro_->isRunning() ? F(" running") : F(" paused");
  body += F("<br>Remaining ");
  uint32_t remaining = pomodoro_->remainingSeconds(millis());
  body += String(remaining / 60);
  body += F(":");
  if (remaining % 60 < 10) body += F("0");
  body += String(remaining % 60);
  body += F("</p></div>");
  body += F("</section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Fallback Actions</h2><div class=\"row actions\">");
  const char *actions[][2] = {{"poke", "Poke"}, {"feed", "Feed"}, {"play", "Play"}, {"pet", "Pet"}, {"sleep", "Sleep"}, {"wake", "Wake"}, {"love", "Love"}, {"modeCycle", "Cycle mode"}, {"modeFace", "Face mode"}, {"modePomodoro", "Pomodoro mode"}, {"modeSettings", "Setting mode"}};
  for (const auto &action : actions) {
    body += F("<form method=\"post\" action=\"/action\"><input type=\"hidden\" name=\"action\" value=\"");
    body += action[0];
    body += F("\">");
    body += tokenField();
    body += F("<button type=\"submit\">");
    body += action[1];
    body += F("</button></form>");
  }
  const char *pomoActions[][2] = {{"pomoStart", "Start"}, {"pomoPause", "Pause"}, {"pomoReset", "Reset"}};
  for (const auto &action : pomoActions) {
    body += F("<form method=\"post\" action=\"/action\"><input type=\"hidden\" name=\"action\" value=\"");
    body += action[0];
    body += F("\">");
    body += tokenField();
    body += F("<button type=\"submit\">");
    body += action[1];
    body += F("</button></form>");
  }
  body += F("</div></section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Hardware</h2><p class=\"muted\">OLED ");
  body += OLED_WIDTH;
  body += F("x");
  body += OLED_HEIGHT;
  body += F(", SDA GPIO ");
  body += I2C_SDA_PIN;
  body += F(", SCL GPIO ");
  body += I2C_SCL_PIN;
  body += F(", face touch GPIO ");
  body += FACE_TOUCH_PIN;
  body += F(", action touch GPIO ");
  body += ACTION_TOUCH_PIN;
  body += F(".</p><div class=\"row\"><a class=\"button danger\" href=\"/reset");
  body += tokenQuery();
  body += F("\">Forget Wi-Fi</a><a class=\"button ghost\" href=\"/status\">Simple status</a></div></section>");

  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Dashboard"), body));
}

void WebDashboard::handleAction() {
  if (!requireToken()) return;
  String action = server_->hasArg("action") ? server_->arg("action") : server_->arg("name");
  action.trim();
  if (!applyAction(action)) {
    server_->send(400, F("text/plain"), F("Unknown action"));
    return;
  }
  redirectHome();
}

bool WebDashboard::applyAction(const String &action) {
  uint32_t now = millis();

  if (action == "poke") {
    state_->stats.happiness = clampStat(state_->stats.happiness + 2);
    triggerReaction(*state_, FaceId::Poke, "Poked", now);
  } else if (action == "feed") {
    state_->stats.fullness = clampStat(state_->stats.fullness + 24);
    state_->stats.happiness = clampStat(state_->stats.happiness + 6);
    state_->stats.energy = clampStat(state_->stats.energy + 3);
    triggerReaction(*state_, state_->stats.fullness > 92 ? FaceId::Full : FaceId::Feed, "Fed", now);
  } else if (action == "play") {
    state_->stats.fullness = clampStat(state_->stats.fullness - 8);
    state_->stats.happiness = clampStat(state_->stats.happiness + 20);
    state_->stats.energy = clampStat(state_->stats.energy - 12);
    triggerReaction(*state_, FaceId::Playful, "Played", now);
  } else if (action == "pet") {
    state_->stats.happiness = clampStat(state_->stats.happiness + 18);
    state_->stats.energy = clampStat(state_->stats.energy + 4);
    triggerReaction(*state_, FaceId::Love, "Petted", now, PETTING_FACE_MS);
  } else if (action == "sleep") {
    state_->sleepRequested = true;
    state_->stats.energy = clampStat(state_->stats.energy + 28);
    triggerReaction(*state_, FaceId::Sleepy, "Sleeping", now);
  } else if (action == "wake") {
    state_->sleepRequested = false;
    state_->stats.energy = clampStat(state_->stats.energy + 4);
    triggerReaction(*state_, FaceId::Wake, "Awake", now);
  } else if (action == "love") {
    state_->stats.happiness = clampStat(state_->stats.happiness + 15);
    triggerReaction(*state_, FaceId::Love, "Loved", now);
  } else if (action == "pomoStart") {
    pomodoro_->start(now);
    state_->companionMode = CompanionMode::Pomodoro;
    triggerReaction(*state_, FaceId::Pomodoro, "Pomodoro started", now);
  } else if (action == "pomoPause") {
    pomodoro_->pause(now);
    state_->companionMode = CompanionMode::Pomodoro;
    triggerReaction(*state_, FaceId::Pomodoro, "Pomodoro paused", now);
  } else if (action == "pomoToggle") {
    pomodoro_->startPause(now);
    state_->companionMode = CompanionMode::Pomodoro;
    triggerReaction(*state_, FaceId::Pomodoro, "Pomodoro toggle", now);
  } else if (action == "pomoReset") {
    pomodoro_->reset(now);
    state_->companionMode = CompanionMode::Pomodoro;
    triggerReaction(*state_, FaceId::Pomodoro, "Pomodoro reset", now);
  } else if (action == "doneHydration") {
    reminders_->markDone(ReminderKind::Hydration, now);
    state_->activeReminder = ReminderKind::None;
    triggerReaction(*state_, FaceId::Proud, "Hydrated", now);
  } else if (action == "doneStretch") {
    reminders_->markDone(ReminderKind::Stretch, now);
    state_->activeReminder = ReminderKind::None;
    triggerReaction(*state_, FaceId::Proud, "Stretched", now);
  } else if (action == "forceWeather") {
    if (forceWeatherSync_) forceWeatherSync_();
    triggerReaction(*state_, FaceId::Proud, "Weather synced", now);
  } else if (action == "modeCycle") {
    setDashboardMode(*state_, *pomodoro_, nextDashboardMode(state_->companionMode), now);
  } else if (action == "modeIdle" || action == "modeFace") {
    setDashboardMode(*state_, *pomodoro_, CompanionMode::Idle, now);
  } else if (action == "modePomodoro") {
    setDashboardMode(*state_, *pomodoro_, CompanionMode::Pomodoro, now);
  } else if (action == "modeSettings") {
    setDashboardMode(*state_, *pomodoro_, CompanionMode::Settings, now);
  } else {
    return false;
  }

  if (saveState_) saveState_();
  return true;
}

void WebDashboard::handleFacePreview() {
  if (!requireToken()) return;
  String id = server_->arg("face");
  if (!applyFacePreview(id)) {
    server_->send(400, F("text/plain"), F("Unknown face"));
    return;
  }
  redirectHome();
}

bool WebDashboard::applyFacePreview(const String &id) {
  FaceId face = faceFromId(id, FaceId::Neutral);
  if (id.length() == 0 || id != faceId(face)) {
    return false;
  }
  triggerReaction(*state_, face, String("Preview: ") + faceName(face), millis(), 7000);
  if (saveState_) saveState_();
  return true;
}

void WebDashboard::handleSettings() {
  if (!requireToken()) return;
  applySettingsFromRequest();
  redirectHome();
}

bool WebDashboard::applySettingsFromRequest() {
  if (server_->hasArg("userName")) state_->userName = cleanUserName(server_->arg("userName"));
  if (server_->hasArg("brightness")) state_->displaySettings.brightness = constrain(server_->arg("brightness").toInt(), 1, 100);
  state_->displaySettings.inverted = server_->hasArg("invert");
  state_->displaySettings.idleAnimationEnabled = server_->hasArg("idleAnim");
  if (server_->hasArg("focus")) state_->pomodoroSettings.focusMinutes = constrain(server_->arg("focus").toInt(), 1, 120);
  state_->reminderSettings.hydrationEnabled = server_->hasArg("hydrationOn");
  state_->reminderSettings.stretchEnabled = server_->hasArg("stretchOn");
  if (server_->hasArg("hydration")) state_->reminderSettings.hydrationMinutes = constrain(server_->arg("hydration").toInt(), 5, 240);
  if (server_->hasArg("stretch")) state_->reminderSettings.stretchMinutes = constrain(server_->arg("stretch").toInt(), 5, 240);
  state_->weather.enabled = server_->hasArg("weatherOn");
  if (server_->hasArg("lat")) state_->weather.latitude = server_->arg("lat").toFloat();
  if (server_->hasArg("lon")) state_->weather.longitude = server_->arg("lon").toFloat();
  if (server_->hasArg("tz")) state_->weather.timezone = server_->arg("tz");
  if (server_->hasArg("tzOff")) state_->weather.timezoneOffsetMinutes = constrain(server_->arg("tzOff").toInt(), -720, 840);
  state_->weather.manualWeather = server_->hasArg("manualWeather");
  state_->weather.manualSeason = server_->hasArg("manualSeason");
  if (server_->hasArg("weatherTheme")) state_->weather.overrideTheme = weatherThemeFromId(server_->arg("weatherTheme"));
  if (server_->hasArg("seasonTheme")) state_->weather.overrideSeason = seasonThemeFromId(server_->arg("seasonTheme"));
  if (server_->hasArg("tapWindow")) state_->touchSettings.tapWindowMs = constrain(server_->arg("tapWindow").toInt(), 180, 900);
  if (server_->hasArg("longPress")) state_->touchSettings.longPressMs = constrain(server_->arg("longPress").toInt(), 500, 2500);
  if (server_->hasArg("annoyedPokes")) state_->touchSettings.annoyedPokeCount = constrain(server_->arg("annoyedPokes").toInt(), 2, 12);
  if (server_->hasArg("angryPokes")) state_->touchSettings.angryPokeCount = constrain(server_->arg("angryPokes").toInt(), 3, 20);

  pomodoro_->applySettings(state_->pomodoroSettings, millis());
  reminders_->applySettings(state_->reminderSettings, millis());
  weather_->saveSettings();
  state_->lastAction = "Settings saved";
  if (saveRuntime_) saveRuntime_();
  if (saveState_) saveState_();
  if (applyDisplay_) applyDisplay_();
  return true;
}

bool WebDashboard::applySettingsFromJson() {
  String body = server_->arg("plain");
  if (body.length() == 0) {
    return false;
  }

  DynamicJsonDocument doc(2304);
  if (deserializeJson(doc, body)) {
    return false;
  }

  JsonObject root = doc.as<JsonObject>();
  JsonObject companion = root["companion"];
  if (!companion.isNull()) {
    if (!companion["userName"].isNull()) state_->userName = cleanUserName(companion["userName"].as<String>());
  }

  JsonObject display = root["display"];
  if (!display.isNull()) {
    if (!display["brightness"].isNull()) state_->displaySettings.brightness = constrain(display["brightness"].as<int>(), 1, 100);
    if (!display["inverted"].isNull()) state_->displaySettings.inverted = display["inverted"].as<bool>();
    if (!display["idleAnimationEnabled"].isNull()) state_->displaySettings.idleAnimationEnabled = display["idleAnimationEnabled"].as<bool>();
  }

  JsonObject pomo = root["pomodoro"];
  if (!pomo.isNull()) {
    if (!pomo["focusMinutes"].isNull()) state_->pomodoroSettings.focusMinutes = constrain(pomo["focusMinutes"].as<int>(), 1, 120);
  }

  JsonObject reminder = root["reminders"];
  if (!reminder.isNull()) {
    if (!reminder["hydrationEnabled"].isNull()) state_->reminderSettings.hydrationEnabled = reminder["hydrationEnabled"].as<bool>();
    if (!reminder["stretchEnabled"].isNull()) state_->reminderSettings.stretchEnabled = reminder["stretchEnabled"].as<bool>();
    if (!reminder["hydrationMinutes"].isNull()) state_->reminderSettings.hydrationMinutes = constrain(reminder["hydrationMinutes"].as<int>(), 5, 240);
    if (!reminder["stretchMinutes"].isNull()) state_->reminderSettings.stretchMinutes = constrain(reminder["stretchMinutes"].as<int>(), 5, 240);
  }

  JsonObject weather = root["weather"];
  if (!weather.isNull()) {
    if (!weather["enabled"].isNull()) state_->weather.enabled = weather["enabled"].as<bool>();
    if (!weather["latitude"].isNull()) state_->weather.latitude = weather["latitude"].as<float>();
    if (!weather["longitude"].isNull()) state_->weather.longitude = weather["longitude"].as<float>();
    if (!weather["timezone"].isNull()) state_->weather.timezone = weather["timezone"].as<const char *>();
    if (!weather["timezoneOffsetMinutes"].isNull()) state_->weather.timezoneOffsetMinutes = constrain(weather["timezoneOffsetMinutes"].as<int>(), -720, 840);
    if (!weather["manualWeather"].isNull()) state_->weather.manualWeather = weather["manualWeather"].as<bool>();
    if (!weather["manualSeason"].isNull()) state_->weather.manualSeason = weather["manualSeason"].as<bool>();
    if (!weather["overrideTheme"].isNull()) state_->weather.overrideTheme = weatherThemeFromId(String(weather["overrideTheme"].as<const char *>()), state_->weather.overrideTheme);
    if (!weather["overrideSeason"].isNull()) state_->weather.overrideSeason = seasonThemeFromId(String(weather["overrideSeason"].as<const char *>()), state_->weather.overrideSeason);
  }

  JsonObject touch = root["touch"];
  if (!touch.isNull()) {
    if (!touch["tapWindowMs"].isNull()) state_->touchSettings.tapWindowMs = constrain(touch["tapWindowMs"].as<int>(), 180, 900);
    if (!touch["longPressMs"].isNull()) state_->touchSettings.longPressMs = constrain(touch["longPressMs"].as<int>(), 500, 2500);
    if (!touch["annoyedPokeCount"].isNull()) state_->touchSettings.annoyedPokeCount = constrain(touch["annoyedPokeCount"].as<int>(), 2, 12);
    if (!touch["angryPokeCount"].isNull()) state_->touchSettings.angryPokeCount = constrain(touch["angryPokeCount"].as<int>(), 3, 20);
  }

  pomodoro_->applySettings(state_->pomodoroSettings, millis());
  reminders_->applySettings(state_->reminderSettings, millis());
  weather_->saveSettings();
  state_->lastAction = "Settings saved";
  if (saveRuntime_) saveRuntime_();
  if (saveState_) saveState_();
  if (applyDisplay_) applyDisplay_();
  return true;
}

void WebDashboard::handleApiAction() {
  if (!requireToken()) return;
  DynamicJsonDocument doc(256);
  String body = server_->arg("plain");
  String action;
  if (body.length() > 0 && !deserializeJson(doc, body)) {
    action = doc["action"] | "";
  }
  if (action.length() == 0 && server_->hasArg("action")) {
    action = server_->arg("action");
  }
  action.trim();
  if (!applyAction(action)) {
    sendJsonError(400, "Unknown action");
    return;
  }
  sendStateJson();
}

void WebDashboard::handleApiFace() {
  if (!requireToken()) return;
  DynamicJsonDocument doc(256);
  String body = server_->arg("plain");
  String face;
  if (body.length() > 0 && !deserializeJson(doc, body)) {
    face = doc["face"] | "";
  }
  if (face.length() == 0 && server_->hasArg("face")) {
    face = server_->arg("face");
  }
  face.trim();
  if (face.length() == 0 || !applyFacePreview(face)) {
    sendJsonError(400, "Unknown face");
    return;
  }
  sendStateJson();
}

void WebDashboard::handleApiSettings() {
  if (!requireToken()) return;
  if (!applySettingsFromJson()) {
    sendJsonError(400, "Invalid settings JSON");
    return;
  }
  sendSettingsJson();
}

void WebDashboard::handleWifiSave() {
  String ssid = server_->arg("ssid");
  String password = server_->arg("password");
  ssid.trim();
  if (ssid.length() == 0) {
    server_->send(400, F("text/html"), pageShell(F("Missing Wi-Fi Name"), F("<h1>Missing Wi-Fi Name</h1><p>Please enter a network name.</p><a class=\"button\" href=\"/\">Back</a>")));
    return;
  }

  String body;
  body += F("<h1>Details Saved</h1><p>The ESP32 is trying to join ");
  body += htmlEscape(ssid);
  body += F(". Watch the OLED for the new IP address.</p><a class=\"button\" href=\"/status\">Check status</a>");
  server_->send(200, F("text/html"), pageShell(F("Connecting"), body));
  if (saveWifi_) saveWifi_(ssid, password);
}

void WebDashboard::handleForgetWifi() {
  if (!requireToken()) return;
  server_->send(200, F("text/html"), pageShell(F("Wi-Fi Removed"), F("<h1>Saved Wi-Fi Removed</h1><p>The setup network will be available again after restart.</p>")));
  if (forgetWifi_) forgetWifi_();
}

void WebDashboard::sendDiscoverJson() {
  String json;
  json.reserve(900);
  json += F("{\"ok\":true,\"device\":\"");
  json += jsonEscape(DEVICE_NAME);
  json += F("\",\"apiVersion\":1,\"hostname\":\"");
  json += MDNS_HOSTNAME;
  json += F(".local\",\"hostedDashboard\":\"");
  json += HOSTED_DASHBOARD_URL;
  json += F("\",\"ip\":\"");
  json += WiFi.isConnected() ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  json += F("\",\"mode\":\"");
  json += deviceModeName(state_->deviceMode);
  json += F("\",\"setupSsid\":\"");
  json += SETUP_AP_SSID;
  json += F("\",\"authRequired\":true,\"endpoints\":[\"/api/state\",\"/api/action\",\"/api/face\",\"/api/settings\",\"/api/discover\"]}");
  sendJson(200, json);
}

void WebDashboard::sendSettingsJson() {
  String json;
  json.reserve(2000);
  json += F("{\"ok\":true,\"companion\":{\"userName\":\"");
  json += jsonEscape(state_->userName);
  json += F("\"},\"display\":{\"brightness\":");
  json += state_->displaySettings.brightness;
  json += F(",\"inverted\":");
  json += state_->displaySettings.inverted ? F("true") : F("false");
  json += F(",\"idleAnimationEnabled\":");
  json += state_->displaySettings.idleAnimationEnabled ? F("true") : F("false");
  json += F("},\"pomodoro\":{\"focusMinutes\":");
  json += state_->pomodoroSettings.focusMinutes;
  json += F("},\"reminders\":{\"hydrationEnabled\":");
  json += state_->reminderSettings.hydrationEnabled ? F("true") : F("false");
  json += F(",\"stretchEnabled\":");
  json += state_->reminderSettings.stretchEnabled ? F("true") : F("false");
  json += F(",\"hydrationMinutes\":");
  json += state_->reminderSettings.hydrationMinutes;
  json += F(",\"stretchMinutes\":");
  json += state_->reminderSettings.stretchMinutes;
  json += F("},\"weather\":{\"enabled\":");
  json += state_->weather.enabled ? F("true") : F("false");
  json += F(",\"latitude\":");
  json += String(state_->weather.latitude, 4);
  json += F(",\"longitude\":");
  json += String(state_->weather.longitude, 4);
  json += F(",\"timezone\":\"");
  json += jsonEscape(state_->weather.timezone);
  json += F("\",\"timezoneOffsetMinutes\":");
  json += state_->weather.timezoneOffsetMinutes;
  json += F(",\"manualWeather\":");
  json += state_->weather.manualWeather ? F("true") : F("false");
  json += F(",\"manualSeason\":");
  json += state_->weather.manualSeason ? F("true") : F("false");
  json += F(",\"overrideTheme\":\"");
  json += weatherThemeId(state_->weather.overrideTheme);
  json += F("\",\"overrideSeason\":\"");
  json += seasonThemeId(state_->weather.overrideSeason);
  json += F("\"},\"touch\":{\"tapWindowMs\":");
  json += state_->touchSettings.tapWindowMs;
  json += F(",\"longPressMs\":");
  json += state_->touchSettings.longPressMs;
  json += F(",\"annoyedPokeCount\":");
  json += state_->touchSettings.annoyedPokeCount;
  json += F(",\"angryPokeCount\":");
  json += state_->touchSettings.angryPokeCount;
  json += F("}}");
  sendJson(200, json);
}

void WebDashboard::sendStateJson() {
  uint32_t remaining = pomodoro_->remainingSeconds(millis());
  String json;
  json.reserve(1800);
  json += F("{\"device\":\"");
  json += jsonEscape(DEVICE_NAME);
  json += F("\",\"mode\":\"");
  json += deviceModeName(state_->deviceMode);
  json += F("\",\"companionMode\":\"");
  json += companionModeName(state_->companionMode);
  json += F("\",\"ip\":\"");
  json += WiFi.isConnected() ? WiFi.localIP().toString() : String("");
  json += F("\",\"ssid\":\"");
  json += WiFi.isConnected() ? jsonEscape(WiFi.SSID()) : String("");
  json += F("\",\"face\":\"");
  json += faceId(state_->currentFace);
  json += F("\",\"fullness\":");
  json += state_->stats.fullness;
  json += F(",\"happiness\":");
  json += state_->stats.happiness;
  json += F(",\"energy\":");
  json += state_->stats.energy;
  json += F(",\"pomodoro\":{\"phase\":\"");
  json += F("Pomodoro");
  json += F("\",\"running\":");
  json += pomodoro_->isRunning() ? F("true") : F("false");
  json += F(",\"remainingSeconds\":");
  json += remaining;
  json += F("},\"weather\":{\"theme\":\"");
  json += weatherThemeId(state_->weather.manualWeather ? state_->weather.overrideTheme : state_->weather.theme);
  json += F("\",\"temperatureC\":");
  json += String(state_->weather.temperatureC, 1);
  json += F(",\"weatherCode\":");
  json += state_->weather.weatherCode;
  json += F(",\"hasData\":");
  json += state_->weather.hasData ? F("true") : F("false");
  json += F(",\"lastSyncAgeSeconds\":");
  if (state_->weather.lastWeatherSyncAt == 0) {
    json += F("-1");
  } else {
    json += String((millis() - state_->weather.lastWeatherSyncAt) / 1000UL);
  }
  json += F(",\"isDay\":");
  json += state_->weather.isDay ? F("true") : F("false");
  json += F(",\"season\":\"");
  json += seasonThemeId(weather_->resolvedSeason(millis()));
  json += F("\",\"moon\":\"");
  json += moonPhaseName(state_->weather.moon);
  json += F("\"}}");
  sendJson(200, json);
}
