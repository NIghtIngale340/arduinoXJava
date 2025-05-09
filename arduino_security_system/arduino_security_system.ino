#include <IRremote.h>

// Pin Definitions
const int IR_RECEIVE_PIN = 11;  // IR receiver pin (HX1838/VS1838)
const int RED_LED_PIN = 6;      // Red LED pin
const int GREEN_LED_PIN = 5;    // Green LED pin
const int BUZZER_PIN = 7;       // Buzzer pin
const int TRIG_PIN = 9;         // Ultrasonic sensor trigger pin
const int ECHO_PIN = 10;        // Ultrasonic sensor echo pin

// Constants
const unsigned long IR_DISARM_CODE = 0xE916FF00;  // IR code for disarm button (fixed format)
const long SENSOR_TIMEOUT = 2000;     // Timeout for ultrasonic sensor
const int DISTANCE_THRESHOLD = 100;   // Distance threshold in cm for motion detection
const int BUZZER_FREQUENCY = 2000;    // Frequency for passive buzzer
const int BUZZER_DURATION = 500;      // Duration for buzzer tone

// System state
bool isArmed = true;
bool isAlarmActive = false;
unsigned long lastMotionTime = 0;
unsigned long lastStatusUpdateTime = 0;
const int ALARM_INTERVAL = 1000;  // Alarm blink interval in milliseconds
const int STATUS_UPDATE_INTERVAL = 2000;  // Status update interval

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN);
  
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialize system state
  updateSystemState();
  
  Serial.println("System initialized");
}

void loop() {
  // Check for IR remote input
  if (IrReceiver.decode()) {
    handleIRInput();
    IrReceiver.resume();
  }
  
  // Check for motion if system is armed
  if (isArmed) {
    checkMotion();
  }
  
  // Handle alarm state
  if (isAlarmActive) {
    handleAlarm();
  }
  
  // Check for incoming commands from Java application
  checkSerialCommands();
  
  // Send periodic status updates to Java application
  unsigned long currentTime = millis();
  if (currentTime - lastStatusUpdateTime >= STATUS_UPDATE_INTERVAL) {
    sendStatusUpdate();
    lastStatusUpdateTime = currentTime;
  }
  
  delay(50);  // Small delay to prevent overwhelming the serial port
}

void handleIRInput() {
  // Debug print
  Serial.print("IR Code received: 0x");
  Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
  
  if (IrReceiver.decodedIRData.decodedRawData == IR_DISARM_CODE) {
    toggleSystemState();
  }
}

void toggleSystemState() {
  isArmed = !isArmed;
  isAlarmActive = false;
  updateSystemState();
  
  Serial.print("System ");
  Serial.println(isArmed ? "armed" : "disarmed");
}

void checkMotion() {
  long distance = getDistance();
  
  // Debug distance readings periodically
  static unsigned long lastDebugTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastDebugTime >= 5000) {
    Serial.print("Current distance: ");
    Serial.println(distance);
    lastDebugTime = currentTime;
  }
  
  if (distance < DISTANCE_THRESHOLD && distance > 0) {
    if (!isAlarmActive) {
      isAlarmActive = true;
      lastMotionTime = millis();
      Serial.println("Motion detected! Alarm triggered!");
    }
  }
}

void handleAlarm() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastMotionTime >= ALARM_INTERVAL) {
    digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
    // Use tone() for passive buzzer
    tone(BUZZER_PIN, BUZZER_FREQUENCY, BUZZER_DURATION);
    lastMotionTime = currentTime;
  }
}

void updateSystemState() {
  if (isArmed) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    noTone(BUZZER_PIN);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    noTone(BUZZER_PIN);
  }
}

long getDistance() {
  // Clear the TRIG_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Set the TRIG_PIN HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the ECHO_PIN, return the sound wave travel time in microseconds
  long duration = pulseIn(ECHO_PIN, HIGH, SENSOR_TIMEOUT);
  
  // Calculate the distance
  long distance = duration * 0.034 / 2;
  
  // If the distance is 0 or greater than the threshold, return a large value
  if (distance == 0 || distance > DISTANCE_THRESHOLD) {
    return DISTANCE_THRESHOLD + 1;
  }
  
  return distance;
}

void sendStatusUpdate() {
  String status = isArmed ? "ARMED" : "DISARMED";
  if (isAlarmActive) {
    status = "ALERT";
  }
  Serial.print("STATUS:");
  Serial.println(status);
  lastStatusUpdateTime = millis();
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    Serial.print("Received command: ");
    Serial.println(command);
    
    if (command == "ARM") {
      isArmed = true;
      isAlarmActive = false;
      updateSystemState();
      Serial.println("System armed by Java application");
    } 
    else if (command == "DISARM") {
      isArmed = false;
      isAlarmActive = false;
      updateSystemState();
      Serial.println("System disarmed by Java application");
    }
    else if (command == "SILENCE") {
      isAlarmActive = false;
      updateSystemState();
      Serial.println("Alarm silenced by Java application");
    }
    else {
      Serial.println("Unknown command");
    }
  }
}