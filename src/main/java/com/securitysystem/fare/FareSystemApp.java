package com.securitysystem.fare;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.FileWriter;
import java.io.IOException;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import javax.swing.filechooser.FileNameExtensionFilter;

import com.fazecast.jSerialComm.SerialPort;

public class FareSystemApp extends JFrame {
    private static final long serialVersionUID = 1L;
    
    private SerialPort serialPort;
    private JTextArea logArea;
    private JLabel beepBalanceLabel;
    private JLabel singleBalanceLabel;
    private JLabel currentStationLabel;
    private JTextField topupAmountField;
    private JComboBox<String> cardTypeCombo;    private ScheduledExecutorService executor;
    private FareSystemState state;
    private DatabaseManager databaseManager;    public FareSystemApp() {
        // Initialize database first
        databaseManager = new DatabaseManager();
        
        // Load the state from database
        state = databaseManager.loadSystemState();
        
        // Set up the frame
        setTitle("Metro Fare System");
        setSize(600, 400);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);
        
        // Add window closing listener to clean up resources
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                // Save state to database before closing
                if (databaseManager != null && state != null) {
                    databaseManager.saveSystemState(state);
                    databaseManager.closeConnection();
                }
                if (executor != null) {
                    executor.shutdown();
                }
                if (serialPort != null && serialPort.isOpen()) {
                    serialPort.closePort();
                }
            }
        });
        
        // Create the UI components
        createUI();
        
        // Update UI with loaded state
        updateUI();
          // Initialize serial communication
        initializeSerial();
        
        // Start periodic auto-save (every 30 seconds)
        startPeriodicAutoSave();
    }
    
    private void createUI() {
        // Main panel with BorderLayout
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        // Top panel with status information
        JPanel topPanel = new JPanel(new GridLayout(1, 3, 10, 0));
        beepBalanceLabel = new JLabel("Beep Balance: 0 PHP");
        singleBalanceLabel = new JLabel("Single Balance: 0 PHP");
        currentStationLabel = new JLabel("Current Station: None");
        topPanel.add(beepBalanceLabel);
        topPanel.add(singleBalanceLabel);
        topPanel.add(currentStationLabel);
        
        // Center panel with log area
        logArea = new JTextArea();
        logArea.setEditable(false);
        JScrollPane scrollPane = new JScrollPane(logArea);
        
        // Bottom panel with controls
        JPanel bottomPanel = new JPanel(new GridLayout(2, 1, 0, 10));
        
        // Top-up panel
        JPanel topupPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        topupPanel.add(new JLabel("Top-up Amount:"));
        topupAmountField = new JTextField(8);
        topupPanel.add(topupAmountField);
        
        cardTypeCombo = new JComboBox<>(new String[]{"Beep Card", "Single Journey"});
        topupPanel.add(cardTypeCombo);
        
        JButton topupButton = new JButton("Top-up");
        topupButton.addActionListener(e -> handleTopup());
        topupPanel.add(topupButton);
        
        // Reset panel
        JPanel resetPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        resetPanel.add(new JLabel("Admin Password:"));
        JPasswordField passwordField = new JPasswordField(8);
        resetPanel.add(passwordField);
        
        JButton resetButton = new JButton("Reset Balances");
        resetButton.addActionListener(e -> handleReset(new String(passwordField.getPassword())));
        resetPanel.add(resetButton);
        
        // Add distance reset button
        JButton resetDistButton = new JButton("Reset Distance");
        resetDistButton.addActionListener(e -> {
            String password = JOptionPane.showInputDialog(this, 
                "Enter admin password to reset distance:", 
                "Reset Distance", JOptionPane.QUESTION_MESSAGE);
            if (password != null && !password.isEmpty()) {
                sendCommand("CMD:RESET_DIST," + password);
            } else {
                log("Error: Password required");
            }
        });
        resetPanel.add(resetDistButton);        // Add history button
        JButton historyButton = new JButton("History");
        historyButton.addActionListener(e -> showHistoryDialog());
        resetPanel.add(historyButton);
        
        // Add panels to the bottom panel
        bottomPanel.add(topupPanel);
        bottomPanel.add(resetPanel);
        
        // Add components to the main panel
        mainPanel.add(topPanel, BorderLayout.NORTH);
        mainPanel.add(scrollPane, BorderLayout.CENTER);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);
        
        // Set the content pane
        setContentPane(mainPanel);
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
        sendCommand("CMD:GET_DIST");
    }

    private void readSerial() {
        if (serialPort.bytesAvailable() > 0) {
            byte[] buffer = new byte[serialPort.bytesAvailable()];
            int numRead = serialPort.readBytes(buffer, buffer.length);
            String data = new String(buffer, 0, numRead);
            
            // Process each line
            for (String line : data.split("\n")) {
                if (line.trim().isEmpty()) continue;
                
                SwingUtilities.invokeLater(() -> {
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
                    } else if (line.startsWith("DIST:")) {
                        handleDistanceUpdate(line.substring(5));
                    }
                });
            }
        }
    }    private void handleBalanceUpdate(String data) {
        try {
            String[] parts = data.trim().split(",");
            if (parts.length == 2) {
                int beepBalance = Integer.parseInt(parts[0].trim());
                int singleBalance = Integer.parseInt(parts[1].trim());
                state.setBeepBalance(beepBalance);
                state.setSingleBalance(singleBalance);
                updateBalanceDisplay();
                
                // Save state to database
                if (databaseManager != null) {
                    databaseManager.saveSystemState(state);
                }
            }
        } catch (NumberFormatException e) {
            log("Error parsing balance data: " + data);
        }
    }    private void handleStationUpdate(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 3) {
            int line = Integer.parseInt(parts[0]);
            int station = Integer.parseInt(parts[1]);
            String stationName = parts[2];
            state.setCurrentLine(line);
            state.setCurrentStation(station);
            state.setCurrentStationName(stationName);
            updateStationDisplay();
            
            // Save state to database
            if (databaseManager != null) {
                databaseManager.saveSystemState(state);
            }
        }
    }
    
    private void handleDistanceUpdate(String data) {
        try {
            int totalDistance = Integer.parseInt(data.trim());
            // Only log if the distance has changed
            if (totalDistance != state.getTotalDistance()) {
                log("Total distance traveled: " + totalDistance + " km");
                state.setTotalDistance(totalDistance);
            }
        } catch (NumberFormatException e) {
            log("Error parsing distance data: " + data);
        }
    }    private void handleTripRecord(String data) {
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
            
            // Save to database
            if (databaseManager != null) {
                databaseManager.saveTripRecord(record);
            }
            
            log("Trip: " + record.toString());
        }
    }    private void handleTopupRecord(String data) {
        String[] parts = data.split(",");
        if (parts.length >= 4) {
            TopupRecord record = new TopupRecord(
                parts[0], // card type
                Integer.parseInt(parts[1]), // amount
                Integer.parseInt(parts[2]), // new balance
                parts[3]  // datetime
            );
            state.addTopupRecord(record);
            
            // Save to database
            if (databaseManager != null) {
                databaseManager.saveTopupRecord(record);
            }
            
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

            String cardType = cardTypeCombo.getSelectedItem().equals("Beep Card") ? "BEEP" : "SINGLE";
            sendCommand(String.format("CMD:TOPUP,%s,%d", cardType, amount));
            topupAmountField.setText("");
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
        logArea.append(message + "\n");
        // Auto-scroll to the bottom
        logArea.setCaretPosition(logArea.getDocument().getLength());
    }
    
    // Getter for state (for testing purposes)
    public FareSystemState getFareSystemState() {
        return state;
    }

    private void showHistoryDialog() {
        JDialog historyDialog = new JDialog(this, "Trip History", true);
        historyDialog.setSize(800, 600);
        historyDialog.setLocationRelativeTo(this);
        
        // Main panel with BorderLayout
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        // Summary panel at the top
        JPanel summaryPanel = createSummaryPanel();
        
        // Trip list in the center
        JPanel tripListPanel = createTripListPanel();
        
        // Button panel at the bottom
        JPanel buttonPanel = createHistoryButtonPanel(historyDialog);
        
        mainPanel.add(summaryPanel, BorderLayout.NORTH);
        mainPanel.add(tripListPanel, BorderLayout.CENTER);
        mainPanel.add(buttonPanel, BorderLayout.SOUTH);
        
        historyDialog.setContentPane(mainPanel);
        historyDialog.setVisible(true);
    }
    
    private JPanel createSummaryPanel() {
        JPanel summaryPanel = new JPanel(new GridLayout(1, 3, 20, 5));
        summaryPanel.setBorder(BorderFactory.createTitledBorder("Trip Summary"));
        
        int totalTrips = state.getTripRecords().size();
        int totalFare = state.getTripRecords().stream().mapToInt(TripRecord::getFare).sum();
        int totalDistance = state.getTripRecords().stream().mapToInt(TripRecord::getDistance).sum();
        
        // Create individual summary cards
        JPanel tripsCard = createSummaryCard("Total Trips", String.valueOf(totalTrips), "🚇");
        JPanel fareCard = createSummaryCard("Total Fare", "₱" + totalFare, "💰");
        JPanel distanceCard = createSummaryCard("Total Distance", totalDistance + " km", "📏");
        
        summaryPanel.add(tripsCard);
        summaryPanel.add(fareCard);
        summaryPanel.add(distanceCard);
        
        return summaryPanel;
    }
    
    private JPanel createSummaryCard(String title, String value, String icon) {
        JPanel card = new JPanel(new BorderLayout());
        card.setBorder(BorderFactory.createEtchedBorder());
        
        JLabel titleLabel = new JLabel("<html><center>" + icon + "<br/><b>" + title + "</b></center></html>");
        JLabel valueLabel = new JLabel("<html><center><font size='5' color='blue'><b>" + value + "</b></font></center></html>");
        
        card.add(titleLabel, BorderLayout.NORTH);
        card.add(valueLabel, BorderLayout.CENTER);
        
        return card;
    }
    
    private JPanel createTripListPanel() {
        JPanel tripListPanel = new JPanel(new BorderLayout());
        tripListPanel.setBorder(BorderFactory.createTitledBorder("Trip Details"));
        
        // Create list model with trip records
        String[] tripData = state.getTripRecords().stream()
            .map(this::formatTripRecord)
            .toArray(String[]::new);
        
        JList<String> tripList = new JList<>(tripData);
        tripList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        
        JScrollPane scrollPane = new JScrollPane(tripList);
        tripListPanel.add(scrollPane, BorderLayout.CENTER);
        
        // Add delete selected trip button
        JPanel listButtonPanel = new JPanel(new FlowLayout());
        JButton deleteSelectedButton = new JButton("Delete Selected Trip");
        deleteSelectedButton.addActionListener(e -> {
            int selectedIndex = tripList.getSelectedIndex();
            if (selectedIndex >= 0) {
                int result = JOptionPane.showConfirmDialog(
                    tripListPanel,
                    "Are you sure you want to delete this trip?",
                    "Confirm Delete",
                    JOptionPane.YES_NO_OPTION
                );                if (result == JOptionPane.YES_OPTION) {
                    TripRecord recordToDelete = state.getTripRecords().get(selectedIndex);
                    state.getTripRecords().remove(selectedIndex);
                    
                    // Delete from database
                    if (databaseManager != null) {
                        databaseManager.deleteTripRecord(recordToDelete);
                    }
                    
                    // Refresh the dialog
                    SwingUtilities.getWindowAncestor(tripListPanel).dispose();
                    showHistoryDialog();
                }
            } else {
                JOptionPane.showMessageDialog(tripListPanel, "Please select a trip to delete.");
            }
        });
        listButtonPanel.add(deleteSelectedButton);
        tripListPanel.add(listButtonPanel, BorderLayout.SOUTH);
        
        return tripListPanel;
    }
    
    private JPanel createHistoryButtonPanel(JDialog dialog) {
        JPanel buttonPanel = new JPanel(new FlowLayout());
        
        JButton exportButton = new JButton("Export to File");
        exportButton.addActionListener(e -> exportHistoryToFile(dialog));
          JButton clearAllButton = new JButton("Clear All History");
        clearAllButton.addActionListener(e -> {
            int result = JOptionPane.showConfirmDialog(
                dialog,
                "Are you sure you want to clear all trip history?\nThis will permanently delete all data from the database.",
                "Confirm Clear All",
                JOptionPane.YES_NO_OPTION
            );
            if (result == JOptionPane.YES_OPTION) {
                state.getTripRecords().clear();
                state.getTopupRecords().clear();
                
                // Clear data from database
                if (databaseManager != null) {
                    databaseManager.clearAllData();
                }
                
                dialog.dispose();
                showHistoryDialog();
                log("All data cleared from memory and database");
            }
        });
        
        JButton closeButton = new JButton("Close");
        closeButton.addActionListener(e -> dialog.dispose());
        
        buttonPanel.add(exportButton);
        buttonPanel.add(clearAllButton);
        buttonPanel.add(closeButton);
        
        return buttonPanel;
    }
    
    private void exportHistoryToFile(JDialog parent) {
        if (state.getTripRecords().isEmpty()) {
            JOptionPane.showMessageDialog(parent, "No trip history to export.");
            return;
        }
        
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setFileFilter(new FileNameExtensionFilter("Text Files", "txt"));
        fileChooser.setSelectedFile(new java.io.File("trip_history_" + 
            LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyyMMdd_HHmmss")) + ".txt"));
        
        int result = fileChooser.showSaveDialog(parent);
        if (result == JFileChooser.APPROVE_OPTION) {
            try (FileWriter writer = new FileWriter(fileChooser.getSelectedFile())) {
                writer.write("Metro Fare System - Trip History Report\n");
                writer.write("Generated on: " + LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")) + "\n");
                writer.write("=" + "=".repeat(50) + "\n\n");
                
                // Summary
                int totalTrips = state.getTripRecords().size();
                int totalFare = state.getTripRecords().stream().mapToInt(TripRecord::getFare).sum();
                int totalDistance = state.getTripRecords().stream().mapToInt(TripRecord::getDistance).sum();
                
                writer.write("SUMMARY:\n");
                writer.write("Total Trips: " + totalTrips + "\n");
                writer.write("Total Fare: ₱" + totalFare + "\n");
                writer.write("Total Distance: " + totalDistance + " km\n\n");
                
                writer.write("TRIP DETAILS:\n");
                writer.write("-".repeat(80) + "\n");
                
                for (int i = 0; i < state.getTripRecords().size(); i++) {
                    TripRecord record = state.getTripRecords().get(i);
                    String lineName = record.getLine() == 1 ? "LRT-1" : "LRT-2";
                    writer.write(String.format("%d. %s | %s\n", 
                        i + 1, record.getDatetime(), lineName));
                    writer.write(String.format("   %s → %s\n", 
                        getStationName(record.getLine(), record.getOrigin()),
                        getStationName(record.getLine(), record.getDestination())));
                    writer.write(String.format("   Fare: ₱%d | Distance: %d km | %s %s\n\n", 
                        record.getFare(), record.getDistance(), 
                        record.getTicketType(), record.getCardType()));
                }
                
                JOptionPane.showMessageDialog(parent, 
                    "Trip history exported successfully to:\n" + fileChooser.getSelectedFile().getAbsolutePath());
                    
            } catch (IOException ex) {
                JOptionPane.showMessageDialog(parent, 
                    "Error exporting trip history: " + ex.getMessage(), 
                    "Export Error", JOptionPane.ERROR_MESSAGE);
            }
        }
    }
    
    private String formatTripRecord(TripRecord record) {
        String lineName = record.getLine() == 1 ? "LRT-1" : "LRT-2";
        String originStation = getStationName(record.getLine(), record.getOrigin());
        String destStation = getStationName(record.getLine(), record.getDestination());
        
        return String.format("<html><b>%s</b> | %s<br/>" +
                           "&nbsp;&nbsp;%s → %s<br/>" +
                           "&nbsp;&nbsp;<font color='green'>₱%d</font> | " +
                           "<font color='blue'>%d km</font> | " +
                           "<font color='purple'>%s %s</font></html>", 
            record.getDatetime(),
            lineName,
            originStation,
            destStation,
            record.getFare(),
            record.getDistance(),
            record.getTicketType(),
            record.getCardType()
        );
    }
    
    private String getStationName(int line, int stationIndex) {
        // LRT-1 stations - Match Arduino PROGMEM exactly
        String[] lrt1Stations = {
            "FPJR.", "Balintawak", "Monumento", "5th Avenue", "R. Papa",
            "Abad Santos", "Blumentritt", "Tayuman", "Bambang", "D. Jose",
            "Carriedo", "Central Terminal", "United Nations", "Pedro Gil", "Quirino",
            "Vito Cruz", "Gil Puyat", "Libertad", "EDSA", "Baclaran"
        };
        
        // LRT-2 stations - Match Arduino PROGMEM exactly
        String[] lrt2Stations = {
            "Recto", "Legarda", "Pureza", "V. Mapa", "J. Ruiz",
            "Gilmore", "Betty Go-Belmonte", "Cubao", "Anonas", "Katipunan",
            "Santolan", "Marikina", "Antipolo"
        };
        
        if (line == 1 && stationIndex >= 0 && stationIndex < lrt1Stations.length) {
            return lrt1Stations[stationIndex];
        } else if (line == 2 && stationIndex >= 0 && stationIndex < lrt2Stations.length) {
            return lrt2Stations[stationIndex];
        }
        
        return "Unknown Station";
    }

    public static void main(String[] args) {
        try {
            // Set system look and feel
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (ClassNotFoundException | InstantiationException | IllegalAccessException | UnsupportedLookAndFeelException e) {
            // Fallback to default look and feel
            System.err.println("Could not set system look and feel: " + e.getMessage());
        }
        
        SwingUtilities.invokeLater(() -> {
            FareSystemApp app = new FareSystemApp();
            app.setVisible(true);
        });
    }    private void updateUI() {
        // Update balance labels
        updateBalanceDisplay();
        
        // Update current station label
        updateStationDisplay();
        
        // Log that data was loaded from database (only once at startup)
        if (!state.getTripRecords().isEmpty() || !state.getTopupRecords().isEmpty()) {
            log("Loaded from database - " + state.getTripRecords().size() + " trips, " + 
                state.getTopupRecords().size() + " topups");
        } else {
            log("Database ready - history will be saved automatically");
        }
    }

    private void startPeriodicAutoSave() {
        // Create a scheduler for periodic auto-save
        executor = Executors.newSingleThreadScheduledExecutor();
        
        // Schedule auto-save every 30 seconds
        executor.scheduleAtFixedRate(() -> {
            if (databaseManager != null && state != null) {
                try {
                    databaseManager.saveSystemState(state);
                    // Only log occasionally to avoid spam
                    if (System.currentTimeMillis() % 300000 < 30000) { // Log every 5 minutes
                        SwingUtilities.invokeLater(() -> 
                            log("Auto-saved state to database")
                        );
                    }
                } catch (Exception e) {
                    SwingUtilities.invokeLater(() -> 
                        log("Error during auto-save: " + e.getMessage())
                    );
                }
            }
        }, 30, 30, TimeUnit.SECONDS);
    }
}