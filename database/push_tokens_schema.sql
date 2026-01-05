-- ============================================
-- PUSH_TOKENS TABLE
-- Stores Expo push notification tokens for users
-- ============================================
CREATE TABLE IF NOT EXISTS push_tokens (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    expo_push_token TEXT NOT NULL,
    device_id VARCHAR(100), -- Optional device identifier
    platform VARCHAR(20), -- 'ios' or 'android'
    
    -- Timestamps
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    
    -- Ensure one token per user (can be updated)
    UNIQUE(expo_push_token)
);

-- Index for faster lookups by user_id
CREATE INDEX IF NOT EXISTS idx_push_tokens_user_id ON push_tokens(user_id);

-- Index for faster lookups by expo_push_token
CREATE INDEX IF NOT EXISTS idx_push_tokens_token ON push_tokens(expo_push_token);

-- Composite index for user + token lookups
CREATE INDEX IF NOT EXISTS idx_push_tokens_user_token ON push_tokens(user_id, expo_push_token);

-- ============================================
-- ROW LEVEL SECURITY (RLS) POLICIES
-- ============================================

-- Enable RLS on push_tokens table
ALTER TABLE push_tokens ENABLE ROW LEVEL SECURITY;

-- Policy: Users can view their own push tokens
CREATE POLICY "Users can view own push tokens" ON push_tokens
    FOR SELECT
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = push_tokens.user_id
    ));

-- Policy: Users can insert their own push tokens
CREATE POLICY "Users can insert own push tokens" ON push_tokens
    FOR INSERT
    WITH CHECK (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = push_tokens.user_id
    ));

-- Policy: Users can update their own push tokens
CREATE POLICY "Users can update own push tokens" ON push_tokens
    FOR UPDATE
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = push_tokens.user_id
    ));

-- Policy: Users can delete their own push tokens
CREATE POLICY "Users can delete own push tokens" ON push_tokens
    FOR DELETE
    USING (auth.uid() IN (
        SELECT auth_id FROM users WHERE id = push_tokens.user_id
    ));

-- Policy: Service role can manage all push tokens (for backend operations)
CREATE POLICY "Service role can manage push tokens" ON push_tokens
    FOR ALL
    USING (true)
    WITH CHECK (true);

-- ============================================
-- HELPER FUNCTIONS
-- ============================================

-- Trigger for updating updated_at timestamp
DROP TRIGGER IF EXISTS update_push_tokens_updated_at ON push_tokens;
CREATE TRIGGER update_push_tokens_updated_at
    BEFORE UPDATE ON push_tokens
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

