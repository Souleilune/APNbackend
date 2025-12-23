const express = require('express');
const { createClient } = require('@supabase/supabase-js');

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
 * POST /api/telemetry/device/pair
 * Pair a hardware device to the authenticated user's account
 */
router.post('/device/pair', authenticateToken, async (req, res) => {
  try {
    const { deviceId, name } = req.body;

    if (!deviceId) {
      return res.status(400).json({
        error: 'Missing required field',
        message: 'Device ID is required'
      });
    }

    // Check if device is already paired
    const { data: existingDevice, error: checkError } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('device_id', deviceId)
      .maybeSingle();

    if (checkError && checkError.code !== 'PGRST116') {
      throw checkError;
    }

    if (existingDevice) {
      if (existingDevice.user_id === req.user.id) {
        return res.status(400).json({
          error: 'Already paired',
          message: 'This device is already paired to your account'
        });
      }
      return res.status(400).json({
        error: 'Device unavailable',
        message: 'This device is paired to another account'
      });
    }

    // Create device pairing
    const { data: device, error: createError } = await supabaseAdmin
      .from('devices')
      .insert({
        user_id: req.user.id,
        device_id: deviceId,
        name: name || `Device ${deviceId.slice(-4)}`,
        is_active: true
      })
      .select()
      .single();

    if (createError) {
      throw createError;
    }

    res.status(201).json({
      message: 'Device paired successfully',
      device: {
        id: device.id,
        deviceId: device.device_id,
        name: device.name,
        pairedAt: device.paired_at,
        isActive: device.is_active
      }
    });

  } catch (error) {
    console.error('Device pairing error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * DELETE /api/telemetry/device/:deviceId
 * Unpair a device from the user's account
 */
router.delete('/device/:deviceId', authenticateToken, async (req, res) => {
  try {
    const { deviceId } = req.params;

    const { data, error } = await supabaseAdmin
      .from('devices')
      .delete()
      .eq('device_id', deviceId)
      .eq('user_id', req.user.id)
      .select()
      .single();

    if (error) {
      if (error.code === 'PGRST116') {
        return res.status(404).json({
          error: 'Not found',
          message: 'Device not found or not owned by you'
        });
      }
      throw error;
    }

    res.json({
      message: 'Device unpaired successfully',
      deviceId
    });

  } catch (error) {
    console.error('Device unpair error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/devices
 * Get all devices paired to the authenticated user
 */
router.get('/devices', authenticateToken, async (req, res) => {
  try {
    const { data: devices, error } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('user_id', req.user.id)
      .order('paired_at', { ascending: false });

    if (error) {
      throw error;
    }

    res.json({
      devices: devices.map(d => ({
        id: d.id,
        deviceId: d.device_id,
        name: d.name,
        pairedAt: d.paired_at,
        isActive: d.is_active
      }))
    });

  } catch (error) {
    console.error('Get devices error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/history
 * Get telemetry history for the authenticated user
 * Query params: limit, offset, deviceId, startDate, endDate
 */
router.get('/history', authenticateToken, async (req, res) => {
  try {
    const { 
      limit = 50, 
      offset = 0, 
      deviceId, 
      startDate, 
      endDate 
    } = req.query;

    let query = supabaseAdmin
      .from('telemetry_logs')
      .select('*', { count: 'exact' })
      .eq('user_id', req.user.id)
      .order('received_at', { ascending: false })
      .range(parseInt(offset), parseInt(offset) + parseInt(limit) - 1);

    // Apply optional filters
    if (deviceId) {
      query = query.eq('device_id', deviceId);
    }

    if (startDate) {
      query = query.gte('received_at', startDate);
    }

    if (endDate) {
      query = query.lte('received_at', endDate);
    }

    const { data: logs, error, count } = await query;

    if (error) {
      throw error;
    }

    res.json({
      logs: logs.map(log => ({
        id: log.id,
        deviceId: log.device_id,
        intakeId: log.intake_id,
        scheduleId: log.schedule_id,
        scheduledDatetime: log.scheduled_datetime,
        dose: log.dose,
        medicineName: log.medicine_name,
        compartmentId: log.compartment_id,
        medicineForm: log.medicine_form,
        receivedAt: log.received_at
      })),
      pagination: {
        total: count,
        limit: parseInt(limit),
        offset: parseInt(offset),
        hasMore: (parseInt(offset) + logs.length) < count
      }
    });

  } catch (error) {
    console.error('Get history error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/schedules
 * Get upcoming medication schedules for the authenticated user
 */
router.get('/schedules', authenticateToken, async (req, res) => {
  try {
    const { limit = 10 } = req.query;
    const now = new Date().toISOString();

    const { data: schedules, error } = await supabaseAdmin
      .from('telemetry_logs')
      .select('*')
      .eq('user_id', req.user.id)
      .gte('scheduled_datetime', now)
      .order('scheduled_datetime', { ascending: true })
      .limit(parseInt(limit));

    if (error) {
      throw error;
    }

    res.json({
      schedules: schedules.map(s => ({
        id: s.id,
        deviceId: s.device_id,
        intakeId: s.intake_id,
        scheduleId: s.schedule_id,
        scheduledDatetime: s.scheduled_datetime,
        dose: s.dose,
        medicineName: s.medicine_name,
        compartmentId: s.compartment_id,
        medicineForm: s.medicine_form
      }))
    });

  } catch (error) {
    console.error('Get schedules error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/stats
 * Get telemetry statistics for the authenticated user
 */
router.get('/stats', authenticateToken, async (req, res) => {
  try {
    // Get device count
    const { count: deviceCount, error: deviceError } = await supabaseAdmin
      .from('devices')
      .select('*', { count: 'exact', head: true })
      .eq('user_id', req.user.id)
      .eq('is_active', true);

    if (deviceError) {
      throw deviceError;
    }

    // Get active alerts count
    const { count: activeAlerts, error: alertError } = await supabaseAdmin
      .from('alerts')
      .select('*', { count: 'exact', head: true })
      .eq('user_id', req.user.id)
      .eq('is_active', true);

    if (alertError) {
      throw alertError;
    }

    // Get most recent sensor reading
    const { data: recentReading, error: recentError } = await supabaseAdmin
      .from('sensor_readings')
      .select('*')
      .eq('user_id', req.user.id)
      .order('received_at', { ascending: false })
      .limit(1)
      .maybeSingle();

    if (recentError) {
      throw recentError;
    }

    res.json({
      stats: {
        activeDevices: deviceCount || 0,
        activeAlerts: activeAlerts || 0,
        lastActivity: recentReading?.received_at || null,
        powerStatus: recentReading?.power_status || null
      }
    });

  } catch (error) {
    console.error('Get stats error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/sensors
 * Get sensor readings history
 * Query params: limit, offset, deviceId, startDate, endDate
 */
router.get('/sensors', authenticateToken, async (req, res) => {
  try {
    const { 
      limit = 50, 
      offset = 0, 
      deviceId, 
      startDate, 
      endDate 
    } = req.query;

    let query = supabaseAdmin
      .from('sensor_readings')
      .select('*', { count: 'exact' })
      .eq('user_id', req.user.id)
      .order('received_at', { ascending: false })
      .range(parseInt(offset), parseInt(offset) + parseInt(limit) - 1);

    if (deviceId) {
      query = query.eq('device_id', deviceId);
    }

    if (startDate) {
      query = query.gte('received_at', startDate);
    }

    if (endDate) {
      query = query.lte('received_at', endDate);
    }

    const { data: readings, error, count } = await query;

    if (error) {
      throw error;
    }

    res.json({
      readings: readings.map(r => ({
        id: r.id,
        deviceId: r.device_id,
        water: [r.water_1, r.water_2, r.water_3, r.water_4],
        gas: r.gas_detected,
        temperature: {
          temp1: r.temp_1,
          temp2: r.temp_2
        },
        gyro: {
          movement: r.movement
        },
        power: r.power_status,
        receivedAt: r.received_at
      })),
      pagination: {
        total: count,
        limit: parseInt(limit),
        offset: parseInt(offset),
        hasMore: (parseInt(offset) + readings.length) < count
      }
    });

  } catch (error) {
    console.error('Get sensors error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/sensors/latest
 * Get the latest sensor reading for each device
 */
router.get('/sensors/latest', authenticateToken, async (req, res) => {
  try {
    const { deviceId } = req.query;

    let query = supabaseAdmin
      .from('sensor_readings')
      .select('*')
      .eq('user_id', req.user.id)
      .order('received_at', { ascending: false })
      .limit(1);

    if (deviceId) {
      query = query.eq('device_id', deviceId);
    }

    const { data: reading, error } = await query.maybeSingle();

    if (error) {
      throw error;
    }

    if (!reading) {
      return res.json({ reading: null });
    }

    res.json({
      reading: {
        id: reading.id,
        deviceId: reading.device_id,
        water: [reading.water_1, reading.water_2, reading.water_3, reading.water_4],
        gas: reading.gas_detected,
        temperature: {
          temp1: reading.temp_1,
          temp2: reading.temp_2
        },
        gyro: {
          movement: reading.movement
        },
        power: reading.power_status,
        receivedAt: reading.received_at
      }
    });

  } catch (error) {
    console.error('Get latest sensor error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/alerts
 * Get alerts for the authenticated user
 * Query params: limit, offset, active (boolean), type
 */
router.get('/alerts', authenticateToken, async (req, res) => {
  try {
    const { 
      limit = 50, 
      offset = 0, 
      active,
      type,
      deviceId 
    } = req.query;

    let query = supabaseAdmin
      .from('alerts')
      .select('*', { count: 'exact' })
      .eq('user_id', req.user.id)
      .order('received_at', { ascending: false })
      .range(parseInt(offset), parseInt(offset) + parseInt(limit) - 1);

    if (active !== undefined) {
      query = query.eq('is_active', active === 'true');
    }

    if (type) {
      query = query.eq('alert_type', type);
    }

    if (deviceId) {
      query = query.eq('device_id', deviceId);
    }

    const { data: alerts, error, count } = await query;

    if (error) {
      throw error;
    }

    res.json({
      alerts: alerts.map(a => ({
        id: a.id,
        deviceId: a.device_id,
        alertType: a.alert_type,
        sensor: a.sensor,
        value: a.value,
        isActive: a.is_active,
        receivedAt: a.received_at,
        clearedAt: a.cleared_at
      })),
      pagination: {
        total: count,
        limit: parseInt(limit),
        offset: parseInt(offset),
        hasMore: (parseInt(offset) + alerts.length) < count
      }
    });

  } catch (error) {
    console.error('Get alerts error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/alerts/active
 * Get only active alerts for the authenticated user
 */
router.get('/alerts/active', authenticateToken, async (req, res) => {
  try {
    const { data: alerts, error } = await supabaseAdmin
      .from('alerts')
      .select('*')
      .eq('user_id', req.user.id)
      .eq('is_active', true)
      .order('received_at', { ascending: false });

    if (error) {
      throw error;
    }

    res.json({
      alerts: alerts.map(a => ({
        id: a.id,
        deviceId: a.device_id,
        alertType: a.alert_type,
        sensor: a.sensor,
        value: a.value,
        receivedAt: a.received_at
      })),
      count: alerts.length
    });

  } catch (error) {
    console.error('Get active alerts error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/telemetry/alerts/:alertId/acknowledge
 * Acknowledge/clear an alert manually
 */
router.post('/alerts/:alertId/acknowledge', authenticateToken, async (req, res) => {
  try {
    const { alertId } = req.params;

    const { data: alert, error } = await supabaseAdmin
      .from('alerts')
      .update({
        is_active: false,
        cleared_at: new Date().toISOString()
      })
      .eq('id', alertId)
      .eq('user_id', req.user.id)
      .select()
      .single();

    if (error) {
      if (error.code === 'PGRST116') {
        return res.status(404).json({
          error: 'Not found',
          message: 'Alert not found or not owned by you'
        });
      }
      throw error;
    }

    res.json({
      message: 'Alert acknowledged',
      alert: {
        id: alert.id,
        alertType: alert.alert_type,
        clearedAt: alert.cleared_at
      }
    });

  } catch (error) {
    console.error('Acknowledge alert error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

module.exports = router;

