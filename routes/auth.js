const express = require('express');
const { createClient } = require('@supabase/supabase-js');

const router = express.Router();

// Initialize Supabase client with anon key for auth operations
const supabase = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_ANON_KEY
);

// Initialize admin client for data operations
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

// Validation helper
const validateEmail = (email) => {
  const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  return re.test(email);
};

const validatePassword = (password) => {
  return password && password.length >= 6;
};

/**
 * POST /api/auth/register
 * Register a new user
 */
router.post('/register', async (req, res) => {
  try {
    const { email, password, fullName } = req.body;

    // Validation
    if (!email || !password) {
      return res.status(400).json({
        error: 'Missing required fields',
        message: 'Email and password are required'
      });
    }

    if (!validateEmail(email)) {
      return res.status(400).json({
        error: 'Invalid email',
        message: 'Please provide a valid email address'
      });
    }

    if (!validatePassword(password)) {
      return res.status(400).json({
        error: 'Invalid password',
        message: 'Password must be at least 6 characters long'
      });
    }

    // ✅ Use Admin API to create user with auto-confirmation
    const { data: authData, error: authError } = await supabaseAdmin.auth.admin.createUser({
      email,
      password,
      email_confirm: true,  // 👈 Auto-confirm email
      user_metadata: {
        full_name: fullName || null
      }
    });

    if (authError) {
      console.error('Auth error:', authError);
      
      if (authError.message.includes('already registered') || 
          authError.message.includes('already exists')) {
        return res.status(400).json({
          error: 'User already exists',
          message: 'An account with this email already exists'
        });
      }
      
      return res.status(400).json({
        error: 'Registration failed',
        message: authError.message
      });
    }

    if (!authData.user) {
      return res.status(500).json({
        error: 'Registration failed',
        message: 'User creation failed'
      });
    }

    // Wait a moment for the trigger to complete
    await new Promise(resolve => setTimeout(resolve, 500));

    // Fetch the created profile
    const { data: userData, error: userError } = await supabaseAdmin
      .from('users')
      .select('*')
      .eq('auth_id', authData.user.id)
      .maybeSingle();

    res.status(201).json({
      message: 'User registered successfully',
      user: {
        id: userData?.id || authData.user.id,
        email: authData.user.email,
        fullName: userData?.full_name || fullName,
        createdAt: userData?.created_at || new Date().toISOString()
      }
    });

  } catch (error) {
    console.error('Register error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/auth/login
 * Sign in an existing user
 */
router.post('/login', async (req, res) => {
  try {
    const { email, password } = req.body;

    // Validation
    if (!email || !password) {
      return res.status(400).json({
        error: 'Missing required fields',
        message: 'Email and password are required'
      });
    }

    if (!validateEmail(email)) {
      return res.status(400).json({
        error: 'Invalid email',
        message: 'Please provide a valid email address'
      });
    }

    // Sign in with Supabase
    const { data: authData, error: authError } = await supabase.auth.signInWithPassword({
      email,
      password
    });

    if (authError) {
      console.error('Login error:', authError);
      return res.status(401).json({
        error: 'Authentication failed',
        message: 'Invalid email or password'
      });
    }

    // Get user profile from public.users table
    const { data: userData, error: userError } = await supabaseAdmin
      .from('users')
      .select('*')
      .eq('auth_id', authData.user.id)
      .maybeSingle();

    if (userError) {
      console.error('User profile fetch error:', userError);
    }

    res.json({
      message: 'Login successful',
      session: {
        accessToken: authData.session.access_token,
        refreshToken: authData.session.refresh_token,
        expiresIn: authData.session.expires_in,
        expiresAt: authData.session.expires_at
      },
      user: {
        id: userData?.id || authData.user.id,
        email: authData.user.email,
        fullName: userData?.full_name || authData.user.user_metadata?.full_name,
        createdAt: userData?.created_at
      }
    });

  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/auth/logout
 * Sign out the current user
 */
router.post('/logout', async (req, res) => {
  try {
    const authHeader = req.headers.authorization;
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'No token provided'
      });
    }

    const token = authHeader.substring(7);

    // Sign out from Supabase
    const { error } = await supabaseAdmin.auth.admin.signOut(token);

    if (error) {
      console.error('Logout error:', error);
      return res.status(500).json({
        error: 'Logout failed',
        message: error.message
      });
    }

    res.json({
      message: 'Logout successful'
    });

  } catch (error) {
    console.error('Logout error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/auth/me
 * Get current user profile
 */
router.get('/me', async (req, res) => {
  try {
    const authHeader = req.headers.authorization;
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'No token provided'
      });
    }

    const token = authHeader.substring(7);

    // Verify token and get user
    const { data: { user }, error: authError } = await supabase.auth.getUser(token);

    if (authError || !user) {
      return res.status(401).json({
        error: 'Unauthorized',
        message: 'Invalid or expired token'
      });
    }

    // Get user profile from public.users table
    const { data: userData, error: userError } = await supabaseAdmin
      .from('users')
      .select('*')
      .eq('auth_id', user.id)
      .maybeSingle();

    if (userError) {
      console.error('User profile fetch error:', userError);
    }

    res.json({
      user: {
        id: userData?.id || user.id,
        email: user.email,
        fullName: userData?.full_name || user.user_metadata?.full_name,
        createdAt: userData?.created_at
      }
    });

  } catch (error) {
    console.error('Get user error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/auth/refresh
 * Refresh access token
 */
router.post('/refresh', async (req, res) => {
  try {
    const { refreshToken } = req.body;

    if (!refreshToken) {
      return res.status(400).json({
        error: 'Missing refresh token',
        message: 'Refresh token is required'
      });
    }

    const { data, error } = await supabase.auth.refreshSession({ 
      refresh_token: refreshToken 
    });

    if (error) {
      return res.status(401).json({
        error: 'Token refresh failed',
        message: error.message
      });
    }

    res.json({
      message: 'Token refreshed successfully',
      session: {
        accessToken: data.session.access_token,
        refreshToken: data.session.refresh_token,
        expiresIn: data.session.expires_in,
        expiresAt: data.session.expires_at
      }
    });

  } catch (error) {
    console.error('Refresh token error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * DELETE /api/auth/cleanup/:email
 * DEVELOPMENT ONLY - Delete user from both auth and public tables
 */
router.delete('/cleanup/:email', async (req, res) => {
  try {
    // Only allow in development
    if (process.env.NODE_ENV === 'production') {
      return res.status(403).json({
        error: 'Forbidden',
        message: 'This endpoint is only available in development'
      });
    }

    const { email } = req.params;

    if (!email || !validateEmail(email)) {
      return res.status(400).json({
        error: 'Invalid email',
        message: 'Please provide a valid email address'
      });
    }

    // Get user from public.users table
    const { data: userData } = await supabaseAdmin
      .from('users')
      .select('auth_id')
      .eq('email', email)
      .maybeSingle();

    // Delete from public.users table
    await supabaseAdmin
      .from('users')
      .delete()
      .eq('email', email);

    // Delete from auth.users if we found the auth_id
    if (userData?.auth_id) {
      await supabaseAdmin.auth.admin.deleteUser(userData.auth_id);
    } else {
      // Try to find and delete from auth by email
      const { data: { users } } = await supabaseAdmin.auth.admin.listUsers();
      const authUser = users.find(u => u.email === email);
      
      if (authUser) {
        await supabaseAdmin.auth.admin.deleteUser(authUser.id);
      }
    }

    res.json({
      message: 'User cleaned up successfully',
      email
    });

  } catch (error) {
    console.error('Cleanup error:', error);
    res.status(500).json({
      error: 'Cleanup failed',
      message: error.message
    });
  }
});

module.exports = router;