#include <Wire.h>
#include <MPU6050_tockn.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// -------------------- WIFI CONFIGURATION --------------------
const char* ssid = "PLDTHOMEFIBR432a0";           // ⚠️ USE 2.4GHz (remove 5G from name!)
const char* password = "PLDTWIFIcv3e9";

// -------------------- MQTT CONFIGURATION --------------------
const char* mqtt_broker = "YOUR_CLUSTER.s1.eu.hivemq.cloud";  // ⚠️ Replace with your HiveMQ cluster URL
const int mqtt_port = 8883;
const char* mqtt_username = "YOUR_USERNAME";                   // ⚠️ Replace with your MQTT username
const char* mqtt_password = "YOUR_PASSWORD";                   // ⚠️ Replace with your MQTT password

// HiveMQ Cloud Root CA Certificate (ISRG Root X1 - Let's Encrypt)
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// -------------------- DEVICE ID (from MAC address) --------------------
String deviceId;
String telemetryTopic;
String commandTopic;

// -------------------- PIN CONFIG --------------------
#define WATER1 32
#define WATER2 33
#define WATER3 34
#define WATER4 35

#define GAS_DO 27

#define TEMP1 25
#define TEMP2 26

#define VOLTAGE_SENSOR_1 14
#define VOLTAGE_SENSOR_2 12
#define CURRENT_SENSOR_1 13
#define CURRENT_SENSOR_2 15

#define BUZZER 19

#define SERVO_GYRO_1 16
#define SERVO_GYRO_2 17
#define BREAKER_SERVO_1 18
#define BREAKER_SERVO_2 23

// -------------------- THRESHOLDS --------------------
int waterThreshold = 500;
float tempThreshold = 50.0;
float gyroThreshold = 15.0;
float voltageThreshold = 3000;
float currentThreshold = 2000;

float tempWarningLevel = 40.0;
float voltageWarning = 2500;
float currentWarning = 1500;

// -------------------- SENSOR ENABLE FLAGS --------------------
bool enableVoltageSensors = false;
bool enableCurrentSensors = false;

// -------------------- SERVO CONFIG --------------------
Servo servoGyro1;
Servo servoGyro2;
Servo breakerServo1;
Servo breakerServo2;

int gyroRestAngle = 90;
int gyroShakeAngle1 = 45;
int gyroShakeAngle2 = 135;

int breakerOnAngle = 90;
int breakerOffAngle = 0;

bool breaker1State = true;
bool breaker2State = true;

unsigned long lastGyroTestTime = 0;
unsigned long gyroTestInterval = 10000;
bool autoGyroTest = false;

// -------------------- STATE TRACKING --------------------
unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;
int alertCount = 0;
unsigned long statusInterval = 30000;
unsigned long lastStatusTime = 0;

// Alert state tracking (for cleared alerts)
bool waterAlertActive = false;
bool gasAlertActive = false;
bool tempAlertActive = false;
bool gyroAlertActive = false;
bool powerAlertActive = false;

// -------------------- STATE CHANGE TRACKING --------------------
bool prevWaterDetected = false;
bool prevGasDetected = false;
bool prevTempCritical = false;
bool prevTempWarning = false;
bool prevGyroDetected = false;
bool prevPowerCritical = false;
bool prevPowerWarning = false;

bool prevWifiConnected = false;
bool prevMqttConnected = false;
bool prevBreaker1State = true;
bool prevBreaker2State = true;
bool prevSystemEnabled = true;
bool prevBuzzerEnabled = true;

// Heartbeat
unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 60000;  // Print status every 60 seconds (0 to disable)

// Periodic status update (sends sensor data to database at regular intervals)
unsigned long lastPeriodicUpdate = 0;
unsigned long periodicUpdateInterval = 30000;  // 30 seconds interval for sensor readings
bool enablePeriodicUpdates = true;  // Enabled - sensor data stored every 30 seconds

// WiFi tracking
unsigned long lastWiFiCheck = 0;
bool wifiConnecting = false;

// -------------------- MPU OBJECT --------------------
MPU6050 mpu(Wire);

// -------------------- MQTT CLIENT --------------------
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
bool mqttConnected = false;
unsigned long lastMqttReconnectAttempt = 0;
unsigned long mqttReconnectInterval = 5000;
unsigned long lastDataSendTime = 0;
unsigned long dataSendInterval = 2000;
bool systemEnabled = true;
bool buzzerEnabled = true;

// -------------------- JSON BUFFER --------------------
StaticJsonDocument<1024> jsonDoc;
StaticJsonDocument<512> jsonReceive;

// ===================================================================
// FORWARD DECLARATIONS
// ===================================================================
void sendAlert(String alertType, String details = "");
void sendAlertCleared(String alertType);
void sendSensorData(bool forceImmediate = false);
void sendSystemStatus();
void printWiFiInfo();
void scanNetworks();
void printSensorReadings();
void printFullStatus();
void printHelp();
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool mqttReconnect();
String getDeviceId();

// ===================================================================
// DEVICE ID GENERATION
// ===================================================================
String getDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[13];
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// ===================================================================
// MQTT FUNCTIONS
// ===================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Create a null-terminated string from payload
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.print("📨 MQTT Message on topic: ");
  Serial.println(topic);
  Serial.print("   Payload: ");
  Serial.println(message);
  
  // Parse the JSON command
  DeserializationError error = deserializeJson(jsonReceive, message);
  
  if (error) {
    Serial.println("❌ Error parsing MQTT message");
    return;
  }
  
  handleIncomingCommand();
}

void handleIncomingCommand() {
  if (jsonReceive.containsKey("command")) {
    String command = jsonReceive["command"].as<String>();
    command.toUpperCase();
    Serial.print("📥 Received command: ");
    Serial.println(command);
    
    if (command == "SYSTEM_ON") {
      systemEnabled = true;
      sendAckResponse("System enabled");
      
    } else if (command == "SYSTEM_OFF") {
      systemEnabled = false;
      digitalWrite(BUZZER, LOW);
      sendAckResponse("System disabled");
      
    } else if (command == "BUZZER_ON") {
      buzzerEnabled = true;
      sendAckResponse("Buzzer enabled");
      
    } else if (command == "BUZZER_OFF") {
      buzzerEnabled = false;
      digitalWrite(BUZZER, LOW);
      sendAckResponse("Buzzer muted");
      
    } else if (command == "REQUEST_STATUS") {
      sendSensorData(true);
      
    } else if (command == "CALIBRATE_GYRO") {
      mpu.calcGyroOffsets(true);
      sendAckResponse("Gyro calibrated");
      
    } else if (command == "RESET_SYSTEM") {
      sendAckResponse("System resetting");
      delay(500);
      ESP.restart();
      
    } else if (command == "SHAKE_TEST") {
      testGyroShake();
      sendAckResponse("Shake test completed");
      
    } else if (command == "BREAKER1_ON") {
      setBreakerState(1, true);
      sendAckResponse("Breaker 1 ON");
      
    } else if (command == "BREAKER1_OFF") {
      setBreakerState(1, false);
      sendAckResponse("Breaker 1 OFF");
      
    } else if (command == "BREAKER2_ON") {
      setBreakerState(2, true);
      sendAckResponse("Breaker 2 ON");
      
    } else if (command == "BREAKER2_OFF") {
      setBreakerState(2, false);
      sendAckResponse("Breaker 2 OFF");
      
    } else if (command == "TRIP_ALL") {
      tripAllBreakers();
      sendAckResponse("All breakers tripped");
      
    } else if (command == "RESET_ALL") {
      resetAllBreakers();
      sendAckResponse("All breakers reset");
      
    } else {
      Serial.println("⚠️ Unknown command: " + command);
    }
  }
  else if (jsonReceive.containsKey("set_thresholds")) {
    JsonObject thresholds = jsonReceive["set_thresholds"];
    
    if (thresholds.containsKey("water")) {
      waterThreshold = thresholds["water"];
    }
    if (thresholds.containsKey("temperature")) {
      tempThreshold = thresholds["temperature"];
    }
    if (thresholds.containsKey("gyro")) {
      gyroThreshold = thresholds["gyro"];
    }
    if (thresholds.containsKey("voltage")) {
      voltageThreshold = thresholds["voltage"];
    }
    if (thresholds.containsKey("current")) {
      currentThreshold = thresholds["current"];
    }
    
    sendAckResponse("Thresholds updated");
    sendSystemStatus();
  }
}

bool mqttReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  String clientId = "ESP32-" + deviceId;
  
  Serial.print("📡 MQTT: Connecting to ");
  Serial.print(mqtt_broker);
  Serial.print(" as ");
  Serial.println(clientId);
  
  if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("✅ MQTT: Connected!");
    
    // Subscribe to command topic
    mqttClient.subscribe(commandTopic.c_str(), 1);
    Serial.print("📬 MQTT: Subscribed to ");
    Serial.println(commandTopic);
    
    // Send initial status
    sendSystemStatus();
    
    return true;
  } else {
    Serial.print("❌ MQTT: Connection failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void publishMessage(const char* payload) {
  if (!mqttClient.connected()) {
    Serial.println("⚠️ MQTT: Not connected, cannot publish");
    return;
  }
  
  if (mqttClient.publish(telemetryTopic.c_str(), payload, false)) {
    Serial.println("📤 MQTT: Message published");
  } else {
    Serial.println("❌ MQTT: Publish failed");
  }
}

// ===================================================================
// SERVO CONTROL FUNCTIONS
// ===================================================================
void initServos() {
  servoGyro1.attach(SERVO_GYRO_1);
  servoGyro2.attach(SERVO_GYRO_2);
  breakerServo1.attach(BREAKER_SERVO_1);
  breakerServo2.attach(BREAKER_SERVO_2);
  
  servoGyro1.write(gyroRestAngle);
  servoGyro2.write(gyroRestAngle);
  breakerServo1.write(breakerOnAngle);
  breakerServo2.write(breakerOnAngle);
  
  delay(500);
}

void testGyroShake() {
  Serial.println("🔧 Starting 30-second gyro shake test...");
  
  // Ensure servos are attached before starting
  if (!servoGyro1.attached()) {
    Serial.println("⚠️  Servo 1 not attached, reattaching...");
    servoGyro1.attach(SERVO_GYRO_1);
    delay(100);
  }
  if (!servoGyro2.attached()) {
    Serial.println("⚠️  Servo 2 not attached, reattaching...");
    servoGyro2.attach(SERVO_GYRO_2);
    delay(100);
  }
  
  // Move to rest position first
  servoGyro1.write(gyroRestAngle);
  servoGyro2.write(gyroRestAngle);
  delay(200);
  
  unsigned long shakeStartTime = millis();
  unsigned long shakeDuration = 30000;
  int cycleCount = 0;
  
  Serial.println("📳 Servos activated - shaking for 30 seconds...");
  
  while (millis() - shakeStartTime < shakeDuration) {
    // Ensure servos stay attached during operation
    if (!servoGyro1.attached()) {
      servoGyro1.attach(SERVO_GYRO_1);
    }
    if (!servoGyro2.attached()) {
      servoGyro2.attach(SERVO_GYRO_2);
    }
    
    // Shake pattern: alternate between angles
    servoGyro1.write(gyroShakeAngle1);
    servoGyro2.write(gyroShakeAngle2);
    delay(150);
    
    servoGyro1.write(gyroShakeAngle2);
    servoGyro2.write(gyroShakeAngle1);
    delay(150);
    
    servoGyro1.write(gyroRestAngle);
    servoGyro2.write(gyroRestAngle);
    delay(100);
    
    cycleCount++;
    
    unsigned long elapsed = millis() - shakeStartTime;
    if (elapsed % 5000 < 400) {
      Serial.print("  ⏱️  ");
      Serial.print(elapsed / 1000);
      Serial.print("s elapsed (");
      Serial.print(cycleCount);
      Serial.println(" cycles)...");
    }
    
    // Keep MQTT alive during long operation
    mqttClient.loop();
    
    // Small yield to prevent watchdog issues
    yield();
  }
  
  // Return to rest position
  servoGyro1.write(gyroRestAngle);
  servoGyro2.write(gyroRestAngle);
  delay(200);
  
  Serial.print("✅ 30-second gyro test complete (");
  Serial.print(cycleCount);
  Serial.println(" cycles)");
}

void setBreakerState(int breakerNum, bool state) {
  if (breakerNum == 1) {
    breaker1State = state;
    if (state) {
      for (int pos = breakerOffAngle; pos <= breakerOnAngle; pos += 10) {
        breakerServo1.write(pos);
        delay(10);
      }
    } else {
      for (int pos = breakerOnAngle; pos >= breakerOffAngle; pos -= 10) {
        breakerServo1.write(pos);
        delay(10);
      }
    }
    breakerServo1.write(state ? breakerOnAngle : breakerOffAngle);
  } else if (breakerNum == 2) {
    breaker2State = state;
    if (state) {
      for (int pos = breakerOffAngle; pos <= breakerOnAngle; pos += 10) {
        breakerServo2.write(pos);
        delay(10);
      }
    } else {
      for (int pos = breakerOnAngle; pos >= breakerOffAngle; pos -= 10) {
        breakerServo2.write(pos);
        delay(10);
      }
    }
    breakerServo2.write(state ? breakerOnAngle : breakerOffAngle);
  }
}

void tripAllBreakers() {
  setBreakerState(1, false);
  setBreakerState(2, false);
}

void resetAllBreakers() {
  setBreakerState(1, true);
  setBreakerState(2, true);
}

// ===================================================================
// BUZZER PATTERNS
// ===================================================================
void buzzWater() {
  if (!buzzerEnabled) return;
  digitalWrite(BUZZER, HIGH);
  delay(300);
  digitalWrite(BUZZER, LOW);
  delay(300);
}

void buzzGas() {
  if (!buzzerEnabled) return;
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
  delay(100);
}

void buzzTemp() {
  if (!buzzerEnabled) return;
  digitalWrite(BUZZER, HIGH);
  delay(600);
  digitalWrite(BUZZER, LOW);
  delay(300);
}

void buzzGyro() {
  if (!buzzerEnabled) return;
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(120);
    digitalWrite(BUZZER, LOW);
    delay(120);
  }
}

void buzzPower() {
  if (!buzzerEnabled) return;
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(400);
    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}

void buzzCritical() {
  if (!buzzerEnabled) return;
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(80);
    digitalWrite(BUZZER, LOW);
    delay(80);
  }
}

void buzzWarning() {
  if (!buzzerEnabled) return;
  digitalWrite(BUZZER, HIGH);
  delay(150);
  digitalWrite(BUZZER, LOW);
  delay(500);
}

// ===================================================================
// SENSOR READING FUNCTIONS
// ===================================================================
float readTemp(int pin) {
  int raw = analogRead(pin);
  return (raw * 3.3 / 4095.0) * 100;
}

float convertVoltage(int reading) {
  return (reading * 3.3 / 4095.0) * 5.0;
}

float convertCurrent(int reading) {
  float voltage = reading * 3.3 / 4095.0;
  return (voltage - 1.65) / 0.066;
}

// ===================================================================
// HELPER FUNCTIONS FOR SERIAL OUTPUT
// ===================================================================
void printWiFiInfo() {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║         WiFi Information          ║");
  Serial.println("╠═══════════════════════════════════╣");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("║  Status: ✅ CONNECTED             ║");
    Serial.print("║  SSID: ");
    String ssidStr = WiFi.SSID();
    Serial.print(ssidStr);
    for(int i = ssidStr.length(); i < 24; i++) Serial.print(" ");
    Serial.println("║");
    Serial.print("║  IP: ");
    Serial.print(WiFi.localIP());
    Serial.println("            ║");
    Serial.print("║  Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm               ║");
    Serial.print("║  MQTT: ");
    Serial.print(mqttClient.connected() ? "✅ Connected" : "⏳ Waiting");
    Serial.println("           ║");
  } else {
    Serial.println("║  Status: ❌ DISCONNECTED          ║");
  }
  Serial.println("╚═══════════════════════════════════╝\n");
}

void scanNetworks() {
  Serial.println("\n🔍 Scanning for WiFi networks...\n");
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("   No networks found!");
  } else {
    for (int i = 0; i < n; i++) {
      Serial.print("   ");
      Serial.print(i + 1);
      Serial.print(". ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm) ");
      
      // Warn about 5GHz
      if (WiFi.SSID(i).indexOf("5G") >= 0 || WiFi.SSID(i).indexOf("5g") >= 0) {
        Serial.print("⚠️ 5GHz - NOT COMPATIBLE!");
      } else {
        Serial.print("✅ 2.4GHz");
      }
      Serial.println();
    }
    Serial.println("\n   ⚠️  ESP32 only supports 2.4GHz networks!\n");
  }
}

void printSensorReadings() {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║       CURRENT SENSOR VALUES       ║");
  Serial.println("╠═══════════════════════════════════╣");
  
  int w1 = analogRead(WATER1);
  int w2 = analogRead(WATER2);
  int w3 = analogRead(WATER3);
  int w4 = analogRead(WATER4);
  Serial.printf("║  Water: %d, %d, %d, %d\n", w1, w2, w3, w4);
  Serial.printf("║         (threshold: %d)\n", waterThreshold);
  
  float t1 = readTemp(TEMP1);
  float t2 = readTemp(TEMP2);
  Serial.printf("║  Temp: %.1f°C, %.1f°C\n", t1, t2);
  Serial.printf("║        (threshold: %.1f°C)\n", tempThreshold);
  
  mpu.update();
  float gyro = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
  Serial.printf("║  Gyro: %.2f\n", gyro);
  Serial.printf("║        (threshold: %.2f)\n", gyroThreshold);
  
  Serial.print("║  Gas: ");
  Serial.println((digitalRead(GAS_DO) == LOW) ? "⚠️ DETECTED" : "Normal");
  
  if (enableVoltageSensors || enableCurrentSensors) {
    Serial.println("║  Power:");
    if (enableVoltageSensors) {
      Serial.printf("║    V1=%d V2=%d\n", analogRead(VOLTAGE_SENSOR_1), analogRead(VOLTAGE_SENSOR_2));
    }
    if (enableCurrentSensors) {
      Serial.printf("║    C1=%d C2=%d\n", analogRead(CURRENT_SENSOR_1), analogRead(CURRENT_SENSOR_2));
    }
  }
  
  Serial.println("╚═══════════════════════════════════╝\n");
}

void printFullStatus() {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║         SYSTEM STATUS             ║");
  Serial.println("╠═══════════════════════════════════╣");
  
  Serial.print("║  Device ID: ");
  Serial.print(deviceId);
  Serial.println("      ║");
  
  Serial.print("║  WiFi:    ");
  Serial.println((WiFi.status() == WL_CONNECTED) ? "✅ Connected          ║" : "❌ Disconnected       ║");
  
  Serial.print("║  MQTT:    ");
  Serial.println(mqttClient.connected() ? "✅ Connected          ║" : "⏳ Waiting            ║");
  
  Serial.print("║  System:  ");
  Serial.println(systemEnabled ? "✅ Enabled            ║" : "❌ Disabled           ║");
  
  Serial.print("║  Buzzer:  ");
  Serial.println(buzzerEnabled ? "🔊 Enabled            ║" : "🔇 Muted              ║");
  
  Serial.print("║  Breakers: ");
  Serial.print(breaker1State ? "1:ON " : "1:OFF");
  Serial.print(" ");
  Serial.println(breaker2State ? "2:ON            ║" : "2:OFF           ║");
  
  unsigned long uptime = millis() / 1000;
  Serial.print("║  Uptime:  ");
  Serial.print(uptime / 3600); Serial.print("h ");
  Serial.print((uptime % 3600) / 60); Serial.print("m ");
  Serial.print(uptime % 60); Serial.println("s             ║");
  
  Serial.println("╠═══════════════════════════════════╣");
  Serial.println("║         ACTIVE ALERTS             ║");
  Serial.println("╠═══════════════════════════════════╣");
  
  bool anyAlert = false;
  if (waterAlertActive) { Serial.println("║  💧 Water detected              ║"); anyAlert = true; }
  if (gasAlertActive) { Serial.println("║  💨 Gas detected                ║"); anyAlert = true; }
  if (tempAlertActive) { Serial.println("║  🔥 High temperature            ║"); anyAlert = true; }
  if (gyroAlertActive) { Serial.println("║  📳 Ground movement             ║"); anyAlert = true; }
  if (powerAlertActive) { Serial.println("║  ⚡ Power abnormal              ║"); anyAlert = true; }
  if (!anyAlert) { Serial.println("║  ✅ No active alerts            ║"); }
  
  Serial.println("╚═══════════════════════════════════╝\n");
}

void printHelp() {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║         AVAILABLE COMMANDS        ║");
  Serial.println("╠═══════════════════════════════════╣");
  Serial.println("║  status  - System status          ║");
  Serial.println("║  sensors - Current sensor values  ║");
  Serial.println("║  wifi    - WiFi information       ║");
  Serial.println("║  scan    - Scan WiFi networks     ║");
  Serial.println("║  shake   - Test gyro servos       ║");
  Serial.println("║  b1on/b1off - Breaker 1 control   ║");
  Serial.println("║  b2on/b2off - Breaker 2 control   ║");
  Serial.println("║  tripall - Trip all breakers      ║");
  Serial.println("║  resetall - Reset all breakers    ║");
  Serial.println("║  help    - Show this help         ║");
  Serial.println("╚═══════════════════════════════════╝\n");
}

// ===================================================================
// COMMUNICATION FUNCTIONS (MQTT)
// ===================================================================
void sendSensorData(bool forceImmediate) {
  if (!mqttClient.connected() || !systemEnabled) return;
  
  unsigned long currentTime = millis();
  if (!forceImmediate && (currentTime - lastDataSendTime < dataSendInterval)) {
    return;
  }
  
  jsonDoc.clear();
  
  JsonArray waterArray = jsonDoc.createNestedArray("water");
  waterArray.add(analogRead(WATER1));
  waterArray.add(analogRead(WATER2));
  waterArray.add(analogRead(WATER3));
  waterArray.add(analogRead(WATER4));
  
  jsonDoc["gas"] = (digitalRead(GAS_DO) == LOW);
  
  JsonObject tempObj = jsonDoc.createNestedObject("temperature");
  tempObj["temp1"] = readTemp(TEMP1);
  tempObj["temp2"] = readTemp(TEMP2);
  
  JsonObject gyroObj = jsonDoc.createNestedObject("gyro");
  float gyroMovement = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
  gyroObj["movement"] = gyroMovement;
  gyroObj["x"] = mpu.getGyroX();
  gyroObj["y"] = mpu.getGyroY();
  gyroObj["z"] = mpu.getGyroZ();
  
  if (enableVoltageSensors || enableCurrentSensors) {
    JsonObject powerObj = jsonDoc.createNestedObject("power_readings");
    
    if (enableVoltageSensors) {
      powerObj["voltage1"] = convertVoltage(analogRead(VOLTAGE_SENSOR_1));
      powerObj["voltage2"] = convertVoltage(analogRead(VOLTAGE_SENSOR_2));
    }
    
    if (enableCurrentSensors) {
      powerObj["current1"] = convertCurrent(analogRead(CURRENT_SENSOR_1));
      powerObj["current2"] = convertCurrent(analogRead(CURRENT_SENSOR_2));
    }
  }
  
  JsonObject breakerObj = jsonDoc.createNestedObject("breakers");
  breakerObj["breaker1"] = breaker1State;
  breakerObj["breaker2"] = breaker2State;
  
  jsonDoc["system_enabled"] = systemEnabled;
  jsonDoc["buzzer_enabled"] = buzzerEnabled;
  jsonDoc["uptime"] = millis() / 1000;
  
  String output;
  serializeJson(jsonDoc, output);
  publishMessage(output.c_str());
  
  lastDataSendTime = currentTime;
}

void sendAlert(String alertType, String details) {
  if (!mqttClient.connected()) return;
  
  jsonDoc.clear();
  jsonDoc["alert"] = alertType;
  
  if (alertType == "WATER_DETECTED") {
    // Include which sensor triggered and its value
    int w1 = analogRead(WATER1);
    int w2 = analogRead(WATER2);
    int w3 = analogRead(WATER3);
    int w4 = analogRead(WATER4);
    
    if (w1 > waterThreshold) {
      jsonDoc["sensor"] = "WATER1";
      jsonDoc["value"] = w1;
    } else if (w2 > waterThreshold) {
      jsonDoc["sensor"] = "WATER2";
      jsonDoc["value"] = w2;
    } else if (w3 > waterThreshold) {
      jsonDoc["sensor"] = "WATER3";
      jsonDoc["value"] = w3;
    } else if (w4 > waterThreshold) {
      jsonDoc["sensor"] = "WATER4";
      jsonDoc["value"] = w4;
    }
    // Include all water sensor readings for reference
    jsonDoc["water1"] = w1;
    jsonDoc["water2"] = w2;
    jsonDoc["water3"] = w3;
    jsonDoc["water4"] = w4;
    
  } else if (alertType == "GAS_LEAK_DETECTED") {
    // Gas sensor is digital, but we include it for consistency
    jsonDoc["sensor"] = "GAS";
    jsonDoc["value"] = 1;  // 1 = gas detected
    
  } else if (alertType == "HIGH_TEMPERATURE") {
    float t1 = readTemp(TEMP1);
    float t2 = readTemp(TEMP2);
    jsonDoc["value"] = max(t1, t2);
    jsonDoc["temp1"] = t1;
    jsonDoc["temp2"] = t2;
    if (t1 >= t2) {
      jsonDoc["sensor"] = "TEMP1";
    } else {
      jsonDoc["sensor"] = "TEMP2";
    }
    
  } else if (alertType == "GROUND_MOVEMENT_DETECTED") {
    float intensity = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
    jsonDoc["intensity"] = intensity;
    jsonDoc["value"] = intensity;
    jsonDoc["sensor"] = "GYRO";
    jsonDoc["gyro_x"] = mpu.getGyroX();
    jsonDoc["gyro_y"] = mpu.getGyroY();
    jsonDoc["gyro_z"] = mpu.getGyroZ();
    
  } else if (alertType == "POWER_ABNORMAL") {
    jsonDoc["sensor"] = "POWER";
    if (enableVoltageSensors) {
      float v1 = convertVoltage(analogRead(VOLTAGE_SENSOR_1));
      float v2 = convertVoltage(analogRead(VOLTAGE_SENSOR_2));
      jsonDoc["voltage1"] = v1;
      jsonDoc["voltage2"] = v2;
      jsonDoc["value"] = max(v1, v2);
    }
    if (enableCurrentSensors) {
      float c1 = convertCurrent(analogRead(CURRENT_SENSOR_1));
      float c2 = convertCurrent(analogRead(CURRENT_SENSOR_2));
      jsonDoc["current1"] = c1;
      jsonDoc["current2"] = c2;
      if (!enableVoltageSensors) {
        jsonDoc["value"] = max(c1, c2);
      }
    }
    
  } else if (alertType == "MULTIPLE_HAZARDS") {
    // For multiple hazards, include a summary
    jsonDoc["sensor"] = "MULTIPLE";
    jsonDoc["value"] = details.toInt();  // Number of active hazards
  }
  
  if (details.length() > 0) {
    jsonDoc["details"] = details;
  }
  
  jsonDoc["timestamp"] = millis() / 1000;
  jsonDoc["threshold"] = getThresholdForAlert(alertType);
  
  String output;
  serializeJson(jsonDoc, output);
  publishMessage(output.c_str());
}

// Helper function to get the threshold for each alert type
float getThresholdForAlert(String alertType) {
  if (alertType == "WATER_DETECTED") return waterThreshold;
  if (alertType == "HIGH_TEMPERATURE") return tempThreshold;
  if (alertType == "GROUND_MOVEMENT_DETECTED") return gyroThreshold;
  if (alertType == "POWER_ABNORMAL") return voltageThreshold;
  return 0;
}

void sendAlertCleared(String alertType) {
  if (!mqttClient.connected()) return;
  
  jsonDoc.clear();
  jsonDoc["status"] = "ALERT_CLEARED";
  jsonDoc["type"] = alertType;
  jsonDoc["timestamp"] = millis() / 1000;
  
  String output;
  serializeJson(jsonDoc, output);
  publishMessage(output.c_str());
}

void sendSystemStatus() {
  if (!mqttClient.connected()) return;
  
  jsonDoc.clear();
  jsonDoc["message_type"] = "SYSTEM_STATUS";
  jsonDoc["device_id"] = deviceId;
  jsonDoc["system_enabled"] = systemEnabled;
  jsonDoc["buzzer_enabled"] = buzzerEnabled;
  
  JsonObject thresh = jsonDoc.createNestedObject("thresholds");
  thresh["water"] = waterThreshold;
  thresh["temperature"] = tempThreshold;
  thresh["gyro"] = gyroThreshold;
  thresh["voltage"] = voltageThreshold;
  thresh["current"] = currentThreshold;
  
  JsonObject sensors = jsonDoc.createNestedObject("sensors");
  sensors["voltage_enabled"] = enableVoltageSensors;
  sensors["current_enabled"] = enableCurrentSensors;
  
  JsonObject breakers = jsonDoc.createNestedObject("breakers");
  breakers["breaker1"] = breaker1State;
  breakers["breaker2"] = breaker2State;
  
  jsonDoc["uptime"] = millis() / 1000;
  jsonDoc["firmware_version"] = "3.1-MQTT-AlertOnly";
  
  String output;
  serializeJson(jsonDoc, output);
  publishMessage(output.c_str());
}

void sendAckResponse(String message) {
  if (!mqttClient.connected()) return;
  
  jsonDoc.clear();
  jsonDoc["status"] = "OK";
  jsonDoc["message"] = message;
  jsonDoc["device_id"] = deviceId;
  
  String output;
  serializeJson(jsonDoc, output);
  publishMessage(output.c_str());
}

// ===================================================================
// SETUP
// ===================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(GAS_DO, INPUT);
  pinMode(BUZZER, OUTPUT);
  
  initServos();

  Wire.begin(21, 22);
  mpu.begin();
  Serial.println("Calibrating gyro...");
  mpu.calcGyroOffsets(true);

  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  ESP32 SAFETY MONITORING SYSTEM   ║");
  Serial.println("║     Version 3.1 (Alert-Only)      ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  // ==================== WIFI SETUP ====================
  Serial.print("🌐 Connecting to: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    prevWifiConnected = true;
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║      ✅ WiFi CONNECTED!           ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.print("║  IP: ");
    Serial.print(WiFi.localIP());
    Serial.println("            ║");
    Serial.print("║  Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm               ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    // Generate Device ID from MAC address
    deviceId = getDeviceId();
    telemetryTopic = "apn/device/" + deviceId + "/telemetry";
    commandTopic = "apn/device/" + deviceId + "/commands";
    
    Serial.println("📱 Device Configuration:");
    Serial.print("   Device ID: ");
    Serial.println(deviceId);
    Serial.print("   Telemetry Topic: ");
    Serial.println(telemetryTopic);
    Serial.print("   Command Topic: ");
    Serial.println(commandTopic);
    Serial.println();
    
    // Setup MQTT with TLS
    espClient.setCACert(root_ca);
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(1024);  // Increase buffer for larger messages
    
    // Connect to MQTT
    mqttReconnect();
    
  } else {
    Serial.println("\n❌ WiFi FAILED - Check SSID/password");
    Serial.println("   ⚠️  Remember: ESP32 only works with 2.4GHz WiFi!");
    Serial.println("   Type 'scan' to see available networks\n");
  }
  
  Serial.println("✅ System Ready - Type 'help' for commands");
  Serial.println("─────────────────────────────────────────\n");
  
  // Initialize previous states
  prevBreaker1State = breaker1State;
  prevBreaker2State = breaker2State;
  prevSystemEnabled = systemEnabled;
  prevBuzzerEnabled = buzzerEnabled;
  
  // Startup beep
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  
  lastHeartbeat = millis();
}

// ===================================================================
// MAIN LOOP
// ===================================================================
void loop() {
  unsigned long currentTime = millis();

  // Handle MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      // Attempt reconnection with backoff
      if (currentTime - lastMqttReconnectAttempt > mqttReconnectInterval) {
        lastMqttReconnectAttempt = currentTime;
        if (mqttReconnect()) {
          lastMqttReconnectAttempt = 0;
        }
      }
    } else {
      mqttClient.loop();
    }
  }
  
  // ==================== WIFI STATUS CHECK ====================
  if (currentTime - lastWiFiCheck > 10000) {
    lastWiFiCheck = currentTime;
    
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    // Only print on WiFi state change
    if (wifiConnected != prevWifiConnected) {
      if (wifiConnected) {
        Serial.println("\n╔═══════════════════════════════════╗");
        Serial.println("║      ✅ WiFi CONNECTED!           ║");
        Serial.println("╠═══════════════════════════════════╣");
        Serial.print("║  IP: ");
        Serial.print(WiFi.localIP());
        Serial.println("            ║");
        Serial.print("║  Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm               ║");
        Serial.println("╚═══════════════════════════════════╝\n");
        
        wifiConnecting = false;
      } else {
        Serial.println("\n⚠️  WiFi DISCONNECTED!\n");
      }
      prevWifiConnected = wifiConnected;
    }
    
    // Try to reconnect if disconnected
    if (!wifiConnected && !wifiConnecting) {
      wifiConnecting = true;
      WiFi.disconnect(true);
      delay(500);
      WiFi.begin(ssid, password);
    }
  }
  
  // ==================== MQTT CONNECTION CHANGE ====================
  bool currentMqttConnected = mqttClient.connected();
  if (currentMqttConnected != prevMqttConnected) {
    if (currentMqttConnected) {
      Serial.println("📡 MQTT CONNECTED to cloud");
    } else {
      Serial.println("📡 MQTT DISCONNECTED");
    }
    prevMqttConnected = currentMqttConnected;
  }

  // ==================== SERIAL COMMANDS ====================
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "shake") {
      testGyroShake();
    } else if (command == "b1on") {
      setBreakerState(1, false);
    } else if (command == "b1off") {
      setBreakerState(1, true);
    } else if (command == "b2on") {
      setBreakerState(2, false);
    } else if (command == "b2off") {
      setBreakerState(2, true);
    } else if (command == "tripall") {
      tripAllBreakers();
    } else if (command == "resetall") {
      resetAllBreakers();
    } else if (command == "status") {
      printFullStatus();
    } else if (command == "wifi") {
      printWiFiInfo();
    } else if (command == "scan") {
      scanNetworks();
    } else if (command == "sensors") {
      printSensorReadings();
    } else if (command == "help") {
      printHelp();
    }
  }

  // Only run monitoring if system is enabled
  if (!systemEnabled) {
    if (prevSystemEnabled) {
      Serial.println("⏸️  System DISABLED");
      prevSystemEnabled = false;
    }
    delay(100);
    return;
  } else if (!prevSystemEnabled) {
    Serial.println("▶️  System ENABLED");
    prevSystemEnabled = true;
  }

  // ==================== READ SENSORS ====================
  int w1 = analogRead(WATER1);
  int w2 = analogRead(WATER2);
  int w3 = analogRead(WATER3);
  int w4 = analogRead(WATER4);

  bool waterDetected = (w1 > waterThreshold) || (w2 > waterThreshold) ||
                       (w3 > waterThreshold) || (w4 > waterThreshold);

  bool gasDetected = (digitalRead(GAS_DO) == LOW);

  float t1 = readTemp(TEMP1);
  float t2 = readTemp(TEMP2);
  float maxTemp = max(t1, t2);

  bool tempCritical = (t1 >= tempThreshold || t2 >= tempThreshold);
  bool tempWarning = (maxTemp >= tempWarningLevel && maxTemp < tempThreshold);

  mpu.update();
  float gyroMovement = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
  bool gyroDetected = gyroMovement > gyroThreshold;

  int v1 = analogRead(VOLTAGE_SENSOR_1);
  int v2 = analogRead(VOLTAGE_SENSOR_2);
  int c1 = analogRead(CURRENT_SENSOR_1);
  int c2 = analogRead(CURRENT_SENSOR_2);

  bool powerCritical = false;
  bool powerWarning = false;
  
  if (enableVoltageSensors || enableCurrentSensors) {
    powerCritical =
      (enableVoltageSensors && (v1 > voltageThreshold || v2 > voltageThreshold)) ||
      (enableCurrentSensors && (c1 > currentThreshold || c2 > currentThreshold));
      
    powerWarning =
      (enableVoltageSensors && ((v1 > voltageWarning && v1 <= voltageThreshold) ||
                                (v2 > voltageWarning && v2 <= voltageThreshold))) ||
      (enableCurrentSensors && ((c1 > currentWarning && c1 <= currentThreshold) ||
                                (c2 > currentWarning && c2 <= currentThreshold)));
  }

  int hazardCount = 0;
  if (waterDetected) hazardCount++;
  if (gasDetected) hazardCount++;
  if (tempCritical) hazardCount++;
  if (gyroDetected) hazardCount++;
  if (powerCritical) hazardCount++;

  bool multipleHazards = hazardCount >= 2;

  // ==================== PRINT ONLY ON STATE CHANGES ====================
  
  // Water state change
  if (waterDetected != prevWaterDetected) {
    if (waterDetected) {
      Serial.println("\n🚨 ALERT: 💧 WATER DETECTED!");
      Serial.printf("   Sensors: W1=%d W2=%d W3=%d W4=%d (threshold: %d)\n\n", 
                    w1, w2, w3, w4, waterThreshold);
    } else {
      Serial.println("✅ CLEARED: Water level normal\n");
    }
    prevWaterDetected = waterDetected;
  }
  
  // Gas state change
  if (gasDetected != prevGasDetected) {
    if (gasDetected) {
      Serial.println("\n🚨 ALERT: 💨 GAS LEAK DETECTED!\n");
    } else {
      Serial.println("✅ CLEARED: Gas level normal\n");
    }
    prevGasDetected = gasDetected;
  }
  
  // Temperature critical state change
  if (tempCritical != prevTempCritical) {
    if (tempCritical) {
      Serial.println("\n🚨 ALERT: 🔥 HIGH TEMPERATURE!");
      Serial.printf("   Temp1=%.1f°C Temp2=%.1f°C (threshold: %.1f°C)\n\n", 
                    t1, t2, tempThreshold);
    } else {
      Serial.println("✅ CLEARED: Temperature normal\n");
    }
    prevTempCritical = tempCritical;
  }
  
  // Temperature warning state change
  if (!tempCritical && tempWarning != prevTempWarning) {
    if (tempWarning) {
      Serial.printf("⚠️  WARNING: Temperature elevated (%.1f°C)\n", maxTemp);
    }
    prevTempWarning = tempWarning;
  }
  
  // Gyro state change
  if (gyroDetected != prevGyroDetected) {
    if (gyroDetected) {
      Serial.println("\n🚨 ALERT: 📳 GROUND MOVEMENT DETECTED!");
      Serial.printf("   Intensity: %.2f (threshold: %.2f)\n\n", 
                    gyroMovement, gyroThreshold);
    } else {
      Serial.println("✅ CLEARED: Movement stopped\n");
    }
    prevGyroDetected = gyroDetected;
  }
  
  // Power critical state change
  if (powerCritical != prevPowerCritical) {
    if (powerCritical) {
      Serial.println("\n🚨 ALERT: ⚡ POWER ABNORMAL!");
      if (enableVoltageSensors) {
        Serial.printf("   Voltage: V1=%d V2=%d\n", v1, v2);
      }
      if (enableCurrentSensors) {
        Serial.printf("   Current: C1=%d C2=%d\n", c1, c2);
      }
      Serial.println();
    } else {
      Serial.println("✅ CLEARED: Power normal\n");
    }
    prevPowerCritical = powerCritical;
  }
  
  // Power warning state change
  if (!powerCritical && powerWarning != prevPowerWarning) {
    if (powerWarning) {
      Serial.println("⚠️  WARNING: Power levels elevated");
    }
    prevPowerWarning = powerWarning;
  }
  
  // Breaker state changes
  if (breaker1State != prevBreaker1State) {
    Serial.printf("⚡ Breaker 1: %s\n", breaker1State ? "ON" : "OFF (TRIPPED)");
    prevBreaker1State = breaker1State;
  }
  if (breaker2State != prevBreaker2State) {
    Serial.printf("⚡ Breaker 2: %s\n", breaker2State ? "ON" : "OFF (TRIPPED)");
    prevBreaker2State = breaker2State;
  }
  
  // Buzzer state change
  if (buzzerEnabled != prevBuzzerEnabled) {
    Serial.printf("🔊 Buzzer: %s\n", buzzerEnabled ? "ENABLED" : "MUTED");
    prevBuzzerEnabled = buzzerEnabled;
  }
  
  // Multiple hazards warning
  if (multipleHazards) {
    Serial.printf("🚨🚨 CRITICAL: %d HAZARDS ACTIVE! 🚨🚨\n", hazardCount);
  }

  // ==================== HEARTBEAT ====================
  if (heartbeatInterval > 0 && (currentTime - lastHeartbeat >= heartbeatInterval)) {
    if (!waterDetected && !gasDetected && !tempCritical && !gyroDetected && !powerCritical) {
      Serial.printf("💚 System OK | Uptime: %lus | WiFi: %s | MQTT: %s\n",
                    currentTime / 1000,
                    (WiFi.status() == WL_CONNECTED) ? "✅" : "❌",
                    mqttClient.connected() ? "✅" : "⏳");
    }
    lastHeartbeat = currentTime;
  }

  // ==================== ALERT HANDLING ====================
  if (multipleHazards) {
    buzzCritical();
    tripAllBreakers();
    if (currentTime - lastAlertTime > alertCooldown) {
      sendAlert("MULTIPLE_HAZARDS", String(hazardCount) + " active");
      lastAlertTime = currentTime;
    }
  }
  else if (waterDetected) {
    buzzWater();
    tripAllBreakers();
    if (!waterAlertActive) {
      sendAlert("WATER_DETECTED");
      waterAlertActive = true;
    }
  } 
  else if (gasDetected) {
    buzzGas();
    tripAllBreakers();
    if (!gasAlertActive) {
      sendAlert("GAS_LEAK_DETECTED");
      gasAlertActive = true;
    }
  }
  else if (tempCritical) {
    buzzTemp();
    tripAllBreakers();
    if (!tempAlertActive) {
      sendAlert("HIGH_TEMPERATURE");
      tempAlertActive = true;
    }
  }
  else if (gyroDetected) {
    buzzGyro();
    if (!gyroAlertActive) {
      sendAlert("GROUND_MOVEMENT_DETECTED");
      gyroAlertActive = true;
    }
  }
  else if (powerCritical) {
    buzzPower();
    tripAllBreakers();
    if (!powerAlertActive) {
      sendAlert("POWER_ABNORMAL");
      powerAlertActive = true;
    }
  }
  else if (tempWarning || powerWarning) {
    buzzWarning();
  }
  else {
    // All clear - send cleared alerts
    if (waterAlertActive) {
      sendAlertCleared("WATER");
      waterAlertActive = false;
    }
    if (gasAlertActive) {
      sendAlertCleared("GAS");
      gasAlertActive = false;
    }
    if (tempAlertActive) {
      sendAlertCleared("TEMPERATURE");
      tempAlertActive = false;
    }
    if (gyroAlertActive) {
      sendAlertCleared("MOVEMENT");
      gyroAlertActive = false;
    }
    if (powerAlertActive) {
      sendAlertCleared("POWER");
      powerAlertActive = false;
    }
    
    digitalWrite(BUZZER, LOW);
  }

  // Optional: Send periodic sensor data (only if enabled, default is OFF)
  // This is useful for confirming device is alive, but set to 5 minute intervals
  if (enablePeriodicUpdates && periodicUpdateInterval > 0) {
    if (currentTime - lastPeriodicUpdate >= periodicUpdateInterval) {
      sendSensorData(true);
      lastPeriodicUpdate = currentTime;
    }
  }

  delay(100);
}

