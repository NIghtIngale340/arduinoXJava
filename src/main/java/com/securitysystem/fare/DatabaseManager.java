package com.securitysystem.fare;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;

public class DatabaseManager {
    private static final String DB_NAME = "fare_system.db";
    private static final String DB_URL = "jdbc:sqlite:" + DB_NAME;
    private Connection connection;

    public DatabaseManager() {
        initializeDatabase();
    }

    private void initializeDatabase() {
        try {
            connection = DriverManager.getConnection(DB_URL);
            createTables();
            System.out.println("Database initialized successfully");
        } catch (SQLException e) {
            System.err.println("Error initializing database: " + e.getMessage());
        }
    }

    private void createTables() throws SQLException {
        // Create system state table
        String createSystemStateTable = """
            CREATE TABLE IF NOT EXISTS system_state (
                id INTEGER PRIMARY KEY,
                beep_balance INTEGER NOT NULL,
                single_balance INTEGER NOT NULL,
                current_line INTEGER NOT NULL,
                current_station INTEGER NOT NULL,
                current_station_name TEXT NOT NULL,
                current_user TEXT,
                current_datetime TEXT,
                total_distance INTEGER NOT NULL
            )
        """;

        // Create trip records table
        String createTripRecordsTable = """
            CREATE TABLE IF NOT EXISTS trip_records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                line INTEGER NOT NULL,
                origin INTEGER NOT NULL,
                destination INTEGER NOT NULL,
                fare INTEGER NOT NULL,
                distance INTEGER NOT NULL,
                datetime TEXT NOT NULL,
                ticket_type TEXT NOT NULL,
                card_type TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """;

        // Create topup records table
        String createTopupRecordsTable = """
            CREATE TABLE IF NOT EXISTS topup_records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                card_type TEXT NOT NULL,
                amount INTEGER NOT NULL,
                new_balance INTEGER NOT NULL,
                datetime TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """;

        try (Statement stmt = connection.createStatement()) {
            stmt.execute(createSystemStateTable);
            stmt.execute(createTripRecordsTable);
            stmt.execute(createTopupRecordsTable);
            
            // Initialize system state if it doesn't exist
            initializeSystemState();
        }
    }

    private void initializeSystemState() throws SQLException {
        String checkQuery = "SELECT COUNT(*) FROM system_state";
        try (Statement stmt = connection.createStatement();
             ResultSet rs = stmt.executeQuery(checkQuery)) {
            
            if (rs.next() && rs.getInt(1) == 0) {
                // Insert default system state
                String insertQuery = """
                    INSERT INTO system_state 
                    (id, beep_balance, single_balance, current_line, current_station, 
                     current_station_name, current_user, current_datetime, total_distance)
                    VALUES (1, 0, 0, 0, 0, 'None', '', '', 0)
                """;
                stmt.execute(insertQuery);
            }
        }
    }

    public FareSystemState loadSystemState() {
        FareSystemState state = new FareSystemState();
        
        String query = "SELECT * FROM system_state WHERE id = 1";
        try (PreparedStatement pstmt = connection.prepareStatement(query);
             ResultSet rs = pstmt.executeQuery()) {
            
            if (rs.next()) {
                state.setBeepBalance(rs.getInt("beep_balance"));
                state.setSingleBalance(rs.getInt("single_balance"));
                state.setCurrentLine(rs.getInt("current_line"));
                state.setCurrentStation(rs.getInt("current_station"));
                state.setCurrentStationName(rs.getString("current_station_name"));
                state.setCurrentUser(rs.getString("current_user"));
                state.setCurrentDateTime(rs.getString("current_datetime"));
                state.setTotalDistance(rs.getInt("total_distance"));
            }
        } catch (SQLException e) {
            System.err.println("Error loading system state: " + e.getMessage());
        }
        
        // Load trip records
        List<TripRecord> tripRecords = loadTripRecords();
        for (TripRecord record : tripRecords) {
            state.addTripRecord(record);
        }
        
        // Load topup records
        List<TopupRecord> topupRecords = loadTopupRecords();
        for (TopupRecord record : topupRecords) {
            state.addTopupRecord(record);
        }
        
        return state;
    }

    public void saveSystemState(FareSystemState state) {
        String query = """
            UPDATE system_state SET 
                beep_balance = ?, single_balance = ?, current_line = ?, 
                current_station = ?, current_station_name = ?, current_user = ?, 
                current_datetime = ?, total_distance = ?
            WHERE id = 1
        """;
        
        try (PreparedStatement pstmt = connection.prepareStatement(query)) {
            pstmt.setInt(1, state.getBeepBalance());
            pstmt.setInt(2, state.getSingleBalance());
            pstmt.setInt(3, state.getCurrentLine());
            pstmt.setInt(4, state.getCurrentStation());
            pstmt.setString(5, state.getCurrentStationName());
            pstmt.setString(6, state.getCurrentUser());
            pstmt.setString(7, state.getCurrentDateTime());
            pstmt.setInt(8, state.getTotalDistance());
            
            pstmt.executeUpdate();
        } catch (SQLException e) {
            System.err.println("Error saving system state: " + e.getMessage());
        }
    }

    public void saveTripRecord(TripRecord record) {
        String query = """
            INSERT INTO trip_records 
            (line, origin, destination, fare, distance, datetime, ticket_type, card_type)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """;
        
        try (PreparedStatement pstmt = connection.prepareStatement(query)) {
            pstmt.setInt(1, record.getLine());
            pstmt.setInt(2, record.getOrigin());
            pstmt.setInt(3, record.getDestination());
            pstmt.setInt(4, record.getFare());
            pstmt.setInt(5, record.getDistance());
            pstmt.setString(6, record.getDatetime());
            pstmt.setString(7, record.getTicketType());
            pstmt.setString(8, record.getCardType());
            
            pstmt.executeUpdate();
        } catch (SQLException e) {
            System.err.println("Error saving trip record: " + e.getMessage());
        }
    }

    public void saveTopupRecord(TopupRecord record) {
        String query = """
            INSERT INTO topup_records 
            (card_type, amount, new_balance, datetime)
            VALUES (?, ?, ?, ?)
        """;
        
        try (PreparedStatement pstmt = connection.prepareStatement(query)) {
            pstmt.setString(1, record.getCardType());
            pstmt.setInt(2, record.getAmount());
            pstmt.setInt(3, record.getNewBalance());
            pstmt.setString(4, record.getDatetime());
            
            pstmt.executeUpdate();
        } catch (SQLException e) {
            System.err.println("Error saving topup record: " + e.getMessage());
        }
    }

    public void deleteTripRecord(TripRecord record) {
        String query = """
            DELETE FROM trip_records WHERE 
            line = ? AND origin = ? AND destination = ? AND 
            fare = ? AND distance = ? AND datetime = ? AND 
            ticket_type = ? AND card_type = ?
        """;
        
        try (PreparedStatement pstmt = connection.prepareStatement(query)) {
            pstmt.setInt(1, record.getLine());
            pstmt.setInt(2, record.getOrigin());
            pstmt.setInt(3, record.getDestination());
            pstmt.setInt(4, record.getFare());
            pstmt.setInt(5, record.getDistance());
            pstmt.setString(6, record.getDatetime());
            pstmt.setString(7, record.getTicketType());
            pstmt.setString(8, record.getCardType());
            
            int deleted = pstmt.executeUpdate();
            if (deleted > 0) {
                System.out.println("Trip record deleted from database");
            }
        } catch (SQLException e) {
            System.err.println("Error deleting trip record: " + e.getMessage());
        }
    }

    public List<TripRecord> loadTripRecords() {
        List<TripRecord> records = new ArrayList<>();
        String query = "SELECT * FROM trip_records ORDER BY created_at DESC";
        
        try (PreparedStatement pstmt = connection.prepareStatement(query);
             ResultSet rs = pstmt.executeQuery()) {
            
            while (rs.next()) {
                TripRecord record = new TripRecord(
                    rs.getInt("line"),
                    rs.getInt("origin"),
                    rs.getInt("destination"),
                    rs.getInt("fare"),
                    rs.getInt("distance"),
                    rs.getString("datetime"),
                    rs.getString("ticket_type"),
                    rs.getString("card_type")
                );
                records.add(record);
            }
        } catch (SQLException e) {
            System.err.println("Error loading trip records: " + e.getMessage());
        }
        
        return records;
    }

    public List<TopupRecord> loadTopupRecords() {
        List<TopupRecord> records = new ArrayList<>();
        String query = "SELECT * FROM topup_records ORDER BY created_at DESC";
        
        try (PreparedStatement pstmt = connection.prepareStatement(query);
             ResultSet rs = pstmt.executeQuery()) {
            
            while (rs.next()) {
                TopupRecord record = new TopupRecord(
                    rs.getString("card_type"),
                    rs.getInt("amount"),
                    rs.getInt("new_balance"),
                    rs.getString("datetime")
                );
                records.add(record);
            }
        } catch (SQLException e) {
            System.err.println("Error loading topup records: " + e.getMessage());
        }
        
        return records;
    }

    public void clearAllData() {
        try (Statement stmt = connection.createStatement()) {
            stmt.execute("DELETE FROM trip_records");
            stmt.execute("DELETE FROM topup_records");
            stmt.execute("UPDATE system_state SET beep_balance = 0, single_balance = 0, current_line = 0, current_station = 0, current_station_name = 'None', current_user = '', current_datetime = '', total_distance = 0 WHERE id = 1");
            System.out.println("All data cleared from database");
        } catch (SQLException e) {
            System.err.println("Error clearing data: " + e.getMessage());
        }
    }

    public void closeConnection() {
        if (connection != null) {
            try {
                connection.close();
                System.out.println("Database connection closed");
            } catch (SQLException e) {
                System.err.println("Error closing database connection: " + e.getMessage());
            }
        }
    }

    // Get database statistics
    public String getDatabaseStats() {
        StringBuilder stats = new StringBuilder();
        
        try (Statement stmt = connection.createStatement()) {
            // Count trip records
            ResultSet rs = stmt.executeQuery("SELECT COUNT(*) FROM trip_records");
            if (rs.next()) {
                stats.append("Trip Records: ").append(rs.getInt(1)).append("\n");
            }
            
            // Count topup records
            rs = stmt.executeQuery("SELECT COUNT(*) FROM topup_records");
            if (rs.next()) {
                stats.append("Topup Records: ").append(rs.getInt(1)).append("\n");
            }
            
            // Get total fare collected
            rs = stmt.executeQuery("SELECT SUM(fare) FROM trip_records");
            if (rs.next()) {
                int totalFare = rs.getInt(1);
                stats.append("Total Fare Collected: ₱").append(totalFare).append("\n");
            }
            
            // Get total topup amount
            rs = stmt.executeQuery("SELECT SUM(amount) FROM topup_records");
            if (rs.next()) {
                int totalTopup = rs.getInt(1);
                stats.append("Total Topup Amount: ₱").append(totalTopup);
            }
            
        } catch (SQLException e) {
            stats.append("Error retrieving statistics: ").append(e.getMessage());
        }
        
        return stats.toString();
    }
}
