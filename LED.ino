#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

const int RED_LED = 25;
const int YELLOW_LED = 26;
const int GREEN_LED = 27;

enum LedMode { MODE_OFF, MODE_ON, MODE_BLINK };

LedMode redMode = MODE_OFF;
LedMode yellowMode = MODE_OFF;
LedMode greenMode = MODE_OFF;

unsigned long lastBlinkTime = 0;
bool blinkState = false;  // Shared for all LEDs (can be per-LED if needed)
const unsigned long BLINK_INTERVAL = 500;  // ms


void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  Serial.begin(115200);
  SerialBT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth Started. Type commands...");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    SerialBT.write(c);
  }

  if (SerialBT.available()) {
    String input = SerialBT.readStringUntil('\n');
    input.trim();
    Serial.println("Received command: " + input);

    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);

    String action, only, color;
    bool isOnly = false;

    // Parsing the input string into action, only, and color
    if (firstComma != -1 && secondComma != -1) {
      action = input.substring(0, firstComma);
      only = input.substring(firstComma + 1, secondComma);
      color = input.substring(secondComma + 1);
      isOnly = only.equalsIgnoreCase("only");
    } else if (firstComma != -1) {
      action = input.substring(0, firstComma);
      color = input.substring(firstComma + 1);
      isOnly = false;
    } else {
      SerialBT.println("Invalid format. Use: on/off/blink[,only],color");
      return;
    }

    action.toLowerCase();
    color.toLowerCase();

    // Helper function to set the LED mode
    auto setMode = [](LedMode &led, LedMode mode) {
      led = mode;
    };

    // Helper function to apply 'only' action
    auto applyOnly = [&](LedMode mode, String target) {
      redMode = (target == "red") ? mode : MODE_OFF;
      yellowMode = (target == "yellow") ? mode : MODE_OFF;
      greenMode = (target == "green") ? mode : MODE_OFF;
    };

    LedMode targetMode;
    if (action == "on") targetMode = MODE_ON;
    else if (action == "off") targetMode = MODE_OFF;
    else if (action == "blink") targetMode = MODE_BLINK;
    else {
      SerialBT.println("Unknown action: " + action);
      return;
    }

    bool colorMatched = false;

    if (isOnly) {
      if (color == "all") {
        redMode = yellowMode = greenMode = targetMode;
        SerialBT.println("All LEDs set to " + action + " (only)");
        return;
      }

      // If action is "off", we should turn the others ON, not OFF
      if (targetMode == MODE_OFF) {
        redMode = (color != "red") ? MODE_ON : MODE_OFF;
        yellowMode = (color != "yellow") ? MODE_ON : MODE_OFF;
        greenMode = (color != "green") ? MODE_ON : MODE_OFF;
      } else {
        redMode = (color == "red") ? targetMode : MODE_OFF;
        yellowMode = (color == "yellow") ? targetMode : MODE_OFF;
        greenMode = (color == "green") ? targetMode : MODE_OFF;
      }

      SerialBT.println("Only " + color + " set to " + action);
      colorMatched = true;
    } else {
      // Handle actions for each individual LED or all LEDs
      if (color == "red" || color == "all") {
        redMode = targetMode;
        action.toUpperCase();
        SerialBT.println("Red LED " + action);
        colorMatched = true;
      }
      if (color == "yellow" || color == "all") {
        yellowMode = targetMode;
        action.toUpperCase();
        SerialBT.println("Yellow LED " + action);
        colorMatched = true;
      }
      if (color == "green" || color == "all") {
        greenMode = targetMode;
        action.toUpperCase();
        SerialBT.println("Green LED " + action);
        colorMatched = true;
      }
    }

    if (!colorMatched) {
      SerialBT.println("Unknown color: " + color);
    }
  }

  // Blinking logic (applied globally to all LEDs)
  unsigned long currentTime = millis();
  if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
    blinkState = !blinkState;
    lastBlinkTime = currentTime;
  }

  // Apply LED states
  digitalWrite(RED_LED,   redMode == MODE_ON ? HIGH : redMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);
  digitalWrite(YELLOW_LED, yellowMode == MODE_ON ? HIGH : yellowMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);
  digitalWrite(GREEN_LED, greenMode == MODE_ON ? HIGH : greenMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);

  delay(20);
}