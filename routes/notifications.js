const express = require('express');
const { createClient } = require('@supabase/supabase-js');
const { Expo } = require('expo-server-sdk');

const router = express.Router();

// Initialize Supabase clients
const supabase = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_ANON_KEY
);

const supabaseAdmin = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_ROLE_KEY,
  {
    auth: {
      autoRefreshToken: false,
      persistSession: false
    }
  }
);

// Initialize Expo SDK for token validation
const expo = new Expo();

/**
 * Middleware to verify authentication token
 */
const authenticateToken = async (req, res, next) => {
  try {
    const authHeader = req.headers.authorization;
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'No token provided'
      });
    }

    const token = authHeader.substring(7);
    const { data: { user }, error } = await supabase.auth.getUser(token);

    if (error || !user) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'Invalid or expired token'
      });
    }

    // Get user from public.users table
    const { data: userData } = await supabaseAdmin
      .from('users')
      .select('*')
      .eq('auth_id', user.id)
      .maybeSingle();

    req.user = {
      authId: user.id,
      id: userData?.id,
      email: user.email,
    };
    
    next();
  } catch (error) {
    console.error('Auth middleware error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
};

/**
 * POST /api/notifications/register
 * Register or update push notification token for the authenticated user
 */
router.post('/register', authenticateToken, async (req, res) => {
  try {
    const { expoPushToken, deviceId, platform } = req.body;

    if (!expoPushToken) {
      return res.status(400).json({
        error: 'Missing required field',
        message: 'expoPushToken is required'
      });
    }

    // Validate Expo push token format
    if (!Expo.isExpoPushToken(expoPushToken)) {
      return res.status(400).json({
        error: 'Invalid token format',
        message: 'The provided token is not a valid Expo push token'
      });
    }

    const userId = req.user.id;

    // Check if token already exists for this user
    const { data: existingToken, error: checkError } = await supabaseAdmin
      .from('push_tokens')
      .select('*')
      .eq('expo_push_token', expoPushToken)
      .maybeSingle();

    if (checkError && checkError.code !== 'PGRST116') {
      throw checkError;
    }

    if (existingToken) {
      // Update existing token (update user_id if it changed, or device_id/platform)
      const { error: updateError } = await supabaseAdmin
        .from('push_tokens')
        .update({
          user_id: userId,
          device_id: deviceId || existingToken.device_id,
          platform: platform || existingToken.platform,
          updated_at: new Date().toISOString(),
        })
        .eq('id', existingToken.id);

      if (updateError) {
        throw updateError;
      }

      console.log(`✅ Updated push token for user ${userId}`);
      return res.json({
        success: true,
        message: 'Push token updated',
        tokenId: existingToken.id
      });
    }

    // Check if user already has a token (optional: limit to one token per user)
    // For now, we allow multiple tokens per user (multiple devices)
    // Insert new token
    const { data: newToken, error: insertError } = await supabaseAdmin
      .from('push_tokens')
      .insert({
        user_id: userId,
        expo_push_token: expoPushToken,
        device_id: deviceId || null,
        platform: platform || null,
      })
      .select()
      .single();

    if (insertError) {
      throw insertError;
    }

    console.log(`✅ Registered push token for user ${userId}`);
    res.json({
      success: true,
      message: 'Push token registered',
      tokenId: newToken.id
    });
  } catch (error) {
    console.error('❌ Error registering push token:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * DELETE /api/notifications/unregister
 * Remove push notification token for the authenticated user
 */
router.delete('/unregister', authenticateToken, async (req, res) => {
  try {
    const { expoPushToken } = req.body;
    const userId = req.user.id;

    let query = supabaseAdmin
      .from('push_tokens')
      .delete()
      .eq('user_id', userId);

    // If specific token provided, delete only that token
    if (expoPushToken) {
      query = query.eq('expo_push_token', expoPushToken);
    }

    const { error } = await query;

    if (error) {
      throw error;
    }

    console.log(`✅ Unregistered push token(s) for user ${userId}`);
    res.json({
      success: true,
      message: 'Push token(s) unregistered'
    });
  } catch (error) {
    console.error('❌ Error unregistering push token:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/notifications/tokens
 * Get all push tokens for the authenticated user (for debugging)
 */
router.get('/tokens', authenticateToken, async (req, res) => {
  try {
    const userId = req.user.id;

    const { data: tokens, error } = await supabaseAdmin
      .from('push_tokens')
      .select('id, expo_push_token, device_id, platform, created_at, updated_at')
      .eq('user_id', userId);

    if (error) {
      throw error;
    }

    res.json({
      success: true,
      tokens: tokens || [],
      count: tokens?.length || 0
    });
  } catch (error) {
    console.error('❌ Error fetching push tokens:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

module.exports = router;

