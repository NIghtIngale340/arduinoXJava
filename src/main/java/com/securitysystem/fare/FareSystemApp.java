package com.securitysystem.fare;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import com.fazecast.jSerialComm.SerialPort;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.control.PasswordField;
import javafx.scene.control.TextArea;
import javafx.scene.control.TextField;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

public class FareSystemApp extends Application {
    private SerialPort serialPort;
    private TextArea logArea;
    private Label beepBalanceLabel;
    private Label singleBalanceLabel;
    private Label currentStationLabel;
    private TextField topupAmountField;
    private ComboBox<String> cardTypeCombo;
    private ScheduledExecutorService executor;
    private FareSystemState state;

    @Override
    public void start(Stage primaryStage) {
        state = new FareSystemState();
        
        // Create the main layout
        BorderPane root = new BorderPane();
        root.setPadding(new Insets(10));

        // Create the top section with balances and current station
        HBox topBox = new HBox(20);
        beepBalanceLabel = new Label("Beep Balance: 0 PHP");
        singleBalanceLabel = new Label("Single Balance: 0 PHP");
        currentStationLabel = new Label("Current Station: None");
        topBox.getChildren().addAll(beepBalanceLabel, singleBalanceLabel, currentStationLabel);
        root.setTop(topBox);

        // Create the center section with log area
        logArea = new TextArea();
        logArea.setEditable(false);
        logArea.setPrefRowCount(10);
        root.setCenter(logArea);

        // Create the bottom section with controls
        VBox bottomBox = new VBox(10);
        
        // Top-up controls
        HBox topupBox = new HBox(10);
        Label topupLabel = new Label("Top-up Amount:");
        topupAmountField = new TextField();
        topupAmountField.setPrefWidth(100);
        cardTypeCombo = new ComboBox<>();
        cardTypeCombo.getItems().addAll("Beep Card", "Single Journey");
        cardTypeCombo.setValue("Beep Card");
        Button topupButton = new Button("Top-up");
        topupButton.setOnAction(e -> handleTopup());
        topupBox.getChildren().addAll(topupLabel, topupAmountField, cardTypeCombo, topupButton);
        
        // Reset controls
        HBox resetBox = new HBox(10);
        Label passwordLabel = new Label("Admin Password:");
        PasswordField passwordField = new PasswordField();
        passwordField.setPrefWidth(100);
        Button resetButton = new Button("Reset Balances");
        resetButton.setOnAction(e -> handleReset(passwordField.getText()));
        resetBox.getChildren().addAll(passwordLabel, passwordField, resetButton);
        
        bottomBox.getChildren().addAll(topupBox, resetBox);
        root.setBottom(bottomBox);

        // Create the scene
        Scene scene = new Scene(root, 600, 400);
        primaryStage.setTitle("Metro Fare System");
        primaryStage.setScene(scene);
        primaryStage.show();

        // Initialize serial communication
        initializeSerial();
    }

    private void initializeSerial() {
        // Find the Arduino port
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort port : ports) {
            if (port.getSystemPortName().equals("COM5")) {
                serialPort = port;
                break;
            }
        }

        if (serialPort == null) {
            log("Error: No Arduino found on COM5");
            return;
        }

        // Configure the serial port
        serialPort.setBaudRate(9600);
        serialPort.setNumDataBits(8);
        serialPort.setNumStopBits(1);
        serialPort.setParity(SerialPort.NO_PARITY);

        // Open the port
        if (!serialPort.openPort()) {
            log("Error: Could not open port COM5");
            return;
        }

        // Start reading from serial port
        executor = Executors.newSingleThreadScheduledExecutor();
        executor.scheduleAtFixedRate(this::readSerial, 0, 100, TimeUnit.MILLISECONDS);

        // Request initial state
        sendCommand("CMD:GET_BAL");
        sendCommand("CMD:GET_STN");
        sendCommand("CMD:GET_USER");
    }

    private void readSerial() {
        if (serialPort.bytesAvailable() > 0) {
            byte[] buffer = new byte[serialPort.bytesAvailable()];
            int numRead = serialPort.readBytes(buffer, buffer.length);
            String data = new String(buffer, 0, numRead);
            
            // Process each line
            for (String line : data.split("\n")) {
                if (line.trim().isEmpty()) continue;
                
                Platform.runLater(() -> {
                    if (line.startsWith("BAL:")) {
                        handleBalanceUpdate(line.substring(4));
                    } else if (line.startsWith("STN:")) {
                        handleStationUpdate(line.substring(4));
                    } else if (line.startsWith("TRIP:")) {
                        handleTripRecord(line.substring(5));
                    } else if (line.startsWith("TOPUP:")) {
                        handleTopupRecord(line.substring(6));
                    } else if (line.startsWith("USER:")) {
                        handleUserInfo(line.substring(5));
                    } else if (line.startsWith("MSG:")) {
                        handleMessage(line.substring(4));
                    } else if (line.startsWith("ERR:")) {
                        handleError(line.substring(4));
                    }
                });
            }
        }
    }

    private void handleBalanceUpdate(String data) {
        try {
            String[] parts = data.trim().split(",");
            if (parts.length == 2) {
                int beepBalance = Integer.parseInt(parts[0].trim());
                int singleBalance = Integer.parseInt(parts[1].trim());
                state.setBeepBalance(beepBalance);
                state.setSingleBalance(singleBalance);
                updateBalanceDisplay();
            }
        } catch (NumberFormatException e) {
            log("Error parsing balance data: " + data);
        }
    }

    private void handleStationUpdate(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 3) {
            int line = Integer.parseInt(parts[0]);
            int station = Integer.parseInt(parts[1]);
            String stationName = parts[2];
            state.setCurrentLine(line);
            state.setCurrentStation(station);
            state.setCurrentStationName(stationName);
            updateStationDisplay();
        }
    }

    private void handleTripRecord(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 8) {
            TripRecord record = new TripRecord(
                Integer.parseInt(parts[0]), // line
                Integer.parseInt(parts[1]), // origin
                Integer.parseInt(parts[2]), // dest
                Integer.parseInt(parts[3]), // fare
                Integer.parseInt(parts[4]), // distance
                parts[5], // datetime
                parts[6], // ticket type
                parts[7]  // card type
            );
            state.addTripRecord(record);
            log("Trip: " + record.toString());
        }
    }

    private void handleTopupRecord(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 4) {
            TopupRecord record = new TopupRecord(
                parts[0], // card type
                Integer.parseInt(parts[1]), // amount
                Integer.parseInt(parts[2]), // new balance
                parts[3]  // datetime
            );
            state.addTopupRecord(record);
            log("Top-up: " + record.toString());
        }
    }

    private void handleUserInfo(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 2) {
            state.setCurrentUser(parts[0]);
            state.setCurrentDateTime(parts[1]);
            log("User: " + parts[0] + " (" + parts[1] + ")");
        }
    }

    private void handleMessage(String message) {
        log("Message: " + message);
    }

    private void handleError(String errorCode) {
        String errorMessage = "Error " + errorCode + ": ";
        switch (errorCode) {
            case "001": errorMessage += "Invalid command format"; break;
            case "002": errorMessage += "Invalid amount"; break;
            case "003": errorMessage += "Invalid card type"; break;
            case "004": errorMessage += "Invalid parameters"; break;
            case "005": errorMessage += "Authentication failed"; break;
            default: errorMessage += "Unknown error"; break;
        }
        log(errorMessage);
    }

    private void updateBalanceDisplay() {
        beepBalanceLabel.setText(String.format("Beep Balance: %d PHP", state.getBeepBalance()));
        singleBalanceLabel.setText(String.format("Single Balance: %d PHP", state.getSingleBalance()));
    }

    private void updateStationDisplay() {
        String lineName = state.getCurrentLine() == 1 ? "LRT1" : 
                         state.getCurrentLine() == 2 ? "LRT2" : "MRT3";
        currentStationLabel.setText(String.format("Current Station: %s - %s", 
            lineName, state.getCurrentStationName()));
    }

    private void handleTopup() {
        try {
            String amountText = topupAmountField.getText().trim();
            if (amountText.isEmpty()) {
                log("Error: Please enter an amount");
                return;
            }
            
            int amount = Integer.parseInt(amountText);
            if (amount <= 0) {
                log("Error: Amount must be greater than 0");
                return;
            }
            
            if (amount > 10000) {
                log("Error: Maximum top-up amount is 10,000 PHP");
                return;
            }

            String cardType = cardTypeCombo.getValue().equals("Beep Card") ? "BEEP" : "SINGLE";
            sendCommand(String.format("CMD:TOPUP,%s,%d", cardType, amount));
            topupAmountField.clear();
        } catch (NumberFormatException e) {
            log("Error: Please enter a valid number");
        }
    }

    private void handleReset(String password) {
        if (password.isEmpty()) {
            log("Error: Password required");
            return;
        }
        sendCommand("CMD:RESET_BAL," + password);
    }

    private void sendCommand(String command) {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.writeBytes((command + "\n").getBytes(), command.length() + 1);
        }
    }

    private void log(String message) {
        logArea.appendText(message + "\n");
    }

    @Override
    public void stop() {
        if (executor != null) {
            executor.shutdown();
        }
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
} 