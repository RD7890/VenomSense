#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TCS34725.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize Screen and Sensor
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_60X);

// ESP32 Onboard LED for Danger Indicator
const int blueLedPin = 2; 

// =========================================
// RGB to HSV Converter (Crucial for Saturation/Fadedness)
// =========================================
void rgbToHsv(float r, float g, float b, float &h, float &s, float &v) {
  float maxC = max(r, max(g, b));
  float minC = min(r, min(g, b));
  float delta = maxC - minC;

  v = maxC; // Value (Brightness)
  
  if (maxC != 0) {
    s = delta / maxC; // Saturation (0.0 to 1.0) -> This determines Faded vs Bold
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

void setup() {
  Serial.begin(115200);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(blueLedPin, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Screen Error");
    for(;;);
  }
  
  if (!tcs.begin()) {
    display.clearDisplay();
    display.setCursor(10, 20);
    display.setTextColor(WHITE);
    display.print("SENSOR MISSING!");
    display.display();
    while (1);
  }

  // Boot Screen
  display.clearDisplay();
  display.setTextSize(2);      
  display.setTextColor(WHITE); 
  display.setCursor(5, 20);     
  display.println("VenomSense");
  display.setTextSize(1);
  display.setCursor(30, 45);     
  display.println("Starting...");
  display.display();
  delay(2000); 
}

void loop() {
  uint16_t r_raw, g_raw, b_raw, c_raw;
  tcs.getRawData(&r_raw, &g_raw, &b_raw, &c_raw);

  // Proximity Lock: If nothing is in front of the sensor
  if (c_raw < 300) { 
    digitalWrite(blueLedPin, LOW);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(15, 30);
    display.println("INSERT CARTRIDGE");
    display.display();
    delay(200);
    return; 
  }

  // Calculate Hue, Saturation, and Value
  float h, s, v;
  rgbToHsv(r_raw, g_raw, b_raw, h, s, v);

  // LOGIC: Is it Red? (Red Hue is usually between 0-30 or 330-360 degrees)
  bool isRed = (h < 30.0 || h > 330.0);
  
  // LOGIC: Calculate Venom % based on Saturation
  // Saturation runs from 0.0 (White/Faded) to 1.0 (Bold/Solid Color)
  int venomPercent = 0;
  if (isRed) {
    venomPercent = (int)(s * 100.0); 
  }

  display.clearDisplay();

  // If Red is detected and saturation is above 10% (ignores pure white paper noise)
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
    // SAFE UI
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
}