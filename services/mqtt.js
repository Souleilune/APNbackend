const mqtt = require('mqtt');
const EventEmitter = require('events');

class MQTTService extends EventEmitter {
  constructor() {
    super();
    this.client = null;
    this.isConnected = false;
    this.topicPrefix = process.env.MQTT_TOPIC_PREFIX || 'apn/device';
  }

  /**
   * Connect to the HiveMQ Cloud broker
   */
  connect() {
  const brokerUrl = process.env.MQTT_BROKER_URL;
  const port = process.env.MQTT_PORT || 8883;
  const username = process.env.MQTT_USERNAME;
  const password = process.env.MQTT_PASSWORD;
 
  if (!brokerUrl || !username || !password) {
    console.error('❌ MQTT: Missing required environment variables (MQTT_BROKER_URL, MQTT_USERNAME, MQTT_PASSWORD)');
    return;
  }
 
  const connectUrl = `mqtts://${brokerUrl}:${port}`;
 
  console.log(`📡 MQTT: Connecting to ${brokerUrl}:${port}...`);
 
  this.client = mqtt.connect(connectUrl, {
    username,
    password,
    protocol: 'mqtts',
    rejectUnauthorized: true,
    reconnectPeriod: 5000,
    connectTimeout: 30000,
    keepalive: 60,
    clean: true,
    clientId: `apn-backend-${Date.now()}`,
    // Increase buffer size for larger messages
    protocolVersion: 5, // Use MQTT 5.0 for better large message support
    properties: {
      maximumPacketSize: 268435455, // Maximum allowed (256MB)
    },
    // Additional options for better connection stability
    will: {
      topic: `${this.topicPrefix}/backend/status`,
      payload: JSON.stringify({ status: 'offline' }),
      qos: 1,
      retain: false,
    },
  });
 
  this._setupEventHandlers();
}
  /**
   * Setup MQTT client event handlers
   */
  _setupEventHandlers() {
    this.client.on('connect', () => {
      this.isConnected = true;
      console.log('✅ MQTT: Connected to HiveMQ Cloud');
      this._subscribeToTopics();
    });

    this.client.on('reconnect', () => {
      console.log('🔄 MQTT: Reconnecting...');
      this.isConnected = false;
    });

    this.client.on('disconnect', () => {
      this.isConnected = false;
      console.log('⚠️ MQTT: Disconnected');
    });

    this.client.on('error', (error) => {
      console.error('❌ MQTT Error:', error.message);
      
      // Handle keepalive timeout gracefully - don't crash the server
      if (error.message && error.message.includes('Keepalive timeout')) {
        console.log('🔄 MQTT: Keepalive timeout detected - connection will be re-established');
        this.isConnected = false;
        // Don't emit error for keepalive timeouts - let the client reconnect automatically
        // The reconnectPeriod: 5000 in connect() will handle automatic reconnection
        return;
      }
      
      // For other errors, emit but don't crash
      // Only emit if there are listeners to prevent unhandled error events
      if (this.listenerCount('error') > 0) {
        this.emit('error', error);
      } else {
        // If no listeners, just log the error
        console.error('❌ MQTT Error (no listeners):', error);
      }
    });

    this.client.on('offline', () => {
      this.isConnected = false;
      console.log('📴 MQTT: Client offline');
    });

    this.client.on('close', () => {
      this.isConnected = false;
      console.log('🔌 MQTT: Connection closed');
    });

    this.client.on('end', () => {
      this.isConnected = false;
      console.log('🔚 MQTT: Connection ended');
    });

    this.client.on('message', (topic, message) => {
      this._handleMessage(topic, message);
    });
  }

  /**
   * Subscribe to telemetry topics
   * Uses wildcard + to match any device_id
   */
  _subscribeToTopics() {
    const telemetryTopic = `${this.topicPrefix}/+/telemetry`;
    
    this.client.subscribe(telemetryTopic, { qos: 1 }, (err, granted) => {
      if (err) {
        console.error('❌ MQTT: Subscription error:', err.message);
        return;
      }
      console.log(`📬 MQTT: Subscribed to ${telemetryTopic}`);
      granted.forEach((g) => {
        console.log(`   - Topic: ${g.topic}, QoS: ${g.qos}`);
      });
    });
  }

  /**
   * Handle incoming MQTT messages
   * @param {string} topic - The MQTT topic
   * @param {Buffer} message - The message payload
   */
  __handleMessage(topic, message) {
  try {
    // Extract device_id from topic (e.g., apn/device/DEVICE123/telemetry)
    const topicParts = topic.split('/');
    const deviceId = topicParts[2]; // Index 2 is the device_id
    
    // Convert buffer to string
    const messageStr = message.toString();
    
    // Check if message is complete JSON (should start with { and end with })
    if (!messageStr.trim().startsWith('{') || !messageStr.trim().endsWith('}')) {
      console.error('❌ MQTT: Incomplete JSON message received');
      console.error('   Device:', deviceId);
      console.error('   Message length:', messageStr.length);
      console.error('   First 100 chars:', messageStr.substring(0, 100));
      console.error('   Last 100 chars:', messageStr.substring(Math.max(0, messageStr.length - 100)));
      
      // Log to help debug on ESP32 side
      console.error('   ⚠️  ESP32 may need to increase publish buffer or split messages');
      return;
    }
    
    // Parse JSON payload
    const payload = JSON.parse(messageStr);
    
    // Determine message type based on payload structure
    const messageType = this._determineMessageType(payload);
    
    console.log(`📨 MQTT: Received ${messageType} from device ${deviceId}`);
    
    // Only log full payload for alerts and important messages
    if (messageType === 'alert' || messageType === 'alert_cleared') {
      console.log('   Payload:', JSON.stringify(payload, null, 2));
    } else {
      console.log('   Type: sensor_reading, Device:', deviceId);
    }
 
    // Emit typed event with parsed data
    this.emit('telemetry', {
      deviceId,
      topic,
      messageType,
      payload,
      receivedAt: new Date().toISOString(),
    });
 
  } catch (error) {
    console.error('❌ MQTT: Error parsing message:', error.message);
    console.error('   Topic:', topic);
    console.error('   Message length:', message.length);
    console.error('   Raw message (first 500 chars):', message.toString().substring(0, 500));
    
    // Try to identify the issue
    const messageStr = message.toString();
    if (messageStr.length > 0) {
      const lastChar = messageStr[messageStr.length - 1];
      if (lastChar === ',') {
        console.error('   💡 Message appears truncated (ends with comma)');
        console.error('   💡 ESP32 needs larger buffer or message chunking');
      }
    }
  }
}
 

  /**
   * Determine the type of message based on payload structure
   * @param {object} payload - The parsed payload
   * @returns {string} - The message type
   */
  _determineMessageType(payload) {
    if (payload.alert) {
      return 'alert';
    }
    if (payload.status === 'ALERT_CLEARED') {
      return 'alert_cleared';
    }
    if (payload.power) {
      return 'power_status';
    }
    if (payload.water || payload.gas !== undefined || payload.temperature || payload.gyro) {
      return 'sensor_reading';
    }
    // Legacy medication payload support
    if (payload.medicine_name || payload.schedule_id) {
      return 'medication';
    }
    return 'unknown';
  }

  /**
   * Publish a message to a topic
   * @param {string} topic - The topic to publish to
   * @param {object} message - The message payload
   */
  publish(topic, message) {
    if (!this.isConnected) {
      console.error('❌ MQTT: Cannot publish - not connected');
      return false;
    }

    const payload = JSON.stringify(message);
    this.client.publish(topic, payload, { qos: 1 }, (err) => {
      if (err) {
        console.error('❌ MQTT: Publish error:', err.message);
        return;
      }
      console.log(`📤 MQTT: Published to ${topic}`);
    });
    return true;
  }

  /**
   * Disconnect from the broker
   */
  disconnect() {
    if (this.client) {
      this.client.end(true, () => {
        console.log('👋 MQTT: Disconnected gracefully');
      });
    }
  }

  /**
   * Check if connected
   */
  getConnectionStatus() {
    return {
      connected: this.isConnected,
      clientId: this.client?.options?.clientId,
    };
  }
}

// Export singleton instance
const mqttService = new MQTTService();
module.exports = mqttService;

