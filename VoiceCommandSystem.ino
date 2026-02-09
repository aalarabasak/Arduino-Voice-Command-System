#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <TimerOne.h>

// LCD pinleri: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(8, 11, A2, A3, A4, A5); // RS, E, D4, D5, D6, D7

// Fan pinleri
const int IN1 = 13;
const int IN2 = 12; 
const int ENA = 3; 

// RGB LED pinleri (common anode)
const int RED_PIN = 4;
const int GREEN_PIN = 5;
const int BLUE_PIN = 6;

// EEPROM adresleri
#define EEPROM_FAN_STATE_ADDR     0
#define EEPROM_RED_ADDR           1
#define EEPROM_GREEN_ADDR         2
#define EEPROM_BLUE_ADDR          3
#define EEPROM_MOTOR_SPEED_ADDR   4
#define EEPROM_DIRECTION_ADDR     5
#define EEPROM_LED_SECONDS_ADDR   6 // Stores the initial seconds for LED timer, or 0 if off

int motorSpeed = 120; // Default motor speed
bool isRunning = false;
bool isForward = true; // Default fan direction
bool randomLightsActive = false;
unsigned long lastChangeTime = 0;
unsigned long currentDelay = 500; // Initial delay for random lights
unsigned long lastCommandTime = 0; // Tracks time of the last serial command
const unsigned long commandTimeout = 10000; // Timeout for random lights if no commands
unsigned long lastFanCommandTime = 0; // Tracks time of the last fan-specific command
const unsigned long fanTimeout = 10000; // Timeout for fan auto-off
volatile bool timeoutTriggered = false; // Flag set by Timer1 when LED timeout is reached
volatile int secondsPassed = 0;         // Counter for LED on-time
const int timeoutSeconds = 10;          // Duration for LED auto-off
enum CommandType { NONE, LED_COMMAND };
volatile CommandType lastCommandType = NONE; 

void onTimeout() {
  secondsPassed++;
  if (secondsPassed >= timeoutSeconds) {
    timeoutTriggered = true;
  }
}

void startLEDTimer(int startFrom = 0) {
  Timer1.detachInterrupt(); // Detach any existing interrupt
  Timer1.stop();            // Stop the timer
  secondsPassed = startFrom;
  timeoutTriggered = false;
  EEPROM.update(EEPROM_LED_SECONDS_ADDR, startFrom); 
  delay(10); 
  Timer1.initialize(1000000); // (1,000,000 microseconds)
  Timer1.attachInterrupt(onTimeout); 
}

void stopLEDTimer() {
  Timer1.detachInterrupt();
  Timer1.stop();
  secondsPassed = 0; 
  EEPROM.update(EEPROM_LED_SECONDS_ADDR, 0); 
}

void setup() {
  // Initialize Fan motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  analogWrite(ENA, 0); 

  // Initialize RGB LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();

  // Initialize Serial communication
  Serial.begin(9600);
  randomSeed(analogRead(0)); 

  // --- Restore Fan State from EEPROM ---
  byte fanState = EEPROM.read(EEPROM_FAN_STATE_ADDR);
  motorSpeed = EEPROM.read(EEPROM_MOTOR_SPEED_ADDR);
  isForward = EEPROM.read(EEPROM_DIRECTION_ADDR); // 0 for false (reverse), non-zero for true (forward)

  if (fanState == 1) {
    applyDirection(); // This will use the 'isForward' value read from EEPROM
    analogWrite(ENA, motorSpeed); // This will use the 'motorSpeed' value read from EEPROM
    isRunning = true;
    lcd.setCursor(0, 0);
    lcd.print("Fan restored ON");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Fan restored OFF");
  }

  // --- Restore LED State from EEPROM ---
  byte r = EEPROM.read(EEPROM_RED_ADDR);
  byte g = EEPROM.read(EEPROM_GREEN_ADDR);
  byte b = EEPROM.read(EEPROM_BLUE_ADDR);
  int restoredSeconds = EEPROM.read(EEPROM_LED_SECONDS_ADDR);
  setColor(r, g, b); 

  if ((r > 0 || g > 0 || b > 0) && restoredSeconds < timeoutSeconds) {
    lcd.setCursor(0, 1);
    lcd.print("LED restored on");
    startLEDTimer(restoredSeconds);
    lastCommandType = LED_COMMAND;
  } else {
    setColor(0, 0, 0);
    lcd.setCursor(0, 1);
    lcd.print("LED restored off");
  }
}

void loop() {
  // Check for incoming serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove whitespace
    Serial.println("Received: " + command); // Echo command to Serial Monitor
    lastCommandTime = millis(); // Update time of last general command

    // --- Fan Control Commands ---
    if (command == "turn on fan") {
      applyDirection();
      analogWrite(ENA, motorSpeed);
      isRunning = true;
      EEPROM.update(EEPROM_FAN_STATE_ADDR, 1);
      EEPROM.update(EEPROM_MOTOR_SPEED_ADDR, motorSpeed); 
      EEPROM.update(EEPROM_DIRECTION_ADDR, isForward);    
      lcd.clear(); 
      lcd.print("Fan turned on");
      lastFanCommandTime = millis(); // Update time of last fan command
    }
    else if (command == "turn off fan") {
      digitalWrite(IN1, LOW); 
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 0);
      isRunning = false;
      EEPROM.update(EEPROM_FAN_STATE_ADDR, 0);
      lcd.clear(); lcd.print("Fan turned off");
      lastFanCommandTime = millis();
    }
    else if (command == "increase fan speed") {
      motorSpeed = min(motorSpeed + 20, 165); // Max speed 165 (can be up to 255)
      if (isRunning) analogWrite(ENA, motorSpeed);
      EEPROM.update(EEPROM_MOTOR_SPEED_ADDR, motorSpeed);
      lcd.clear(); lcd.print("Fan speed +");
      lastFanCommandTime = millis();
    }
    else if (command == "decrease fan speed") {
      motorSpeed = max(motorSpeed - 20, 0); // Min speed 0
      if (isRunning) analogWrite(ENA, motorSpeed);
      EEPROM.update(EEPROM_MOTOR_SPEED_ADDR, motorSpeed);
      lcd.clear(); lcd.print("Fan speed -");
      lastFanCommandTime = millis();
    }
    else if (command == "change fan direction") {
      isForward = !isForward;
      EEPROM.update(EEPROM_DIRECTION_ADDR, isForward);
      if (isRunning) applyDirection();
      lcd.clear(); lcd.print("Dir changed");
      lastFanCommandTime = millis();
    }
    // --- LED Control Commands ---
    else if (command == "turn on red light") {
      randomLightsActive = false;
      setColor(255, 0, 0);
      lcd.clear(); lcd.print("Red LED on");
      lastCommandType = LED_COMMAND;
      startLEDTimer(); // Start timer from 0
    }
    else if (command == "turn on green light") {
      randomLightsActive = false;
      setColor(0, 255, 0);
      lcd.clear(); lcd.print("Green LED on");
      lastCommandType = LED_COMMAND;
      startLEDTimer();
    }
    else if (command == "turn on blue light") {
      randomLightsActive = false;
      setColor(0, 0, 255);
      lcd.clear(); lcd.print("Blue LED on");
      lastCommandType = LED_COMMAND;
      startLEDTimer();
    }
    else if (command == "turn on yellow light") {
      randomLightsActive = false;
      setColor(255, 255, 0);
      lcd.clear(); lcd.print("Yellow LED on");
      lastCommandType = LED_COMMAND;
      startLEDTimer();
    }
    else if (command == "turn on pink light") {
      randomLightsActive = false;
      setColor(255, 192, 203);
      lcd.clear(); lcd.print("Pink LED on");
      lastCommandType = LED_COMMAND;
      startLEDTimer();
    }
    else if (command == "turn on light blue light") {
      randomLightsActive = false;
      setColor(151, 255, 255);
      lcd.clear(); lcd.print("LightBlue on");
      lastCommandType = LED_COMMAND;
      startLEDTimer();
    }
    else if (command == "turn off light") {
      randomLightsActive = false;
      setColor(0, 0, 0); 
      lcd.clear(); lcd.print("Lights off");
      stopLEDTimer(); 
    }
    else if (command == "random lights") {
      randomLightsActive = true;
      stopLEDTimer(); 
      lcd.clear(); lcd.print("Random lights ON");
    }
    // --- System Commands ---
    else if (command == "reset") {
      for (int i = 0; i <= EEPROM_LED_SECONDS_ADDR; i++) EEPROM.update(i, 0); // Clear all used EEPROM
      lcd.clear(); lcd.print("EEPROM RESET");
      motorSpeed = 120; // Reset to default
      isRunning = false;
      isForward = true; // Reset to default
      randomLightsActive = false;
      setColor(0,0,0); // Turn off LED and update EEPROM for color
      stopLEDTimer();    // Stop timer and update EEPROM for timer seconds
      digitalWrite(IN1, LOW); 
      digitalWrite(IN2, LOW);
      analogWrite(ENA, 0);
    }
  }
  // --- Automatic Timeout Logic ---
  // Fan auto-off if no fan command received for fanTimeout duration
  if (isRunning && (millis() - lastFanCommandTime > fanTimeout)) {
    digitalWrite(IN1, LOW); 
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
    isRunning = false;
    EEPROM.update(EEPROM_FAN_STATE_ADDR, 0);
    lcd.clear(); lcd.print("Fan auto-OFF");
    lastCommandType = NONE; 
  }

  // LED auto-off if its timer (Timer1) has triggered
  if (timeoutTriggered) {
    if (lastCommandType == LED_COMMAND) { 
      setColor(0, 0, 0);
      lcd.clear(); lcd.print("LED auto-OFF");
      stopLEDTimer(); 
    }
    timeoutTriggered = false; 
    lastCommandType = NONE; 
  }

  // Random lights auto-off if no general command received for commandTimeout duration
  if (randomLightsActive && (millis() - lastCommandTime > commandTimeout)) {
    randomLightsActive = false;
    setColor(0, 0, 0); // Turn off LEDs
    lcd.clear(); lcd.print("Lights auto-OFF");
  }

  // --- Random Lights Effect ---
  if (randomLightsActive) {
    unsigned long now = millis();
    if (now - lastChangeTime > currentDelay) {
      lastChangeTime = now;
      int colors[6][3] = {
        {255, 0, 0},   // Red
        {0, 255, 0},   // Green
        {0, 0, 255},   // Blue
        {255, 255, 0}, // Yellow
        {255, 192, 203},// Pink
        {151, 255, 255} // Light Blue
      };
      int randIndex = random(0, 6); // Pick a random color index
      setColor(colors[randIndex][0], colors[randIndex][1], colors[randIndex][2]);
      currentDelay = random(100, 300); // Set a new random delay for the next change
    }
  }
}

// Function to apply fan direction
void applyDirection() {
  if (isForward) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
}

// Function to set RGB LED color (Common Anode)
void setColor(int redVal, int greenVal, int blueVal) {
  analogWrite(RED_PIN, 255 - redVal);
  analogWrite(GREEN_PIN, 255 - greenVal);
  analogWrite(BLUE_PIN, 255 - blueVal);

  EEPROM.update(EEPROM_RED_ADDR, redVal);
  EEPROM.update(EEPROM_GREEN_ADDR, greenVal);
  EEPROM.update(EEPROM_BLUE_ADDR, blueVal);
}

