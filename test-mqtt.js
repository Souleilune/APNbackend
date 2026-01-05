require('dotenv').config();
const mqtt = require('mqtt');

// Test configuration
const CONNECTION_TIMEOUT = 30000; // 30 seconds
const MESSAGE_WAIT_TIMEOUT = 10000; // 10 seconds
const TEST_DEVICE_ID = 'test-device-' + Date.now();

// Test results
const testResults = {
  connection: false,
  subscription: false,
  publish: false,
  messageReceived: false,
};

let client = null;
let connectionTimeout = null;
let messageTimeout = null;
let receivedMessages = [];

/**
 * Validate required environment variables
 */
function validateEnvironment() {
  const required = ['MQTT_BROKER_URL', 'MQTT_USERNAME', 'MQTT_PASSWORD'];
  const missing = required.filter(key => !process.env[key]);

  if (missing.length > 0) {
    console.error('❌ Missing required environment variables:');
    missing.forEach(key => console.error(`   - ${key}`));
    console.error('\nPlease ensure your .env file contains all required MQTT configuration.');
    process.exit(1);
  }

  console.log('✅ Environment variables validated');
  return true;
}

/**
 * Setup MQTT client event handlers
 */
function setupEventHandlers() {
  client.on('connect', () => {
    clearTimeout(connectionTimeout);
    testResults.connection = true;
    console.log('✅ MQTT: Successfully connected to broker');
    console.log(`   Client ID: ${client.options.clientId}`);
    testSubscription();
  });

  client.on('reconnect', () => {
    console.log('🔄 MQTT: Reconnecting...');
  });

  client.on('disconnect', () => {
    testResults.connection = false;
    console.log('⚠️ MQTT: Disconnected from broker');
  });

  client.on('error', (error) => {
    clearTimeout(connectionTimeout);
    console.error('❌ MQTT Error:', error.message);
    console.error('   Details:', error);
    cleanupAndExit(1);
  });

  client.on('offline', () => {
    testResults.connection = false;
    console.log('📴 MQTT: Client went offline');
  });

  client.on('message', (topic, message) => {
    testResults.messageReceived = true;
    clearTimeout(messageTimeout);
    
    try {
      const payload = JSON.parse(message.toString());
      receivedMessages.push({ topic, payload, timestamp: new Date().toISOString() });
      
      console.log(`\n📨 MQTT: Received message on topic "${topic}"`);
      console.log('   Payload:', JSON.stringify(payload, null, 2));
      console.log('   Timestamp:', receivedMessages[receivedMessages.length - 1].timestamp);
    } catch (error) {
      console.log(`\n📨 MQTT: Received message on topic "${topic}"`);
      console.log('   Raw message:', message.toString());
      console.log('   ⚠️ Failed to parse as JSON:', error.message);
    }
  });
}

/**
 * Test subscription to telemetry topics
 */
function testSubscription() {
  const topicPrefix = process.env.MQTT_TOPIC_PREFIX || 'apn/device';
  const telemetryTopic = `${topicPrefix}/+/telemetry`;
  
  console.log(`\n📬 Testing subscription to: ${telemetryTopic}`);
  
  client.subscribe(telemetryTopic, { qos: 1 }, (err, granted) => {
    if (err) {
      console.error('❌ MQTT: Subscription error:', err.message);
      cleanupAndExit(1);
      return;
    }
    
    testResults.subscription = true;
    console.log('✅ MQTT: Successfully subscribed');
    granted.forEach((g) => {
      console.log(`   - Topic: ${g.topic}, QoS: ${g.qos}`);
    });
    
    // Test publishing after successful subscription
    testPublish();
  });
}

/**
 * Test publishing a message
 */
function testPublish() {
  const topicPrefix = process.env.MQTT_TOPIC_PREFIX || 'apn/device';
  const testTopic = `${topicPrefix}/${TEST_DEVICE_ID}/telemetry`;
  const testMessage = {
    messageType: 'test',
    payload: {
      test: true,
      timestamp: new Date().toISOString(),
      source: 'mqtt-test-script'
    }
  };

  console.log(`\n📤 Testing publish to: ${testTopic}`);
  console.log('   Message:', JSON.stringify(testMessage, null, 2));

  client.publish(testTopic, JSON.stringify(testMessage), { qos: 1 }, (err) => {
    if (err) {
      console.error('❌ MQTT: Publish error:', err.message);
      // Don't exit on publish error, continue to wait for messages
    } else {
      testResults.publish = true;
      console.log('✅ MQTT: Message published successfully');
    }
    
    // Wait for messages
    waitForMessages();
  });
}

/**
 * Wait for incoming messages with timeout
 */
function waitForMessages() {
  console.log(`\n⏳ Waiting for messages (timeout: ${MESSAGE_WAIT_TIMEOUT / 1000}s)...`);
  console.log('   (You can send test messages from your ESP32 or another MQTT client)');
  
  messageTimeout = setTimeout(() => {
    if (receivedMessages.length === 0) {
      console.log('\n⚠️ No messages received during the wait period');
      console.log('   This is normal if no devices are currently sending telemetry.');
    } else {
      console.log(`\n✅ Received ${receivedMessages.length} message(s) during test`);
    }
    printTestSummary();
    cleanupAndExit(0);
  }, MESSAGE_WAIT_TIMEOUT);
}

/**
 * Print test summary
 */
function printTestSummary() {
  console.log('\n' + '='.repeat(50));
  console.log('TEST SUMMARY');
  console.log('='.repeat(50));
  console.log(`Connection:     ${testResults.connection ? '✅ PASS' : '❌ FAIL'}`);
  console.log(`Subscription:   ${testResults.subscription ? '✅ PASS' : '❌ FAIL'}`);
  console.log(`Publish:        ${testResults.publish ? '✅ PASS' : '❌ FAIL'}`);
  console.log(`Messages Rx:    ${testResults.messageReceived ? '✅ PASS' : '⚠️  NONE'}`);
  
  if (receivedMessages.length > 0) {
    console.log(`\nTotal messages received: ${receivedMessages.length}`);
  }
  
  const allCriticalTestsPassed = testResults.connection && testResults.subscription;
  console.log(`\nOverall Status: ${allCriticalTestsPassed ? '✅ PASS' : '❌ FAIL'}`);
  console.log('='.repeat(50));
}

/**
 * Cleanup and exit
 */
function cleanupAndExit(exitCode) {
  if (connectionTimeout) {
    clearTimeout(connectionTimeout);
  }
  if (messageTimeout) {
    clearTimeout(messageTimeout);
  }
  
  if (client) {
    console.log('\n👋 Disconnecting from MQTT broker...');
    client.end(true, () => {
      console.log('✅ Disconnected gracefully');
      process.exit(exitCode);
    });
  } else {
    process.exit(exitCode);
  }
}

/**
 * Main test function
 */
function runTest() {
  console.log('🧪 MQTT Connection Test Script');
  console.log('='.repeat(50));
  
  // Validate environment
  if (!validateEnvironment()) {
    process.exit(1);
  }

  // Get configuration
  const brokerUrl = process.env.MQTT_BROKER_URL;
  const port = process.env.MQTT_PORT || 8883;
  const username = process.env.MQTT_USERNAME;
  const password = process.env.MQTT_PASSWORD;
  const connectUrl = `mqtts://${brokerUrl}:${port}`;

  console.log(`\n📡 Connecting to MQTT broker...`);
  console.log(`   URL: ${brokerUrl}:${port}`);
  console.log(`   Protocol: mqtts (TLS)`);
  console.log(`   Username: ${username}`);
  console.log(`   Timeout: ${CONNECTION_TIMEOUT / 1000}s`);

  // Create MQTT client with same configuration as service
  client = mqtt.connect(connectUrl, {
    username,
    password,
    protocol: 'mqtts',
    rejectUnauthorized: true,
    reconnectPeriod: 5000,
    connectTimeout: CONNECTION_TIMEOUT,
    keepalive: 60,
    clean: true,
    clientId: `apn-test-${Date.now()}`,
  });

  // Setup event handlers
  setupEventHandlers();

  // Set connection timeout
  connectionTimeout = setTimeout(() => {
    if (!testResults.connection) {
      console.error('\n❌ Connection timeout: Failed to connect within', CONNECTION_TIMEOUT / 1000, 'seconds');
      console.error('   Please check:');
      console.error('   - MQTT_BROKER_URL is correct');
      console.error('   - MQTT_USERNAME and MQTT_PASSWORD are valid');
      console.error('   - Network connectivity');
      console.error('   - Firewall settings');
      cleanupAndExit(1);
    }
  }, CONNECTION_TIMEOUT);

  // Handle process termination
  process.on('SIGINT', () => {
    console.log('\n\n⚠️ Test interrupted by user');
    printTestSummary();
    cleanupAndExit(1);
  });

  process.on('SIGTERM', () => {
    console.log('\n\n⚠️ Test terminated');
    printTestSummary();
    cleanupAndExit(1);
  });
}

// Run the test
runTest();
