-- ============================================
-- APN Telemetry Database Schema
-- Run these SQL commands in Supabase SQL Editor
-- ============================================

-- Enable UUID extension if not already enabled
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ============================================
-- DEVICES TABLE
-- Stores hardware devices paired to user accounts
-- ============================================
CREATE TABLE IF NOT EXISTS devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    device_id VARCHAR(100) UNIQUE NOT NULL,
    name VARCHAR(255),
    paired_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    is_active BOOLEAN DEFAULT true,
    
    -- Timestamps
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for faster lookups by device_id
CREATE INDEX IF NOT EXISTS idx_devices_device_id ON devices(device_id);

-- Index for faster lookups by user_id
CREATE INDEX IF NOT EXISTS idx_devices_user_id ON devices(user_id);

-- Index for active devices lookup
CREATE INDEX IF NOT EXISTS idx_devices_active ON devices(device_id, is_active) WHERE is_active = true;

-- ============================================
-- SENSOR_READINGS TABLE
-- Stores periodic sensor data from ESP32
-- ============================================
CREATE TABLE IF NOT EXISTS sensor_readings (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(100) NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    
    -- Water sensors (array of 4 readings)
    water_1 INTEGER,
    water_2 INTEGER,
    water_3 INTEGER,
    water_4 INTEGER,
    
    -- Gas detection
    gas_detected BOOLEAN DEFAULT false,
    
    -- Temperature sensors
    temp_1 DECIMAL(5,2),
    temp_2 DECIMAL(5,2),
    
    -- Gyroscope / Movement
    movement DECIMAL(5,2),
    
    -- Power status
    power_status VARCHAR(20), -- 'MAIN' or 'BACKUP_UPS'
    
    -- Metadata
    received_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for faster lookups by device_id
CREATE INDEX IF NOT EXISTS idx_sensor_device_id ON sensor_readings(device_id);

-- Index for faster lookups by user_id
CREATE INDEX IF NOT EXISTS idx_sensor_user_id ON sensor_readings(user_id);

-- Index for time-based queries
CREATE INDEX IF NOT EXISTS idx_sensor_received_at ON sensor_readings(received_at DESC);

-- Composite index for user + time queries
CREATE INDEX IF NOT EXISTS idx_sensor_user_time ON sensor_readings(user_id, received_at DESC);

-- ============================================
-- ALERTS TABLE
-- Stores alert notifications from ESP32
-- ============================================
CREATE TABLE IF NOT EXISTS alerts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(100) NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    
    -- Alert information
    alert_type VARCHAR(50) NOT NULL, -- 'WATER_DETECTED', 'GAS_LEAK_DETECTED', 'HIGH_TEMPERATURE', 'GROUND_MOVEMENT_DETECTED'
    sensor VARCHAR(50),              -- e.g., 'WATER3' for water alerts
    value DECIMAL(10,2),             -- e.g., temperature value or movement intensity
    
    -- Status
    is_active BOOLEAN DEFAULT true,
    cleared_at TIMESTAMP WITH TIME ZONE,
    
    -- Metadata
    received_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for faster lookups
CREATE INDEX IF NOT EXISTS idx_alerts_device_id ON alerts(device_id);
CREATE INDEX IF NOT EXISTS idx_alerts_user_id ON alerts(user_id);
CREATE INDEX IF NOT EXISTS idx_alerts_active ON alerts(user_id, is_active) WHERE is_active = true;
CREATE INDEX IF NOT EXISTS idx_alerts_type ON alerts(alert_type);
CREATE INDEX IF NOT EXISTS idx_alerts_received_at ON alerts(received_at DESC);

-- ============================================
-- TELEMETRY_LOGS TABLE (Legacy - for medication data if needed)
-- Stores telemetry data received from hardware devices
-- ============================================
CREATE TABLE IF NOT EXISTS telemetry_logs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(100) NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    
    -- Telemetry payload fields
    intake_id VARCHAR(100),
    schedule_id VARCHAR(100),
    scheduled_datetime TIMESTAMP WITH TIME ZONE,
    dose VARCHAR(100),
    medicine_name VARCHAR(255),
    compartment_id VARCHAR(100),
    medicine_form VARCHAR(100),
    
    -- Metadata
    received_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    -- Timestamps
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
);

-- Index for faster lookups by device_id
CREATE INDEX IF NOT EXISTS idx_telemetry_device_id ON telemetry_logs(device_id);

-- Index for faster lookups by user_id
CREATE INDEX IF NOT EXISTS idx_telemetry_user_id ON telemetry_logs(user_id);

-- Index for time-based queries
CREATE INDEX IF NOT EXISTS idx_telemetry_received_at ON telemetry_logs(received_at DESC);

-- ============================================
-- ROW LEVEL SECURITY (RLS) POLICIES
-- ============================================

-- Enable RLS on devices table
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;

-- Policy: Users can only see their own devices
CREATE POLICY "Users can view own devices" ON devices
    FOR SELECT
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = devices.user_id
    ));

-- Policy: Users can insert their own devices
CREATE POLICY "Users can insert own devices" ON devices
    FOR INSERT
    WITH CHECK (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = devices.user_id
    ));

-- Policy: Users can update their own devices
CREATE POLICY "Users can update own devices" ON devices
    FOR UPDATE
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = devices.user_id
    ));

-- Policy: Users can delete their own devices
CREATE POLICY "Users can delete own devices" ON devices
    FOR DELETE
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = devices.user_id
    ));

-- Enable RLS on sensor_readings table
ALTER TABLE sensor_readings ENABLE ROW LEVEL SECURITY;

-- Policy: Users can only see their own sensor readings
CREATE POLICY "Users can view own sensor readings" ON sensor_readings
    FOR SELECT
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = sensor_readings.user_id
    ));

-- Policy: Service role can insert sensor readings (for backend)
CREATE POLICY "Service role can insert sensor readings" ON sensor_readings
    FOR INSERT
    WITH CHECK (true);

-- Enable RLS on alerts table
ALTER TABLE alerts ENABLE ROW LEVEL SECURITY;

-- Policy: Users can only see their own alerts
CREATE POLICY "Users can view own alerts" ON alerts
    FOR SELECT
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = alerts.user_id
    ));

-- Policy: Service role can insert/update alerts (for backend)
CREATE POLICY "Service role can insert alerts" ON alerts
    FOR INSERT
    WITH CHECK (true);

CREATE POLICY "Service role can update alerts" ON alerts
    FOR UPDATE
    USING (true);

-- Enable RLS on telemetry_logs table
ALTER TABLE telemetry_logs ENABLE ROW LEVEL SECURITY;

-- Policy: Users can only see their own telemetry
CREATE POLICY "Users can view own telemetry" ON telemetry_logs
    FOR SELECT
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = telemetry_logs.user_id
    ));

-- Policy: Service role can insert telemetry (for backend)
CREATE POLICY "Service role can insert telemetry" ON telemetry_logs
    FOR INSERT
    WITH CHECK (true);

-- ============================================
-- HELPER FUNCTIONS
-- ============================================

-- Function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Trigger for devices table
DROP TRIGGER IF EXISTS update_devices_updated_at ON devices;
CREATE TRIGGER update_devices_updated_at
    BEFORE UPDATE ON devices
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();
