const STORAGE_KEY = "tinyCompanionDevice";
const TOKEN_STORAGE_KEY = "tinyCompanionToken";
const COMMON_BASES = ["192.168.0.", "192.168.1.", "192.168.29.", "192.168.31.", "10.0.0."];
const FACES = [
  "neutral", "cheerful", "greeting", "happy", "playful", "hungry", "sleepy", "excited", "sad", "love",
  "poke", "feed", "full", "wake", "proud", "pomodoro", "break", "hydration",
  "sunny", "rainy", "cloudy", "stormy", "foggy", "windy", "hot", "cold", "time-weather-info",
  "morning", "afternoon", "evening", "night", "new-moon", "crescent-moon",
  "half-moon", "full-moon", "spring", "summer", "monsoon", "autumn", "winter",
  "annoyed", "angry", "dizzy", "ignored", "bored", "lonely", "low-battery", "error"
];
const WEATHER_THEMES = ["auto", "sunny", "rainy", "cloudy", "stormy", "foggy", "windy", "hot", "cold"];
const SEASONS = ["auto", "spring", "summer", "monsoon", "autumn", "winter"];

let deviceBase = "";
let pollTimer = 0;
let latestState = null;
let latestSettings = null;

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => Array.from(document.querySelectorAll(selector));

function normalizeAddress(value) {
  let address = value.trim();
  if (!address) return "";
  address = address.replace(/^https?:\/\//, "").replace(/\/+$/, "");
  return `http://${address}`;
}

async function fetchWithTimeout(url, options = {}, timeoutMs = 1400) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);
  try {
    return await fetch(url, {
      cache: "no-store",
      mode: "cors",
      targetAddressSpace: "local",
      ...options,
      signal: controller.signal,
    });
  } finally {
    clearTimeout(timer);
  }
}

async function api(path, options = {}) {
  if (!deviceBase) throw new Error("No device connected");
  const { authRequired = true, timeoutMs, ...fetchOptions } = options;
  const token = $("#accessToken")?.value.trim() || localStorage.getItem(TOKEN_STORAGE_KEY) || "";
  const apiPath = authRequired && token ? appendToken(path, token) : path;
  const headers = { ...(fetchOptions.headers || {}) };
  const response = await fetchWithTimeout(`${deviceBase}${apiPath}`, {
    ...fetchOptions,
    headers,
  }, timeoutMs || 2500);
  if (response.status === 401) throw new Error("Access token required or incorrect");
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

function appendToken(path, token) {
  const separator = path.includes("?") ? "&" : "?";
  return `${path}${separator}token=${encodeURIComponent(token)}`;
}

function setStatus(online, message) {
  $("#statusChip").textContent = online ? "Online" : "Offline";
  $("#statusChip").classList.toggle("online", online);
  $("#connectionCopy").textContent = message;
}

function updateFaceArt(face) {
  const art = $("#faceArt");
  art.className = `face-art face-${face || "neutral"}`;
}

function updateMetrics(state) {
  for (const key of ["fullness", "happiness", "energy"]) {
    const value = Number(state[key] || 0);
    $(`#${key}`).value = value;
    $(`#${key}Text`).textContent = `${value}%`;
  }
}

function formatTimer(seconds) {
  const safe = Math.max(0, Number(seconds || 0));
  const minutes = Math.floor(safe / 60);
  const secs = safe % 60;
  return `${String(minutes).padStart(2, "0")}:${String(secs).padStart(2, "0")}`;
}

function updateState(state) {
  latestState = state;
  setStatus(true, `${state.device || "Tiny Companion"} at ${deviceBase.replace("http://", "")}`);
  updateFaceArt(state.face);
  updateMetrics(state);
  $("#pomoPhase").textContent = `Pomodoro ${state.pomodoro?.running ? "running" : "paused"}`;
  $("#pomoTimer").textContent = formatTimer(state.pomodoro?.remainingSeconds);
  $("#pomoStartBtn").disabled = Boolean(state.pomodoro?.running);
  $("#pomoPauseBtn").disabled = !state.pomodoro?.running;
  $("#rawState").textContent = JSON.stringify(state, null, 2);
}

async function loadState() {
  const state = await api("/api/state", { authRequired: false });
  updateState(state);
  return state;
}

async function loadSettings() {
  latestSettings = await api("/api/settings");
  fillSettings(latestSettings);
  return latestSettings;
}

async function connect(address) {
  const base = normalizeAddress(address);
  if (!base) throw new Error("Enter a device IP address");
  $("#connectHint").textContent = "Connecting...";
  deviceBase = base;
  const discovery = await api("/api/discover", { authRequired: false, timeoutMs: 2200 });
  localStorage.setItem(STORAGE_KEY, deviceBase.replace("http://", ""));
  const token = $("#accessToken").value.trim();
  if (token) localStorage.setItem(TOKEN_STORAGE_KEY, token);
  $("#deviceAddress").value = deviceBase.replace("http://", "");
  $("#connectHint").textContent = `Connected to ${discovery.device || "Tiny Companion"}.`;
  await loadState();
  try {
    await loadSettings();
  } catch (err) {
    $("#connectHint").textContent = `${err.message}. Copy the token from http://${deviceBase.replace("http://", "")}/ and reconnect.`;
  }
  startPolling();
}

function startPolling() {
  clearInterval(pollTimer);
  pollTimer = setInterval(() => {
    loadState().catch((err) => {
      setStatus(false, `Connection lost: ${err.message}`);
    });
  }, 3000);
}

async function sendAction(action) {
  const state = await api("/api/action", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ action }),
  });
  updateState(state);
}

async function previewFace(face) {
  const state = await api("/api/face", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ face }),
  });
  updateState(state);
}

async function saveSettings(patch) {
  latestSettings = await api("/api/settings", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(patch),
  });
  fillSettings(latestSettings);
  await loadState();
}

function fillSettings(settings) {
  if (!settings?.ok) return;

  const pomo = $("#pomodoroForm");
  pomo.focusMinutes.value = settings.pomodoro.focusMinutes;

  const reminder = $("#reminderForm");
  reminder.hydrationEnabled.checked = settings.reminders.hydrationEnabled;
  reminder.hydrationMinutes.value = settings.reminders.hydrationMinutes;
  reminder.stretchEnabled.checked = settings.reminders.stretchEnabled;
  reminder.stretchMinutes.value = settings.reminders.stretchMinutes;

  const weather = $("#weatherForm");
  weather.enabled.checked = settings.weather.enabled;
  weather.latitude.value = settings.weather.latitude;
  weather.longitude.value = settings.weather.longitude;
  weather.timezone.value = settings.weather.timezone;
  weather.timezoneOffsetMinutes.value = settings.weather.timezoneOffsetMinutes;
  weather.manualWeather.checked = settings.weather.manualWeather;
  weather.overrideTheme.value = settings.weather.overrideTheme;
  weather.manualSeason.checked = settings.weather.manualSeason;
  weather.overrideSeason.value = settings.weather.overrideSeason;

  const device = $("#deviceForm");
  device.userName.value = settings.companion?.userName || "Friend";
  device.brightness.value = settings.display.brightness;
  device.inverted.checked = settings.display.inverted;
  device.idleAnimationEnabled.checked = settings.display.idleAnimationEnabled;
  device.tapWindowMs.value = settings.touch.tapWindowMs;
  device.longPressMs.value = settings.touch.longPressMs;
  device.annoyedPokeCount.value = settings.touch.annoyedPokeCount;
  device.angryPokeCount.value = settings.touch.angryPokeCount;
}

async function scanLocalNetwork() {
  $("#connectHint").textContent = "Scanning common addresses. This can take a little while.";
  const candidates = [];
  const saved = localStorage.getItem(STORAGE_KEY);
  if (saved) candidates.push(saved);
  candidates.push("tinycompanion.local");
  for (const base of COMMON_BASES) {
    for (let i = 2; i < 255; i++) candidates.push(`${base}${i}`);
  }

  let index = 0;
  const workers = Array.from({ length: 16 }, async () => {
    while (index < candidates.length) {
      const candidate = candidates[index++];
      try {
        const base = normalizeAddress(candidate);
        const res = await fetchWithTimeout(`${base}/api/discover`, {}, 650);
        if (!res.ok) continue;
        const json = await res.json();
        if (json.device === "Tiny Companion") {
          await connect(candidate);
          return true;
        }
      } catch (_) {
        // Discovery is best-effort; blocked probes are expected in browsers.
      }
    }
    return false;
  });

  const results = await Promise.all(workers);
  if (!results.some(Boolean)) {
    $("#connectHint").textContent = "No device found. Enter the IP shown on the OLED and connect manually.";
  }
}

function installStaticOptions() {
  $("#weatherThemeSelect").innerHTML = WEATHER_THEMES.map((item) => `<option value="${item}">${item}</option>`).join("");
  $("#seasonSelect").innerHTML = SEASONS.map((item) => `<option value="${item}">${item}</option>`).join("");
  $("#faceGrid").innerHTML = FACES.map((face) => `<button data-face="${face}">${face}</button>`).join("");
}

function installEvents() {
  $$(".tab").forEach((button) => {
    button.addEventListener("click", () => {
      $$(".tab").forEach((item) => item.classList.remove("active"));
      $$(".tab-panel").forEach((item) => item.classList.remove("active"));
      button.classList.add("active");
      $(`#${button.dataset.tab}`).classList.add("active");
    });
  });

  $("#connectBtn").addEventListener("click", () => {
    connect($("#deviceAddress").value).catch((err) => {
      setStatus(false, "Could not connect");
      $("#connectHint").textContent = `${err.message}. Try opening http://${$("#deviceAddress").value}/api/state directly once.`;
    });
  });
  $("#scanBtn").addEventListener("click", () => scanLocalNetwork());

  document.body.addEventListener("click", (event) => {
    const action = event.target.closest("[data-action]")?.dataset.action;
    if (action) sendAction(action).catch((err) => $("#connectHint").textContent = err.message);
    const face = event.target.closest("[data-face]")?.dataset.face;
    if (face) previewFace(face).catch((err) => $("#connectHint").textContent = err.message);
  });

  $("#pomodoroForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const form = event.currentTarget;
    saveSettings({ pomodoro: {
      focusMinutes: Number(form.focusMinutes.value),
    }}).catch((err) => $("#connectHint").textContent = err.message);
  });

  $("#reminderForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const form = event.currentTarget;
    saveSettings({ reminders: {
      hydrationEnabled: form.hydrationEnabled.checked,
      hydrationMinutes: Number(form.hydrationMinutes.value),
      stretchEnabled: form.stretchEnabled.checked,
      stretchMinutes: Number(form.stretchMinutes.value),
    }}).catch((err) => $("#connectHint").textContent = err.message);
  });

  $("#weatherForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const form = event.currentTarget;
    saveSettings({ weather: {
      enabled: form.enabled.checked,
      latitude: Number(form.latitude.value),
      longitude: Number(form.longitude.value),
      timezone: form.timezone.value,
      timezoneOffsetMinutes: Number(form.timezoneOffsetMinutes.value),
      manualWeather: form.manualWeather.checked,
      overrideTheme: form.overrideTheme.value,
      manualSeason: form.manualSeason.checked,
      overrideSeason: form.overrideSeason.value,
    }}).catch((err) => $("#connectHint").textContent = err.message);
  });

  $("#deviceForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const form = event.currentTarget;
    saveSettings({
      companion: {
        userName: form.userName.value.trim() || "Friend",
      },
      display: {
        brightness: Number(form.brightness.value),
        inverted: form.inverted.checked,
        idleAnimationEnabled: form.idleAnimationEnabled.checked,
      },
      touch: {
        tapWindowMs: Number(form.tapWindowMs.value),
        longPressMs: Number(form.longPressMs.value),
        annoyedPokeCount: Number(form.annoyedPokeCount.value),
        angryPokeCount: Number(form.angryPokeCount.value),
      },
    }).catch((err) => $("#connectHint").textContent = err.message);
  });
}

function boot() {
  installStaticOptions();
  installEvents();
  const saved = localStorage.getItem(STORAGE_KEY);
  if (saved) {
    $("#deviceAddress").value = saved;
    $("#accessToken").value = localStorage.getItem(TOKEN_STORAGE_KEY) || "";
    connect(saved).catch(() => setStatus(false, "Saved device not reachable"));
  } else {
    $("#deviceAddress").value = "tinycompanion.local";
    $("#accessToken").value = localStorage.getItem(TOKEN_STORAGE_KEY) || "";
  }
}

boot();
