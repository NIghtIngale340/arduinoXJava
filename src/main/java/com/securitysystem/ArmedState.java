package com.securitysystem;

public class ArmedState implements SecuritySystemState {
    private final SecuritySystemApp app;

    public ArmedState(SecuritySystemApp app) {
        this.app = app;
    }

    @Override
    public void arm() {
        // Already armed, do nothing
    }

    @Override
    public void disarm() {
        app.sendCommand("disarm");
    }

    @Override
    public void silence() {
        // Cannot silence when armed
    }
} 