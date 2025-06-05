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
        
        // Get origin and destination station names
        String originStation = getStationName(line, origin);
        String destStation = getStationName(line, destination);
        
        return String.format("%s: %s to %s, Fare: %d PHP, Distance: %d km (%s, %s)", 
            lineName, originStation, destStation, fare, distance, ticketType, cardType);
    }
      // Helper method to get station names based on line and index
    private String getStationName(int line, int stationIndex) {
        if (line == 1) {            // LRT1 stations - Match Arduino PROGMEM exactly
            String[] lrt1Stations = {
                "FPJR.", "Balintawak", "Monumento", "5th Avenue", "R. Papa",
                "Abad Santos", "Blumentritt", "Tayuman", "Bambang", "D. Jose",
                "Carriedo", "Central Terminal", "United Nations", "Pedro Gil", "Quirino",
                "Vito Cruz", "Gil Puyat", "Libertad", "EDSA", "Baclaran"
            };
            if (stationIndex >= 0 && stationIndex < lrt1Stations.length) {
                return lrt1Stations[stationIndex];
            }
        } else if (line == 2) {
            // LRT2 stations - Match Arduino PROGMEM exactly
            String[] lrt2Stations = {
                "Recto", "Legarda", "Pureza", "V. Mapa", "J. Ruiz",
                "Gilmore", "Betty Go-Belmonte", "Cubao", "Anonas", "Katipunan",
                "Santolan", "Marikina", "Antipolo"
            };
            if (stationIndex >= 0 && stationIndex < lrt2Stations.length) {
                return lrt2Stations[stationIndex];
            }
        }
        return "Station " + stationIndex; // Fallback if station not found
    }
}