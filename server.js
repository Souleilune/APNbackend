require('dotenv').config();
const express = require('express');
const cors = require('cors');
const http = require('http');
const { createClient } = require('@supabase/supabase-js');
const authRoutes = require('./routes/auth');
const telemetryRoutes = require('./routes/telemetry');
const mqttService = require('./services/mqtt');
const websocketService = require('./services/websocket');

const app = express();
const server = http.createServer(app);
const PORT = process.env.PORT || 3000;

// Initialize Supabase Admin client for MQTT message processing
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

// CORS Configuration
const allowedOrigins = process.env.ALLOWED_ORIGINS 
  ? process.env.ALLOWED_ORIGINS.split(',') 
  : ['http://localhost:8081', 
      'exp://localhost:8081',
      'http://192.168.0.11:8081', // Replace with your actual IP
      'exp://192.168.0.11:8081'];

app.use(cors({
  origin: function(origin, callback) {
    // Allow requests with no origin (mobile apps, Postman, etc.)
    if (!origin) return callback(null, true);
    
    if (allowedOrigins.indexOf(origin) === -1) {
      const msg = 'The CORS policy for this site does not allow access from the specified Origin.';
      return callback(new Error(msg), false);
    }
    return callback(null, true);
  },
  credentials: true
}));

// Middleware
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Request logging middleware
app.use((req, res, next) => {
  console.log(`${new Date().toISOString()} - ${req.method} ${req.path}`);
  next();
});

// Health check endpoint
app.get('/', (req, res) => {
  res.json({ 
    message: 'APN Telemetry API Server',
    status: 'running',
    version: '1.0.0'
  });
});

// API Routes
app.use('/api/auth', authRoutes);
app.use('/api/telemetry', telemetryRoutes);

// 404 Handler
app.use((req, res) => {
  res.status(404).json({ 
    error: 'Not Found',
    message: `Cannot ${req.method} ${req.path}`
  });
});

// Global Error Handler
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(err.status || 500).json({
    error: err.message || 'Internal Server Error',
    ...(process.env.NODE_ENV === 'development' && { stack: err.stack })
  });
});

/**
 * Handle MQTT telemetry messages
 */
async function handleTelemetry(data) {
  const { deviceId, messageType, payload, receivedAt } = data;

  try {
    // Get device owner
    const { data: device, error: deviceError } = await supabaseAdmin
      .from('devices')
      .select('user_id, is_active')
      .eq('device_id', deviceId)
      .maybeSingle();

    if (deviceError || !device) {
      console.error(`❌ MQTT: Device ${deviceId} not found in database`);
      return;
    }

    if (!device.is_active) {
      console.log(`⚠️ MQTT: Device ${deviceId} is inactive, ignoring message`);
      return;
    }

    const userId = device.user_id;

    // Route to appropriate handler based on message type
    switch (messageType) {
      case 'alert':
        await handleAlert(deviceId, userId, payload, receivedAt);
        break;
      case 'alert_cleared':
        await handleAlertCleared(deviceId, userId, payload, receivedAt);
        break;
      case 'sensor_reading':
        await handleSensorReading(deviceId, userId, payload, receivedAt);
        break;
      case 'power_status':
        await handlePowerStatus(deviceId, userId, payload, receivedAt);
        break;
      default:
        console.log(`⚠️ MQTT: Unknown message type: ${messageType}`);
    }

    // Forward to WebSocket clients
    websocketService.sendToUser(userId, {
      deviceId,
      messageType,
      payload,
      receivedAt,
    });

  } catch (error) {
    console.error('❌ MQTT: Error handling telemetry:', error);
  }
}

/**
 * Handle alert messages from ESP32
 */
async function handleAlert(deviceId, userId, payload, receivedAt) {
  try {
    const alertType = payload.alert;
    const sensor = payload.sensor || null;
    const value = payload.value !== undefined ? payload.value : null;

    // Insert alert into database
    const { data: alert, error } = await supabaseAdmin
      .from('alerts')
      .insert({
        device_id: deviceId,
        user_id: userId,
        alert_type: alertType,
        sensor: sensor,
        value: value,
        is_active: true,
        received_at: receivedAt,
      })
      .select()
      .single();

    if (error) {
      console.error('❌ Error storing alert:', error.message);
      return;
    }

    console.log(`✅ Alert stored: ${alertType} for device ${deviceId}`);

    // If it's a gas leak detected alert, also create a sensor reading entry
    if (alertType === 'GAS_LEAK_DETECTED') {
      const gasSensorReading = {
        device_id: deviceId,
        user_id: userId,
        water_1: null,
        water_2: null,
        water_3: null,
        water_4: null,
        gas_detected: true,
        temp_1: null,
        temp_2: null,
        movement: null,
        power_status: null,
        received_at: receivedAt,
      };
      const { error: sensorError } = await supabaseAdmin
        .from('sensor_readings')
        .insert(gasSensorReading);

      if (sensorError) {
        console.error('❌ Error storing gas detection sensor reading:', sensorError.message);
      } else {
        console.log('✅ Gas detection sensor reading stored for chart display');
      }
    }
  } catch (error) {
    console.error('❌ Error handling alert:', error);
  }
}

/**
 * Handle alert cleared messages
 */
async function handleAlertCleared(deviceId, userId, payload, receivedAt) {
  try {
    const alertType = payload.alert || payload.alertType;
    const sensor = payload.sensor || null;

    // Update all active alerts of this type for this device
    const updateQuery = supabaseAdmin
      .from('alerts')
      .update({
        is_active: false,
        cleared_at: receivedAt,
      })
      .eq('device_id', deviceId)
      .eq('user_id', userId)
      .eq('is_active', true);

    if (alertType) {
      updateQuery.eq('alert_type', alertType);
    }
    if (sensor) {
      updateQuery.eq('sensor', sensor);
    }

    const { error } = await updateQuery;

    if (error) {
      console.error('❌ Error clearing alerts:', error.message);
      return;
    }

    console.log(`✅ Alerts cleared for device ${deviceId}`);
  } catch (error) {
    console.error('❌ Error handling alert cleared:', error);
  }
}

/**
 * Handle sensor reading messages
 */
async function handleSensorReading(deviceId, userId, payload, receivedAt) {
  try {
    // Extract movement from gyro object (gyro.movement) or use direct movement value
    const movementValue = payload.gyro?.movement !== undefined 
      ? payload.gyro.movement 
      : (payload.movement || null);

    // Extract temperature from temperature object (temperature.temp1/temp2) or use direct values
    const temp1 = payload.temperature?.temp1 !== undefined 
      ? payload.temperature.temp1 
      : (payload.temp_1 !== undefined ? payload.temp_1 : null);
    const temp2 = payload.temperature?.temp2 !== undefined 
      ? payload.temperature.temp2 
      : (payload.temp_2 !== undefined ? payload.temp_2 : null);

    // Check water detection by zone (threshold: 500)
    // Zone 1: sensors 1 & 2, Zone 2: sensors 3 & 4
    const waterThreshold = 500;
    const zone1Water1 = payload.water?.[0] || 0;
    const zone1Water2 = payload.water?.[1] || 0;
    const zone2Water3 = payload.water?.[2] || 0;
    const zone2Water4 = payload.water?.[3] || 0;
    
    // Zone detection: if either sensor in zone exceeds threshold, zone has water
    const zone1Detected = (zone1Water1 > waterThreshold) || (zone1Water2 > waterThreshold);
    const zone2Detected = (zone2Water3 > waterThreshold) || (zone2Water4 > waterThreshold);
    
    // Store water detection as boolean (1 = detected, 0 = not detected)
    // Use first sensor of each zone to store the detection status
    const sensorReading = {
      device_id: deviceId,
      user_id: userId,
      water_1: zone1Detected ? 1 : 0,
      water_2: zone1Detected ? 1 : 0, // Keep same value for zone consistency
      water_3: zone2Detected ? 1 : 0,
      water_4: zone2Detected ? 1 : 0, // Keep same value for zone consistency
      gas_detected: payload.gas !== undefined ? Boolean(payload.gas) : null,
      temp_1: temp1,
      temp_2: temp2,
      movement: movementValue,
      power_status: payload.power || null,
      received_at: receivedAt,
    };

    const { error } = await supabaseAdmin
      .from('sensor_readings')
      .insert(sensorReading);

    if (error) {
      console.error('❌ Error storing sensor reading:', error.message);
      return;
    }

    console.log(`✅ Sensor reading stored for device ${deviceId}`);
    if (zone1Detected) {
      console.log(`   💧 Zone 1: Water detected`);
    }
    if (zone2Detected) {
      console.log(`   💧 Zone 2: Water detected`);
    }

    // Check for water alerts by zone
    const zones = [
      { zone: 1, detected: zone1Detected },
      { zone: 2, detected: zone2Detected },
    ];

    for (const zoneInfo of zones) {
      if (zoneInfo.detected) {
        // Check if an active alert already exists for this zone
        const { data: existingAlert, error: checkError } = await supabaseAdmin
          .from('alerts')
          .select('id')
          .eq('device_id', deviceId)
          .eq('user_id', userId)
          .eq('alert_type', 'WATER_DETECTED')
          .eq('sensor', `ZONE${zoneInfo.zone}`)
          .eq('is_active', true)
          .maybeSingle();

        if (checkError) {
          console.error(`❌ Error checking for existing water alert (ZONE${zoneInfo.zone}):`, checkError.message);
          continue;
        }

        // Only create alert if one doesn't already exist
        if (!existingAlert) {
          const { error: alertError } = await supabaseAdmin
            .from('alerts')
            .insert({
              device_id: deviceId,
              user_id: userId,
              alert_type: 'WATER_DETECTED',
              sensor: `ZONE${zoneInfo.zone}`,
              value: 1, // Boolean value: 1 = detected
              is_active: true,
              received_at: receivedAt,
            });

          if (alertError) {
            console.error(`❌ Error storing water alert (ZONE${zoneInfo.zone}):`, alertError.message);
          } else {
            console.log(`✅ Water alert created: ZONE${zoneInfo.zone} for device ${deviceId}`);
          }
        }
      }
    }
  } catch (error) {
    console.error('❌ Error handling sensor reading:', error);
  }
}

/**
 * Handle power status messages
 */
async function handlePowerStatus(deviceId, userId, payload, receivedAt) {
  try {
    // Store power status as a sensor reading
    const sensorReading = {
      device_id: deviceId,
      user_id: userId,
      water_1: null,
      water_2: null,
      water_3: null,
      water_4: null,
      gas_detected: null,
      temp_1: null,
      temp_2: null,
      movement: null,
      power_status: payload.power || payload.status || null,
      received_at: receivedAt,
    };

    const { error } = await supabaseAdmin
      .from('sensor_readings')
      .insert(sensorReading);

    if (error) {
      console.error('❌ Error storing power status:', error.message);
      return;
    }

    console.log(`✅ Power status stored for device ${deviceId}`);
  } catch (error) {
    console.error('❌ Error handling power status:', error);
  }
}

// Initialize MQTT Service
mqttService.on('telemetry', handleTelemetry);
mqttService.connect();

// Initialize WebSocket Service
websocketService.initialize(server);
websocketService.startHeartbeat(30000);

// Setup MQTT command handling for WebSocket
websocketService.on('device_command', async (data) => {
  const { userId, deviceId, command } = data;

  try {
    // Verify device ownership
    const { data: device, error: deviceError } = await supabaseAdmin
      .from('devices')
      .select('user_id, is_active')
      .eq('device_id', deviceId)
      .maybeSingle();

    if (deviceError || !device) {
      console.error(`❌ WebSocket: Device ${deviceId} not found`);
      websocketService.sendErrorToUser(userId, 'DEVICE_NOT_FOUND', `Device ${deviceId} not found`);
      return;
    }

    if (device.user_id !== userId) {
      console.error(`❌ WebSocket: User ${userId} does not own device ${deviceId}`);
      websocketService.sendErrorToUser(userId, 'UNAUTHORIZED', `User does not own device ${deviceId}`);
      return;
    }

    if (!device.is_active) {
      console.error(`❌ WebSocket: Device ${deviceId} is inactive`);
      websocketService.sendErrorToUser(userId, 'DEVICE_INACTIVE', `Device ${deviceId} is inactive`);
      return;
    }

    // Publish command to MQTT
    const topic = `apn/device/${deviceId}/commands`;
    const message = {
      command: command,
      timestamp: new Date().toISOString(),
    };
    
    console.log(`📤 MQTT: Publishing command "${command}" to topic "${topic}"`);
    console.log(`   Message:`, JSON.stringify(message, null, 2));

    const published = mqttService.publish(topic, message);
    if (published) {
      console.log(`✅ WebSocket: Command ${command} sent to device ${deviceId}`);
    } else {
      websocketService.sendErrorToUser(userId, 'MQTT_ERROR', 'Failed to publish command to MQTT');
    }
  } catch (error) {
    console.error('❌ WebSocket: Error handling device command:', error);
    websocketService.sendErrorToUser(userId, 'INTERNAL_ERROR', error.message);
  }
});

// Start Server
server.listen(PORT, () => {
  console.log(`
🚀 APN Telemetry Backend Server
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Environment: ${process.env.NODE_ENV || 'development'}
Port: ${PORT}
API Endpoint: http://localhost:${PORT}
Health Check: http://localhost:${PORT}/
Auth Routes: http://localhost:${PORT}/api/auth
Telemetry Routes: http://localhost:${PORT}/api/telemetry
WebSocket: ws://localhost:${PORT}/ws/telemetry
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  `);
});

// Graceful Shutdown
process.on('SIGTERM', () => {
  console.log('SIGTERM signal received: closing HTTP server');
  mqttService.disconnect();
  websocketService.close();
  server.close(() => {
    console.log('HTTP server closed');
    process.exit(0);
  });
});

process.on('SIGINT', () => {
  console.log('SIGINT signal received: closing HTTP server');
  mqttService.disconnect();
  websocketService.close();
  server.close(() => {
    console.log('HTTP server closed');
    process.exit(0);
  });
});