-- ============================================
-- Migration: Add location column to sockets table
-- ============================================
-- This migration adds the missing 'location' column to the sockets table
-- Run this SQL in your Supabase SQL Editor

-- Add location column to sockets table
ALTER TABLE public.sockets 
ADD COLUMN IF NOT EXISTS location VARCHAR(255);

-- Add a comment to document the column
COMMENT ON COLUMN public.sockets.location IS 'Optional location description for the socket';

