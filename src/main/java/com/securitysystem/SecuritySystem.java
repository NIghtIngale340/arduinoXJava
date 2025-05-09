package com.securitysystem;

import com.fazecast.jSerialComm.SerialPort;
import com.fazecast.jSerialComm.SerialPortDataListener;
import com.fazecast.jSerialComm.SerialPortEvent;

public class SecuritySystem {
    private SerialPort serialPort;
    private boolean isArmed;
    private static final String[] POSSIBLE_PORTS = {"COM3", "COM4", "COM5", "COM6"}; // Common Arduino ports

    public SecuritySystem() {
        initializeSerialPort();
    }

    private void initializeSerialPort() {
        String portName = findArduinoPort();
        if (portName != null) {
            serialPort = SerialPort.getCommPort(portName);
            serialPort.setBaudRate(9600);
            serialPort.setNumDataBits(8);
            serialPort.setNumStopBits(1);
            serialPort.setParity(SerialPort.NO_PARITY);
            
            if (serialPort.openPort()) {
                serialPort.addDataListener(new SerialPortDataListener() {
                    @Override
                    public int getListeningEvents() {
                        return SerialPort.LISTENING_EVENT_DATA_AVAILABLE;
                    }

                    @Override
                    public void serialEvent(SerialPortEvent event) {
                        if (event.getEventType() == SerialPort.LISTENING_EVENT_DATA_AVAILABLE) {
                            byte[] buffer = new byte[serialPort.bytesAvailable()];
                            serialPort.readBytes(buffer, buffer.length);
                            String data = new String(buffer);
                            processSerialData(data);
                        }
                    }
                });
            } else {
                System.err.println("Failed to open port: " + portName);
            }
        } else {
            System.err.println("No Arduino found!");
        }
    }

    private String findArduinoPort() {
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort port : ports) {
            for (String possiblePort : POSSIBLE_PORTS) {
                if (port.getSystemPortName().equals(possiblePort)) {
                    return possiblePort;
                }
            }
        }
        return null;
    }

    private void processSerialData(String data) {
        if (data != null) {
            data = data.trim();
            if (data.equals("SYSTEM_ARMED")) {
                isArmed = true;
                System.out.println("System is now armed");
            } else if (data.equals("SYSTEM_DISARMED")) {
                isArmed = false;
                System.out.println("System is now disarmed");
            } else if (data.equals("INTRUSION_DETECTED")) {
                System.out.println("WARNING: Intrusion detected!");
            } else if (data.startsWith("STATUS:")) {
                String status = data.substring(7);
                isArmed = status.equals("ARMED");
                System.out.println("System status: " + status);
            }
        }
    }

    public boolean isArmed() {
        return isArmed;
    }

    public void close() {
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
    }
} 