package com.securitysystem;

public class AlertState implements SecuritySystemState {
    private final SecuritySystemApp app;

    public AlertState(SecuritySystemApp app) {
        this.app = app;
    }

    @Override
    public void arm() {
        // Cannot arm while in alert state
    }

    @Override
    public void disarm() {
        app.sendCommand("disarm");
    }

    @Override
    public void silence() {
        app.sendCommand("silence");
    }
} 