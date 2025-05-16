package com.securitysystem.fare;

import java.util.ArrayList;
import java.util.List;

public class FareSystemState {
    private int beepBalance;
    private int singleBalance;
    private int currentLine;
    private int currentStation;
    private String currentStationName;
    private String currentUser;
    private String currentDateTime;
    private List<TripRecord> tripRecords;
    private List<TopupRecord> topupRecords;

    public FareSystemState() {
        this.beepBalance = 0;
        this.singleBalance = 0;
        this.currentLine = 0;
        this.currentStation = 0;
        this.currentStationName = "None";
        this.currentUser = "";
        this.currentDateTime = "";
        this.tripRecords = new ArrayList<>();
        this.topupRecords = new ArrayList<>();
    }

    public int getBeepBalance() {
        return beepBalance;
    }

    public void setBeepBalance(int beepBalance) {
        this.beepBalance = beepBalance;
    }

    public int getSingleBalance() {
        return singleBalance;
    }

    public void setSingleBalance(int singleBalance) {
        this.singleBalance = singleBalance;
    }

    public int getCurrentLine() {
        return currentLine;
    }

    public void setCurrentLine(int currentLine) {
        this.currentLine = currentLine;
    }

    public int getCurrentStation() {
        return currentStation;
    }

    public void setCurrentStation(int currentStation) {
        this.currentStation = currentStation;
    }

    public String getCurrentStationName() {
        return currentStationName;
    }

    public void setCurrentStationName(String currentStationName) {
        this.currentStationName = currentStationName;
    }

    public String getCurrentUser() {
        return currentUser;
    }

    public void setCurrentUser(String currentUser) {
        this.currentUser = currentUser;
    }

    public String getCurrentDateTime() {
        return currentDateTime;
    }

    public void setCurrentDateTime(String currentDateTime) {
        this.currentDateTime = currentDateTime;
    }

    public List<TripRecord> getTripRecords() {
        return tripRecords;
    }

    public void addTripRecord(TripRecord record) {
        this.tripRecords.add(record);
    }

    public List<TopupRecord> getTopupRecords() {
        return topupRecords;
    }

    public void addTopupRecord(TopupRecord record) {
        this.topupRecords.add(record);
    }
} 