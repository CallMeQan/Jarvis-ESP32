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
bool blinkState = false;  // Shared for all LEDs
const unsigned long BLINK_INTERVAL = 500;  // ms

void processCommand(String input);

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  Serial.begin(115200);
  SerialBT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth Started. Type commands...");
}

unsigned long lastCommandTime = 0;
int commandDelay = 0;
String pendingCommands[10];
int pendingCount = 0;
int pendingIndex = 0;

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    SerialBT.write(c);
  }

  // If there are pending commands, process with 2s delay between each
  if (pendingCount > 0 && millis() - lastCommandTime >= commandDelay) {
    processCommand(pendingCommands[pendingIndex]);
    pendingIndex++;
    lastCommandTime = millis();
    commandDelay = 2000;
    if (pendingIndex >= pendingCount) {
      // Reset after all commands processed
      pendingCount = 0;
      pendingIndex = 0;
      commandDelay = 0;
    }
  }

  // If not processing pending commands, check for new input
  if (pendingCount == 0 && SerialBT.available()) {
    String input = SerialBT.readStringUntil('\n');
    input.trim();
    SerialBT.println("Received command(s): " + input);

    // Split by ';' and store as pending commands
    pendingCount = 0;
    pendingIndex = 0;
    int start = 0;
    while (start < input.length() && pendingCount < 10) {
      int sep = input.indexOf(';', start);
      String cmd;
      if (sep == -1) {
        cmd = input.substring(start);
        start = input.length();
      } else {
        cmd = input.substring(start, sep);
        start = sep + 1;
      }
      cmd.trim();
      if (cmd.length() > 0) {
        pendingCommands[pendingCount++] = cmd;
      }
    }
    lastCommandTime = millis();
    commandDelay = 0; // Process the first command immediately
  }

  // Blinking logic (runs **every** loop, not just after a command)
  unsigned long currentTime = millis();
  if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
    blinkState = !blinkState;
    lastBlinkTime = currentTime;
  }

  // Apply LED states (runs **every** loop, not just after a command)
  digitalWrite(RED_LED,   redMode == MODE_ON ? HIGH : redMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);
  digitalWrite(YELLOW_LED, yellowMode == MODE_ON ? HIGH : yellowMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);
  digitalWrite(GREEN_LED, greenMode == MODE_ON ? HIGH : greenMode == MODE_BLINK ? (blinkState ? HIGH : LOW) : LOW);

  delay(10); // Small delay for stability
}

void processCommand(String input) {
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
    if (color == "red" || color == "all") {
      redMode = targetMode;
      SerialBT.println("Red LED " + action);
      colorMatched = true;
    }
    if (color == "yellow" || color == "all") {
      yellowMode = targetMode;
      SerialBT.println("Yellow LED " + action);
      colorMatched = true;
    }
    if (color == "green" || color == "all") {
      greenMode = targetMode;
      SerialBT.println("Green LED " + action);
      colorMatched = true;
    }
  }

  if (!colorMatched) {
    SerialBT.println("Unknown color: " + color);
  }
}
