package com.securitysystem;

import java.awt.BorderLayout;
import java.awt.Font;
import java.awt.GridLayout;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;

import com.fazecast.jSerialComm.SerialPort;

public class SecuritySystemApp extends JFrame {
    private SerialPort serialPort;
    private final JLabel statusLabel;
    private final JButton armButton;
    private final JButton disarmButton;
    private final JButton silenceButton;
    private SecuritySystemState currentState;
    private boolean isAuthenticated = false;
    private final JTextArea logArea;

    public SecuritySystemApp() {
        setTitle("Security System Control Panel");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(400, 300);
        setLayout(new BorderLayout());

        // Create main panel
        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new GridLayout(4, 1, 10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        // Status display
        statusLabel = new JLabel("Status: Initializing...", SwingConstants.CENTER);
        statusLabel.setFont(new Font("Arial", Font.BOLD, 16));
        mainPanel.add(statusLabel);

        // Control buttons
        JPanel buttonPanel = new JPanel(new GridLayout(1, 3, 10, 0));
        armButton = new JButton("Arm System");
        disarmButton = new JButton("Disarm System");
        silenceButton = new JButton("Silence Alarm");
        
        buttonPanel.add(armButton);
        buttonPanel.add(disarmButton);
        buttonPanel.add(silenceButton);

        mainPanel.add(buttonPanel);

        // Add authentication panel
        JPanel authPanel = createAuthPanel();
        mainPanel.add(authPanel);

        // Add log panel
        logArea = new JTextArea(5, 30);
        logArea.setEditable(false);
        JScrollPane logScroll = new JScrollPane(logArea);
        mainPanel.add(logScroll);

        add(mainPanel, BorderLayout.CENTER);

        // Initialize state
        currentState = new ArmedState(this);

        // Add action listeners
        setupActionListeners();

        // Initialize serial communication
        initializeSerialPort();
    }

    private JPanel createAuthPanel() {
        JPanel panel = new JPanel(new GridLayout(2, 2, 5, 5));
        JTextField usernameField = new JTextField();
        JPasswordField passwordField = new JPasswordField();
        JButton loginButton = new JButton("Login");

        panel.add(new JLabel("Username:"));
        panel.add(usernameField);
        panel.add(new JLabel("Password:"));
        panel.add(passwordField);

        loginButton.addActionListener(e -> {
            String username = usernameField.getText();
            String password = new String(passwordField.getPassword());
            if (authenticate(username, password)) {
                isAuthenticated = true;
                enableControls(true);
                JOptionPane.showMessageDialog(this, "Login successful!");
            } else {
                JOptionPane.showMessageDialog(this, "Invalid credentials!", "Error", JOptionPane.ERROR_MESSAGE);
            }
        });

        panel.add(loginButton);
        return panel;
    }

    private void setupActionListeners() {
        armButton.addActionListener(e -> {
            if (isAuthenticated) {
                currentState.arm();
            } else {
                JOptionPane.showMessageDialog(this, "Please login first!");
            }
        });

        disarmButton.addActionListener(e -> {
            if (isAuthenticated) {
                currentState.disarm();
            } else {
                JOptionPane.showMessageDialog(this, "Please login first!");
            }
        });

        silenceButton.addActionListener(e -> {
            if (isAuthenticated) {
                currentState.silence();
            } else {
                JOptionPane.showMessageDialog(this, "Please login first!");
            }
        });
    }

    private void initializeSerialPort() {
        SerialPort[] ports = SerialPort.getCommPorts();
        if (ports.length == 0) {
            JOptionPane.showMessageDialog(this, "No serial ports found!",
                    "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        // Use COM5 specifically
        serialPort = SerialPort.getCommPort("COM5");
        serialPort.setBaudRate(9600);
        serialPort.setNumDataBits(8);
        serialPort.setNumStopBits(1);
        serialPort.setParity(SerialPort.NO_PARITY);

        if (!serialPort.openPort()) {
            JOptionPane.showMessageDialog(this, "Error opening COM5 port. Please make sure Arduino is connected.",
                    "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        // Start reading thread
        new Thread(() -> {
            byte[] buffer = new byte[1024];
            while (serialPort.isOpen()) {
                int bytesRead = serialPort.readBytes(buffer, buffer.length);
                if (bytesRead > 0) {
                    String data = new String(buffer, 0, bytesRead).trim();
                    updateStatus(data);
                }
            }
        }).start();
    }

    private void updateStatus(String status) {
        SwingUtilities.invokeLater(() -> {
            statusLabel.setText("Status: " + status);
            logArea.append(status + "\n");
            switch (status) {
                case "armed":
                    setState(new ArmedState(this));
                    break;
                case "disarmed":
                    setState(new DisarmedState(this));
                    break;
                case "alert":
                    setState(new AlertState(this));
                    break;
            }
        });
    }

    public void setState(SecuritySystemState state) {
        this.currentState = state;
    }

    public void sendCommand(String command) {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.writeBytes((command + "\n").getBytes(), command.length() + 1);
        }
    }

    private boolean authenticate(String username, String password) {
        // Simple authentication - replace with proper authentication in production
        return "admin".equals(username) && "password".equals(password);
    }

    private void enableControls(boolean enable) {
        armButton.setEnabled(enable);
        disarmButton.setEnabled(enable);
        silenceButton.setEnabled(enable);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            new SecuritySystemApp().setVisible(true);
        });
    }
} 