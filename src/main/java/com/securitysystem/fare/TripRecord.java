package com.securitysystem.fare;

public class TripRecord {
    private int line;
    private int origin;
    private int destination;
    private int fare;
    private int distance;
    private String datetime;
    private String ticketType;
    private String cardType;

    public TripRecord(int line, int origin, int destination, int fare, int distance, 
                     String datetime, String ticketType, String cardType) {
        this.line = line;
        this.origin = origin;
        this.destination = destination;
        this.fare = fare;
        this.distance = distance;
        this.datetime = datetime;
        this.ticketType = ticketType;
        this.cardType = cardType;
    }

    public int getLine() {
        return line;
    }

    public int getOrigin() {
        return origin;
    }

    public int getDestination() {
        return destination;
    }

    public int getFare() {
        return fare;
    }

    public int getDistance() {
        return distance;
    }

    public String getDatetime() {
        return datetime;
    }

    public String getTicketType() {
        return ticketType;
    }

    public String getCardType() {
        return cardType;
    }

    @Override
    public String toString() {
        String lineName = line == 1 ? "LRT1" : line == 2 ? "LRT2" : "MRT3";
        return String.format("%s: %s to %s, Fare: %d PHP, Distance: %dm (%s, %s)", 
            lineName, origin, destination, fare, distance, ticketType, cardType);
    }
}