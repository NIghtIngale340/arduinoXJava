# Metro Fare System - UML Class Diagram

## Class Diagram Structure

This UML Class Diagram represents the complete architecture of the Metro Fare System, showing all Java classes, their attributes, methods, and relationships.

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                  FareSystemApp                                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ - serialVersionUID: long                                                           │
│ - serialPort: SerialPort                                                           │
│ - logArea: JTextArea                                                               │
│ - beepBalanceLabel: JLabel                                                         │
│ - singleBalanceLabel: JLabel                                                       │
│ - currentStationLabel: JLabel                                                      │
│ - topupAmountField: JTextField                                                     │
│ - cardTypeCombo: JComboBox<String>                                                 │
│ - executor: ScheduledExecutorService                                               │
│ - state: FareSystemState                                                           │
│ - databaseManager: DatabaseManager                                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ + FareSystemApp()                                                                  │
│ - createUI(): void                                                                 │
│ - initializeSerial(): void                                                         │
│ - readSerial(): void                                                               │
│ - handleBalanceUpdate(data: String): void                                          │
│ - handleStationUpdate(data: String): void                                          │
│ - handleDistanceUpdate(data: String): void                                         │
│ - handleTripRecord(data: String): void                                             │
│ - handleTopupRecord(data: String): void                                            │
│ - handleUserInfo(data: String): void                                               │
│ - handleMessage(message: String): void                                             │
│ - handleError(errorCode: String): void                                             │
│ - updateBalanceDisplay(): void                                                     │
│ - updateStationDisplay(): void                                                     │
│ - handleTopup(): void                                                              │
│ - handleReset(password: String): void                                              │
│ - sendCommand(command: String): void                                               │
│ - log(message: String): void                                                       │
│ + getFareSystemState(): FareSystemState                                            │
│ - showHistoryDialog(): void                                                        │
│ - createSummaryPanel(): JPanel                                                     │
│ - createSummaryCard(title: String, value: String, icon: String): JPanel           │
│ - createTripListPanel(): JPanel                                                    │
│ - createHistoryButtonPanel(dialog: JDialog): JPanel                               │
│ - exportHistoryToFile(parent: JDialog): void                                      │
│ - formatTripRecord(record: TripRecord): String                                     │
│ - getStationName(line: int, stationIndex: int): String                            │
│ - updateUI(): void                                                                 │
│ - startPeriodicAutoSave(): void                                                    │
│ + main(args: String[]): void                                                       │
└─────────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        │ extends
                                        ▼
                              ┌─────────────────┐
                              │     JFrame      │
                              └─────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                FareSystemState                                      │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ - beepBalance: int                                                                 │
│ - singleBalance: int                                                               │
│ - currentLine: int                                                                 │
│ - currentStation: int                                                              │
│ - currentStationName: String                                                       │
│ - currentUser: String                                                              │
│ - currentDateTime: String                                                          │
│ - totalDistance: int                                                               │
│ - tripRecords: List<TripRecord>                                                    │
│ - topupRecords: List<TopupRecord>                                                  │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ + FareSystemState()                                                                │
│ + getBeepBalance(): int                                                            │
│ + setBeepBalance(beepBalance: int): void                                           │
│ + getSingleBalance(): int                                                          │
│ + setSingleBalance(singleBalance: int): void                                       │
│ + getCurrentLine(): int                                                            │
│ + setCurrentLine(currentLine: int): void                                           │
│ + getCurrentStation(): int                                                         │
│ + setCurrentStation(currentStation: int): void                                     │
│ + getCurrentStationName(): String                                                  │
│ + setCurrentStationName(currentStationName: String): void                          │
│ + getCurrentUser(): String                                                         │
│ + setCurrentUser(currentUser: String): void                                        │
│ + getCurrentDateTime(): String                                                     │
│ + setCurrentDateTime(currentDateTime: String): void                                │
│ + getTotalDistance(): int                                                          │
│ + setTotalDistance(totalDistance: int): void                                       │
│ + getTripRecords(): List<TripRecord>                                               │
│ + addTripRecord(record: TripRecord): void                                          │
│ + getTopupRecords(): List<TopupRecord>                                             │
│ + addTopupRecord(record: TopupRecord): void                                        │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                  TripRecord                                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ - line: int                                                                        │
│ - origin: int                                                                      │
│ - destination: int                                                                 │
│ - fare: int                                                                        │
│ - distance: int                                                                    │
│ - datetime: String                                                                 │
│ - ticketType: String                                                               │
│ - cardType: String                                                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ + TripRecord(line: int, origin: int, destination: int, fare: int,                  │
│              distance: int, datetime: String, ticketType: String,                  │
│              cardType: String)                                                     │
│ + getLine(): int                                                                   │
│ + getOrigin(): int                                                                 │
│ + getDestination(): int                                                            │
│ + getFare(): int                                                                   │
│ + getDistance(): int                                                               │
│ + getDatetime(): String                                                            │
│ + getTicketType(): String                                                          │
│ + getCardType(): String                                                            │
│ + toString(): String                                                               │
│ - getStationName(line: int, stationIndex: int): String                            │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│                                 TopupRecord                                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ - cardType: String                                                                 │
│ - amount: int                                                                      │
│ - newBalance: int                                                                  │
│ - datetime: String                                                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ + TopupRecord(cardType: String, amount: int, newBalance: int, datetime: String)    │
│ + getCardType(): String                                                            │
│ + getAmount(): int                                                                 │
│ + getNewBalance(): int                                                             │
│ + getDatetime(): String                                                            │
│ + toString(): String                                                               │
└─────────────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────────────┐
│                               DatabaseManager                                       │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ - DB_NAME: String = "fare_system.db"                                              │
│ - DB_URL: String = "jdbc:sqlite:" + DB_NAME                                       │
│ - connection: Connection                                                           │
├─────────────────────────────────────────────────────────────────────────────────────┤
│ + DatabaseManager()                                                                │
│ - initializeDatabase(): void                                                       │
│ - createTables(): void                                                             │
│ - initializeSystemState(): void                                                    │
│ + saveSystemState(state: FareSystemState): void                                    │
│ + loadSystemState(): FareSystemState                                               │
│ + saveTripRecord(record: TripRecord): void                                         │
│ + saveTopupRecord(record: TopupRecord): void                                       │
│ + loadTripRecords(): List<TripRecord>                                              │
│ + loadTopupRecords(): List<TopupRecord>                                            │
│ + deleteTripRecord(record: TripRecord): void                                       │
│ + clearAllData(): void                                                             │
│ + closeConnection(): void                                                          │
└─────────────────────────────────────────────────────────────────────────────────────┘

## Class Relationships

FareSystemApp ◆─────────── FareSystemState
    │                           │
    │                           │ 1..*
    │                           ▼
    │                    ┌─────────────┐
    │                    │ TripRecord  │
    │                    └─────────────┘
    │                           │
    │                           │ 1..*
    │                           ▼
    │                    ┌─────────────┐
    │                    │TopupRecord  │
    │                    └─────────────┘
    │
    ◆────────────── DatabaseManager

## Relationship Legend:
- ◆ = Composition (has-a relationship)
- ▼ = Aggregation (contains)
- 1..* = One to many relationship
```

## Package Structure
```
com.securitysystem.fare
├── FareSystemApp.java      (Main GUI Application - extends JFrame)
├── FareSystemState.java    (State Management Class)
├── TripRecord.java         (Trip Data Model)
├── TopupRecord.java        (Top-up Data Model)
└── DatabaseManager.java    (Database Operations)
```

## Key Design Patterns Used:

### 1. **Model-View-Controller (MVC)**
- **Model**: `FareSystemState`, `TripRecord`, `TopupRecord`
- **View**: `FareSystemApp` (Swing GUI components)
- **Controller**: `FareSystemApp` (Event handlers and business logic)

### 2. **Data Access Object (DAO)**
- **DAO**: `DatabaseManager` (Handles all database operations)

### 3. **Observer Pattern**
- **Subject**: Arduino hardware (via serial communication)
- **Observer**: `FareSystemApp` (Updates GUI based on hardware events)

### 4. **Singleton-like Pattern**
- **DatabaseManager**: Single connection instance per application

## Class Dependencies:

1. **FareSystemApp** depends on:
   - `FareSystemState` (composition)
   - `DatabaseManager` (composition)
   - `TripRecord` (uses for data handling)
   - `TopupRecord` (uses for data handling)
   - External: `SerialPort`, Swing components

2. **FareSystemState** depends on:
   - `TripRecord` (aggregation - contains list)
   - `TopupRecord` (aggregation - contains list)

3. **DatabaseManager** depends on:
   - `FareSystemState` (for persistence)
   - `TripRecord` (for trip data persistence)
   - `TopupRecord` (for top-up data persistence)
   - External: SQLite JDBC driver

4. **TripRecord** and **TopupRecord** are independent value objects with no dependencies.

## Multiplicity:
- FareSystemApp : FareSystemState = 1:1
- FareSystemApp : DatabaseManager = 1:1
- FareSystemState : TripRecord = 1:*
- FareSystemState : TopupRecord = 1:*

This class diagram shows a well-structured object-oriented design with clear separation of concerns, proper encapsulation, and effective use of composition and aggregation relationships.
