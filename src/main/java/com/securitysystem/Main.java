package com.securitysystem;

public class Main {
    public static void main(String[] args) {
        SecuritySystem securitySystem = new SecuritySystem();
        
        // Add shutdown hook to properly close the serial port
        Runtime.getRuntime().addShutdownHook(new Thread(securitySystem::close));
        
        // Keep the application running using a CountDownLatch
        try {
            // Create a latch that will never count down
            final Object lock = new Object();
            synchronized (lock) {
                lock.wait(); // This will keep the main thread alive
            }
        } catch (InterruptedException e) {
            System.err.println("Application interrupted: " + e.getMessage());
        }
    }
} 