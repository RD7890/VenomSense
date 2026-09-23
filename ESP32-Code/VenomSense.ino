// =============================================
//  VenomSense — ESP32 Firmware
//  Snake Venom Detection via Color Saturation
//  WiFi AP + WebSocket + JSON Data Provider
// =============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TCS34725.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// =========================================
// Configuration
// =========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const char* AP_SSID     = "VenomSense";
const char* AP_PASS     = "venom1234";
const int   WS_PORT     = 81;
const int   HTTP_PORT   = 80;

// =========================================
// Hardware Initialization
// =========================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_60X);
const int blueLedPin = 2;

// =========================================
// Servers
// =========================================
WebServer server(HTTP_PORT);
WebSocketsServer webSocket(WS_PORT);

// =========================================
// Deep Scan State
// =========================================
bool deepScanActive         = false;
unsigned long deepScanStart = 0;
const unsigned long DEEP_SCAN_DURATION = 3000; // 3 seconds
float deepScanSatSum        = 0;
int   deepScanReadings      = 0;

// =========================================
// Broadcast Timing
// =========================================
unsigned long lastBroadcast = 0;
const unsigned long BROADCAST_INTERVAL = 150; // 150ms for instant quick scans

// =========================================
// RGB to HSV Converter (Crucial for Saturation/Fadedness)
// =========================================
void rgbToHsv(float r, float g, float b, float &h, float &s, float &v) {
  float maxC = max(r, max(g, b));
  float minC = min(r, min(g, b));
  float delta = maxC - minC;

  v = maxC; // Value (Brightness)

  if (maxC != 0) {
    s = delta / maxC; // Saturation (0.0 to 1.0) -> Determines Faded vs Bold
  } else {
    s = 0;
    h = -1;
    return;
  }

  if (delta == 0) {
    h = 0;
  } else {
    if (r == maxC) h = (g - b) / delta;
    else if (g == maxC) h = 2.0 + (b - r) / delta;
    else h = 4.0 + (r - g) / delta;
    h *= 60.0;
    if (h < 0) h += 360.0; // Hue (0 to 360 degrees)
  }
}

// =========================================
// Broadcast JSON to all WebSocket clients
// =========================================
void broadcastScan(int venom, const char* status, bool isDeepScan,
                   float hue, float saturation,
                   uint16_t r, uint16_t g, uint16_t b, uint16_t c,
                   int readings) {
  StaticJsonDocument<384> doc;

  doc["type"]        = isDeepScan ? "deepScanResult" : "scan";
  doc["venom"]       = venom;
  doc["status"]      = status;
  doc["isDeepScan"]  = isDeepScan;
  doc["hue"]         = serialized(String(hue, 2));
  doc["saturation"]  = serialized(String(saturation, 3));

  JsonObject raw = doc.createNestedObject("raw");
  raw["r"] = r;
  raw["g"] = g;
  raw["b"] = b;
  raw["c"] = c;

  if (isDeepScan) {
    doc["readings"] = readings;
  }

  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
}

void broadcastStatus(const char* type) {
  StaticJsonDocument<64> doc;
  doc["type"] = type;
  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
}

void broadcastProgress(int progress) {
  StaticJsonDocument<96> doc;
  doc["type"]     = "deepScanProgress";
  doc["progress"] = progress;
  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
}

void broadcastNoCartridge() {
  StaticJsonDocument<128> doc;
  doc["type"]       = "scan";
  doc["venom"]      = 0;
  doc["status"]     = "no_cartridge";
  doc["isDeepScan"] = false;
  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);
}

// =========================================
// WebSocket Event Handler
// =========================================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client #%u disconnected\n", num);
      break;

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client #%u connected from %s\n", num, ip.toString().c_str());
      // Send a welcome/status message
      StaticJsonDocument<96> doc;
      doc["type"]   = "connected";
      doc["device"] = "VenomSense";
      String json;
      serializeJson(doc, json);
      webSocket.sendTXT(num, json);
      break;
    }

    case WStype_TEXT: {
      String msg = String((char*)payload);
      Serial.printf("[WS] Received: %s\n", msg.c_str());

      if (msg == "deepScan" && !deepScanActive) {
        deepScanActive   = true;
        deepScanStart    = millis();
        deepScanSatSum   = 0;
        deepScanReadings = 0;
        broadcastStatus("deepScanStarted");
        Serial.println("[DEEP SCAN] Started");
      }
      break;
    }

    default:
      break;
  }
}

// =========================================
// HTTP Routes
// =========================================
void setupHTTPRoutes() {
  server.on("/", []() {
    String info = "VenomSense Device Active\n";
    info += "WebSocket: ws://" + WiFi.softAPIP().toString() + ":" + String(WS_PORT) + "\n";
    info += "Clients: " + String(webSocket.connectedClients()) + "\n";
    info += "Uptime: " + String(millis() / 1000) + "s\n";
    server.send(200, "text/plain", info);
  });

  server.on("/status", []() {
    StaticJsonDocument<128> doc;
    doc["device"]  = "VenomSense";
    doc["uptime"]  = millis() / 1000;
    doc["clients"] = webSocket.connectedClients();
    doc["wsPort"]  = WS_PORT;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });
}

// =========================================
// Setup
// =========================================
void setup() {
  Serial.begin(115200);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(blueLedPin, LOW);

  // --- OLED Init ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Screen Error");
    for (;;);
  }

  // --- Sensor Init ---
  if (!tcs.begin()) {
    display.clearDisplay();
    display.setCursor(10, 20);
    display.setTextColor(WHITE);
    display.print("SENSOR MISSING!");
    display.display();
    while (1);
  }

  // --- Boot Screen ---
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5, 20);
  display.println("VenomSense");
  display.setTextSize(1);
  display.setCursor(30, 45);
  display.println("Starting...");
  display.display();
  delay(1500);

  // --- WiFi Access Point ---
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.println("-------------------------------");
  Serial.println("VenomSense AP Started");
  Serial.print("SSID: "); Serial.println(AP_SSID);
  Serial.print("Pass: "); Serial.println(AP_PASS);
  Serial.print("IP:   "); Serial.println(ip);
  Serial.print("WS:   ws://"); Serial.print(ip); Serial.print(":"); Serial.println(WS_PORT);
  Serial.println("-------------------------------");

  // Show network info on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(5, 5);
  display.println("WiFi AP Ready!");
  display.setCursor(5, 20);
  display.print("SSID: ");
  display.println(AP_SSID);
  display.setCursor(5, 35);
  display.print("IP: ");
  display.println(ip);
  display.setCursor(5, 50);
  display.print("WS Port: ");
  display.println(WS_PORT);
  display.display();
  delay(2500);

  // --- WebSocket Server ---
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // --- HTTP Server ---
  setupHTTPRoutes();
  server.begin();

  Serial.println("VenomSense Ready!");
}

// =========================================
// Main Loop
// =========================================
void loop() {
  // Always handle server events
  webSocket.loop();
  server.handleClient();

  // --- Read Sensor ---
  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  // ===========================
  // Proximity Lock: No Cartridge
  // ===========================
  if (c_raw < 300) {
    digitalWrite(blueLedPin, LOW);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(15, 30);
    display.println("INSERT CARTRIDGE");
    display.display();

    if (millis() - lastBroadcast > BROADCAST_INTERVAL) {
      broadcastNoCartridge();
      lastBroadcast = millis();
    }

    delay(200);
    return;
  }

  // --- Calculate HSV ---
  float h, s, v;
  rgbToHsv(r_raw, g_raw, b_raw, h, s, v);
  bool isRed = (h < 30.0 || h > 330.0);

  // ===========================
  // DEEP SCAN MODE
  // ===========================
  if (deepScanActive) {
    unsigned long elapsed = millis() - deepScanStart;

    if (elapsed < DEEP_SCAN_DURATION) {
      // --- Accumulate readings ---
      if (isRed) {
        deepScanSatSum += s;
        deepScanReadings++;
      }

      // --- OLED: Deep Scan Progress ---
      float progress = (float)elapsed / DEEP_SCAN_DURATION;
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(15, 8);
      display.println("DEEP SCANNING...");

      // Progress bar
      display.drawRect(10, 28, 108, 16, WHITE);
      display.fillRect(12, 30, (int)(104 * progress), 12, WHITE);

      // Percentage text
      display.setCursor(45, 50);
      display.setTextSize(1);
      display.print((int)(progress * 100));
      display.println("% done");
      display.display();

      // Broadcast progress (faster interval during deep scan)
      if (millis() - lastBroadcast > 150) {
        broadcastProgress((int)(progress * 100));
        lastBroadcast = millis();
      }

      return; // Stay in deep scan mode
    } else {
      // --- Deep Scan Complete ---
      deepScanActive = false;
      float avgSat   = deepScanReadings > 0 ? deepScanSatSum / deepScanReadings : 0;
      int deepVenom  = (int)(avgSat * 100.0);
      const char* status = (isRed && deepVenom > 10) ? "danger" : "safe";

      Serial.printf("[DEEP SCAN] Complete — Venom: %d%%, Readings: %d\n", deepVenom, deepScanReadings);

      // Broadcast deep scan result
      broadcastScan(deepVenom, status, true, h, avgSat, r_raw, g_raw, b_raw, c_raw, deepScanReadings);

      // --- OLED: Deep Scan Result ---
      display.clearDisplay();
      display.fillRect(0, 0, 128, 18, WHITE);
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(10, 5);
      display.println("DEEP SCAN RESULT");

      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(5, 24);
      display.print("Venom:");
      display.print(deepVenom);
      display.println("%");

      display.setTextSize(1);
      display.setCursor(5, 48);
      display.print("Samples: ");
      display.print(deepScanReadings);
      display.print(" | ");
      display.print(deepVenom >= 50 ? "BOLD" : "FADED");
      display.display();

      // Instead of a blocking delay(3000) which freezes WebSocket transmission,
      // we use a non-blocking loop so the app receives the result instantly.
      unsigned long waitStart = millis();
      while(millis() - waitStart < 3000) {
        webSocket.loop();
        yield();
      }
      return;
    }
  }

  // ===========================
  // NORMAL REAL-TIME SCAN MODE
  // ===========================
  int venomPercent = 0;
  if (isRed) {
    venomPercent = (int)(s * 100.0);
  }

  display.clearDisplay();

  // --- DANGER UI ---
  if (isRed && venomPercent > 10) {
    digitalWrite(blueLedPin, HIGH); // Danger LED ON

    // DANGER HEADER (Solid Inverted Box)
    display.fillRect(0, 0, 128, 22, WHITE);
    display.setTextSize(2);
    display.setTextColor(BLACK);
    display.setCursor(20, 4);
    display.println("DANGER!");

    // PERCENTAGE DISPLAY
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 30);
    display.print("Venom:");
    display.print(venomPercent);
    display.println("%");

    // FADED vs BOLD SUBTEXT
    display.setTextSize(1);
    display.setCursor(10, 52);
    if (venomPercent >= 50) {
      display.println("HIGH TOXIN (BOLD)");
    } else {
      display.println("LOW TOXIN (FADED)");
    }

    display.display();

    // Warning Flash Effect
    delay(200);
    display.invertDisplay(true);
    delay(200);
    display.invertDisplay(false);

  } else {
    // --- SAFE UI ---
    digitalWrite(blueLedPin, LOW);

    // SAFE HEADER (Outlined Box)
    display.drawRoundRect(0, 0, 128, 22, 3, WHITE);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(40, 4);
    display.println("SAFE");

    display.setTextSize(1);
    display.setCursor(15, 35);
    display.println("NO TOXIN DETECTED");

    display.setCursor(35, 50);
    display.println("Venom: 0%");

    display.display();
    delay(500);
  }

  // --- Broadcast data via WebSocket ---
  if (millis() - lastBroadcast > BROADCAST_INTERVAL) {
    const char* status = (isRed && venomPercent > 10) ? "danger" : "safe";
    broadcastScan(venomPercent, status, false, h, s, r_raw, g_raw, b_raw, c_raw, 0);
    lastBroadcast = millis();
  }
}
