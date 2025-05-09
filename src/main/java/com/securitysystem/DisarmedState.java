package com.securitysystem;

public class DisarmedState implements SecuritySystemState {
    private final SecuritySystemApp app;

    public DisarmedState(SecuritySystemApp app) {
        this.app = app;
    }

    @Override
    public void arm() {
        app.sendCommand("arm");
    }

    @Override
    public void disarm() {
        // Already disarmed, do nothing
    }

    @Override
    public void silence() {
        // Cannot silence when disarmed
    }
} 