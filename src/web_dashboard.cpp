#include "web_dashboard.h"

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
  server_->on("/reset", HTTP_GET, [this]() { handleForgetWifi(); });
  server_->onNotFound([this]() {
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

void WebDashboard::redirectHome() {
  server_->sendHeader(F("Location"), F("/"));
  server_->send(303, F("text/plain"), F("See Other"));
}

void WebDashboard::sendSetupPage() {
  String body;
  body.reserve(4200);
  body += F("<h1>Tiny Companion Setup</h1>");
  body += F("<p>Enter your home Wi-Fi details. After saving, reconnect to your normal Wi-Fi and open the IP shown on the OLED.</p>");
  body += F("<form method=\"post\" action=\"/save\">");
  body += F("<label for=\"ssid\">Wi-Fi name</label><input id=\"ssid\" name=\"ssid\" autocomplete=\"off\" required>");
  body += F("<label for=\"password\">Wi-Fi password</label><input id=\"password\" name=\"password\" type=\"password\" autocomplete=\"current-password\">");
  body += F("<button type=\"submit\">Save and connect</button></form>");
  body += F("<div class=\"row\"><a class=\"button secondary\" href=\"/test\">Test server</a><a class=\"button secondary\" href=\"/status\">Status</a></div>");
  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Setup"), body));
}

void WebDashboard::sendPlainOk() {
  server_->send(200, F("text/plain"), F("Tiny Companion web server OK"));
}

void WebDashboard::handleCaptivePortal() {
  server_->sendHeader(F("Location"), F("http://192.168.4.1/"), true);
  server_->send(302, F("text/plain"), F("Tiny Companion setup"));
}

void WebDashboard::sendHomePage() {
  if (state_->deviceMode == DeviceMode::SetupPortal && !WiFi.isConnected()) {
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
  } else if (state_->deviceMode == DeviceMode::SetupPortal) {
    body += F("<p class=\"status\">Setup portal is running. Connect to ");
    body += SETUP_AP_SSID;
    body += F(" and open 192.168.4.1.</p>");
  } else {
    body += F("<p class=\"status\">Not connected yet.</p>");
  }
  body += F("<div class=\"row\"><a class=\"button\" href=\"/\">Dashboard</a><a class=\"button secondary\" href=\"/setup\">Configure Wi-Fi</a><a class=\"button secondary\" href=\"/reset\">Forget saved Wi-Fi</a></div>");
  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Status"), body));
}

void WebDashboard::sendDashboardPage() {
  String body;
  body.reserve(19000);
  body += F("<header><div><h1>Tiny Companion</h1><p>Face-first desk companion dashboard.</p></div><span class=\"pill\">");
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
    body += F("<p class=\"status\">Not connected. Use setup mode to add Wi-Fi.</p>");
  }
  body += F("<div class=\"row\" style=\"margin-top:12px\"><a class=\"button ghost\" href=\"/setup\">Wi-Fi setup</a><a class=\"button ghost\" href=\"/api/state\">State JSON</a></div></div>");

  body += F("<div class=\"card\"><h2>Companion</h2>");
  body += metricBlock(F("Fullness"), state_->stats.fullness);
  body += metricBlock(F("Happiness"), state_->stats.happiness);
  body += metricBlock(F("Energy"), state_->stats.energy);
  body += F("<p class=\"muted\">Face: ");
  body += faceName(state_->currentFace);
  body += F("<br>Last action: ");
  body += htmlEscape(state_->lastAction);
  body += F("</p></div>");

  body += F("<div class=\"card\"><h2>Weather Context</h2><p class=\"status\">");
  body += weatherThemeName(state_->weather.manualWeather ? state_->weather.overrideTheme : state_->weather.theme);
  body += F("<br>");
  body += String(state_->weather.temperatureC, 1);
  body += F(" C, code ");
  body += state_->weather.weatherCode;
  body += F("<br>");
  body += seasonThemeName(weather_->resolvedSeason(millis()));
  body += F(", ");
  body += moonPhaseName(state_->weather.moon);
  body += F("</p><form method=\"post\" action=\"/action\"><input type=\"hidden\" name=\"action\" value=\"forceWeather\"><button type=\"submit\">Sync weather</button></form></div>");
  body += F("</section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Actions</h2><div class=\"row actions\">");
  const char *actions[][2] = {{"poke", "Poke"}, {"feed", "Feed"}, {"play", "Play"}, {"sleep", "Sleep"}, {"wake", "Wake"}, {"love", "Love"}, {"doneHydration", "Hydrated"}};
  for (const auto &action : actions) {
    body += F("<form method=\"post\" action=\"/action\"><input type=\"hidden\" name=\"action\" value=\"");
    body += action[0];
    body += F("\"><button type=\"submit\">");
    body += action[1];
    body += F("</button></form>");
  }
  body += F("</div></section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Pomodoro</h2><p class=\"status\">");
  body += pomodoro_->phaseName();
  body += pomodoro_->isRunning() ? F(" running") : F(" paused");
  body += F("<br>Remaining ");
  uint32_t remaining = pomodoro_->remainingSeconds(millis());
  body += String(remaining / 60);
  body += F(":");
  if (remaining % 60 < 10) body += F("0");
  body += String(remaining % 60);
  body += F("</p><div class=\"row actions\">");
  const char *pomoActions[][2] = {{"pomoStart", "Start/Pause"}, {"pomoReset", "Reset"}, {"pomoSwitch", "Switch phase"}};
  for (const auto &action : pomoActions) {
    body += F("<form method=\"post\" action=\"/action\"><input type=\"hidden\" name=\"action\" value=\"");
    body += action[0];
    body += F("\"><button type=\"submit\">");
    body += action[1];
    body += F("</button></form>");
  }
  body += F("</div></section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Face Preview</h2><div class=\"grid\">");
  size_t faceCount;
  const FaceSpec *specs = faceSpecs(faceCount);
  for (size_t i = 0; i < faceCount; i++) {
    body += F("<form class=\"mood");
    if (specs[i].id == state_->currentFace) body += F(" active");
    body += F("\" method=\"post\" action=\"/face\"><input type=\"hidden\" name=\"face\" value=\"");
    body += specs[i].name;
    body += F("\"><button type=\"submit\"><strong>");
    body += specs[i].name;
    body += F("</strong><span class=\"muted\">");
    body += specs[i].description;
    body += F("</span></button></form>");
  }
  body += F("</div></section>");

  body += F("<section class=\"card\" style=\"margin-top:12px\"><h2>Settings</h2><form method=\"post\" action=\"/settings\"><div class=\"grid\"><div>");
  body += F("<label>OLED brightness ");
  body += state_->displaySettings.brightness;
  body += F("%</label><input type=\"range\" min=\"1\" max=\"100\" name=\"brightness\" value=\"");
  body += state_->displaySettings.brightness;
  body += F("\"><label class=\"check\">");
  body += checkboxInput(F("idleAnim"), state_->displaySettings.idleAnimationEnabled);
  body += F("Show faces on OLED</label><label class=\"check\">");
  body += checkboxInput(F("invert"), state_->displaySettings.inverted);
  body += F("Invert OLED colors</label></div><div>");
  body += F("<label>Focus minutes</label><input name=\"focus\" type=\"number\" min=\"1\" max=\"120\" value=\"");
  body += state_->pomodoroSettings.focusMinutes;
  body += F("\"><label>Short break minutes</label><input name=\"shortBreak\" type=\"number\" min=\"1\" max=\"60\" value=\"");
  body += state_->pomodoroSettings.shortBreakMinutes;
  body += F("\"><label>Long break minutes</label><input name=\"longBreak\" type=\"number\" min=\"1\" max=\"90\" value=\"");
  body += state_->pomodoroSettings.longBreakMinutes;
  body += F("\"><label>Rounds before long break</label><input name=\"rounds\" type=\"number\" min=\"1\" max=\"12\" value=\"");
  body += state_->pomodoroSettings.roundsBeforeLongBreak;
  body += F("\"></div><div>");
  body += F("<label class=\"check\">");
  body += checkboxInput(F("hydrationOn"), state_->reminderSettings.hydrationEnabled);
  body += F("Hydration reminders</label><label>Hydration minutes</label><input name=\"hydration\" type=\"number\" min=\"5\" max=\"240\" value=\"");
  body += state_->reminderSettings.hydrationMinutes;
  body += F("\"><label class=\"check\">");
  body += checkboxInput(F("stretchOn"), state_->reminderSettings.stretchEnabled);
  body += F("Stretch reminders</label><label>Stretch minutes</label><input name=\"stretch\" type=\"number\" min=\"5\" max=\"240\" value=\"");
  body += state_->reminderSettings.stretchMinutes;
  body += F("\"></div><div>");
  body += F("<label class=\"check\">");
  body += checkboxInput(F("weatherOn"), state_->weather.enabled);
  body += F("Weather sync</label><label>Latitude</label><input name=\"lat\" value=\"");
  body += String(state_->weather.latitude, 4);
  body += F("\"><label>Longitude</label><input name=\"lon\" value=\"");
  body += String(state_->weather.longitude, 4);
  body += F("\"><label>Timezone</label><input name=\"tz\" value=\"");
  body += htmlEscape(state_->weather.timezone);
  body += F("\"><label>UTC offset minutes</label><input name=\"tzOff\" type=\"number\" min=\"-720\" max=\"840\" value=\"");
  body += state_->weather.timezoneOffsetMinutes;
  body += F("\"></div><div>");
  body += F("<label class=\"check\">");
  body += checkboxInput(F("manualWeather"), state_->weather.manualWeather);
  body += F("Manual weather override</label><label>Weather face</label><select name=\"weatherTheme\">");
  const WeatherTheme themes[] = {WeatherTheme::Unknown, WeatherTheme::Sunny, WeatherTheme::Rainy, WeatherTheme::Cloudy, WeatherTheme::Stormy, WeatherTheme::Foggy, WeatherTheme::Windy, WeatherTheme::Hot, WeatherTheme::Cold};
  for (WeatherTheme theme : themes) {
    body += F("<option value=\"");
    body += weatherThemeId(theme);
    body += F("\"");
    if (theme == state_->weather.overrideTheme) body += F(" selected");
    body += F(">");
    body += weatherThemeName(theme);
    body += F("</option>");
  }
  body += F("</select><label class=\"check\">");
  body += checkboxInput(F("manualSeason"), state_->weather.manualSeason);
  body += F("Manual season override</label><label>Season face</label><select name=\"seasonTheme\">");
  const SeasonTheme seasons[] = {SeasonTheme::Auto, SeasonTheme::Spring, SeasonTheme::Summer, SeasonTheme::Monsoon, SeasonTheme::Autumn, SeasonTheme::Winter};
  for (SeasonTheme season : seasons) {
    body += F("<option value=\"");
    body += seasonThemeId(season);
    body += F("\"");
    if (season == state_->weather.overrideSeason) body += F(" selected");
    body += F(">");
    body += seasonThemeName(season);
    body += F("</option>");
  }
  body += F("</select></div><div>");
  body += F("<label>Tap window ms</label><input name=\"tapWindow\" type=\"number\" min=\"180\" max=\"900\" value=\"");
  body += state_->touchSettings.tapWindowMs;
  body += F("\"><label>Long press ms</label><input name=\"longPress\" type=\"number\" min=\"500\" max=\"2500\" value=\"");
  body += state_->touchSettings.longPressMs;
  body += F("\"><label>Annoyed poke count</label><input name=\"annoyedPokes\" type=\"number\" min=\"2\" max=\"12\" value=\"");
  body += state_->touchSettings.annoyedPokeCount;
  body += F("\"><label>Angry poke count</label><input name=\"angryPokes\" type=\"number\" min=\"3\" max=\"20\" value=\"");
  body += state_->touchSettings.angryPokeCount;
  body += F("\"></div></div><button type=\"submit\" style=\"margin-top:14px\">Save settings</button></form></section>");

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
  body += F(".</p><div class=\"row\"><a class=\"button danger\" href=\"/reset\">Forget Wi-Fi</a><a class=\"button ghost\" href=\"/status\">Simple status</a></div></section>");

  server_->send(200, F("text/html"), pageShell(F("Tiny Companion Dashboard"), body));
}

void WebDashboard::handleAction() {
  String action = server_->hasArg("action") ? server_->arg("action") : server_->arg("name");
  action.trim();
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
    state_->companionMode = CompanionMode::Pomodoro;
    pomodoro_->startPause(now);
    triggerReaction(*state_, pomodoro_->phase() == PomodoroPhase::Focus ? FaceId::Focused : FaceId::BreakTime, "Pomodoro toggle", now);
  } else if (action == "pomoReset") {
    state_->companionMode = CompanionMode::Pomodoro;
    pomodoro_->reset(now);
    triggerReaction(*state_, FaceId::Focused, "Pomodoro reset", now);
  } else if (action == "pomoSwitch") {
    state_->companionMode = CompanionMode::Pomodoro;
    pomodoro_->switchPhase(now);
    triggerReaction(*state_, pomodoro_->phase() == PomodoroPhase::Focus ? FaceId::Focused : FaceId::BreakTime, "Pomodoro phase", now);
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
  } else {
    server_->send(400, F("text/plain"), F("Unknown action"));
    return;
  }

  if (saveState_) saveState_();
  redirectHome();
}

void WebDashboard::handleFacePreview() {
  String id = server_->arg("face");
  FaceId face = faceFromId(id, state_->currentFace);
  triggerReaction(*state_, face, String("Preview: ") + faceName(face), millis(), 7000);
  if (saveState_) saveState_();
  redirectHome();
}

void WebDashboard::handleSettings() {
  if (server_->hasArg("brightness")) state_->displaySettings.brightness = constrain(server_->arg("brightness").toInt(), 1, 100);
  state_->displaySettings.inverted = server_->hasArg("invert");
  state_->displaySettings.idleAnimationEnabled = server_->hasArg("idleAnim");
  if (server_->hasArg("focus")) state_->pomodoroSettings.focusMinutes = constrain(server_->arg("focus").toInt(), 1, 120);
  if (server_->hasArg("shortBreak")) state_->pomodoroSettings.shortBreakMinutes = constrain(server_->arg("shortBreak").toInt(), 1, 60);
  if (server_->hasArg("longBreak")) state_->pomodoroSettings.longBreakMinutes = constrain(server_->arg("longBreak").toInt(), 1, 90);
  if (server_->hasArg("rounds")) state_->pomodoroSettings.roundsBeforeLongBreak = constrain(server_->arg("rounds").toInt(), 1, 12);
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
  redirectHome();
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
  server_->send(200, F("text/html"), pageShell(F("Wi-Fi Removed"), F("<h1>Saved Wi-Fi Removed</h1><p>The setup network will be available again after restart.</p>")));
  if (forgetWifi_) forgetWifi_();
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
  json += pomodoro_->phaseName();
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
  json += F(",\"isDay\":");
  json += state_->weather.isDay ? F("true") : F("false");
  json += F(",\"season\":\"");
  json += seasonThemeId(weather_->resolvedSeason(millis()));
  json += F("\",\"moon\":\"");
  json += moonPhaseName(state_->weather.moon);
  json += F("\"}}");
  server_->send(200, F("application/json"), json);
}
