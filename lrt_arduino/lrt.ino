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
#define STATE_SETTINGS 9        // New state for settings menu
#define STATE_RESET_CONFIRM 10  // New state for reset confirmation
int currentState = STATE_MAIN_MENU;

// Settings password (default: 1234)
const char settingsPassword[] = "1234";

// Current user and date information
char currentUser[16] = "NIghtIngale340";
char currentDateTime[20] = "2025-05-16 15:13";

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
int totalDistance = 0;

// Current journey variables
byte currentLine = 1;
byte currentStation = 0;
byte stationCount = 0;
byte destinationStation = 0;

// EEPROM addresses
#define BEEP_BALANCE_ADDR 0
#define SINGLE_BALANCE_ADDR 4
#define CURRENT_LINE_ADDR 8
#define CURRENT_STATION_ADDR 9

// Screen refresh flag
bool needsRefresh = true;

// Timer variables for scrolling text
unsigned long lastScrollTime = 0;
const unsigned long scrollInterval = 500;  // Scroll every 500ms
int scrollPosition = 0;

// Serial communication variables
char inputBuffer[32];
byte bufferIndex = 0;
boolean commandComplete = false;
unsigned long lastStatusUpdate = 0;
const unsigned long statusInterval = 5000;  // Send status updates every 5 seconds

// LRT1 Station Names (only those needed for transfers and specific functionality)
const char lrt1_station_0[] PROGMEM = "Roosevelt";
const char lrt1_station_9[] PROGMEM = "Doroteo Jose";
const char lrt1_station_17[] PROGMEM = "EDSA";

const char* const lrt1_key_stations[] PROGMEM = {
  lrt1_station_0, lrt1_station_9, lrt1_station_17
};

// LRT2 Station Names (only those needed for transfers and specific functionality)
const char lrt2_station_0[] PROGMEM = "Recto";
const char lrt2_station_7[] PROGMEM = "Cubao";

const char* const lrt2_key_stations[] PROGMEM = {
  lrt2_station_0, lrt2_station_7
};

// Segment distances for LRT1 (in meters)
const int lrt1_distances[] PROGMEM = {
  1870, 2250, 1087, 954, 660, 927, 671, 618, 648, 685,
  725, 1214, 754, 794, 827, 1061, 730, 1010, 588
};

// Segment distances for LRT2 (in meters)
const int lrt2_distances[] PROGMEM = {
  1050, 1389, 1350, 1357, 928, 1075, 1164, 955, 438, 1970, 1000, 1000
};

// Base fare matrix for distance calculation (simplified)
const byte fare_base[5] PROGMEM = { 13, 15, 18, 20, 25 }; // Base fares for distances

// Function prototypes
void displayMainMenu();
void displayTicketTypeMenu();
void displayCardTypeMenu();
void displayLineMenu();
void displaySettingsMenu();
void displayTransferOptions(byte line, byte station);
void displayStations(byte line);
int getDistance(byte line, byte start, byte end);
int getFare(byte line, byte origin, byte dest);
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
void getStationName(byte line, byte station, char* buffer);
void displayStatusBar();
void resetBalances();
char* readPassword(byte maxDigits);
bool authenticateUser(const char* enteredPassword);
void scrollStationName(byte line, byte station);
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
void displayUserInfo();
void processSerialData();
void handleSerialCommand(char* command);
void sendBalanceUpdate();
void sendStationUpdate();
void sendUserInfo();
void sendTripRecord(byte line, byte origin, byte dest, int fare, int distance);
void sendTopupRecord(byte cardType, int amount, int newBalance);
void sendMessage(const char* message);
void sendError(const char* errorCode);

void setup() {
  // Initialize Serial communication with the Java application
  Serial.begin(9600);
  
  // Initialize hardware
  lcd.init();
  lcd.backlight();

  turnstileServo.attach(SERVO_PIN);
  closeTurnstile();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Load saved balances from EEPROM
  loadBalances();
  loadCurrentStation();

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
  sendUserInfo();

  displayMainMenu();
}

void loop() {
  // Process Serial input
  processSerialData();
  
  // Process keypad input
  char key = readKey();

  // Display status bar (balance and current station) in relevant states
  if ((millis() - lastScrollTime) >= scrollInterval) {
    if (currentState == STATE_MAIN_MENU || currentState == STATE_LINE_SELECT || 
        currentState == STATE_STATION_SELECT || currentState == STATE_DEST_SELECT) {
      displayStatusBar();
    }
    lastScrollTime = millis();
  }

  // Periodic status updates to Java app
  if ((millis() - lastStatusUpdate) >= statusInterval) {
    sendBalanceUpdate();
    sendStationUpdate();
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
  }

  // Check ultrasonic sensor for turnstile simulation
  if (currentState == STATE_DEST_SELECT) {
    long distance = measureDistance();
    // If someone approaches within 10cm
    if (distance < 10 && distance > 0) {
      Serial.println(F("MSG:Passenger detected!"));
      // Process turnstile entry logic
      processPassengerEntry();
    }
  }
}

// Process serial data coming from Java app
void processSerialData() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    // Process complete commands when newline is received
    if (inChar == '\n') {
      inputBuffer[bufferIndex] = '\0'; // Terminate the string
      handleSerialCommand(inputBuffer);
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
          
          if (strcmp(cardTypeStr, "BEEP") == 0) {
            beepBalance += amount;
            saveBalances();
            sendMessage("Beep card topped up");
            sendTopupRecord(TICKET_BEEP, amount, beepBalance);
          } 
          else if (strcmp(cardTypeStr, "SINGLE") == 0) {
            singleJourneyBalance += amount;
            saveBalances();
            sendMessage("Single journey card topped up");
            sendTopupRecord(TICKET_SINGLE, amount, singleJourneyBalance);
          } 
          else {
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
    else {
      sendError("004"); // Invalid parameters
    }
  }
}

// Display status bar (balance and current station)
void displayStatusBar() {
  static byte statusBarState = 0;

  // Don't update if we're in a state where we don't want to show the status bar
  if (currentState == STATE_TOPUP_BEEP || currentState == STATE_TOPUP_SINGLE || 
      currentState == STATE_TICKET_TYPE || currentState == STATE_CARD_TYPE || 
      currentState == STATE_RESET_CONFIRM) {
    return;
  }

  lcd.setCursor(0, 0);

  // Alternating display between balance and current station
  switch (statusBarState) {
    case 0:
      // Show balance info
      lcd.print(F("B:"));
      lcd.print(beepBalance);
      lcd.print(F("P S:"));
      lcd.print(singleJourneyBalance);
      lcd.print(F("P   "));
      break;

    case 1:
      // Show current station info
      if (currentLine > 0) {
        lcd.print(F("Line "));
        if (currentLine == 1) lcd.print(F("LRT1"));
        else if (currentLine == 2) lcd.print(F("LRT2"));

        lcd.print(F(" Stn:"));
        scrollStationName(currentLine, currentStation);
      } else {
        lcd.print(F("No active trip   "));
      }
      break;

    case 2:
      // Show user and time info
      lcd.print(currentUser);
      lcd.print(F("        "));
      break;
  }

  // Toggle for next display (now cycling through 3 states)
  statusBarState = (statusBarState + 1) % 3;
}

// Display user information
void displayUserInfo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("User: "));
  lcd.print(currentUser);
  lcd.setCursor(0, 1);
  lcd.print(currentDateTime);
  delay(2000);
}

// Scroll station name if too long
void scrollStationName(byte line, byte station) {
  char stationName[15];
  getStationName(line, station, stationName);

  int nameLength = strlen(stationName);

  // If name is shorter than display space, just print it
  if (nameLength <= 5) {
    lcd.print(stationName);
    for (int i = 0; i < (5 - nameLength); i++) {
      lcd.print(" ");
    }
  }
  // Otherwise scroll it
  else {
    // Calculate how many positions to use for scrolling
    int scrollMax = nameLength - 5;

    // Get current scroll position
    if (scrollPosition > scrollMax) {
      scrollPosition = 0;
    }

    // Print substring from current scroll position
    for (int i = 0; i < 5; i++) {
      if ((scrollPosition + i) < nameLength) {
        lcd.print(stationName[scrollPosition + i]);
      } else {
        lcd.print(" ");
      }
    }

    // Update scroll position
    scrollPosition++;
  }
}

// Get station name dynamically to save memory
void getStationName(byte line, byte station, char* buffer) {
  // For key transfer stations, use stored names
  if (line == 1) {
    if (station == 0) {
      strcpy_P(buffer, (char*)pgm_read_word(&lrt1_key_stations[0])); // Roosevelt
    } 
    else if (station == 9) {
      strcpy_P(buffer, (char*)pgm_read_word(&lrt1_key_stations[1])); // Doroteo Jose
    }
    else if (station == 17) {
      strcpy_P(buffer, (char*)pgm_read_word(&lrt1_key_stations[2])); // EDSA
    }
    else {
      // Generate name for other stations
      sprintf(buffer, "LRT1-S%d", station);
    }
  } 
  else if (line == 2) {
    if (station == 0) {
      strcpy_P(buffer, (char*)pgm_read_word(&lrt2_key_stations[0])); // Recto
    }
    else if (station == 7) {
      strcpy_P(buffer, (char*)pgm_read_word(&lrt2_key_stations[1])); // Cubao
    }
    else {
      // Generate name for other stations
      sprintf(buffer, "LRT2-S%d", station);
    }
  } 
  else {
    strcpy(buffer, "Unknown");
  }
}

// Process passenger entry at the turnstile
void processPassengerEntry() {
  int fare = getFare(currentLine, currentStation, destinationStation);

  if (validateFare(fare)) {
    // Successful entry
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Fare: "));
    lcd.print(fare);
    lcd.print(F(" PHP"));
    lcd.setCursor(0, 1);
    lcd.print(F("Access granted"));

    digitalWrite(GREEN_LED_PIN, HIGH);
    openTurnstile();
    soundBuzzer(1);  // Success pattern

    // Calculate journey details
    int distance = getDistance(currentLine, currentStation, destinationStation);
    totalDistance += distance;
    totalFare += fare;

    // Send trip record to Java app
    sendTripRecord(currentLine, currentStation, destinationStation, fare, distance);

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

    // Wait for passenger to pass through
    delay(3000);

    // Check if the passenger has passed through
    long dist = measureDistance();
    if (dist > 30) {  // No one in front of the sensor
      closeTurnstile();
      digitalWrite(GREEN_LED_PIN, LOW);

      // Send updated station information
      sendStationUpdate();

      // Check if current station is a transfer station
      if (isTransferStation(currentLine, currentStation)) {
        currentState = STATE_TRANSFER;
        displayTransferOptions(currentLine, currentStation);
      } else {
        currentState = STATE_DEST_SELECT;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Current: "));
        char stationName[15];
        getStationName(currentLine, currentStation, stationName);
        lcd.print(stationName);
        lcd.setCursor(0, 1);
        lcd.print(F("Select dest.    "));
      }
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

    // Return to destination selection
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Select dest.    "));
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
      lcd.print(F("Thank you!"));
      lcd.setCursor(0, 1);
      lcd.print(F("Goodbye!"));
      delay(2000);
      displayMainMenu();
      break;

    case 'A':  // Display user info
      displayUserInfo();
      delay(3000);
      displayMainMenu();
      break;

    case 'D':  // New: Access settings (hidden option)
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
      lcd.print(F("Dist:"));
      lcd.print(totalDistance);
      lcd.print(F("m"));
      
      // Send summary message to Java app
      char message[40];
      sprintf(message, "Journey complete: %dP, %dm", totalFare, totalDistance);
      sendMessage(message);
      
      delay(3000);

      // Reset journey values
      totalFare = 0;
      totalDistance = 0;

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
      char stationName[15];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Select dest.    "));
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
    char stationName[15];
    getStationName(currentLine, currentStation, stationName);
    lcd.print(stationName);
    lcd.setCursor(0, 1);
    lcd.print(F("Select dest.    "));
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
      char stationName[15];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Select dest.    "));
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

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[15];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate   "));

      int fare = getFare(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[50];
      char originName[15], destName[15];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP", 
              currentLine, originName, destName, fare);
      sendMessage(message);
    }
  } else if (key == '0' && stationCount > 9) {
    // Special case for index 9
    destinationStation = 9;

    if (destinationStation != currentStation) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[15];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate   "));

      int fare = getFare(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[50];
      char originName[15], destName[15];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP", 
              currentLine, originName, destName, fare);
      sendMessage(message);
    }
  } else if (key == '*') {
    // Go back
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

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("To: "));
      char stationName[15];
      getStationName(currentLine, destinationStation, stationName);
      lcd.print(stationName);
      lcd.setCursor(0, 1);
      lcd.print(F("Approach gate   "));

      int fare = getFare(currentLine, currentStation, destinationStation);
      // Send message to Java app about selected journey
      char message[50];
      char originName[15], destName[15];
      getStationName(currentLine, currentStation, originName);
      getStationName(currentLine, destinationStation, destName);
      sprintf(message, "Journey: Line %d from %s to %s, Fare: %d PHP", 
              currentLine, originName, destName, fare);
      sendMessage(message);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Invalid station"));
      delay(2000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Select dest.    "));
      lcd.setCursor(0, 1);
      lcd.print(F("Current: "));
      char stationName[15];
      getStationName(currentLine, currentStation, stationName);
      lcd.print(stationName);
    }
  }
}

// Process transfer selection
void processTransferSelect(char key) {
  switch (key) {
    case '1':  // Transfer to other line
      transferToOtherLine();
      break;
    case '2':  // Continue on current line
      currentState = STATE_DEST_SELECT;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Continue on "));

      if (currentLine == 1) lcd.print(F("LRT1"));
      else if (currentLine == 2) lcd.print(F("LRT2"));

      lcd.setCursor(0, 1);
      lcd.print(F("Select dest.    "));
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
      lcd.print(F("Metro Fare v2.0"));
      lcd.setCursor(0, 1);
      lcd.print(currentDateTime);
      delay(3000);
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

// Read password with asterisk masking
char* readPassword(byte maxDigits) {
  static char password[10];
  byte inputIndex = 0;

  lcd.setCursor(0, 1);

  while (true) {
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
    
    delay(10);  // Small delay to prevent bouncing
  }

  return password;
}

// Authenticate user with password
bool authenticateUser(const char* enteredPassword) {
  return strcmp(enteredPassword, settingsPassword) == 0;
}

// Transfer to another line based on the current transfer station
void transferToOtherLine() {
  byte prevLine = currentLine;
  char stationName[15];
  getStationName(currentLine, currentStation, stationName);

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

  // Send transfer message
  char message[40];
  char newStationName[15];
  getStationName(currentLine, currentStation, newStationName);
  sprintf(message, "Transfer: Line %d to Line %d at %s", 
          prevLine, currentLine, newStationName);
  sendMessage(message);

  lcd.clear();
  lcd.setCursor(0, 0);
  if (prevLine == 1) lcd.print(F("LRT1>"));
  else if (prevLine == 2) lcd.print(F("LRT2>"));

  if (currentLine == 1) lcd.print(F("LRT1"));
  else if (currentLine == 2) lcd.print(F("LRT2"));

  lcd.setCursor(0, 1);
  lcd.print(F("Now at: "));
  lcd.print(newStationName);

  delay(2000);

  currentState = STATE_DEST_SELECT;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Select dest."));
  lcd.setCursor(0, 1);
  lcd.print(F("on new line"));
}

// Check if station is a transfer point
bool isTransferStation(byte line, byte station) {
  if (line == 1) {
    // Check for Doroteo Jose (transfer to LRT2 Recto)
    return (station == 9);
  } else if (line == 2) {
    // Check for Recto (transfer to LRT1 Doroteo Jose)
    return (station == 0);
  }
  return false;
}

// Get fare between two stations - simplified to save memory
int getFare(byte line, byte origin, byte dest) {
  // Calculate distance between stations
  int distance = getDistance(line, origin, dest);
  int fare;
  
  // Simplified fare calculation based on distance
  if (distance < 1000) {
    fare = pgm_read_byte(&fare_base[0]); // Base fare for short trips
  } else if (distance < 3000) {
    fare = pgm_read_byte(&fare_base[1]);
  } else if (distance < 6000) {
    fare = pgm_read_byte(&fare_base[2]);
  } else if (distance < 10000) {
    fare = pgm_read_byte(&fare_base[3]);
  } else {
    fare = pgm_read_byte(&fare_base[4]);
  }

  // Apply discounts
  if (userCardType == CARD_STUDENT) {
    fare = (int)(fare * 0.8 + 0.5);  // 20% discount for students with rounding
  } else if (userCardType == CARD_PWD || userCardType == CARD_SENIOR) {
    fare = (int)(fare * 0.7 + 0.5);  // 30% discount for PWD/Seniors with rounding
  }

  return fare;
}

// Calculate distance between stations
int getDistance(byte line, byte start, byte end) {
  int distance = 0;
  byte startIdx = min(start, end);
  byte endIdx = max(start, end);

  if (line == 1) {
    for (byte i = startIdx; i < endIdx; i++) {
      distance += pgm_read_word(&lrt1_distances[i]);
    }
  } else if (line == 2) {
    for (byte i = startIdx; i < endIdx; i++) {
      distance += pgm_read_word(&lrt2_distances[i]);
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

// Save balance values to EEPROM
void saveBalances() {
  // Save beep balance (4 bytes)
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(BEEP_BALANCE_ADDR + i, (beepBalance >> (8 * (3 - i))) & 0xFF);
  }

  // Save single journey balance (4 bytes)
  for (byte i = 0; i < 4; i++) {
    EEPROM.write(SINGLE_BALANCE_ADDR + i, (singleJourneyBalance >> (8 * (3 - i))) & 0xFF);
  }
}

// Save current station and line to EEPROM
void saveCurrentStation() {
  EEPROM.write(CURRENT_LINE_ADDR, currentLine);
  EEPROM.write(CURRENT_STATION_ADDR, currentStation);
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
  lcd.print(F("1.Beep 2.Single"));
  lcd.setCursor(0, 1);
  lcd.print(F("3.Start 4.Exit"));
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
  lcd.print(F("1.Reset 2.Info"));
}

// Display transfer options menu
void displayTransferOptions(byte line, byte station) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Transfer Point:"));

  lcd.setCursor(0, 1);
  lcd.print(F("1.Trans 2.Stay"));
}

// Display stations for the selected line
void displayStations(byte line) {
  // Only display the first few stations on LCD
  // User can scroll or enter station number directly
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Enter station #"));
  lcd.setCursor(0, 1);
  lcd.print(F("or # for more"));

  // Send list of stations to serial for Java app
  if (line == 1) {
    Serial.println(F("MSG:LRT1 Stations selected"));
    for (byte i = 0; i < 20; i++) {
      char stationName[15];
      getStationName(line, i, stationName);
      Serial.print(F("MSG:"));
      Serial.print(i + 1);
      Serial.print(F(". "));
      Serial.println(stationName);
    }
  } else if (line == 2) {
    Serial.println(F("MSG:LRT2 Stations selected"));
    for (byte i = 0; i < 13; i++) {
      char stationName[15];
      getStationName(line, i, stationName);
      Serial.print(F("MSG:"));
      Serial.print(i + 1);
      Serial.print(F(". "));
      Serial.println(stationName);
    }
  }
}

// Read a key from keypad with timeout
char readKey() {
  char key = keypad.getKey();
  if (key) {
    // Make a beep sound for button press feedback
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
  }
  return key;
}

// Read a multi-digit number from the keypad
int readNumber(byte maxDigits) {
  char input[5] = "";
  byte inputIndex = 0;

  lcd.setCursor(0, 1);
  lcd.print(">");

  while (true) {
    char key = keypad.getKey();

    if (key) {
      // Provide feedback
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);

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
  sendMessage("Turnstile opened");
}

// Close turnstile by rotating servo
void closeTurnstile() {
  turnstileServo.write(SERVO_CLOSED);
  sendMessage("Turnstile closed");
}

// Measure distance using ultrasonic sensor
long measureDistance() {
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
  if (duration == 0) {
    return -1;  // No echo received
  }

  // Calculate distance in cm
  // Sound travels at 343m/s, or 0.0343cm/μs
  // Time is round-trip, so divide by 2
  return duration * 0.0343 / 2;
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
  char stationName[15];
  getStationName(currentLine, currentStation, stationName);
  
  Serial.print(F("STN:"));
  Serial.print(currentLine);
  Serial.print(F(","));
  Serial.print(currentStation);
  Serial.print(F(","));
  Serial.println(stationName);
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
  char originName[15], destName[15];
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