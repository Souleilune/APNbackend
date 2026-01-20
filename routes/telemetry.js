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

    console.log(`🔗 Pairing request: deviceId=${deviceId}, userId=${req.user.id}, name=${name || 'not provided'}`);

    // Check if device is already paired
    const { data: existingDevice, error: checkError } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('device_id', deviceId)
      .maybeSingle();

    console.log(`🔍 Device lookup result: ${existingDevice ? `Found (userId: ${existingDevice.user_id})` : 'Not found'}`);

    if (checkError && checkError.code !== 'PGRST116') {
      throw checkError;
    }

    if (existingDevice) {
      if (existingDevice.user_id === req.user.id) {
        // Device already paired to this user - update it if needed and return success
        const updateData = {};
        if (name && name !== existingDevice.name) {
          updateData.name = name;
        }
        // Ensure device is active
        if (!existingDevice.is_active) {
          updateData.is_active = true;
        }

        // Update device if needed
        if (Object.keys(updateData).length > 0) {
          const { data: updatedDevice, error: updateError } = await supabaseAdmin
            .from('devices')
            .update(updateData)
            .eq('id', existingDevice.id)
            .select()
            .single();

          if (updateError) {
            throw updateError;
          }

          return res.status(200).json({
            message: 'Device already paired - updated successfully',
            device: {
              id: updatedDevice.id,
              deviceId: updatedDevice.device_id,
              name: updatedDevice.name,
              pairedAt: updatedDevice.paired_at,
              isActive: updatedDevice.is_active
            }
          });
        }

        // Device exists and is already correctly configured
        return res.status(200).json({
          message: 'Device already paired to your account',
          device: {
            id: existingDevice.id,
            deviceId: existingDevice.device_id,
            name: existingDevice.name,
            pairedAt: existingDevice.paired_at,
            isActive: existingDevice.is_active
          }
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
        // Convert stored 0/1 values to zone-based booleans
        // Zone 1: water_1 (or water_2, they're the same), Zone 2: water_3 (or water_4, they're the same)
        // Return as array for compatibility: [zone1, zone1, zone2, zone2]
        water: [
          r.water_1 === 1 ? 1 : 0, // Zone 1 detection
          r.water_1 === 1 ? 1 : 0, // Zone 1 detection (duplicate for array format)
          r.water_3 === 1 ? 1 : 0, // Zone 2 detection
          r.water_3 === 1 ? 1 : 0  // Zone 2 detection (duplicate for array format)
        ],
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
        // Convert stored 0/1 values to zone-based booleans
        // Zone 1: water_1 (or water_2, they're the same), Zone 2: water_3 (or water_4, they're the same)
        // Return as array for compatibility: [zone1, zone1, zone2, zone2]
        water: [
          reading.water_1 === 1 ? 1 : 0, // Zone 1 detection
          reading.water_1 === 1 ? 1 : 0, // Zone 1 detection (duplicate for array format)
          reading.water_3 === 1 ? 1 : 0, // Zone 2 detection
          reading.water_3 === 1 ? 1 : 0  // Zone 2 detection (duplicate for array format)
        ],
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

/**
 * POST /api/telemetry/alerts/:alertId/archive
 * Archive an alert by moving it to archived_alerts table
 */
router.post('/alerts/:alertId/archive', authenticateToken, async (req, res) => {
  try {
    const { alertId } = req.params;

    // First, get the alert to verify ownership and get all data
    const { data: alert, error: fetchError } = await supabaseAdmin
      .from('alerts')
      .select('*')
      .eq('id', alertId)
      .eq('user_id', req.user.id)
      .single();

    if (fetchError) {
      if (fetchError.code === 'PGRST116') {
        return res.status(404).json({
          error: 'Not found',
          message: 'Alert not found or not owned by you'
        });
      }
      throw fetchError;
    }

    // Insert into archived_alerts table
    const { data: archivedAlert, error: archiveError } = await supabaseAdmin
      .from('archived_alerts')
      .insert({
        id: alert.id,
        device_id: alert.device_id,
        user_id: alert.user_id,
        alert_type: alert.alert_type,
        sensor: alert.sensor,
        value: alert.value,
        is_active: alert.is_active,
        cleared_at: alert.cleared_at,
        received_at: alert.received_at,
        archived_at: new Date().toISOString()
      })
      .select()
      .single();

    if (archiveError) {
      console.error('Error archiving alert:', archiveError);
      throw archiveError;
    }

    // Delete from alerts table
    const { error: deleteError } = await supabaseAdmin
      .from('alerts')
      .delete()
      .eq('id', alertId)
      .eq('user_id', req.user.id);

    if (deleteError) {
      console.error('Error deleting alert after archiving:', deleteError);
      // Try to clean up the archived alert if deletion fails
      await supabaseAdmin
        .from('archived_alerts')
        .delete()
        .eq('id', alertId);
      throw deleteError;
    }

    res.json({
      message: 'Alert archived successfully',
      alert: {
        id: archivedAlert.id,
        alertType: archivedAlert.alert_type,
        archivedAt: archivedAlert.archived_at
      }
    });

  } catch (error) {
    console.error('Archive alert error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/telemetry/sockets/scan
 * Scan for sensors in a socket
 */
router.post('/sockets/scan', authenticateToken, async (req, res) => {
  try {
    const { socketName } = req.body;

    if (!socketName) {
      return res.status(400).json({
        error: 'Missing required field',
        message: 'Socket name is required'
      });
    }

    console.log(`🔍 Scanning for sensors in socket: ${socketName}, userId: ${req.user.id}`);

    // Get all active devices for this user
    const { data: devices, error: devicesError } = await supabaseAdmin
      .from('devices')
      .select('*')
      .eq('user_id', req.user.id)
      .eq('is_active', true);

    if (devicesError) {
      throw devicesError;
    }

    // Get latest sensor readings for each device to check if sensors are active
    const sensors = [];
    
    for (const device of devices || []) {
      const { data: latestReading, error: readingError } = await supabaseAdmin
        .from('sensor_readings')
        .select('*')
        .eq('device_id', device.device_id)
        .order('received_at', { ascending: false })
        .limit(1)
        .maybeSingle();

      if (readingError && readingError.code !== 'PGRST116') {
        console.error(`Error fetching reading for device ${device.device_id}:`, readingError);
        continue;
      }

      // If we have a recent reading (within last 5 minutes), consider sensors active
      if (latestReading) {
        const readingTime = new Date(latestReading.received_at);
        const now = new Date();
        const minutesSinceReading = (now - readingTime) / (1000 * 60);

        if (minutesSinceReading <= 5) {
          sensors.push({
            id: device.id,
            deviceId: device.device_id,
            name: device.name || `Device ${device.device_id.slice(-4)}`,
            type: 'ESP32 Sensor Device',
          });
        }
      }
    }

    res.json({
      sensors: sensors,
      count: sensors.length
    });

  } catch (error) {
    console.error('Scan sensors error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * GET /api/telemetry/sockets
 * Get all sockets for the authenticated user
 */
router.get('/sockets', authenticateToken, async (req, res) => {
  try {
    const { data: sockets, error } = await supabaseAdmin
      .from('sockets')
      .select(`
        *,
        socket_devices (
          device_id,
          devices (
            id,
            device_id,
            name
          )
        )
      `)
      .eq('user_id', req.user.id)
      .order('created_at', { ascending: false });

    if (error) {
      throw error;
    }

    // Process sockets and handle cases where socket_devices might be null
    const processedSockets = sockets.map(socket => {
      let devices = [];
      if (socket.socket_devices && Array.isArray(socket.socket_devices)) {
        devices = socket.socket_devices
          .filter(sd => sd.devices)
          .map(sd => ({
            id: sd.devices.id,
            deviceId: sd.devices.device_id,
            name: sd.devices.name
          }));
      }

      return {
        id: socket.id,
        name: socket.name,
        location: socket.location,
        createdAt: socket.created_at,
        updatedAt: socket.updated_at,
        devices
      };
    });

    res.json({
      sockets: processedSockets
    });

  } catch (error) {
    console.error('Get sockets error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

/**
 * POST /api/telemetry/sockets
 * Create a new socket
 */
router.post('/sockets', authenticateToken, async (req, res) => {
  try {
    const { socketName, location, sensorIds } = req.body;

    if (!socketName) {
      return res.status(400).json({
        error: 'Missing required field',
        message: 'Socket name is required'
      });
    }

    console.log(`🔌 Creating socket: ${socketName}, location: ${location || 'not provided'}, userId: ${req.user.id}, sensorIds: ${sensorIds?.length || 0}`);

    // Validation: Check if there are additional sensor readings available that aren't already associated with existing sockets
    // Get all devices associated with existing sockets
    const { data: existingSockets, error: socketsError } = await supabaseAdmin
      .from('sockets')
      .select(`
        socket_devices (
          device_id
        )
      `)
      .eq('user_id', req.user.id);

    if (socketsError) {
      throw socketsError;
    }

    // Extract all device IDs already associated with sockets
    const associatedDeviceIds = new Set();
    existingSockets?.forEach(socket => {
      socket.socket_devices?.forEach(sd => {
        if (sd.device_id) {
          associatedDeviceIds.add(sd.device_id);
        }
      });
    });

    // Get all user's devices
    const { data: userDevices, error: devicesError } = await supabaseAdmin
      .from('devices')
      .select('id, device_id')
      .eq('user_id', req.user.id)
      .eq('is_active', true);

    if (devicesError) {
      throw devicesError;
    }

    // Check if there are devices with sensor readings not associated with any socket
    const unassociatedDevices = userDevices?.filter(device => !associatedDeviceIds.has(device.id)) || [];
    
    // Check if any of these unassociated devices have recent sensor readings
    let hasAdditionalSensorData = false;
    for (const device of unassociatedDevices) {
      const { data: latestReading, error: readingError } = await supabaseAdmin
        .from('sensor_readings')
        .select('*')
        .eq('device_id', device.device_id)
        .order('received_at', { ascending: false })
        .limit(1)
        .maybeSingle();

      if (readingError && readingError.code !== 'PGRST116') {
        console.error(`Error checking readings for device ${device.device_id}:`, readingError);
        continue;
      }

      if (latestReading) {
        const readingTime = new Date(latestReading.received_at);
        const now = new Date();
        const minutesSinceReading = (now - readingTime) / (1000 * 60);
        
        // If we have a recent reading (within last 5 minutes), we have additional sensor data
        if (minutesSinceReading <= 5) {
          hasAdditionalSensorData = true;
          break;
        }
      }
    }

    // If no additional sensor data is available, return error
    if (!hasAdditionalSensorData && unassociatedDevices.length === 0) {
      return res.status(400).json({
        error: 'Sensor validation failed',
        message: 'sensors are not set correctly'
      });
    }
    
    // Verify that all sensorIds belong to the user's devices
    if (sensorIds && sensorIds.length > 0) {
      const { data: userDevicesForValidation, error: devicesValidationError } = await supabaseAdmin
        .from('devices')
        .select('id')
        .eq('user_id', req.user.id)
        .in('id', sensorIds);

      if (devicesValidationError) {
        throw devicesValidationError;
      }

      if (userDevicesForValidation.length !== sensorIds.length) {
        return res.status(400).json({
          error: 'Invalid sensors',
          message: 'Some sensor IDs do not belong to your devices'
        });
      }
    }

    // Create socket record
    const { data: socket, error: socketError } = await supabaseAdmin
      .from('sockets')
      .insert({
        user_id: req.user.id,
        name: socketName,
        location: location || null,
      })
      .select()
      .single();

    if (socketError) {
      throw socketError;
    }

    // Link devices to socket if sensorIds provided
    if (sensorIds && sensorIds.length > 0) {
      const socketDeviceInserts = sensorIds.map(deviceId => ({
        socket_id: socket.id,
        device_id: deviceId,
      }));

      const { error: linkError } = await supabaseAdmin
        .from('socket_devices')
        .insert(socketDeviceInserts);

      if (linkError) {
        // Rollback socket creation if linking fails
        await supabaseAdmin
          .from('sockets')
          .delete()
          .eq('id', socket.id);
        throw linkError;
      }
    }
    
    res.status(201).json({
      message: 'Socket created successfully',
      socket: {
        id: socket.id,
        name: socket.name,
        location: socket.location,
        createdAt: socket.created_at,
        sensorCount: sensorIds?.length || 0
      }
    });

  } catch (error) {
    console.error('Create socket error:', error);
    res.status(500).json({
      error: 'Internal server error',
      message: error.message
    });
  }
});

module.exports = router;

