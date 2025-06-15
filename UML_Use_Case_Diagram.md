# Metro Fare System - UML Use Case Diagram

## Use Case Diagram

This Use Case Diagram represents all the functional requirements and interactions between actors and the Metro Fare System.

```
                    Metro Fare System Use Case Diagram

    ┌─────────────────────────────────────────────────────────────────────────────────┐
    │                          Metro Fare System                                      │
    │                                                                                 │
    │  ┌─────────────────────┐      ┌─────────────────────┐      ┌──────────────────┐ │
    │  │   Card Management   │      │  Journey Planning   │      │  System Monitoring│ │
    │  │                     │      │                     │      │                  │ │
    │  │ ○ Top-up Beep Card  │      │ ○ Select Line       │      │ ○ View Balances  │ │
    │  │                     │      │                     │      │                  │ │
    │  │ ○ Top-up Single     │      │ ○ Select Origin     │      │ ○ View Current   │ │
    │  │   Journey Card      │      │   Station           │      │   Station        │ │
    │  │                     │      │                     │      │                  │ │
    │  │ ○ Check Balance     │      │ ○ Select Destination│      │ ○ View Trip      │ │
    │  │                     │      │   Station           │      │   History        │ │
    │  └─────────────────────┘      │                     │      │                  │ │
    │                               │ ○ Calculate Fare    │      │ ○ Monitor Real-  │ │
    │  ┌─────────────────────┐      │                     │      │   time Status    │ │
    │  │  Payment & Travel   │      │ ○ Process Payment   │      └──────────────────┘ │
    │  │                     │      │                     │                           │
    │  │ ○ Make Payment      │      │ ○ Update Location   │      ┌──────────────────┐ │
    │  │                     │      │                     │      │ Data Management  │ │
    │  │ ○ Pass Through      │      └─────────────────────┘      │                  │ │
    │  │   Turnstile         │                                   │ ○ Export Trip    │ │
    │  │                     │      ┌─────────────────────┐      │   History        │ │
    │  │ ○ Validate Fare     │      │   Transfer System   │      │                  │ │
    │  │                     │      │                     │      │ ○ Clear History  │ │
    │  │ ○ Complete Trip     │      │ ○ Detect Transfer   │      │                  │ │
    │  │                     │      │   Station           │      │ ○ Auto-save      │ │
    │  └─────────────────────┘      │                     │      │   Data           │ │
    │                               │ ○ Switch Lines      │      │                  │ │
    │  ┌─────────────────────┐      │                     │      │ ○ Load Saved     │ │
    │  │ System Settings     │      │ ○ Continue Journey  │      │   State          │ │
    │  │                     │      │                     │      └──────────────────┘ │
    │  │ ○ Reset Balances    │      └─────────────────────┘                           │
    │  │                     │                                                        │
    │  │ ○ Reset Distance    │      ┌─────────────────────┐      ┌──────────────────┐ │
    │  │                     │      │  Hardware Control   │      │ Communication    │ │
    │  │ ○ View System Info  │      │                     │      │                  │ │
    │  │                     │      │ ○ Control Turnstile │      │ ○ Send Commands  │ │
    │  │ ○ Authenticate      │      │                     │      │   to Arduino     │ │
    │  │   Admin             │      │ ○ Sound Buzzer      │      │                  │ │
    │  └─────────────────────┘      │                     │      │ ○ Receive Data   │ │
    │                               │ ○ Control LEDs      │      │   from Arduino   │ │
    │                               │                     │      │                  │ │
    │                               │ ○ Display Messages  │      │ ○ Monitor        │ │
    │                               │   on LCD            │      │   Connection     │ │
    │                               │                     │      │                  │ │
    │                               │ ○ Measure Distance  │      │ ○ Handle         │ │
    │                               │   (Ultrasonic)      │      │   Disconnection  │ │
    │                               └─────────────────────┘      └──────────────────┘ │
    └─────────────────────────────────────────────────────────────────────────────────┘

```

## Actors and Their Relationships

```
         Passenger                    System Administrator              Arduino Hardware
            │                              │                                │
            │                              │                                │
    ┌───────┼───────┐              ┌───────┼───────┐                ┌───────┼───────┐
    │               │              │               │                │               │
    ▼               ▼              ▼               ▼                ▼               ▼
┌─────────┐   ┌─────────┐    ┌─────────┐   ┌─────────┐      ┌─────────┐   ┌─────────┐
│Card Mgmt│   │Journey  │    │Settings │   │Data Mgmt│      │Hardware │   │Comm.    │
│         │   │Planning │    │         │   │         │      │Control  │   │         │
└─────────┘   └─────────┘    └─────────┘   └─────────┘      └─────────┘   └─────────┘
```

## Detailed Use Case Descriptions

### **Actor: Passenger**

#### **UC1: Top-up Card**
- **Primary Actor**: Passenger
- **Goal**: Add money to Beep Card or Single Journey Card
- **Preconditions**: Card available, valid amount entered
- **Main Flow**:
  1. Select card type (Beep/Single Journey)
  2. Enter top-up amount
  3. Confirm transaction
  4. System updates balance
  5. Display new balance

#### **UC2: Plan Journey**
- **Primary Actor**: Passenger
- **Goal**: Plan and execute a metro journey
- **Preconditions**: Sufficient card balance
- **Main Flow**:
  1. Select ticket type (Single/Beep)
  2. Choose card type (Regular/Student/PWD/Senior)
  3. Select metro line (LRT1/LRT2)
  4. Choose origin station
  5. Select destination station
  6. System calculates fare and distance
  7. Display journey details

#### **UC3: Make Payment and Travel**
- **Primary Actor**: Passenger
- **Goal**: Pay fare and pass through turnstile
- **Preconditions**: Journey planned, sufficient balance
- **Main Flow**:
  1. Validate sufficient balance
  2. Deduct fare from selected card
  3. Open turnstile
  4. Passenger passes through
  5. Close turnstile
  6. Update current location
  7. Record trip in database

#### **UC4: Handle Transfer**
- **Primary Actor**: Passenger
- **Goal**: Transfer between metro lines
- **Preconditions**: At transfer station
- **Main Flow**:
  1. System detects transfer station
  2. Display transfer options
  3. Passenger selects continue or transfer
  4. If transfer: switch to other line
  5. If continue: proceed on same line

### **Actor: System Administrator**

#### **UC5: Manage System Settings**
- **Primary Actor**: System Administrator
- **Goal**: Configure and maintain system
- **Preconditions**: Admin authentication
- **Main Flow**:
  1. Enter admin password
  2. Access settings menu
  3. Choose operation (reset balances/distance)
  4. Confirm action
  5. System updates configuration

#### **UC6: Manage Data**
- **Primary Actor**: System Administrator
- **Goal**: Handle trip and top-up data
- **Preconditions**: System running, data available
- **Main Flow**:
  1. Access history dialog
  2. View trip summary and details
  3. Export data to file (optional)
  4. Delete selected trips (optional)
  5. Clear all data (optional)

#### **UC7: Monitor System**
- **Primary Actor**: System Administrator
- **Goal**: Monitor real-time system status
- **Preconditions**: System operational
- **Main Flow**:
  1. View current balances
  2. Monitor current station/line
  3. Check total distance traveled
  4. Review recent transactions
  5. Monitor Arduino connection status

### **Actor: Arduino Hardware**

#### **UC8: Control Hardware Components**
- **Primary Actor**: Arduino Hardware
- **Goal**: Manage physical system components
- **Preconditions**: Hardware initialized
- **Main Flow**:
  1. Control turnstile servo motor
  2. Manage LED indicators
  3. Sound buzzer for different events
  4. Display information on LCD
  5. Measure distance with ultrasonic sensor

#### **UC9: Communicate with Java Application**
- **Primary Actor**: Arduino Hardware
- **Goal**: Exchange data with desktop application
- **Preconditions**: Serial connection established
- **Main Flow**:
  1. Send balance updates
  2. Send station/location updates
  3. Send trip records
  4. Send top-up records
  5. Receive commands from Java app
  6. Handle connection monitoring

## Use Case Relationships

### **Include Relationships**:
- "Make Payment" **includes** "Validate Fare"
- "Plan Journey" **includes** "Calculate Fare"
- "Top-up Card" **includes** "Check Balance"
- "Manage Data" **includes** "Auto-save Data"

### **Extend Relationships**:
- "Handle Transfer" **extends** "Plan Journey"
- "Apply Discount" **extends** "Calculate Fare"
- "Handle Connection Error" **extends** "Communicate with Java Application"

### **Generalization**:
- "Top-up Beep Card" and "Top-up Single Journey Card" are specializations of "Top-up Card"
- "Reset Balances" and "Reset Distance" are specializations of "Manage System Settings"

## System Boundaries

The system includes:
- **Hardware Interface**: Arduino with sensors, LCD, keypad, servo
- **Desktop Application**: Java Swing GUI for monitoring and administration
- **Database Layer**: SQLite for persistent data storage
- **Communication Layer**: Serial communication between hardware and software

## Actor Roles Summary

| Actor | Primary Responsibilities |
|-------|-------------------------|
| **Passenger** | Card management, journey planning, payment, travel |
| **System Administrator** | System configuration, data management, monitoring |
| **Arduino Hardware** | Hardware control, sensor management, communication |

This use case diagram provides a comprehensive view of all functional requirements and interactions within the Metro Fare System, clearly defining the responsibilities of each actor and the system's capabilities.
