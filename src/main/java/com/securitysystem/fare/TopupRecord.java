package com.securitysystem.fare;

public class TopupRecord {
    private String cardType;
    private int amount;
    private int newBalance;
    private String datetime;

    public TopupRecord(String cardType, int amount, int newBalance, String datetime) {
        this.cardType = cardType;
        this.amount = amount;
        this.newBalance = newBalance;
        this.datetime = datetime;
    }

    public String getCardType() {
        return cardType;
    }

    public int getAmount() {
        return amount;
    }

    public int getNewBalance() {
        return newBalance;
    }

    public String getDatetime() {
        return datetime;
    }

    @Override
    public String toString() {
        return String.format("%s: +%d PHP, New Balance: %d PHP", 
            cardType, amount, newBalance);
    }
} 