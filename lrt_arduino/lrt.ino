#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// Hardware pins
#define SERVO_PIN 12
#define BUZZER_PIN 13
#define TRIG_PIN 11
#define ECHO_PIN 10
#define GREEN_LED_PIN A0
#define RED_LED_PIN A1

// Display settings
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo settings
Servo turnstileServo;
#define SERVO_CLOSED 0
#define SERVO_OPEN 90

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 9, 8, 7, 6 };
byte colPins[COLS] = { 5, 4, 3, 2 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// System states
#define STATE_MAIN_MENU 0
#define STATE_TOPUP_BEEP 1
#define STATE_TOPUP_SINGLE 2
#define STATE_TICKET_TYPE 3
#define STATE_CARD_TYPE 4
#define STATE_LINE_SELECT 5
#define STATE_STATION_SELECT 6
#define STATE_DEST_SELECT 7
#define STATE_TRANSFER 8
#define STATE_SETTINGS 9        // State for settings menu
#define STATE_RESET_CONFIRM 10  // State for reset confirmation
#define STATE_TRANSACTION_COMPLETE 11  // New state for completed transactions
int currentState = STATE_MAIN_MENU;

// Settings password (default: 1234)
const char settingsPassword[] = "1234";

// Current user and date information
char currentUser[16] = "NIghtIngale340";
char currentDateTime[30] = "2025-05-18 02:43";

// Variables for fare calculation
int beepBalance = 100;  // Starting with 100 pesos
int singleJourneyBalance = 0;
#define TICKET_SINGLE 1
#define TICKET_BEEP 2
#define CARD_REGULAR 1
#define CARD_STUDENT 2
#define CARD_PWD 3
#define CARD_SENIOR 4
byte ticketType = TICKET_BEEP;    // Use byte instead of string
byte userCardType = CARD_REGULAR;  // Use byte instead of string
int totalFare = 0;
int totalDistance = 0;  // Track total distance
int currentTripDistance = 0;  // Distance for current trip

// Current journey variables
byte currentLine = 1;
byte currentStation = 0;
byte stationCount = 0;
byte destinationStation = 255;  // Use 255 to indicate no destination selected
boolean transactionInProgress = false;  // Flag to prevent multiple transactions
boolean destinationSelected = false;  // Flag to track if destination is selected
boolean turnstileOpen = false;  // Track turnstile state to prevent duplicate messages

// EEPROM addresses
#define BEEP_BALANCE_ADDR 0
#define SINGLE_BALANCE_ADDR 4
#define CURRENT_LINE_ADDR 8
#define CURRENT_STATION_ADDR 9
#define TOTAL_DISTANCE_ADDR 10  // New EEPROM address for total distance

// Serial communication variables
char inputBuffer[32];
byte bufferIndex = 0;
boolean commandComplete = false;
unsigned long lastStatusUpdate = 0;
const unsigned long statusInterval = 5000;  // Send status updates every 5 seconds

// State timeout variables to prevent system getting stuck
unsigned long stateStartTime = 0;
const unsigned long transactionTimeout = 10000;  // 10 seconds timeout for transactions

// LRT1 Station Names
const char lrt1_station_0[] PROGMEM = "FPJR.";
const char lrt1_station_1[] PROGMEM = "Balintawak";
const char lrt1_station_2[] PROGMEM = "Monumento";
const char lrt1_station_3[] PROGMEM = "5th Avenue";
const char lrt1_station_4[] PROGMEM = "R. Papa";
const char lrt1_station_5[] PROGMEM = "Abad Santos";
const char lrt1_station_6[] PROGMEM = "Blumentritt";
const char lrt1_station_7[] PROGMEM = "Tayuman";
const char lrt1_station_8[] PROGMEM = "Bambang";
const char lrt1_station_9[] PROGMEM = "D. Jose";
const char lrt1_station_10[] PROGMEM = "Carriedo";
const char lrt1_station_11[] PROGMEM = "Central Terminal";
const char lrt1_station_12[] PROGMEM = "United Nations";
const char lrt1_station_13[] PROGMEM = "Pedro Gil";
const char lrt1_station_14[] PROGMEM = "Quirino";
const char lrt1_station_15[] PROGMEM = "Vito Cruz";
const char lrt1_station_16[] PROGMEM = "Gil Puyat";
const char lrt1_station_17[] PROGMEM = "Libertad";
const char lrt1_station_18[] PROGMEM = "EDSA";
const char lrt1_station_19[] PROGMEM = "Baclaran";

const char* const lrt1_stations[] PROGMEM = {
  lrt1_station_0, lrt1_station_1, lrt1_station_2, lrt1_station_3, lrt1_station_4,
  lrt1_station_5, lrt1_station_6, lrt1_station_7, lrt1_station_8, lrt1_station_9,
  lrt1_station_10, lrt1_station_11, lrt1_station_12, lrt1_station_13, lrt1_station_14,
  lrt1_station_15, lrt1_station_16, lrt1_station_17, lrt1_station_18, lrt1_station_19
};

// LRT2 Station Names
const char lrt2_station_0[] PROGMEM = "Recto";
const char lrt2_station_1[] PROGMEM = "Legarda";
const char lrt2_station_2[] PROGMEM = "Pureza";
const char lrt2_station_3[] PROGMEM = "V. Mapa";
const char lrt2_station_4[] PROGMEM = "J. Ruiz";
const char lrt2_station_5[] PROGMEM = "Gilmore";
const char lrt2_station_6[] PROGMEM = "Betty Go-Belmonte";
const char lrt2_station_7[] PROGMEM = "Cubao";
const char lrt2_station_8[] PROGMEM = "Anonas";
const char lrt2_station_9[] PROGMEM = "Katipunan";
const char lrt2_station_10[] PROGMEM = "Santolan";
const char lrt2_station_11[] PROGMEM = "Marikina";
const char lrt2_station_12[] PROGMEM = "Antipolo";

const char* const lrt2_stations[] PROGMEM = {
  lrt2_station_0, lrt2_station_1, lrt2_station_2, lrt2_station_3, lrt2_station_4,
  lrt2_station_5, lrt2_station_6, lrt2_station_7, lrt2_station_8, lrt2_station_9,
  lrt2_station_10, lrt2_station_11, lrt2_station_12
};

// Fare matrices for LRT1 (20x20 stations)
const byte lrt1_fare_matrix[] PROGMEM = {
  13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 23, 24, 25, 26, 27, 28, 29, 30, 33, 35,
  14, 13, 15, 15, 17, 18, 19, 20, 21, 22, 23, 24, 24, 25, 26, 27, 28, 29, 32, 34,
  15, 15, 13, 14, 15, 16, 17, 18, 20, 21, 22, 22, 23, 24, 25, 26, 27, 28, 31, 33,
  16, 15, 14, 13, 15, 16, 17, 17, 19, 20, 21, 21, 22, 23, 24, 25, 26, 27, 30, 32,
  17, 17, 15, 15, 13, 14, 15, 16, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26, 29, 31,
  18, 18, 16, 16, 14, 13, 14, 15, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25, 28, 30,
  19, 19, 17, 17, 15, 14, 13, 14, 16, 17, 17, 18, 19, 20, 21, 22, 23, 24, 27, 29,
  20, 20, 18, 17, 16, 15, 14, 13, 15, 16, 16, 17, 18, 19, 20, 21, 22, 23, 26, 28,
  22, 21, 20, 19, 18, 17, 16, 15, 13, 14, 15, 16, 17, 17, 18, 19, 20, 22, 24, 27,
  23, 22, 21, 20, 19, 18, 17, 16, 14, 13, 14, 15, 16, 16, 17, 18, 19, 20, 23, 26,
  23, 23, 22, 21, 19, 18, 17, 16, 15, 14, 13, 14, 15, 16, 16, 17, 18, 19, 22, 25,
  24, 24, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 14, 15, 16, 16, 17, 18, 21, 24,
  25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 14, 15, 16, 17, 18, 21, 23,
  26, 25, 24, 23, 22, 21, 20, 19, 17, 16, 16, 15, 14, 13, 14, 15, 16, 17, 20, 22,
  27, 26, 25, 24, 23, 22, 21, 20, 18, 17, 16, 16, 15, 14, 13, 14, 15, 16, 19, 21,
  28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 16, 15, 14, 13, 14, 15, 18, 20,
  29, 28, 27, 26, 25, 24, 23, 22, 20, 19, 18, 17, 16, 16, 15, 14, 13, 14, 17, 19,
  30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 16, 18,
  33, 32, 31, 30, 29, 28, 27, 26, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 13, 16,
  35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 16, 13
};

// Fare matrix for LRT2 (using similar structure for 13x13)
// Using same pricing structure as LRT1 for simplicity
const byte lrt2_fare_matrix[] PROGMEM = {
  13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 25, 27, 30,
  14, 13, 14, 15, 16, 17, 18, 19, 21, 22, 24, 26, 29,
  15, 14, 13, 14, 15, 16, 17, 18, 20, 21, 23, 25, 28,
  16, 15, 14, 13, 14, 15, 16, 17, 19, 20, 22, 24, 27,
  17, 16, 15, 14, 13, 14, 15, 16, 18, 19, 21, 23, 26,
  18, 17, 16, 15, 14, 13, 14, 15, 17, 18, 20, 22, 25,
  19, 18, 17, 16, 15, 14, 13, 14, 16, 17, 19, 21, 24,
  20, 19, 18, 17, 16, 15, 14, 13, 15, 16, 18, 20, 23,
  22, 21, 20, 19, 18, 17, 16, 15, 13, 14, 16, 18, 21,
  23, 22, 21, 20, 19, 18, 17, 16, 14, 13, 15, 17, 20,
  25, 24, 23, 22, 21, 20, 19, 18, 16, 15, 13, 15, 18,
  27, 26, 25, 24, 23, 22, 21, 20, 18, 17, 15, 13, 16,
  30, 29, 28, 27, 26, 25, 24, 23, 21, 20, 18, 16, 13
};

// Distance matrices for LRT1 (in kilometers)
const byte lrt1_distance_matrix[] PROGMEM = {
  0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  1, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  3, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  4, 3, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  5, 4, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  6, 5, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  7, 6, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  8, 7, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  9, 8, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  10, 9, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 10, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  12, 11, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7,
  14, 13, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6,
  15, 14, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5,
  16, 15, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4,
  17, 16, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3,
  18, 17, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2,
  19, 18, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1,
  20, 19, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

// Distance matrix for LRT2
const byte lrt2_distance_matrix[] PROGMEM = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 
  2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7,
  6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6,
  7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5,
  8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4,
  9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3,
  10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2,
  11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1,
  12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

// Function prototypes
void displayMainMenu();
void displayTicketTypeMenu();
void displayCardTypeMenu();
void displayLineMenu();
void displaySettingsMenu();
void displayTransferOptions(byte line, byte station);
void displayStations(byte line);
int getFareFromMatrix(byte line, byte origin, byte dest);
int getDistanceFromMatrix(byte line, byte origin, byte dest);
void openTurnstile();
void closeTurnstile();
long measureDistance();
void soundBuzzer(byte pattern);
char readKey();
int readNumber(byte maxDigits);
bool isTransferStation(byte line, byte station);
void transferToOtherLine();
bool validateFare(int fare);
void loadBalances();
void saveBalances();
void saveCurrentStation();
void loadCurrentStation();
void loadTotalDistance();
void saveTotalDistance();
void getStationName(byte line, byte station, char* buffer);
void displayStatusBar();
void resetBalances();
void resetTotalDistance();
char* readPassword(byte maxDigits);
bool authenticateUser(const char* enteredPassword);
void processMainMenu(char key);
void processTopUpBeep(char key);
void processTopUpSingle(char key);
void processTicketType(char key);
void processCardType(char key);
void processLineSelect(char key);
void processStationSelect(char key);
void processDestSelect(char key);
void processTransferSelect(char key);
void processSettingsMenu(char key);
void processResetConfirm(char key);
void processPassengerEntry();
void finishTransaction();
void displayUserInfo();
void processSerialData();
void handleSerialCommand(char* command);
void sendBalanceUpdate();
void sendStationUpdate();
void sendUserInfo();
void sendDistanceUpdate();
void sendTripRecord(byte line, byte origin, byte dest, int fare, int distance);
void sendTopupRecord(byte cardType, int amount, int newBalance);
void sendMessage(const char* message);
void sendError(const char* errorCode);
void checkStateTimeout();

void setup() {
  // Initialize Serial communication with the Java application
  Serial.begin(9600);
  
  // Wait for serial port to initialize
  delay(1000);
  
  // Initialize hardware
  lcd.init();
  lcd.backlight();

  turnstileServo.attach(SERVO_PIN);
  turnstileServo.write(SERVO_CLOSED);  // Initialize turnstile position without message
  turnstileOpen = false;  // Initialize state

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  // Load saved balances and distance from EEPROM
  loadBalances();
  loadCurrentStation();
  loadTotalDistance();

  // Reset transaction flag
  transactionInProgress = false;

  // Initialize state timer
  stateStartTime = millis();

  // Display welcome message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Metro Fare Calc"));
  lcd.setCursor(0, 1);
  lcd.print(F("Welcome, "));
  lcd.print(currentUser);
  delay(2000);

  // Show current date/time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(currentDateTime);
  lcd.setCursor(0, 1);
  lcd.print(F("System ready"));
  delay(2000);

  // Send initial data to Java app
  sendMessage("Arduino connected on COM5");
  sendBalanceUpdate();
  sendStationUpdate();
  sendDistanceUpdate();
  sendUserInfo();

  displayMainMenu();
}

void loop() {
  // Process Serial input
  processSerialData();
  
  // Process keypad input
  char key = readKey();

  // Check for state timeouts
  checkStateTimeout();

  // Display status bar in relevant states
  if (currentState == STATE_MAIN_MENU) {
    displayStatusBar();
  }

  // Periodic status updates to Java app
  if ((millis() - lastStatusUpdate) >= statusInterval) {
    sendBalanceUpdate();
    sendStationUpdate();
    sendDistanceUpdate();
    lastStatusUpdate = millis();
  }

  // Process input based on current state
  switch (currentState) {
    case STATE_MAIN_MENU:
      processMainMenu(key);
      break;
    case STATE_TOPUP_BEEP:
      processTopUpBeep(key);
      break;
    case STATE_TOPUP_SINGLE:
      processTopUpSingle(key);
      break;
    case STATE_TICKET_TYPE:
      processTicketType(key);
      break;
    case STATE_CARD_TYPE:
      processCardType(key);
      break;
    case STATE_LINE_SELECT:
      processLineSelect(key);
      break;
    case STATE_STATION_SELECT:
      processStationSelect(key);
      break;
    case STATE_DEST_SELECT:
      processDestSelect(key);
      break;
    case STATE_TRANSFER:
      processTransferSelect(key);
      break;
    case STATE_SETTINGS:
      processSettingsMenu(key);
      break;
    case STATE_RESET_CONFIRM:
      processResetConfirm(key);
      break;
    case STATE_TRANSACTION_COMPLETE:
      // Wait for timeout or key press to return to destination selection
      if (key != 0) {
        // Any key press returns to destination selection
        finishTransaction();
      }
      break;
  }

  // Check ultrasonic sensor for turnstile simulation
  if (currentState == STATE_DEST_SELECT && !transactionInProgress && destinationSelected) {
    long distance = measureDistance();
    // If someone approaches within 10cm
    if (distance < 10 && distance > 0) {
      Serial.println(F("MSG:Passenger detected!"));
      // Set flag to prevent multiple detections
      transactionInProgress = true;
      // Process turnstile entry logic
      processPassengerEntry();
    }
  }
}

// Check for state timeouts to prevent system getting stuck
void checkStateTimeout() {
  // Only implement timeout for transaction complete state
  if (currentState == STATE_TRANSACTION_COMPLETE) {
    if ((millis() - stateStartTime) > transactionTimeout) {
      finishTransaction(); // Auto-return to destination selection after timeout
    }
  }
}

// Finish transaction and return to appropriate state
// Finish transaction and return to appropriate state
void finishTransaction() {
  // Reset transaction flag
  transactionInProgress = false;
  destinationSelected = false;  // Reset destination selection flag
  destinationStation = 255;     // Reset destination station
  
  // Turn off LED
  digitalWrite(GREEN_LED_PIN, LOW);
  
  // Check if current station is a transfer station
  if (isTransferStation(currentLine, currentStation)) {
    currentState = STATE_TRANSFER;
    displayTransferOptions(currentLine, currentStation);
    return;
  }
  
  // If not a transfer station, return to destination selection state
  currentState = STATE_DEST_SELECT;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  char stationName[30];
  getStationName(currentLine, currentStation, stationName);
  lcd.print(F("Current: "));
  lcd.print(stationName);
  lcd.setCursor(0, 1);
  lcd.print(F("Select dest."));
  
  // Send updated station information
  sendStationUpdate();
}

// Process serial data coming from Java app
void processSerialData() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    // Process complete commands when newline is received
    if (inChar == '\n') {
      if (bufferIndex < 32) {
        inputBuffer[bufferIndex] = '\0'; // Terminate the string
        handleSerialCommand(inputBuffer);
      }
      bufferIndex = 0; // Reset buffer
    } else if (bufferIndex < 31) {
      // Add character to buffer
      inputBuffer[bufferIndex++] = inChar;
    }
  }
}

// Handle commands from Java app
void handleSerialCommand(char* command) {
  // Commands are expected in format: CMD:ACTION,param1,param2,...
  if (strncmp(command, "CMD:", 4) == 0) {
    char* action = command + 4;
    
    if (strncmp(action, "GET_BAL", 7) == 0) {
      // Send current balances
      sendBalanceUpdate();
    } 
    else if (strncmp(action, "GET_STN", 7) == 0) {
      // Send current station
      sendStationUpdate();
    }
    else if (strncmp(action, "GET_DIST", 8) == 0) {
      // Send total distance
      sendDistanceUpdate();
    }
    else if (strncmp(action, "GET_USER", 8) == 0) {
      // Send current user info
      sendUserInfo();
    }
    else if (strncmp(action, "TOPUP", 5) == 0) {
      // Format: CMD:TOPUP,BEEP|SINGLE,amount
      char* firstComma = strchr(action, ',');
      if (firstComma) {
        char* secondComma = strchr(firstComma + 1, ',');
        if (secondComma) {
          *secondComma = '\0'; // Temporarily null-terminate for comparison
          char* cardTypeStr = firstComma + 1;
          int amount = atoi(secondComma + 1);
          *secondComma = ','; // Restore the comma
          
          if (amount <= 0) {
            sendError("002"); // Invalid amount
            return;
          }
          
          // Debug message to see what's being received
          char debugMsg[50];
          sprintf(debugMsg, "Received card type: '%s'", cardTypeStr);
          sendMessage(debugMsg);
          
          // Trim any leading whitespace
          while (*cardTypeStr == ' ') cardTypeStr++;
          
          if (strncmp(cardTypeStr, "BEEP", 4) == 0) {
            beepBalance += amount;
            saveBalances();
            sendMessage("Beep card topped up");
            sendTopupRecord(TICKET_BEEP, amount, beepBalance);
          } 
          else if (strncmp(cardTypeStr, "SINGLE", 6) == 0) {
            singleJourneyBalance += amount;
            saveBalances();
            sendMessage("Single journey card topped up");
            sendTopupRecord(TICKET_SINGLE, amount, singleJourneyBalance);
          } 
          else {
            // Send more detailed error
            char errorMsg[50];
            sprintf(errorMsg, "Invalid card type: '%s'", cardTypeStr);
            sendMessage(errorMsg);
            sendError("003"); // Invalid card type
          }
          
          sendBalanceUpdate();
        } else {
          sendError("001"); // Invalid command format
        }
      } else {
        sendError("001"); // Invalid command format
      }
    }
    else if (strncmp(action, "RESET_BAL", 9) == 0) {
      // Format: CMD:RESET_BAL,password
      char* firstComma = strchr(action, ',');
      
      if (firstComma) {
        char* password = firstComma + 1;
        
        if (strcmp(password, settingsPassword) == 0) {
          resetBalances();
          sendMessage("Balances reset successfully");
          sendBalanceUpdate();
        } 
        else {
          sendError("005"); // Authentication failed
        }
      } 
      else {
        sendError("001"); // Invalid command format
      }
    }
    else if (strncmp(action, "RESET_DIST", 10) == 0) {
      // Format: CMD:RESET_DIST,password
      char* firstComma = strchr(action, ',');
      
      if (firstComma) {
        char* password = firstComma + 1;
        
        if (strcmp(password, settingsPassword) == 0) {
          resetTotalDistance();
          sendMessage("Total distance reset successfully");
          sendDistanceUpdate();
        } 
        else {
          sendError("005"); // Authentication failed
        }
      } 
      else {
        sendError("001"); // Invalid command format
      }
    }
    else {
      sendError("004"); // Invalid parameters
    }
  }
}

// Display status bar (balance, current station, and distance)
void displayStatusBar() {
  // Don't update if we're in a state where we don't want to show the status bar
  if (currentState == STATE_TOPUP_BEEP || currentState == STATE_TOPUP_SINGLE || 
      currentState == STATE_TICKET_TYPE || currentState == STATE_CARD_TYPE || 
      currentState == STATE_RESET_CONFIRM) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("B:"));
  lcd.print(beepBalance);
  lcd.print(F("P S:"));
  lcd.print(singleJourneyBalance);
  lcd.print(F("P"));
  
  lcd.setCursor(0, 1);
  if (currentState == STATE_MAIN_MENU) {
    lcd.print(F("1.Beep 2.Single"));
  } else if (currentState == STATE_LINE_SELECT) {
    lcd.print(F("Select line"));
  } else if (currentState == STATE_STATION_SELECT || currentState == STATE_DEST_SELECT) {
    char stationName[30];
    getStationName(currentLine, currentStation, stationName);
    lcd.print(F("Line "));
    lcd.print(currentLine);
    lcd.print(F(" "));
    lcd.print(stationName);
  }
}

// Display user information
void displayUserInfo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("User: "));
  lcd.print(currentUser);
  lcd.setCursor(0, 1);
  lcd.print(F("Total: "));
  lcd.print(totalDistance);
  lcd.print(F("km"));
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(currentDateTime);
  lcd.setCursor(0, 1);
  lcd.print(F("Press any key"));
  
  // Wait for any key press
  while (!readKey()) {
    processSerialData(); // Continue processing serial data while waiting
    delay(10);
  }
}

// Get station name from PROGMEM - Enhanced version
void getStationName(byte line, byte station, char* buffer) {
  // Clear the buffer first
  memset(buffer, 0, 30);
  
  if (line == 1 && station < 20) {
    // Direct access to individual station strings
    switch(station) {
      case 0: strcpy_P(buffer, lrt1_station_0); break;
      case 1: strcpy_P(buffer, lrt1_station_1); break;
      case 2: strcpy_P(buffer, lrt1_station_2); break;
      case 3: strcpy_P(buffer, lrt1_station_3); break;
      case 4: strcpy_P(buffer, lrt1_station_4); break;
      case 5: strcpy_P(buffer, lrt1_station_5); break;
      case 6: strcpy_P(buffer, lrt1_station_6); break;
      case 7: strcpy_P(buffer, lrt1_station_7); break;
      case 8: strcpy_P(buffer, lrt1_station_8); break;
      case 9: strcpy_P(buffer, lrt1_station_9); break;
      case 10: strcpy_P(buffer, lrt1_station_10); break;
      case 11: strcpy_P(buffer, lrt1_station_11); break;
      case 12: strcpy_P(buffer, lrt1_station_12); break;
      case 13: strcpy_P(buffer, lrt1_station_13); break;
      case 14: strcpy_P(buffer, lrt1_station_14); break;
      case 15: strcpy_P(buffer, lrt1_station_15); break;
      case 16: strcpy_P(buffer, lrt1_station_16); break;
      case 17: strcpy_P(buffer, lrt1_station_17); break;
      case 18: strcpy_P(buffer, lrt1_station_18); break;
      case 19: strcpy_P(buffer, lrt1_station_19); break;
      default: strcpy(buffer, "Unknown"); break;
    }
  } else if (line == 2 && station < 13) {
    // Direct access to individual station strings
    switch(station) {
      case 0: strcpy_P(buffer, lrt2_station_0); break;
      case 1: strcpy_P(buffer, lrt2_station_1); break;
      case 2: strcpy_P(buffer, lrt2_station_2); break;
      case 3: strcpy_P(buffer, lrt2_station_3); break;
      case 4: strcpy_P(buffer, lrt2_station_4); break;
      case 5: strcpy_P(buffer, lrt2_station_5); break;
      case 6: strcpy_P(buffer, lrt2_station_6); break;
      case 7: strcpy_P(buffer, lrt2_station_7); break;
      case 8: strcpy_P(buffer, lrt2_station_8); break;
      case 9: strcpy_P(buffer, lrt2_station_9); break;
      case 10: strcpy_P(buffer, lrt2_station_10); break;
      case 11: strcpy_P(buffer, lrt2_station_11); break;
      case 12: strcpy_P(buffer, lrt2_station_12); break;
      default: strcpy(buffer, "Unknown"); break;
    }
  } else {
    strcpy(buffer, "Unknown");
  }
  
  // Ensure null termination
  buffer[29] = '\0';
}

// Process passenger entry at the turnstile
void processPassengerEntry() {
  // Validate destination is selected before processing
  if (!destinationSelected || destinationStation == 255) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Select dest first"));
    delay(2000);
    return;
  }
  
  int fare = getFareFromMatrix(currentLine, currentStation, destinationStation);
  int tripDistance = getDistanceFromMatrix(currentLine, currentStation, destinationStation);

  if (validateFare(fare)) {
    // Successful entry
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Fare: "));
    lcd.print(fare);
    lcd.print(F(" PHP"));
    lcd.setCursor(0, 1);
    lcd.print(F("Dist: "));
    lcd.print(tripDistance);
    lcd.print(F("km"));

    digitalWrite(GREEN_LED_PIN, HIGH);
    openTurnstile();
    soundBuzzer(1);  // Success pattern

    // Update journey details
    totalFare += fare;
    totalDistance += tripDistance;
    saveTotalDistance(); // Save the updated distance
    
    // Send distance update to Java app
    sendDistanceUpdate();

    // Send trip record to Java app
    sendTripRecord(currentLine, currentStation, destinationStation, fare, tripDistance);

    // Update the current station to the destination
    currentStation = destinationStation;
    saveCurrentStation();  // Save the current station to EEPROM

    // Deduct fare from balance
    if (ticketType == TICKET_BEEP) {
      beepBalance -= fare;
      saveBalances();
    } else {
      singleJourneyBalance -= fare;
      saveBalances();
    }

    // Send updated balance to Java app
    sendBalanceUpdate();

    // Change to transaction complete state with timer
    currentState = STATE_TRANSACTION_COMPLETE;
    stateStartTime = millis();

    // Wait for passenger to pass through
    delay(3000);

    // Check if the passenger has passed through
    long dist = measureDistance();
    if (dist > 30) {  // No one in front of the sensor
      closeTurnstile();
      digitalWrite(GREEN_LED_PIN, LOW); // Turn off green LED
    } else {
      // Start a timer to close the turnstile after a delay
      // regardless of sensor reading (safety feature)
      unsigned long closeStartTime = millis();
      while ((millis() - closeStartTime) < 5000) { // 5 second max wait time
        dist = measureDistance();
        if (dist > 30) {
          break; // Exit the loop if person has passed
        }
        delay(100);
      }
      // Close the turnstile no matter what after timeout
      closeTurnstile();
      digitalWrite(GREEN_LED_PIN, LOW);
    }
      // Check if current station is a transfer station
    if (isTransferStation(currentLine, currentStation)) {
      transactionInProgress = false; // Reset transaction flag first
      destinationSelected = false;   // Reset destination selection
      destinationStation = 255;      // Reset destination station
      currentState = STATE_TRANSFER;
      displayTransferOptions(currentLine, currentStation);
    } else {
      // Stay in transaction complete state, will timeout or user can press any key
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Trip complete"));
      lcd.setCursor(0, 1);
      lcd.print(F("Press any key"));
    }
  } else {
    // Insufficient balance
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Insuf. balance"));
    lcd.setCursor(0, 1);
    lcd.print(F("Need: "));
    lcd.print(fare);
    lcd.print(F(" PHP"));

    digitalWrite(RED_LED_PIN, HIGH);
    soundBuzzer(2);  // Error pattern
    
    // Send message to Java app
    sendMessage("Insufficient balance for trip");
    
    delay(2000);
    digitalWrite(RED_LED_PIN, LOW);
    
    // Reset transaction flag
    transactionInProgress = false;

    // Return to destination selection
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Select dest."));
    displayStations(currentLine);
  }
}

// Process main menu selections
void processMainMenu(char key) {
  switch (key) {
    case '1':  // Top up Beep card
      currentState = STATE_TOPUP_BEEP;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Beep Balance:"));
      lcd.setCursor(0, 1);
      lcd.print(beepBalance);
      lcd.print(F(" PHP"));
      break;

    case '2':  // Top up Single Journey
      currentState = STATE_TOPUP_SINGLE;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Single Balance:"));
      lcd.setCursor(0, 1);
      lcd.print(singleJourneyBalance);
      lcd.print(F(" PHP"));
      break;

    case '3':  // Start Journey
      currentState = STATE_TICKET_TYPE;
      displayTicketTypeMenu();
      break;

    case '4':  // Exit
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Total Distance:"));
      lcd.setCursor(0, 1);
      lcd.print(totalDistance);
      lcd.print(F(" km"));
      delay(2000);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Thank you!"));
      lcd.setCursor(0, 1);
      lcd.print(F("Goodbye!"));
      delay(2000);
      displayMainMenu();
      break;

    case 'A':  // Display user info
      displayUserInfo();
      displayMainMenu();
      break;

    case 'D':  // Access settings (hidden option)
      currentState = STATE_SETTINGS;
      displaySettingsMenu();
      break;
  }
}

// Process top up for Beep card
void processTopUpBeep(char key) {
  if (key == '#') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Enter amount:"));
    lcd.setCursor(0, 1);
    int amount = readNumber(4);  // Read up to 4 digits

    if (amount > 0) {
      beepBalance += amount;
      saveBalances();

      // Send top-up record to Java app
      sendTopupRecord(TICKET_BEEP, amount, beepBalance);
      sendBalanceUpdate();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Top-up success!"));
      lcd.setCursor(0, 1);
      lcd.print(F("New bal: "));
      lcd.print(beepBalance);
      delay(2000);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid amount"));
      delay(2000);
    }

    currentState = STATE_MAIN_MENU;
    displayMainMenu();
  } else if (key == '*') {
    // Go back
    currentState = STATE_MAIN_MENU;
    displayMainMenu();
  }
}

// Process top up for Single Journey
void processTopUpSingle(char key) {
  if (key == '#') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Enter amount:"));
    lcd.setCursor(0, 1);
    int amount = readNumber(4);  // Read up to 4 digits

    if (amount > 0) {
      singleJourneyBalance += amount;
      saveBalances();

      // Send top-up record to Java app
      sendTopupRecord(TICKET_SINGLE, amount, singleJourneyBalance);
      sendBalanceUpdate();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Top-up success!"));
      lcd.setCursor(0, 1);
      lcd.print(F("New bal: "));
      lcd.print(singleJourneyBalance);
      delay(2000);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid amount"));
      delay(2000);
    }

    currentState = STATE_MAIN_MENU;
    displayMainMenu();
  } else if (key == '*') {
    // Go back
    currentState = STATE_MAIN_MENU;
    displayMainMenu();
  }
}

// Process ticket type selection
void processTicketType(char key) {
  switch (key) {
    case '1':  // Single Journey
      ticketType = TICKET_SINGLE;
      currentState = STATE_CARD_TYPE;
      displayCardTypeMenu();
      break;

    case '2':  // Beep Card
      ticketType = TICKET_BEEP;
      userCardType = CARD_REGULAR;
      currentState = STATE_CARD_TYPE;
      displayCardTypeMenu();
      break;

    case '*':  // Go back
      currentState = STATE_MAIN_MENU;
      displayMainMenu();
      break;
  }
}

// Process card type selection
void processCardType(char key) {
  switch (key) {
    case '1':  // Regular
      userCardType = CARD_REGULAR;
      currentState = STATE_LINE_SELECT;
      displayLineMenu();
      break;

    case '2':  // Student
      userCardType = CARD_STUDENT;
      currentState = STATE_LINE_SELECT;
      displayLineMenu();
      break;

    case '3':  // PWD
      userCardType = CARD_PWD;
      currentState = STATE_LINE_SELECT;
      displayLineMenu();
      break;

    case '4':  // Senior
      userCardType = CARD_SENIOR;
      currentState = STATE_LINE_SELECT;
      displayLineMenu();
      break;

    case '*':  // Go back
      currentState = STATE_TICKET_TYPE;
      displayTicketTypeMenu();
      break;
  }
}

// Process line selection
void processLineSelect(char key) {
  switch (key) {
    case '1':  // LRT-1
      currentLine = 1;
      stationCount = 20;
      currentState = STATE_STATION_SELECT;
      lcd.clear();
      lcd.print(F("LRT1: Choose stn"));
      displayStations(currentLine);
      break;

    case '2':  // LRT-2
      currentLine = 2;
      stationCount = 13;
      currentState = STATE_STATION_SELECT;
      lcd.clear();
      lcd.print(F("LRT2: Choose stn"));
      displayStations(currentLine);
      break;

    case '4':  // Exit train
      // Show summary
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Total fare:"));
      lcd.print(totalFare);
      lcd.setCursor(0, 1);
      lcd.print(F("Total dist:"));
      lcd.print(totalDistance);
      
      // Send summary message to Java app
      char message[60];
      sprintf(message, "Journey complete: %dP, %dkm", totalFare, totalDistance);
      sendMessage(message);
      
      delay(3000);

      // Reset journey values
      totalFare = 0;

      currentState = STATE_MAIN_MENU;
      displayMainMenu();
      break;

    case '5':  // Change ticket type
      currentState = STATE_TICKET_TYPE;
      displayTicketTypeMenu();
      break;

    case '*':  // Go back
      currentState = STATE_MAIN_MENU;
      displayMainMenu();
      break;
  }
}

// Process starting station selection
void processStationSelect(char key) {
  if (key >= '1' && key <= '9') {
    int selection = key - '0' - 1;  // Convert key to 0-based index

    if (selection < stationCount) {
      currentStation = selection;
      saveCurrentStation();  // Save to EEPROM
      sendStationUpdate();   // Update Java app
      currentState = STATE_DEST_SELECT;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("From: "));
      char stationName[30];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Select dest."));
    }
  } else if (key == '0' && stationCount > 9) {
    // Special case for index 9
    currentStation = 9;
    saveCurrentStation();  // Save to EEPROM
    sendStationUpdate();   // Update Java app
    currentState = STATE_DEST_SELECT;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("From: "));
    char stationName[30];
    getStationName(currentLine, currentStation, stationName);
    lcd.print(stationName);
    lcd.setCursor(0, 1);
    lcd.print(F("Select dest."));
  } else if (key == '*') {
    // Go back
    currentState = STATE_LINE_SELECT;
    displayLineMenu();
  } else if (key == '#') {
    // Enter custom index for stations beyond 9
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Enter stn #:"));
    lcd.setCursor(0, 1);
    int selection = readNumber(2);  // Read up to 2 digits

    if (selection > 0 && selection <= stationCount) {
      currentStation = selection - 1;  // Convert to 0-based
      saveCurrentStation();            // Save to EEPROM
      sendStationUpdate();             // Update Java app
      currentState = STATE_DEST_SELECT;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("From: "));
      char stationName[30];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Select dest."));
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid station"));
      delay(2000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Choose station:"));
      displayStations(currentLine);
    }
  }
}

// Process destination selection
void processDestSelect(char key) {
  if (key >= '1' && key <= '9') {
    int selection = key - '0' - 1;  // Convert key to 0-based index

    if (selection < stationCount && selection != currentStation) {
      destinationStation = selection;
      destinationSelected = true;  // Set flag when valid destination is selected
      int tripDistance = getDistanceFromMatrix(currentLine, currentStation, destinationStation);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[30];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Dist: "));
      lcd.print(tripDistance);
      lcd.print(F("km"));

      int fare = getFareFromMatrix(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[60];
      char originName[30], destName[30];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP, Dist: %d km", 
              currentLine, originName, destName, fare, tripDistance);
      sendMessage(message);
      
      // Wait briefly to show distance
      delay(2000);
      
      // Prompt user to approach gate
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate"));
    }
  } else if (key == '0' && stationCount > 9) {
    // Special case for index 9
    destinationStation = 9;

    if (destinationStation != currentStation) {
      destinationSelected = true;  // Set flag when valid destination is selected
      int tripDistance = getDistanceFromMatrix(currentLine, currentStation, destinationStation);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[30];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Dist: "));
      lcd.print(tripDistance);
      lcd.print(F("km"));

      int fare = getFareFromMatrix(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[60];
      char originName[30], destName[30];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP, Dist: %d km", 
              currentLine, originName, destName, fare, tripDistance);
      sendMessage(message);
      
      // Wait briefly to show distance
      delay(2000);
      
      // Prompt user to approach gate
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate"));
    }
  } else if (key == '*') {
    // Go back
    destinationSelected = false;  // Reset destination selection flag
    currentState = STATE_STATION_SELECT;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Choose station:"));
    displayStations(currentLine);
  } else if (key == '#') {
    // Enter custom index for stations beyond 9
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Enter dest #:"));
    lcd.setCursor(0, 1);
    int selection = readNumber(2);  // Read up to 2 digits

    if (selection > 0 && selection <= stationCount && (selection - 1) != currentStation) {
      destinationStation = selection - 1;  // Convert to 0-based
      destinationSelected = true;  // Set flag when valid destination is selected
      int tripDistance = getDistanceFromMatrix(currentLine, currentStation, destinationStation);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[30];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Dist: "));
      lcd.print(tripDistance);
      lcd.print(F("km"));

      int fare = getFareFromMatrix(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[60];
      char originName[30], destName[30];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP, Dist: %d km", 
              currentLine, originName, destName, fare, tripDistance);
      sendMessage(message);
      
      // Wait briefly to show distance
      delay(2000);
      
      // Prompt user to approach gate
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate"));
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid station"));
      delay(2000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Select dest."));
      lcd.setCursor(0, 1);
      lcd.print(F("Current: "));
      char stationName[30];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
    }
  }
}

// Process transfer selection
// Update the processTransferSelect function to improve information display

void processTransferSelect(char key) {
  switch (key) {
    case '1':  // Transfer to other line
      transferToOtherLine();
      break;
        case '2':  // Continue on current line
      // Reset destination selection for continuing journey
      destinationSelected = false;
      destinationStation = 255;
      
      currentState = STATE_DEST_SELECT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Continue on "));

      if (currentLine == 1) lcd.print(F("LRT1"));
      else if (currentLine == 2) lcd.print(F("LRT2"));

      char stationName[30];
      getStationName(currentLine, currentStation, stationName);
      
      lcd.setCursor(0, 1);
      lcd.print(F("At: "));
      lcd.print(stationName);
      
      delay(2000);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Current: "));
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Select dest."));
      break;
      
    case '*':  // Go back to line selection
      currentState = STATE_LINE_SELECT;
      displayLineMenu();
      break;
  }
}

// Process settings menu
void processSettingsMenu(char key) {
  switch (key) {
    case '1':  // Reset balances
      currentState = STATE_RESET_CONFIRM;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Enter password:"));
      lcd.setCursor(0, 1);
      break;

    case '2':  // View system info
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Metro Fare v2.1"));
      lcd.setCursor(0, 1);
      lcd.print(F("Total: "));
      lcd.print(totalDistance);
      lcd.print(F("km"));
      delay(3000);
      displaySettingsMenu();
      break;
      
    case '3':  // Reset distance
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Enter password:"));
      lcd.setCursor(0, 1);
      char* enteredPassword = readPassword(4);
      
      // Check password
      if (authenticateUser(enteredPassword)) {
        resetTotalDistance();
        sendMessage("Total distance reset successfully");
        sendDistanceUpdate();

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Distance reset"));
        lcd.setCursor(0, 1);
        lcd.print(F("successfully"));
        delay(2000);
      } else {
        sendMessage("Reset attempt failed - wrong password");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Invalid password"));
        delay(2000);
      }
      displaySettingsMenu();
      break;

    case '*':  // Go back
      currentState = STATE_MAIN_MENU;
      displayMainMenu();
      break;
  }
}

// Process reset confirmation
void processResetConfirm(char key) {
  if (key == '#') {
    // Read password
    char* enteredPassword = readPassword(4);

    // Check password
    if (authenticateUser(enteredPassword)) {
      resetBalances();
      sendMessage("Balances reset successfully");
      sendBalanceUpdate();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Balances reset"));
      lcd.setCursor(0, 1);
      lcd.print(F("successfully"));
      delay(2000);
    } else {
      sendMessage("Reset attempt failed - wrong password");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid password"));
      delay(2000);
    }

    // Return to settings menu
    currentState = STATE_SETTINGS;
    displaySettingsMenu();
  } else if (key == '*') {
    // Cancel and go back
    currentState = STATE_SETTINGS;
    displaySettingsMenu();
  }
}

// Reset all balances
void resetBalances() {
  beepBalance = 100;  // Default starting value
  singleJourneyBalance = 0;
  saveBalances();
}

// Reset total distance
void resetTotalDistance() {
  totalDistance = 0;
  saveTotalDistance();
}

// Read password with asterisk masking
char* readPassword(byte maxDigits) {
  static char password[10];
  byte inputIndex = 0;

  lcd.setCursor(0, 1);

  // Reset password buffer
  memset(password, 0, sizeof(password));

  unsigned long passwordStartTime = millis();
  boolean timeoutOccurred = false;

  while (!timeoutOccurred) {
    char key = keypad.getKey();

    if (key) {
      // Provide feedback
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);

      if (key >= '0' && key <= '9' && inputIndex < maxDigits) {
        password[inputIndex++] = key;
        password[inputIndex] = '\0';  // Null terminate

        // Display asterisks for security
        lcd.setCursor(0, 1);
        for (byte i = 0; i < inputIndex; i++) {
          lcd.print("*");
        }
        lcd.print("    ");      // Clear any old digits
      } else if (key == '#') {  // Confirm entry
        break;
      } else if (key == '*') {  // Backspace
        if (inputIndex > 0) {
          password[--inputIndex] = '\0';

          // Display asterisks
          lcd.setCursor(0, 1);
          for (byte i = 0; i < inputIndex; i++) {
            lcd.print("*");
          }
          lcd.print("    ");  // Clear deleted digit
        }
      }
    }
    
    // Also check for serial commands
    processSerialData();
    
    // Check for timeout
    if ((millis() - passwordStartTime) > 30000) { // 30 seconds timeout
      timeoutOccurred = true;
    }
    
    delay(10);  // Small delay to prevent bouncing
  }

  // If timeout occurred, return empty string
  if (timeoutOccurred) {
    password[0] = '\0';
  }

  return password;
}

// Authenticate user with password
bool authenticateUser(const char* enteredPassword) {
  return strcmp(enteredPassword, settingsPassword) == 0;
}

// Transfer to another line based on the current transfer station
// Transfer to another line based on the current transfer station
// Update the transferToOtherLine() function to improve display information:

void transferToOtherLine() {
  byte prevLine = currentLine;
  byte prevStation = currentStation;
  char prevStationName[30];
  getStationName(prevLine, prevStation, prevStationName);

  // Reset destination selection for the new line
  destinationSelected = false;
  destinationStation = 255;

  // LRT1 Doroteo Jose -> LRT2 Recto
  if (currentLine == 1 && currentStation == 9) {  // Doroteo Jose
    currentLine = 2;
    currentStation = 0;  // Recto station in LRT2
    stationCount = 13;
  }
  // LRT2 Recto -> LRT1 Doroteo Jose
  else if (currentLine == 2 && currentStation == 0) {  // Recto
    currentLine = 1;
    currentStation = 9;  // Doroteo Jose station in LRT1
    stationCount = 20;
  }

  // Save the new station to EEPROM
  saveCurrentStation();
  
  // Send updated station to Java app
  sendStationUpdate();

  // Get current station name
  char newStationName[30];
  getStationName(currentLine, currentStation, newStationName);

  // Send transfer message
  char message[60];
  sprintf(message, "Transfer: Line %d (%s) to Line %d (%s)", 
          prevLine, prevStationName, currentLine, newStationName);
  sendMessage(message);

  // Display transfer information with clearer formatting
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Transferred to:"));
  lcd.setCursor(0, 1);
  lcd.print(F("Line "));
  lcd.print(currentLine);
  lcd.print(F(": "));
  lcd.print(newStationName);
  delay(2000);

  // Now show available stations on this line
  lcd.clear();
  lcd.setCursor(0, 0);
  if (currentLine == 1) {
    lcd.print(F("LRT1 line: 20 stn"));
  } else {
    lcd.print(F("LRT2 line: 13 stn"));
  }
  lcd.setCursor(0, 1);
  lcd.print(F("Current: "));
  lcd.print(newStationName);
  delay(2000);

  // Show first few stations on this line for context
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Stations:"));
  lcd.setCursor(0, 1);
  
  // Show first 3 stations on this line
  byte maxToShow = min(3, stationCount);
  for (byte i = 0; i < maxToShow; i++) {
    char stnName[30];
    getStationName(currentLine, i, stnName);
    
    // Only show first 5 chars of each station name to fit on LCD
    char shortStnName[6];
    strncpy(shortStnName, stnName, 5);
    shortStnName[5] = '\0';
    
    lcd.print(shortStnName);
    if (i < maxToShow - 1) {
      lcd.print(",");
    }
  }
  lcd.print(F("..."));
  delay(2000);

  // Display stations for the new line in Java app
  displayStations(currentLine);

  // Transition to destination selection
  currentState = STATE_DEST_SELECT;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Line "));
  lcd.print(currentLine);
  lcd.print(F(": "));
  lcd.print(newStationName);
  lcd.setCursor(0, 1);
  lcd.print(F("Select dest."));
}

// Check if station is a transfer point
// Check if station is a transfer point
bool isTransferStation(byte line, byte station) {
  // Get station name for logging
  char stationName[30];
  getStationName(line, station, stationName);
  
  // Check if this is a transfer station
  bool isTransfer = false;
  
  if (line == 1) {
    // Check for Doroteo Jose (index 9 in LRT1) -> transfer to LRT2 Recto
    if (station == 9) {
      isTransfer = true;
      sendMessage("Transfer point detected: Doroteo Jose (LRT1) to Recto (LRT2)");
    }
  } else if (line == 2) {
    // Check for Recto (index 0 in LRT2) -> transfer to LRT1 Doroteo Jose
    if (station == 0) {
      isTransfer = true;
      sendMessage("Transfer point detected: Recto (LRT2) to Doroteo Jose (LRT1)");
    }
  }
  
  // Log the check result
  if (!isTransfer) {
    char debugMsg[50];
    sprintf(debugMsg, "Station %s (Line %d) is not a transfer point", stationName, line);
    sendMessage(debugMsg);
  }
  
  return isTransfer;
}

// Get fare directly from fare matrix
int getFareFromMatrix(byte line, byte origin, byte dest) {
  int fare = 0;
  
  if (line == 1) {
    // LRT1 fare matrix is 20x20
    if (origin < 20 && dest < 20) {
      int matrixIndex = origin * 20 + dest;
      fare = pgm_read_byte(&lrt1_fare_matrix[matrixIndex]);
    }
  } else if (line == 2) {
    // LRT2 fare matrix is 13x13
    if (origin < 13 && dest < 13) {
      int matrixIndex = origin * 13 + dest;
      fare = pgm_read_byte(&lrt2_fare_matrix[matrixIndex]);
    }
  }

  // Apply discounts
  if (userCardType == CARD_STUDENT) {
    fare = (int)(fare * 0.8 + 0.5);  // 20% discount for students with rounding
  } else if (userCardType == CARD_PWD || userCardType == CARD_SENIOR) {
    fare = (int)(fare * 0.7 + 0.5);  // 30% discount for PWD/Seniors with rounding
  }

  return fare;
}

// Get distance from distance matrix
int getDistanceFromMatrix(byte line, byte origin, byte dest) {
  int distance = 0;
  
  if (line == 1) {
    // LRT1 distance matrix is 20x20
    if (origin < 20 && dest < 20) {
      int matrixIndex = origin * 20 + dest;
      distance = pgm_read_byte(&lrt1_distance_matrix[matrixIndex]);
    }
  } else if (line == 2) {
    // LRT2 distance matrix is 13x13
    if (origin < 13 && dest < 13) {
      int matrixIndex = origin * 13 + dest;
      distance = pgm_read_byte(&lrt2_distance_matrix[matrixIndex]);
    }
  }

  return distance;
}

// Validate if balance is sufficient for fare
bool validateFare(int fare) {
  if (ticketType == TICKET_BEEP) {
    return beepBalance >= fare;
  } else {
    return singleJourneyBalance >= fare;
  }
}

// Load balance values from EEPROM
void loadBalances() {
  // Read beep balance (stored as 4 bytes)
  beepBalance = 0;
  for (byte i = 0; i < 4; i++) {
    beepBalance = (beepBalance << 8) + EEPROM.read(BEEP_BALANCE_ADDR + i);
  }

  // Read single journey balance (stored as 4 bytes)
  singleJourneyBalance = 0;
  for (byte i = 0; i < 4; i++) {
    singleJourneyBalance = (singleJourneyBalance << 8) + EEPROM.read(SINGLE_BALANCE_ADDR + i);
  }

  // Initial values if EEPROM is empty (first run)
  if (beepBalance > 10000 || beepBalance < 0) {
    beepBalance = 100;  // Default starting value
  }

  if (singleJourneyBalance > 10000 || singleJourneyBalance < 0) {
    singleJourneyBalance = 0;  // Default starting value
  }
}

// Save balance values to EEPROM (minimizing writes)
void saveBalances() {
  // Read current stored values first
  int storedBeep = 0;
  int storedSingle = 0;
  
  for (byte i = 0; i < 4; i++) {
    storedBeep = (storedBeep << 8) + EEPROM.read(BEEP_BALANCE_ADDR + i);
    storedSingle = (storedSingle << 8) + EEPROM.read(SINGLE_BALANCE_ADDR + i);
  }
  
  // Only write if values have changed
  if (storedBeep != beepBalance) {
    // Save beep balance (4 bytes)
    for (byte i = 0; i < 4; i++) {
      EEPROM.write(BEEP_BALANCE_ADDR + i, (beepBalance >> (8 * (3 - i))) & 0xFF);
    }
  }
  
  if (storedSingle != singleJourneyBalance) {
    // Save single journey balance (4 bytes)
    for (byte i = 0; i < 4; i++) {
      EEPROM.write(SINGLE_BALANCE_ADDR + i, (singleJourneyBalance >> (8 * (3 - i))) & 0xFF);
    }
  }
}

// Load total distance from EEPROM
void loadTotalDistance() {
  // Read total distance (stored as 4 bytes)
  totalDistance = 0;
  for (byte i = 0; i < 4; i++) {
    totalDistance = (totalDistance << 8) + EEPROM.read(TOTAL_DISTANCE_ADDR + i);
  }

  // Initial value if EEPROM is empty (first run)
  if (totalDistance > 10000 || totalDistance < 0) {
    totalDistance = 0;  // Default starting value
  }
}

// Save total distance to EEPROM (minimizing writes)
void saveTotalDistance() {
  // Read current stored value first
  int storedDistance = 0;
  
  for (byte i = 0; i < 4; i++) {
    storedDistance = (storedDistance << 8) + EEPROM.read(TOTAL_DISTANCE_ADDR + i);
  }
  
  // Only write if value has changed
  if (storedDistance != totalDistance) {
    // Save total distance (4 bytes)
    for (byte i = 0; i < 4; i++) {
      EEPROM.write(TOTAL_DISTANCE_ADDR + i, (totalDistance >> (8 * (3 - i))) & 0xFF);
    }
  }
}

// Save current station and line to EEPROM
void saveCurrentStation() {
  // Read current stored values first
  byte storedLine = EEPROM.read(CURRENT_LINE_ADDR);
  byte storedStation = EEPROM.read(CURRENT_STATION_ADDR);
  
  // Only write if values have changed
  if (storedLine != currentLine) {
    EEPROM.write(CURRENT_LINE_ADDR, currentLine);
  }
  
  if (storedStation != currentStation) {
    EEPROM.write(CURRENT_STATION_ADDR, currentStation);
  }
}

// Load current station and line from EEPROM
void loadCurrentStation() {
  currentLine = EEPROM.read(CURRENT_LINE_ADDR);
  currentStation = EEPROM.read(CURRENT_STATION_ADDR);

  // Handle initial values
  if (currentLine > 2) {
    currentLine = 1;  // Default to LRT1
  }

  // Set station count based on line
  if (currentLine == 1) {
    stationCount = 20;
    if (currentStation >= stationCount) currentStation = 0;
  } else if (currentLine == 2) {
    stationCount = 13;
    if (currentStation >= stationCount) currentStation = 0;
  }
}

// Display main menu on LCD
void displayMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("B:"));
  lcd.print(beepBalance);
  lcd.print(F("P S:"));
  lcd.print(singleJourneyBalance);
  lcd.print(F("P"));
  lcd.setCursor(0, 1);
  lcd.print(F("1.Beep 2.Single"));
  currentState = STATE_MAIN_MENU;
}

// Display ticket type selection menu
void displayTicketTypeMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Ticket Type:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1.Single  2.Beep"));
}

// Display card type selection menu
void displayCardTypeMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Card Type:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1.Reg 2.Stu 3.PWD"));
}

// Display line selection menu
void displayLineMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Choose Line:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1.LRT1 2.LRT2"));
}

// Display settings menu
void displaySettingsMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Settings Menu:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1.Reset 2.Info 3.Dist"));
}

// Display transfer options menu
// Update the displayTransferOptions function for clearer information

void displayTransferOptions(byte line, byte station) {
  char stationName[30];
  getStationName(line, station, stationName);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("At: "));
  lcd.print(stationName);
  
  lcd.setCursor(0, 1);
  if (line == 1) {
    lcd.print(F("1.To LRT2  2.Stay"));
  } else {
    lcd.print(F("1.To LRT1  2.Stay"));
  }

  // Send additional information to Java app about available transfer
  char message[50];
  if (line == 1) {
    sprintf(message, "Transfer available: LRT1 to LRT2 at %s", stationName);
  } else {
    sprintf(message, "Transfer available: LRT2 to LRT1 at %s", stationName);
  }
  sendMessage(message);
}

// Display stations for the selected line
void displayStations(byte line) {
  // Only display the first few stations on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Enter station #"));
  lcd.setCursor(0, 1);
  lcd.print(F("or # for more"));  // Send list of stations to serial for Java app with clear header
  if (line == 1) {
    Serial.println(F("MSG:===== LRT1 STATIONS ====="));
    Serial.flush(); // Ensure header is sent
    delay(50);
    
    for (byte i = 0; i < 20; i++) {
      char stationName[30];  // Larger buffer
      getStationName(line, i, stationName);
      
      // Add debug check for empty names
      if (strlen(stationName) == 0) {
        strcpy(stationName, "ERROR_EMPTY");
      }
      
      Serial.print(F("MSG:"));
      Serial.print(i + 1);
      Serial.print(F(". "));
      Serial.println(stationName);
      Serial.flush(); // Ensure each line is sent
      
      // Small delay to prevent buffer overflow
      delay(20);
    }
    Serial.println(F("MSG:========================="));
    Serial.flush();
  } else if (line == 2) {
    Serial.println(F("MSG:===== LRT2 STATIONS ====="));
    Serial.flush(); // Ensure header is sent
    delay(50);
    
    for (byte i = 0; i < 13; i++) {
      char stationName[30];  // Larger buffer
      getStationName(line, i, stationName);
      
      // Add debug check for empty names
      if (strlen(stationName) == 0) {
        strcpy(stationName, "ERROR_EMPTY");
      }
      
      Serial.print(F("MSG:"));
      Serial.print(i + 1);
      Serial.print(F(". "));
      Serial.println(stationName);
      Serial.flush(); // Ensure each line is sent
      
      // Small delay to prevent buffer overflow
      delay(20);
    }
    Serial.println(F("MSG:========================="));
    Serial.flush();
  }
}

// Read a key from keypad with timeout
char readKey() {
  unsigned long startTime = millis();
  char key = 0;
  
  // Wait up to 50ms for a key press
  while ((millis() - startTime) < 50) {
    key = keypad.getKey();
    if (key) {
      // Make a beep sound for button press feedback
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    }
    delay(1); // Small delay to prevent hogging CPU
  }
  return key;
}

// Read a multi-digit number from the keypad with timeout
int readNumber(byte maxDigits) {
  char input[5] = "";
  byte inputIndex = 0;

  lcd.setCursor(0, 1);
  lcd.print(">");
  
  unsigned long inputStartTime = millis();
  boolean timeoutOccurred = false;

  while (!timeoutOccurred) {
    char key = keypad.getKey();

    if (key) {
      // Provide feedback
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
      
      // Reset timeout timer on input
      inputStartTime = millis();

      if (key >= '0' && key <= '9' && inputIndex < maxDigits) {
        input[inputIndex++] = key;
        input[inputIndex] = '\0';  // Null terminate

        // Display on LCD
        lcd.setCursor(1, 1);
        lcd.print(input);
        lcd.print("    ");      // Clear any old digits
      } else if (key == '#') {  // Confirm entry
        break;
      } else if (key == '*') {  // Backspace
        if (inputIndex > 0) {
          input[--inputIndex] = '\0';

          // Display on LCD
          lcd.setCursor(1, 1);
          lcd.print(input);
          lcd.print("    ");  // Clear deleted digit
        }
      }
    }
    
    // Also check for serial commands while waiting for keypad input
    processSerialData();
    
    // Check for timeout (30 seconds)
    if ((millis() - inputStartTime) > 30000) {
      timeoutOccurred = true;
    }
    
    delay(10);  // Small delay to prevent bouncing
  }

  // Convert string to integer
  if (inputIndex > 0) {
    return atoi(input);
  } else {
    return 0;  // Return 0 if no input
  }
}

// Open turnstile by rotating servo
void openTurnstile() {
  turnstileServo.write(SERVO_OPEN);
  if (!turnstileOpen) {  // Only send message if state is changing
    sendMessage("Turnstile opened");
    turnstileOpen = true;
  }
}

// Close turnstile by rotating servo
void closeTurnstile() {
  turnstileServo.write(SERVO_CLOSED);
  if (turnstileOpen) {  // Only send message if state is changing
    sendMessage("Turnstile closed");
    turnstileOpen = false;
  }
}

// Measure distance using ultrasonic sensor with debouncing
long measureDistance() {
  // Take multiple readings and average them for debouncing
  long totalDistance = 0;
  byte validReadings = 0;
  const byte numReadings = 3;
  
  for (byte i = 0; i < numReadings; i++) {
    // Clear trigger pin
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // Send 10μs pulse to trigger
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Measure duration of echo pulse with timeout for better performance
    long duration = pulseIn(ECHO_PIN, HIGH, 10000);

    // Check if timeout occurred
    if (duration > 0) {
      // Calculate distance in cm
      // Sound travels at 343m/s, or 0.0343cm/μs
      // Time is round-trip, so divide by 2
      long distance = duration * 0.0343 / 2;
      
      // Only include reasonable readings (0-200cm)
      if (distance > 0 && distance < 200) {
        totalDistance += distance;
        validReadings++;
      }
      
      // Small delay between readings
      delay(10);
    }
  }
  
  // Return average if we have valid readings, otherwise -1
  if (validReadings > 0) {
    return totalDistance / validReadings;
  } else {
    return -1;  // No valid readings
  }
}

// Sound buzzer in different patterns for different events
void soundBuzzer(byte pattern) {
  switch (pattern) {
    case 1:  // Success pattern - single long beep
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
      break;

    case 2:  // Error pattern - three short beeps
      for (byte i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
      break;

    case 3:  // Alert pattern - two medium beeps
      for (byte i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
      }
      break;
  }
}

// Send balance update to Java app
void sendBalanceUpdate() {
  Serial.print(F("BAL:"));
  Serial.print(beepBalance);
  Serial.print(F(","));
  Serial.println(singleJourneyBalance);
}

// Send current station update to Java app
void sendStationUpdate() {
  Serial.print(F("STN:"));
  Serial.print(currentLine);
  Serial.print(F(","));
  Serial.print(currentStation);
  Serial.print(F(","));
  
  char stationName[30];
  getStationName(currentLine, currentStation, stationName);
  Serial.println(stationName);
}

// Send distance update to Java app
void sendDistanceUpdate() {
  Serial.print(F("DIST:"));
  Serial.println(totalDistance);
}

// Send user info to Java app
void sendUserInfo() {
  Serial.print(F("USER:"));
  Serial.print(currentUser);
  Serial.print(F(","));
  Serial.println(currentDateTime);
}

// Send trip record to Java app
void sendTripRecord(byte line, byte origin, byte dest, int fare, int distance) {
  char originName[30], destName[30];
  getStationName(line, origin, originName);
  getStationName(line, dest, destName);
  
  Serial.print(F("TRIP:"));
  Serial.print(line);
  Serial.print(F(","));
  Serial.print(origin);
  Serial.print(F(","));
  Serial.print(dest);
  Serial.print(F(","));
  Serial.print(fare);
  Serial.print(F(","));
  Serial.print(distance);
  Serial.print(F(","));
  Serial.print(currentDateTime);
  Serial.print(F(","));
  Serial.print(ticketType == TICKET_BEEP ? "beep" : "single");
  Serial.print(F(","));
  
  // Convert card type byte to string
  switch (userCardType) {
    case CARD_STUDENT: Serial.println(F("Student")); break;
    case CARD_PWD: Serial.println(F("PWD")); break;
    case CARD_SENIOR: Serial.println(F("Senior")); break;
    default: Serial.println(F("Regular")); break;
  }
}

// Send topup record to Java app
void sendTopupRecord(byte cardType, int amount, int newBalance) {
  Serial.print(F("TOPUP:"));
  Serial.print(cardType == TICKET_BEEP ? "BEEP" : "SINGLE");
  Serial.print(F(","));
  Serial.print(amount);
  Serial.print(F(","));
  Serial.print(newBalance);
  Serial.print(F(","));
  Serial.println(currentDateTime);
}

// Send message to Java app
void sendMessage(const char* message) {
  Serial.print(F("MSG:"));
  Serial.println(message);
}

// Send error to Java app
void sendError(const char* errorCode) {
  Serial.print(F("ERR:"));
  Serial.println(errorCode);
}
