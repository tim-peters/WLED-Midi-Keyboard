#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

#define WLED_IP   "192.168.178.183"

constexpr int8_t  MIDI_RX_PIN = 2;
constexpr int8_t  MIDI_TX_PIN = 1;

constexpr uint16_t LED_START = 0;
constexpr uint16_t LED_END   = 78;

constexpr uint8_t  FIRST_NOTE = 48;
constexpr uint8_t  NUM_KEYS   = 25;
constexpr int8_t   KEY_OFFSET = 0;

constexpr bool     WHITE_KEYS_ONLY          = true;
constexpr bool     VELOCITY_ENABLED         = true;
constexpr uint16_t FADE_OUT_MS_DEFAULT      = 300;
constexpr uint8_t  MAX_BRIGHTNESS_PCT_DEFAULT = 100;

constexpr uint8_t  DEFAULT_COLOR_R = 0xff;
constexpr uint8_t  DEFAULT_COLOR_G = 0x64;
constexpr uint8_t  DEFAULT_COLOR_B = 0x00;

constexpr uint32_t MIN_SEND_INTERVAL_MS = 33;

struct CcConfig { uint8_t ch; uint8_t num; };
constexpr CcConfig CC_COLOR      = {1, 7};
constexpr CcConfig CC_BRIGHTNESS = {2, 7};
constexpr CcConfig CC_FADE       = {3, 7};

struct ActiveNote {
  bool     active;
  uint8_t  r, g, b;
  float    startBrightness;
  float    currentBrightness;
  bool     isFading;
  uint32_t fadeStartMs;
};

ActiveNote activeNotes[NUM_KEYS] = {};

uint8_t ledMap[LED_END + 1][3] = {};

uint8_t  colorR = DEFAULT_COLOR_R;
uint8_t  colorG = DEFAULT_COLOR_G;
uint8_t  colorB = DEFAULT_COLOR_B;
float    hslH = 0.0f, hslS = 0.0f, hslL = 0.0f;
uint8_t  maxBrightnessPct = MAX_BRIGHTNESS_PCT_DEFAULT;
uint16_t fadeOutMs        = FADE_OUT_MS_DEFAULT;

uint32_t lastSendMs    = 0;
bool     sendPending   = false;
uint32_t nextSendAt    = 0;

uint8_t  midiStatus  = 0;
uint8_t  midiData1   = 0;
bool     midiHasData1 = false;

void rgbToHsl(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& l) {
  float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
  float maxC = max(rf, max(gf, bf));
  float minC = min(rf, min(gf, bf));
  l = (maxC + minC) / 2.0f;
  if (maxC == minC) {
    h = 0.0f; s = 0.0f;
  } else {
    float d = maxC - minC;
    s = l > 0.5f ? d / (2.0f - maxC - minC) : d / (maxC + minC);
    if (maxC == rf)      h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);
    else if (maxC == gf) h = (bf - rf) / d + 2.0f;
    else                 h = (rf - gf) / d + 4.0f;
    h *= 60.0f;
  }
}

void hslToRgb(float h, float s, float l, uint8_t& r, uint8_t& g, uint8_t& b) {
  h = fmod(h, 360.0f);
  if (h < 0.0f) h += 360.0f;
  float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
  float x = c * (1.0f - fabsf(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = l - c / 2.0f;
  float rf, gf, bf;
  if (h < 60.0f)       { rf = c; gf = x; bf = 0.0f; }
  else if (h < 120.0f) { rf = x; gf = c; bf = 0.0f; }
  else if (h < 180.0f) { rf = 0.0f; gf = c; bf = x; }
  else if (h < 240.0f) { rf = 0.0f; gf = x; bf = c; }
  else if (h < 300.0f) { rf = x; gf = 0.0f; bf = c; }
  else                 { rf = c; gf = 0.0f; bf = x; }
  r = (uint8_t)round((rf + m) * 255.0f);
  g = (uint8_t)round((gf + m) * 255.0f);
  b = (uint8_t)round((bf + m) * 255.0f);
}

bool isWhiteKey(uint8_t n) {
  uint8_t m = ((n % 12) + 12) % 12;
  return m == 0 || m == 2 || m == 4 || m == 5 || m == 7 || m == 9 || m == 11;
}

uint8_t effectiveFirstNote() {
  int v = (int)FIRST_NOTE - (int)KEY_OFFSET;
  if (v < 0) v = 0;
  if (v > 127 - NUM_KEYS + 1) v = 127 - NUM_KEYS + 1;
  return (uint8_t)v;
}

uint8_t effectiveLastNote() {
  return (uint8_t)(effectiveFirstNote() + NUM_KEYS - 1);
}

int noteToIndex(uint8_t n) {
  int idx = (int)n - (int)effectiveFirstNote();
  if (idx < 0 || idx >= NUM_KEYS) return -1;
  return idx;
}

bool getNoteSegment(uint8_t note, uint16_t& segStart, uint16_t& segEnd) {
  uint16_t usableLeds = (LED_END - LED_START) + 1;
  if (usableLeds == 0) return false;
  uint8_t first = effectiveFirstNote();
  uint8_t last  = effectiveLastNote();

  if (WHITE_KEYS_ONLY) {
    if (!isWhiteKey(note)) return false;
    uint8_t whiteKeys[NUM_KEYS];
    uint8_t numWhite = 0;
    for (int n = first; n <= last; n++) {
      if (isWhiteKey((uint8_t)n)) {
        whiteKeys[numWhite++] = (uint8_t)n;
      }
    }
    if (numWhite == 0) return false;
    int idx = -1;
    for (uint8_t i = 0; i < numWhite; i++) {
      if (whiteKeys[i] == note) { idx = i; break; }
    }
    if (idx < 0) return false;
    uint16_t segSize   = usableLeds / numWhite;
    uint16_t remainder = usableLeds % numWhite;
    uint16_t start = LED_START;
    for (int i = 0; i < idx; i++) {
      start += (i < (int)remainder) ? (segSize + 1) : segSize;
    }
    uint16_t mySize = (idx < (int)remainder) ? (segSize + 1) : segSize;
    segStart = start;
    segEnd   = start + mySize - 1;
    return true;
  }

  int idx = noteToIndex(note);
  if (idx < 0) return false;
  uint16_t segSize   = usableLeds / NUM_KEYS;
  uint16_t remainder = usableLeds % NUM_KEYS;
  uint16_t start = LED_START;
  for (int i = 0; i < idx; i++) {
    start += (i < (int)remainder) ? (segSize + 1) : segSize;
  }
  uint16_t mySize = (idx < (int)remainder) ? (segSize + 1) : segSize;
  segStart = start;
  segEnd   = start + mySize - 1;
  return true;
}

void buildLedColorMap() {
  for (uint16_t i = LED_START; i <= LED_END; i++) {
    ledMap[i][0] = 0;
    ledMap[i][1] = 0;
    ledMap[i][2] = 0;
  }
  uint8_t first = effectiveFirstNote();
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    ActiveNote& n = activeNotes[i];
    if (!n.active) continue;
    uint8_t note = first + i;
    uint16_t segStart, segEnd;
    if (!getNoteSegment(note, segStart, segEnd)) continue;
    float factor = n.currentBrightness / 255.0f;
    uint8_t r = (uint8_t)round(n.r * factor);
    uint8_t g = (uint8_t)round(n.g * factor);
    uint8_t b = (uint8_t)round(n.b * factor);
    for (uint16_t idx = segStart; idx <= segEnd; idx++) {
      if (idx >= LED_START && idx <= LED_END) {
        ledMap[idx][0] = r;
        ledMap[idx][1] = g;
        ledMap[idx][2] = b;
      }
    }
  }
}

String buildWledJson() {
  String json;
  json.reserve(80 + (LED_END - LED_START + 1) * 12);
  json = "{\"seg\":[{\"id\":0,\"i\":[";
  for (uint16_t idx = LED_START; idx <= LED_END; idx++) {
    if (idx > LED_START) json += ",";
    json += idx;
    json += ",[";
    json += ledMap[idx][0];
    json += ",";
    json += ledMap[idx][1];
    json += ",";
    json += ledMap[idx][2];
    json += "]";
  }
  json += "]}]}";
  return json;
}

bool sendToWledNow() {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.setReuse(true);
  String url = "http://" WLED_IP "/json/state";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String payload = buildWledJson();
  int code = http.POST(payload);
  http.end();
  lastSendMs = millis();
  return (code > 0);
}

void scheduleWledSend() {
  uint32_t now = millis();
  if (now - lastSendMs >= MIN_SEND_INTERVAL_MS) {
    sendToWledNow();
  } else {
    nextSendAt  = lastSendMs + MIN_SEND_INTERVAL_MS;
    sendPending = true;
  }
}

void flushPendingWled() {
  if (!sendPending) return;
  if ((int32_t)(millis() - nextSendAt) >= 0) {
    sendPending = false;
    sendToWledNow();
  }
}

void noteOn(uint8_t note, uint8_t velocity) {
  int idx = noteToIndex(note);
  if (idx < 0) {
    Serial.printf("note %d ignored (out of range)\n", note);
    return;
  }
  if (WHITE_KEYS_ONLY && !isWhiteKey(note)) {
    Serial.printf("note %d ignored (black key)\n", note);
    return;
  }
  if (velocity == 0) { noteOff(note); return; }

  float brightness = VELOCITY_ENABLED
    ? (velocity / 127.0f) * (maxBrightnessPct / 100.0f) * 255.0f
    : (maxBrightnessPct / 100.0f) * 255.0f;
  if (brightness > 255.0f) brightness = 255.0f;

  ActiveNote& n = activeNotes[idx];
  n.active            = true;
  n.r                 = colorR;
  n.g                 = colorG;
  n.b                 = colorB;
  n.startBrightness   = brightness;
  n.currentBrightness = brightness;
  n.isFading          = false;
  n.fadeStartMs       = 0;

  buildLedColorMap();
  scheduleWledSend();
}

void noteOff(uint8_t note) {
  int idx = noteToIndex(note);
  if (idx < 0) return;
  ActiveNote& n = activeNotes[idx];
  if (!n.active) return;
  n.startBrightness = n.currentBrightness;
  n.fadeStartMs     = millis();
  n.isFading        = true;
}

void tickFade() {
  bool anyChange = false;
  uint32_t now = millis();
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    ActiveNote& n = activeNotes[i];
    if (!n.active || !n.isFading) continue;
    uint32_t elapsed = now - n.fadeStartMs;
    if (elapsed >= fadeOutMs) {
      n.active = false;
      anyChange = true;
    } else {
      n.currentBrightness = n.startBrightness * (1.0f - (float)elapsed / (float)fadeOutMs);
      anyChange = true;
    }
  }
  if (anyChange) {
    buildLedColorMap();
    scheduleWledSend();
  }
}

bool matchesCcConfig(uint8_t channel, uint8_t ccNum, CcConfig cfg) {
  return ccNum == cfg.num && (cfg.ch == 0 || cfg.ch == channel);
}

void handleCcMessage(uint8_t channel, uint8_t ccNum, uint8_t ccVal) {
  if (matchesCcConfig(channel, ccNum, CC_COLOR)) {
    float hue = (ccVal / 127.0f) * 360.0f;
    hslToRgb(hue, hslS, hslL, colorR, colorG, colorB);
  }
  if (matchesCcConfig(channel, ccNum, CC_BRIGHTNESS)) {
    maxBrightnessPct = (uint8_t)round((ccVal / 127.0f) * 100.0f);
  }
  if (matchesCcConfig(channel, ccNum, CC_FADE)) {
    fadeOutMs = (uint16_t)round((ccVal / 127.0f) * 3000.0f);
  }
}

void dispatchMidi(uint8_t status, uint8_t d1, uint8_t d2) {
  uint8_t cmd     = status & 0xF0;
  uint8_t channel = (status & 0x0F) + 1;
  if (cmd == 0x90) {
    if (d2 == 0) noteOff(d1);
    else         noteOn(d1, d2);
  } else if (cmd == 0x80) {
    noteOff(d1);
  } else if (cmd == 0xB0) {
    handleCcMessage(channel, d1, d2);
  }
}

void handleMidiByte(uint8_t byte) {
  if (byte & 0x80) {
    if (byte == 0xF0 || byte == 0xF7 || byte == 0xFF) {
      midiStatus = 0;
      return;
    }
    midiStatus = byte;
    midiHasData1 = false;
  } else {
    if (midiStatus == 0) return;
    uint8_t cmd = midiStatus & 0xF0;
    if (cmd == 0xC0 || cmd == 0xD0) {
      dispatchMidi(midiStatus, byte, 0);
      midiStatus = 0;
    } else if (!midiHasData1) {
      midiData1 = byte;
      midiHasData1 = true;
    } else {
      dispatchMidi(midiStatus, midiData1, byte);
      midiHasData1 = false;
    }
  }
}

void resetAndClearWled() {
  for (uint16_t i = LED_START; i <= LED_END; i++) {
    ledMap[i][0] = 0;
    ledMap[i][1] = 0;
    ledMap[i][2] = 0;
  }
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    activeNotes[i].active = false;
  }
  sendToWledNow();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial1.begin(31250, SERIAL_8N1, MIDI_RX_PIN, MIDI_TX_PIN);
  Serial.printf("MIDI input on Serial1 (RX=%d, TX=%d) @ 31250 baud\n", MIDI_RX_PIN, MIDI_TX_PIN);

  rgbToHsl(DEFAULT_COLOR_R, DEFAULT_COLOR_G, DEFAULT_COLOR_B, hslH, hslS, hslL);

  Serial.printf("Connecting to WiFi '%s' ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(200);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi connected, IP %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection FAILED (will retry in loop)");
  }

  resetAndClearWled();
  Serial.println("WLED cleared");
  Serial.printf("MIDI range: %d-%d, LEDs: %d-%d, WLED: %s\n",
                effectiveFirstNote(), effectiveLastNote(),
                LED_START, LED_END, WLED_IP);
  Serial.println("Ready");
}

void loop() {
  while (Serial1.available()) {
    handleMidiByte((uint8_t)Serial1.read());
  }

  if (WiFi.status() != WL_CONNECTED) {
    static uint32_t lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 30000) {
      lastReconnectAttempt = millis();
      Serial.println("WiFi reconnect...");
      WiFi.reconnect();
    }
  }

  tickFade();
  flushPendingWled();
}
