-- Migration: Update power_status column to TEXT to store JSON power data
-- This allows storing the full power object with voltage1, voltage2, current1, current2, etc.

ALTER TABLE sensor_readings 
ALTER COLUMN power_status TYPE TEXT;

