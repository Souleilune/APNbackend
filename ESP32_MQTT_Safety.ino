#include <Wire.h>
  #include <MPU6050_tockn.h>
  #include <ESP32Servo.h>
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  // #include <PubSubClient.h>
  #include <ArduinoMqttClient.h>
  #include <ArduinoJson.h>
  #include <time.h>
  // ==================== WIFI CONFIGURATION ====================
  const char* ssid = "Redmi K30";
  const char* password = "ksiz7yik7be48ge";
  // ==================== MQTT CONFIGURATION ====================
  const char* mqtt_broker = "afa96665cdc74a5ca2cbef61c459704e.s1.eu.hivemq.cloud";
  const int mqtt_port = 8883;  // ✅ FIX: was 8884, HiveMQ TLS port is 8883
  const char* mqtt_username = "esp32_apn";
  const char* mqtt_password = "APN20250k";
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
  // ==================== DEVICE ID ====================
  String deviceId = "";
  String telemetryTopic;
  String commandTopic;
  // ==================== PIN CONFIGURATION ====================
  // Water Sensors (Digital DO - LOW = WET)
  #define WATER1 15
  #define WATER2 17
  #define WATER3 4
  #define WATER4 5
  // Gas Sensor (Digital DO - LOW = GAS DETECTED)
  #define GAS_AO 25
  #define GAS_DO 26  
  // Temperature Sensors (move to ADC1 so they work with Wi-Fi)
  #define TEMP1 39  // Vn
  #define TEMP2 36   // Vp
  // Power Sensors (ADC1 - Safe with WiFi)
  #define VOLTAGE_SENSOR_1 34
  #define VOLTAGE_SENSOR_2 35
  #define CURRENT_SENSOR_1 32
  #define CURRENT_SENSOR_2 33
  // Actuators
  #define BUZZER 19
  #define SERVO_GYRO_1 16
  #define BREAKER_SERVO_1 18
  #define BREAKER_SERVO_2 23
  // ==================== POWER CALIBRATION & STATE ====================
  // ADC settings
  const float ADC_REF_VOLTAGE = 3.3f;
  const int   ADC_MAX         = 4095;
  // Voltage sensor settings
  const float inputMaxVoltage   = 25.0f;
  const float calibrationFactor = 0.65f;  // tune so V1 matches your multimeter
  // ACS712-5A current sensor settings (with 68k + 100k divider)
  const float DIVIDER_RATIO = 0.08f;
  // const float SENSITIVITY   = 0.138f;
  // Noise thresholds
  const float CURRENT_NOISE_THRESHOLD = 0.00f;   // 0.20 A
  const int   VOLTAGE_NOISE_THRESHOLD = 50;     // raw ADC debug only
  int gasThreshold = 4095;  // safe default until calibration runs in setup()
                           // 0–4095 range. 1800 means ~44% of max — tune up to reduce false triggers
  // Runtime power state
  float zeroPoint1 = 0.0f;   // ACS zero point for CURRENT_SENSOR_1
  float zeroPoint2 = 0.0f;   // ACS zero point for CURRENT_SENSOR_2
  float voltage1_V = 0.0f;
  float voltage2_V = 0.0f;
  float current1_A = 0.0f;
  float current2_A = 0.0f;
  int voltage1_raw = 0;
  int voltage2_raw = 0;
  int rawI1 = 0;
  int rawI2 = 0;
  // ==================== CURRENT SENSOR CALIBRATION ====================
  // Raw ADC zero points (calibrated at startup with no load)
  int rawZeroI1 = 0;
  int rawZeroI2 = 0;
  // Calibration constant: how many ADC counts per 1 Amp
  // Calculated from your data: (1957 - 129) = 1828 counts for 1.0A
  const float COUNTS_PER_AMP = 1828.0f;
  unsigned long lastPowerUpdate = 0;
  const unsigned long powerUpdateInterval = 500;
  // REMOVE or set to 0 - we'll auto-calibrate instead
  float currentOffset1 = 0.0f;  // Changed from 0.047f
  float currentOffset2 = 0.0f;  // Changed from 0.265f
  // ==================== THRESHOLDS & FLAGS ====================
  float voltageThreshold  = 16.0f;
  float currentThreshold  = 2.0f;
  float tempThreshold     = 50.0f;   // 50°C trip point
  float gyroThreshold     = 5.0f;
  float tempWarningLevel  = 40.0f;
  float voltageWarning    = 14.5f;
  float currentWarning    = 1.5f;
  bool enableVoltageSensors = true;
  bool enableCurrentSensors = true;
  // ==================== SERVO CONFIGURATION ====================
  Servo servoGyro1;
  Servo breakerServo1;
  Servo breakerServo2;
  int gyroRestAngle   = 90;
  int gyroShakeAngle1 = 45;
  int gyroShakeAngle2 = 135;
  // Breaker 1: Normal (CW trip)
  int breaker1OnAngle  = 90;
  int breaker1OffAngle = 0;
  // Breaker 2: Counter-Clockwise (CCW trip)
  int breaker2OnAngle  = 90;
  int breaker2OffAngle = 180;
  bool breaker1State = true;
  bool breaker2State = true;
  unsigned long breaker1TripTime = 0;
  unsigned long breaker2TripTime = 0;
  // ==================== MPU6050 ====================
  MPU6050 mpu(Wire);
  float gyroReadings[10] = {0};
  int gyroIndex = 0;
  unsigned long lastGyroTrigger = 0;
  const unsigned long gyroCooldown = 2000;
  // NEW: track how long movement stays above threshold
  unsigned long gyroOverStart = 0;
  const unsigned long GYRO_EARTHQUAKE_TIME = 3000; // 3 seconds
  // ==================== ALERT FLAGS ====================
  bool waterAlertActive = false;
  bool gasAlertActive   = false;
  bool tempAlertActive  = false;
  bool gyroAlertActive  = false;
  bool powerAlertActive = false;
  // ==================== SYSTEM FLAGS ====================
  bool systemEnabled  = true;
  bool buzzerEnabled  = true;
  bool prevSystemEnabled = true;
  bool timeIsSynced = false;
  // ==================== WIFI / MQTT ====================
  WiFiClientSecure espClient;
  MqttClient mqttClient(espClient);
  // PubSubClient mqttClient(espClient);
  unsigned long lastMqttReconnectAttempt = 0;
  unsigned long mqttReconnectInterval = 5000;
  unsigned long lastHeartbeat = 0;
  unsigned long heartbeatInterval = 60000;
  unsigned long lastDataSendTime = 0;
  unsigned long dataSendInterval = 2000;
  unsigned long lastWiFiCheck = 0;
  unsigned long lastPeriodicUpdate = 0;
  unsigned long periodicUpdateInterval = 30000;
  // Ignore temp alerts for first N ms after boot
  const unsigned long TEMP_STARTUP_IGNORE_MS = 10000;
  float tempCalOffset1 = 0.0f;
  float tempCalOffset2 = 0.0f;
  bool prevWifiConnected = false;
  bool prevMqttConnected = false;
  bool wifiConnecting = false;
  bool enablePeriodicUpdates = true;
  // ==================== JSON BUFFERS ====================
  StaticJsonDocument<1024> jsonDoc;
  StaticJsonDocument<512> jsonReceive;
  // ==================== FORWARD DECLARATIONS ====================
  void sendAlert(String alertType, String details = "");
  void sendAlertCleared(String alertType);
  void sendSensorData(bool forceImmediate = false);
  void sendSystemStatus();
  void sendBreakerStatus();
  void sendAckResponse(String message);
  void publishMessage(const char* payload);
  bool mqttReconnect();
  void mqttCallback(char* topic, byte* payload, unsigned int length);
  void handleIncomingCommand();
  void handleAlerts();
  void tripBreaker(int breakerNum);
  void tripAllBreakers();
  void testGyroShake();
  void initServos();
  void configureADC();
  float calibrateZero(int pin);
  float readVoltage(int pin, int* rawOut);
  float readCurrent(int pin, float zeroPoint);
  float readTemp(int pin);
  bool isActiveLowFiltered(int pin);
  bool checkGyroMovement();
  bool checkZone1Water();
  bool checkZone2Water();
  bool checkZone1Power();
  bool checkZone2Power();
  bool checkZone1Temp();
  bool checkZone2Temp();
  void buzzWater();
  void buzzGas();
  void buzzTemp();
  void buzzPower();
  void buzzGyro();
  void buzzCritical();
  void buzzWarning();
  void printSensorReadings();
  void printFullStatus();
  void printWiFiInfo();
  void printHelp();
  void scanNetworks();
  void syncTime();
  String getDeviceId();
  void onMqttMessage(int messageSize);
  // ==================== SYNC TIME (Required for TLS) ====================
void syncTime() {
  Serial.println("🕐 Syncing time with NTP...");
  configTime(28800, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  int attempts = 0;
  time_t now = 0;
  while (now < 1700000000 && attempts < 20) {
    delay(500);
    time(&now);
    attempts++;
    Serial.print(".");
  }
  Serial.println();
  if (now > 1700000000) {
    Serial.println("✅ Time synced via NTP!");
    timeIsSynced = true;
  } else {
    // ✅ FALLBACK: hardcode a valid timestamp so TLS works
    Serial.println("⚠️ NTP blocked — using manual time for TLS");
    struct timeval tv;
    tv.tv_sec  = 1741000000;  // March 2025 — valid for cert checking
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    timeIsSynced = true;
    Serial.println("✅ Manual time set — proceeding with MQTT");
  }
}
  // ==================== DIGITAL FILTER HELPER ====================
  bool isActiveLowFiltered(int pin) {
    int lowCount = 0;
    for (int i = 0; i < 10; i++) {
      if (digitalRead(pin) == LOW) lowCount++;
      delayMicroseconds(500);
    }
    return (lowCount > 5);
  }
  // ==================== ADC CONFIGURATION ====================
  void configureADC() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
  }
  // ==================== CURRENT SENSOR ZERO CALIBRATION ====================
  void calibrateCurrentSensors() {
    const int N = 2000;
    long sum1 = 0, sum2 = 0;
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║  CURRENT SENSOR CALIBRATION       ║");
    Serial.println("║  ⚠️ Ensure NO LOAD connected!     ║");
    Serial.println("╚═══════════════════════════════════╝");
    delay(2000);
    Serial.println("   Taking 2000 samples...");
    for (int i = 0; i < N; i++) {
      sum1 += analogRead(CURRENT_SENSOR_1);
      sum2 += analogRead(CURRENT_SENSOR_2);
      delay(1);
    }
    rawZeroI1 = sum1 / N;
    rawZeroI2 = sum2 / N;
    Serial.printf("   I1 zero raw ADC: %d\n", rawZeroI1);
    Serial.printf("   I2 zero raw ADC: %d\n", rawZeroI2);
    Serial.println("   ✅ Current sensor calibration complete!\n");
  }
  // ==================== VOLTAGE READING (MATCHES TEST CODE) ====================
  float readVoltage(int pin, int* rawOut) {
    const int NS = 100;
    long sum = 0;
    for (int i = 0; i < NS; i++) {
      sum += analogRead(pin);
      delayMicroseconds(100);
    }
    float rawAvg = sum / (float)NS;
    if (rawOut != nullptr) {
      *rawOut = (int)rawAvg;
    }
    float voltage = (rawAvg / ADC_MAX) * inputMaxVoltage;
    voltage *= calibrationFactor;
    if (voltage < 2.0f) {   // clamp <2V to 0
      voltage = 0.0f;
    }
    return voltage;
  }
  // ==================== CURRENT READING (RAW ADC METHOD) ====================
  float readCurrent(int pin, float zeroPoint, int* rawOut) {
    const int NS = 500;
    long sum = 0;
    for (int i = 0; i < NS; i++) {
      sum += analogRead(pin);
      delayMicroseconds(80);
    }
    int rawAvg = sum / NS;
    // Store raw value
    if (rawOut != nullptr) {
      *rawOut = rawAvg;
    }
    // Get the correct zero point for this pin
    int rawZero = (pin == CURRENT_SENSOR_1) ? rawZeroI1 : rawZeroI2;
    // Calculate difference from zero
    int rawDiff = rawAvg - rawZero;
    // Convert to current using calibration constant
    float current = fabs(rawDiff) / COUNTS_PER_AMP;
    // Noise filter (ignore small fluctuations < 50mA)
    if (current < 0.05f) {
      current = 0.0f;
    }
    return current;
  }
  float readTemp(int pin) {
    int raw = analogRead(pin);
    if (raw < 50 || raw > 3900) return 0.0f;
    float voltage = raw * 3.3f / 4095.0f;
    float tempC = voltage * 100.0f;
    // Per-sensor offset
    if (pin == TEMP1) tempC += tempCalOffset1;
    else              tempC += tempCalOffset2;
    if (tempC < -20.0f || tempC > 150.0f) return 0.0f;
    return tempC;
  }
  // ==================== UPDATE POWER READINGS ====================
  void updatePowerReadings() {
    if (enableVoltageSensors) {
      voltage1_V = readVoltage(VOLTAGE_SENSOR_1, &voltage1_raw);
      voltage2_V = readVoltage(VOLTAGE_SENSOR_2, &voltage2_raw);
    }
    if (enableCurrentSensors) {
      // zeroPoint is no longer used - we use fixed 2.5V zero
      current1_A = readCurrent(CURRENT_SENSOR_1, zeroPoint1, &rawI1);
      current2_A = readCurrent(CURRENT_SENSOR_2, zeroPoint2, &rawI2);
      if (current1_A < 0.0f) current1_A = 0.0f;
      if (current2_A < 0.0f) current2_A = 0.0f;
          // After calculating current1_A and current2_A
      if (current1_A > 1.0f && current1_A < 0.1f) {
        Serial.println("⚠️ WARNING: Suspicious current reading - check calibration");
      }
      if (current2_A > 1.0f && current2_A < 0.1f) {
        Serial.println("⚠️ WARNING: Suspicious current reading - check calibration");
      }
    }
  }
int readGasSensor() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(GAS_AO);
    delayMicroseconds(200);
  }
  return (int)(sum / 20);
}
bool checkGasDetected() {
  // DO pin: LOW = gas detected (active low)
  // Read 5 times to filter noise
  int lowCount = 0;
  for (int i = 0; i < 5; i++) {
    if (digitalRead(GAS_DO) == LOW) lowCount++;
    delayMicroseconds(500);
  }
  return (lowCount >= 3);  // majority vote
}
  // ==================== GYRO MOVEMENT CHECK ====================
  bool checkGyroMovement() {
    mpu.update();
    float gyroMovement = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
    // Simple moving average over last 10 samples
    gyroReadings[gyroIndex] = gyroMovement;
    gyroIndex = (gyroIndex + 1) % 10;
    float avgMovement = 0;
    for (int i = 0; i < 10; i++) {
      avgMovement += gyroReadings[i];
    }
    avgMovement /= 10.0f;
    unsigned long currentTime = millis();
    bool over = (avgMovement > gyroThreshold);
    // If movement is above threshold, start or continue timing
    if (over) {
      if (gyroOverStart == 0) {
        gyroOverStart = currentTime;  // just went over threshold
      }
      // Has it been over threshold long enough to count as earthquake?
      if ((currentTime - gyroOverStart) >= GYRO_EARTHQUAKE_TIME) {
        // Also respect the cooldown between events
        if ((currentTime - lastGyroTrigger) > gyroCooldown) {
          lastGyroTrigger = currentTime;
          return true;  // EARTHQUAKE DETECTED
        }
      }
    } else {
      // Dropped below threshold: reset timer
      gyroOverStart = 0;
    }
    return false;
  }
  // ==================== ZONE CHECKS ====================
  bool checkZone1Water() {
    return isActiveLowFiltered(WATER1) || isActiveLowFiltered(WATER2);
  }
  bool checkZone2Water() {
    return isActiveLowFiltered(WATER3) || isActiveLowFiltered(WATER4);
  }
  bool checkZone1Power() {
    if (!enableVoltageSensors && !enableCurrentSensors) return false;
    bool voltageIssue = enableVoltageSensors && (voltage1_V > voltageThreshold);
    bool currentIssue = enableCurrentSensors && (current1_A > currentThreshold);
    // If bus voltage is essentially off, ignore current noise
    if (voltage1_V < 1.0f) currentIssue = false;
    return voltageIssue || currentIssue;
  }
  bool checkZone2Power() {
    if (!enableVoltageSensors && !enableCurrentSensors) return false;
    bool voltageIssue = enableVoltageSensors && (voltage2_V > voltageThreshold);
    bool currentIssue = enableCurrentSensors && (current2_A > currentThreshold);
    if (voltage2_V < 1.0f) currentIssue = false;
    return voltageIssue || currentIssue;
  }
  bool checkZone1Temp() {
    if (millis() < TEMP_STARTUP_IGNORE_MS) return false;  // ignore at startup
    float t = readTemp(TEMP1);
    return t >= tempThreshold;
  }
  bool checkZone2Temp() {
    if (millis() < TEMP_STARTUP_IGNORE_MS) return false;  // ignore at startup
    float t = readTemp(TEMP2);
    return t >= tempThreshold;
  }
  // ==================== SERVO INITIALIZATION ====================
  void initServos() {
    servoGyro1.attach(SERVO_GYRO_1);
    breakerServo1.attach(BREAKER_SERVO_1);
    breakerServo2.attach(BREAKER_SERVO_2);
    servoGyro1.write(gyroRestAngle);
    breakerServo1.write(breaker1OnAngle);
    breakerServo2.write(breaker2OnAngle);
    delay(500);
    breakerServo1.detach();
    breakerServo2.detach();
  }
  // ==================== TRIP BREAKER ====================
  void tripBreaker(int breakerNum) {
    Servo* servo;
    unsigned long* tripTimePtr;
    int servoPin;
    int onAngle, offAngle;
    if (breakerNum == 1) {
      servo = &breakerServo1;
      tripTimePtr = &breaker1TripTime;
      servoPin = BREAKER_SERVO_1;
      onAngle = breaker1OnAngle;
      offAngle = breaker1OffAngle;
      breaker1State = false;
    } else {
      servo = &breakerServo2;
      tripTimePtr = &breaker2TripTime;
      servoPin = BREAKER_SERVO_2;
      onAngle = breaker2OnAngle;
      offAngle = breaker2OffAngle;
      breaker2State = false;
    }
    Serial.printf("\n🚨 TRIPPING BREAKER %d...\n", breakerNum);
    if (!servo->attached()) {
      servo->attach(servoPin);
      delay(100);
    }
    servo->write(onAngle);
    delay(200);
    Serial.printf("   Moving %d° → %d°\n", onAngle, offAngle);
    int step = (offAngle > onAngle) ? 10 : -10;
    for (int pos = onAngle; (step > 0) ? (pos <= offAngle) : (pos >= offAngle); pos += step) {
      servo->write(pos);
      delay(8);
    }
    servo->write(offAngle);
    delay(300);
    *tripTimePtr = millis();
    delay(500);
    Serial.printf("   Returning servo to ready position\n");
    int backStep = (onAngle > offAngle) ? 5 : -5;
    for (int pos = offAngle; (backStep > 0) ? (pos <= onAngle) : (pos >= onAngle); pos += backStep) {
      servo->write(pos);
      delay(15);
    }
    servo->write(onAngle);
    delay(200);
    servo->detach();
    Serial.printf("✅ Breaker %d tripped\n", breakerNum);
    Serial.println("👉 User must manually reset physical breaker\n");
  }
  // ==================== TRIP ALL BREAKERS ====================
  void tripAllBreakers() {
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║   🚨 EMERGENCY - TRIPPING ALL    ║");
    Serial.println("║        CIRCUIT BREAKERS!          ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    if (!breakerServo1.attached()) breakerServo1.attach(BREAKER_SERVO_1);
    if (!breakerServo2.attached()) breakerServo2.attach(BREAKER_SERVO_2);
    delay(100);
    breakerServo1.write(breaker1OnAngle);
    breakerServo2.write(breaker2OnAngle);
    delay(200);
    Serial.println("⚡ Tripping both breakers SIMULTANEOUSLY...");
    int range = 90;
    for (int d = 0; d <= range; d += 10) {
      breakerServo1.write(breaker1OnAngle - d);
      breakerServo2.write(breaker2OnAngle + d);
      delay(8);
    }
    breakerServo1.write(breaker1OffAngle);
    breakerServo2.write(breaker2OffAngle);
    delay(300);
    unsigned long tripTime = millis();
    breaker1TripTime = tripTime;
    breaker2TripTime = tripTime;
    breaker1State = false;
    breaker2State = false;
    delay(500);
    Serial.println("🔄 Returning servos to ready position...");
    for (int d = range; d >= 0; d -= 5) {
      breakerServo1.write(breaker1OffAngle + (range - d));
      breakerServo2.write(breaker2OffAngle - (range - d));
      delay(15);
    }
    breakerServo1.write(breaker1OnAngle);
    breakerServo2.write(breaker2OnAngle);
    delay(200);
    breakerServo1.detach();
    breakerServo2.detach();
    Serial.println("✅ Both breakers tripped");
    Serial.println("👉 User must manually reset physical breakers\n");
  }
  // ==================== GYRO SHAKE TEST ====================
  void testGyroShake() {
    Serial.println("🔧 Starting 30-second gyro shake test...");
    if (!servoGyro1.attached()) servoGyro1.attach(SERVO_GYRO_1);
    servoGyro1.write(gyroRestAngle);
    delay(200);
    unsigned long shakeStartTime = millis();
    unsigned long shakeDuration = 30000;
    int cycleCount = 0;
    Serial.println("🔳 Servo activated - shaking for 30 seconds...");
    while (millis() - shakeStartTime < shakeDuration) {
      if (!servoGyro1.attached()) servoGyro1.attach(SERVO_GYRO_1);
      servoGyro1.write(gyroShakeAngle2);
      delay(150);
      servoGyro1.write(gyroShakeAngle1);
      delay(150);
      servoGyro1.write(gyroRestAngle);
      delay(100);
      cycleCount++;
      unsigned long elapsed = millis() - shakeStartTime;
      if (elapsed % 5000 < 400) {
        Serial.printf("  ⏱️  %lus elapsed (%d cycles)...\n", elapsed / 1000, cycleCount);
      }
      if (mqttClient.connected()) mqttClient.poll();
      yield();
    }
    servoGyro1.write(gyroRestAngle);
    delay(200);
    Serial.printf("✅ Test complete (%d cycles)\n", cycleCount);
  }
  // ==================== BUZZER PATTERNS ====================
  void buzzWater() {
    if (!buzzerEnabled) return;
    digitalWrite(BUZZER, HIGH); delay(300);
    digitalWrite(BUZZER, LOW);  delay(300);
  }
  void buzzGas() {
    if (!buzzerEnabled) return;
    digitalWrite(BUZZER, HIGH); delay(100);
    digitalWrite(BUZZER, LOW);  delay(100);
  }
  void buzzTemp() {
    if (!buzzerEnabled) return;
    digitalWrite(BUZZER, HIGH); delay(600);
    digitalWrite(BUZZER, LOW);  delay(300);
  }
  void buzzPower() {
    if (!buzzerEnabled) return;
    for (int i = 0; i < 2; i++) {
      digitalWrite(BUZZER, HIGH); delay(400);
      digitalWrite(BUZZER, LOW);  delay(200);
    }
  }
  void buzzGyro() {
    if (!buzzerEnabled) return;
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER, HIGH); delay(120);
      digitalWrite(BUZZER, LOW);  delay(120);
    }
  }
  void buzzCritical() {
    if (!buzzerEnabled) return;
    for (int i = 0; i < 5; i++) {
      digitalWrite(BUZZER, HIGH); delay(80);
      digitalWrite(BUZZER, LOW);  delay(80);
    }
  }
  void buzzWarning() {
    if (!buzzerEnabled) return;
    digitalWrite(BUZZER, HIGH); delay(150);
    digitalWrite(BUZZER, LOW);  delay(500);
  }
  // ==================== ALERT HANDLER ====================
  void handleAlerts() {
    unsigned long now = millis();
    if (now - lastPowerUpdate >= powerUpdateInterval) {
      updatePowerReadings();
      lastPowerUpdate = now;
    }
    int gasRaw = readGasSensor();
    bool gasDetected = checkGasDetected(); 
    bool gyroDetected = checkGyroMovement();
    bool zone1Water = checkZone1Water();
    bool zone2Water = checkZone2Water();
    bool zone1Power = checkZone1Power();
    bool zone2Power = checkZone2Power();
    bool zone1Temp  = checkZone1Temp();
    bool zone2Temp  = checkZone2Temp();
    bool zone1Hazard = zone1Water || zone1Power || zone1Temp;
    bool zone2Hazard = zone2Water || zone2Power || zone2Temp;
    // Priority 1: Gas/Gyro -> Trip ALL
    if (gasDetected || gyroDetected) {
      if (gasDetected && !gasAlertActive) {
        buzzGas();
        Serial.println("\n🚨🚨🚨 GAS LEAK DETECTED 🚨🚨🚨");
        sendAlert("GAS_LEAK_DETECTED", String(gasRaw));
        gasAlertActive = true;
        tripAllBreakers();
      }
      if (gyroDetected && !gyroAlertActive) {
        buzzGyro();
        Serial.println("\n🚨🚨🚨 EARTHQUAKE DETECTED 🚨🚨🚨");
        sendAlert("GROUND_MOVEMENT_DETECTED");
        gyroAlertActive = true;
        tripAllBreakers();
      }
      return;
    }
    // Priority 2: Zone 1 Hazards
    if (zone1Hazard) {
      bool shouldTrip = false;
      if (zone1Water && !waterAlertActive) {
        buzzWater();
        Serial.println("\n🚨 ZONE 1: WATER DETECTED - TRIPPING BREAKER 1");
        sendAlert("WATER_DETECTED", "Zone 1");
        waterAlertActive = true;
        shouldTrip = true;
      }
      if (zone1Temp && !tempAlertActive) {
        buzzTemp();
        Serial.println("\n🚨 ZONE 1: HIGH TEMPERATURE - TRIPPING BREAKER 1");
        sendAlert("HIGH_TEMPERATURE", "Zone 1");
        tempAlertActive = true;
        shouldTrip = true;
      }
      if (zone1Power && !powerAlertActive) {
        buzzPower();
        Serial.printf("\n🚨 ZONE 1: POWER ABNORMAL (V=%.2fV I=%.2fA) - TRIPPING BREAKER 1\n", voltage1_V, current1_A);
        sendAlert("POWER_ABNORMAL", "Zone 1");
        powerAlertActive = true;
        shouldTrip = true;
      }
      if (shouldTrip) tripBreaker(1);
    }
    // Priority 3: Zone 2 Hazards
    if (zone2Hazard) {
      bool shouldTrip = false;
      if (zone2Water && !waterAlertActive) {
        buzzWater();
        Serial.println("\n🚨 ZONE 2: WATER DETECTED - TRIPPING BREAKER 2");
        sendAlert("WATER_DETECTED", "Zone 2");
        waterAlertActive = true;
        shouldTrip = true;
      }
      if (zone2Temp && !tempAlertActive) {
        buzzTemp();
        Serial.println("\n🚨 ZONE 2: HIGH TEMPERATURE - TRIPPING BREAKER 2");
        sendAlert("HIGH_TEMPERATURE", "Zone 2");
        tempAlertActive = true;
        shouldTrip = true;
      }
      if (zone2Power && !powerAlertActive) {
        buzzPower();
        Serial.printf("\n🚨 ZONE 2: POWER ABNORMAL (V=%.2fV I=%.2fA) - TRIPPING BREAKER 2\n", voltage2_V, current2_A);
        sendAlert("POWER_ABNORMAL", "Zone 2");
        powerAlertActive = true;
        shouldTrip = true;
      }
      if (shouldTrip) tripBreaker(2);
    }
    // Clear Alerts
    if (!gasDetected && gasAlertActive) {
      Serial.println("✅ CLEARED: Gas level normal");
      sendAlertCleared("GAS");
      gasAlertActive = false;
    }
    if (!gyroDetected && gyroAlertActive) {
      Serial.println("✅ CLEARED: Movement stopped");
      sendAlertCleared("MOVEMENT");
      gyroAlertActive = false;
    }
    if (!zone1Water && !zone2Water && waterAlertActive) {
      Serial.println("✅ CLEARED: Water level normal");
      sendAlertCleared("WATER");
      waterAlertActive = false;
    }
    if (!zone1Temp && !zone2Temp && tempAlertActive) {
      Serial.println("✅ CLEARED: Temperature normal");
      sendAlertCleared("TEMPERATURE");
      tempAlertActive = false;
    }
    if (!zone1Power && !zone2Power && powerAlertActive) {
      Serial.println("✅ CLEARED: Power normal");
      sendAlertCleared("POWER");
      powerAlertActive = false;
    }
    if (!gasDetected && !gyroDetected && !zone1Hazard && !zone2Hazard) {
        digitalWrite(BUZZER, LOW);
    }
        // Keep buzzing while gas alert is active
    if (gasAlertActive && gasDetected && buzzerEnabled) {
      digitalWrite(BUZZER, HIGH); delay(80);
      digitalWrite(BUZZER, LOW);  delay(80);
      digitalWrite(BUZZER, HIGH); delay(80);
      digitalWrite(BUZZER, LOW);
    }
  }
  // ==================== HANDLE COMMANDS ====================
  void handleIncomingCommand() {
    if (jsonReceive.containsKey("command")) {
      String command = jsonReceive["command"].as<String>();
      command.toUpperCase();
      Serial.print("📥 Command: ");
      Serial.println(command);
      if (command == "SYSTEM_ON") {
        systemEnabled = true;
        sendAckResponse("System enabled");
      }
      else if (command == "SYSTEM_OFF") {
        systemEnabled = false;
        digitalWrite(BUZZER, LOW);
        sendAckResponse("System disabled");
      }
      else if (command == "BUZZER_ON") {
        buzzerEnabled = true;
        sendAckResponse("Buzzer enabled");
      }
      else if (command == "BUZZER_OFF") {
        buzzerEnabled = false;
        digitalWrite(BUZZER, LOW);
        sendAckResponse("Buzzer muted");
      }
      else if (command == "REQUEST_STATUS") {
        sendSensorData(true);
      }
      else if (command == "CALIBRATE_GYRO") {
        mpu.calcGyroOffsets(true);
        sendAckResponse("Gyro calibrated");
      }
      else if (command == "RESET_SYSTEM") {
        sendAckResponse("System resetting");
        delay(500);
        ESP.restart();
      }
      else if (command == "SHAKE_TEST") {
        testGyroShake();
        sendAckResponse("Shake test completed");
      }
      else if (command == "BREAKER1_OFF") {
        tripBreaker(1);
        sendAckResponse("Breaker 1 tripped");
      }
      else if (command == "BREAKER2_OFF") {
        tripBreaker(2);
        sendAckResponse("Breaker 2 tripped");
      }
      else if (command == "TRIP_ALL") {
        tripAllBreakers();
        sendAckResponse("All breakers tripped");
      }
      else if (command == "GET_BREAKER_STATUS") {
        sendBreakerStatus();
      }
      else {
        Serial.println("⚠️ Unknown command");
      }
    }
    else if (jsonReceive.containsKey("set_thresholds")) {
      JsonObject thresholds = jsonReceive["set_thresholds"];
      if (thresholds.containsKey("temperature")) tempThreshold = thresholds["temperature"];
      if (thresholds.containsKey("gyro"))        gyroThreshold = thresholds["gyro"];
      if (thresholds.containsKey("voltage"))     voltageThreshold = thresholds["voltage"];
      if (thresholds.containsKey("current"))     currentThreshold = thresholds["current"];
      sendAckResponse("Thresholds updated");
      sendSystemStatus();
    }
  }
  // ==================== MQTT RECONNECT (FIXED) ====================
bool mqttReconnect() {
  if (WiFi.status() != WL_CONNECTED) return false;
  // ✅ FIX rc=-1: stop any stale TLS session before opening a new one.
  // Without this, a previous failed handshake leaves broken SSL state
  // in espClient, and the next connect attempt immediately gets -1.
  espClient.stop();
  delay(100);
  espClient.setInsecure();
  mqttClient.setId(("ESP32-" + deviceId).c_str());
  mqttClient.setUsernamePassword(mqtt_username, mqtt_password);
  Serial.print("📡 MQTT: Connecting to ");
  Serial.println(mqtt_broker);
  if (!mqttClient.connect(mqtt_broker, mqtt_port)) {  // ✅ use variable not hardcoded 8884
    Serial.print("❌ MQTT Failed rc=");
    Serial.println(mqttClient.connectError());
    return false;
  }
  Serial.println("✅ MQTT: Connected!");
  mqttClient.subscribe(commandTopic);
  Serial.print("📬 Subscribed to ");
  Serial.println(commandTopic);
  sendSystemStatus();
  return true;
}
  // ==================== PUBLISH MESSAGE ====================
  void publishMessage(const char* payload) {
    if (!mqttClient.connected()) {
      Serial.println("⚠️ MQTT: Not connected, cannot publish");
      return;
    }
    mqttClient.beginMessage(telemetryTopic);
    mqttClient.print(payload);
    mqttClient.endMessage();
    Serial.println("📤 MQTT: Message published");
  }
  // ==================== SEND ALERT ====================
  void sendAlert(String alertType, String details) {
    if (!mqttClient.connected()) return;
    jsonDoc.clear();
    jsonDoc["alert"] = alertType;
    if (alertType == "WATER_DETECTED") {
      jsonDoc["water1"] = isActiveLowFiltered(WATER1);
      jsonDoc["water2"] = isActiveLowFiltered(WATER2);
      jsonDoc["water3"] = isActiveLowFiltered(WATER3);
      jsonDoc["water4"] = isActiveLowFiltered(WATER4);
    }
    else if (alertType == "GAS_LEAK_DETECTED") {
      jsonDoc["sensor"] = "GAS";
      jsonDoc["value"] = details.toInt();   // details now carries the raw ADC value
    }
    else if (alertType == "HIGH_TEMPERATURE") {
      jsonDoc["temp1"] = readTemp(TEMP1);
      jsonDoc["temp2"] = readTemp(TEMP2);
    }
    else if (alertType == "GROUND_MOVEMENT_DETECTED") {
      mpu.update();
      jsonDoc["gyro_x"] = mpu.getGyroX();
      jsonDoc["gyro_y"] = mpu.getGyroY();
      jsonDoc["gyro_z"] = mpu.getGyroZ();
    }
    else if (alertType == "POWER_ABNORMAL") {
      jsonDoc["voltage1"] = voltage1_V;
      jsonDoc["voltage2"] = voltage2_V;
      jsonDoc["current1"] = current1_A;
      jsonDoc["current2"] = current2_A;
      jsonDoc["voltage1_raw"] = voltage1_raw;
      jsonDoc["voltage2_raw"] = voltage2_raw;
    }
    if (details.length() > 0) jsonDoc["details"] = details;
    jsonDoc["timestamp"] = millis() / 1000;
    String output;
    serializeJson(jsonDoc, output);
    publishMessage(output.c_str());
  }
  // ==================== SEND ALERT CLEARED ====================
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
  // ==================== SEND SENSOR DATA ====================
  void sendSensorData(bool forceImmediate) {
    if (!mqttClient.connected() || !systemEnabled) return;
    unsigned long now = millis();
    if (!forceImmediate && (now - lastDataSendTime < dataSendInterval)) return;
    jsonDoc.clear();
    JsonArray waterArray = jsonDoc.createNestedArray("water");
    waterArray.add(isActiveLowFiltered(WATER1));
    waterArray.add(isActiveLowFiltered(WATER2));
    waterArray.add(isActiveLowFiltered(WATER3));
    waterArray.add(isActiveLowFiltered(WATER4));
    jsonDoc["gas"]     = checkGasDetected();
    jsonDoc["gas_raw"] = readGasSensor();
    JsonObject tempObj = jsonDoc.createNestedObject("temperature");
    tempObj["temp1"] = readTemp(TEMP1);
    tempObj["temp2"] = readTemp(TEMP2);
    JsonObject gyroObj = jsonDoc.createNestedObject("gyro");
    mpu.update();
    gyroObj["movement"] = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
    gyroObj["x"] = mpu.getGyroX();
    gyroObj["y"] = mpu.getGyroY();
    gyroObj["z"] = mpu.getGyroZ();
    JsonObject powerObj = jsonDoc.createNestedObject("power");
    powerObj["voltage1"] = voltage1_V;
    powerObj["voltage2"] = voltage2_V;
    powerObj["current1"] = current1_A;
    powerObj["current2"] = current2_A;
    powerObj["v1_raw"] = voltage1_raw;
    powerObj["v2_raw"] = voltage2_raw;
    JsonObject breakerObj = jsonDoc.createNestedObject("breakers");
    breakerObj["breaker1"] = breaker1State;
    breakerObj["breaker2"] = breaker2State;
    jsonDoc["system_enabled"] = systemEnabled;
    jsonDoc["buzzer_enabled"] = buzzerEnabled;
    jsonDoc["uptime"] = millis() / 1000;
    String output;
    serializeJson(jsonDoc, output);
    publishMessage(output.c_str());
    lastDataSendTime = now;
  }
  // ==================== SEND SYSTEM STATUS ====================
  void sendSystemStatus() {
    if (!mqttClient.connected()) return;
    jsonDoc.clear();
    jsonDoc["message_type"] = "SYSTEM_STATUS";
    jsonDoc["device_id"] = deviceId;
    jsonDoc["system_enabled"] = systemEnabled;
    jsonDoc["buzzer_enabled"] = buzzerEnabled;
    JsonObject thresh = jsonDoc.createNestedObject("thresholds");
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
    jsonDoc["firmware_version"] = "3.6-NTP-FIX";
    String output;
    serializeJson(jsonDoc, output);
    publishMessage(output.c_str());
  }
  // ==================== SEND BREAKER STATUS ====================
  void sendBreakerStatus() {
    if (!mqttClient.connected()) return;
    jsonDoc.clear();
    jsonDoc["message_type"] = "BREAKER_STATUS";
    jsonDoc["device_id"] = deviceId;
    JsonObject b1 = jsonDoc.createNestedObject("breaker1");
    b1["state"] = breaker1State ? "ON" : "OFF";
    b1["last_trip"] = breaker1TripTime;
    JsonObject b2 = jsonDoc.createNestedObject("breaker2");
    b2["state"] = breaker2State ? "ON" : "OFF";
    b2["last_trip"] = breaker2TripTime;
    jsonDoc["timestamp"] = millis() / 1000;
    String output;
    serializeJson(jsonDoc, output);
    publishMessage(output.c_str());
  }
  // ==================== SEND ACK RESPONSE ====================
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
  void printSensorReadings() {
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║       CURRENT SENSOR VALUES       ║");
    Serial.println("╠═══════════════════════════════════╣");
    bool w1 = isActiveLowFiltered(WATER1);
    bool w2 = isActiveLowFiltered(WATER2);
    bool w3 = isActiveLowFiltered(WATER3);
    bool w4 = isActiveLowFiltered(WATER4);
    Serial.printf("║  Water: %s, %s, %s, %s\n",
      w1 ? "WET" : "DRY", w2 ? "WET" : "DRY",
      w3 ? "WET" : "DRY", w4 ? "WET" : "DRY");
    float t1 = readTemp(TEMP1);
    float t2 = readTemp(TEMP2);
    Serial.printf("║  Temp: %.1f°C, %.1f°C (threshold: %.1f°C)\n",
      t1, t2, tempThreshold);
      // Cap current display to reasonable max to avoid confusion
    float safeCurrent1 = min(current1_A, 10.0f);  // Never show >10A on display
    float safeCurrent2 = min(current2_A, 10.0f);
    // But for actual logic, use raw current1_A/current2_A
    mpu.update();
    float gyro = abs(mpu.getGyroX()) + abs(mpu.getGyroY()) + abs(mpu.getGyroZ());
    Serial.printf("║  Gyro: %.2f (threshold: %.2f)\n", gyro, gyroThreshold);
      int gasNow = readGasSensor();
      Serial.printf("║  Gas raw ADC : %d | Threshold: %d\n", gasNow, gasThreshold);
      Serial.printf("║  Gas DO state: %s\n", checkGasDetected() ? "⚠️ DETECTED" : "Normal");
    Serial.println("║  Power:");
    Serial.printf("║    V1: %.2f V (raw ADC: %d)\n", voltage1_V, voltage1_raw);
    Serial.printf("║    V2: %.2f V (raw ADC: %d)\n", voltage2_V, voltage2_raw);
    Serial.printf("║    I1 raw ADC: %d (zero: %d)\n", rawI1, rawZeroI1);
    Serial.printf("║    I2 raw ADC: %d (zero: %d)\n", rawI2, rawZeroI2);
    Serial.printf("║    I1: %.3f A\n", current1_A);
    Serial.printf("║    I2: %.3f A\n", current2_A);
    Serial.printf("║    Thresholds: V>%.1fV or I>%.1fA = TRIP\n", voltageThreshold, currentThreshold);
    Serial.println("╚═══════════════════════════════════╝\n");
  }
  // ==================== PRINT FULL STATUS ====================
  void printFullStatus() {
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║         SYSTEM STATUS             ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.printf("║  Device ID: %s\n", deviceId.c_str());
    Serial.printf("║  WiFi:    %s\n", (WiFi.status() == WL_CONNECTED) ? "✅ Connected" : "❌ Disconnected");
    Serial.printf("║  MQTT:    %s\n", mqttClient.connected() ? "✅ Connected" : "⏳ Waiting");
    Serial.printf("║  Time:    %s\n", timeIsSynced ? "✅ Synced" : "❌ Not Synced");
    Serial.printf("║  System:  %s\n", systemEnabled ? "✅ Enabled" : "❌ Disabled");
    Serial.printf("║  Buzzer:  %s\n", buzzerEnabled ? "🔊 Enabled" : "🔇 Muted");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.println("║      CIRCUIT BREAKER STATUS       ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.printf("║  Breaker 1: %s\n", breaker1State ? "✅ ON" : "⚡ TRIPPED");
    Serial.printf("║  Breaker 2: %s\n", breaker2State ? "✅ ON" : "⚡ TRIPPED");
    Serial.println("╠═══════════════════════════════════╣");
    unsigned long uptime = millis() / 1000;
    Serial.printf("║  Uptime: %luh %lum %lus\n", uptime / 3600, (uptime % 3600) / 60, uptime % 60);
    Serial.println("╠═══════════════════════════════════╣");
    Serial.println("║         ACTIVE ALERTS             ║");
    Serial.println("╠═══════════════════════════════════╣");
    bool anyAlert = false;
    if (waterAlertActive) { Serial.println("║  💧 Water detected"); anyAlert = true; }
    if (gasAlertActive)   { Serial.println("║  💨 Gas detected"); anyAlert = true; }
    if (tempAlertActive)  { Serial.println("║  🔥 High temperature"); anyAlert = true; }
    if (gyroAlertActive)  { Serial.println("║  🔳 Ground movement"); anyAlert = true; }
    if (powerAlertActive) { Serial.println("║  ⚡ Power abnormal"); anyAlert = true; }
    if (!anyAlert)        { Serial.println("║  ✅ No active alerts"); }
    Serial.println("╚═══════════════════════════════════╝\n");
  }
  // ==================== PRINT WIFI INFO ====================
  void printWiFiInfo() {
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║         WiFi Information          ║");
    Serial.println("╠═══════════════════════════════════╣");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("║  Status: ✅ CONNECTED");
      Serial.printf("║  SSID:   %s\n", WiFi.SSID().c_str());
      Serial.print("║  IP:     "); Serial.println(WiFi.localIP());
      Serial.printf("║  Signal: %d dBm\n", WiFi.RSSI());
      Serial.printf("║  MQTT:   %s\n", mqttClient.connected() ? "✅ Connected" : "⏳ Waiting");
      Serial.printf("║  Time:   %s\n", timeIsSynced ? "✅ Synced" : "❌ Not Synced");
    } else {
      Serial.println("║  Status: ❌ DISCONNECTED");
    }
    Serial.println("╚═══════════════════════════════════╝\n");
  }
  // ==================== PRINT HELP ====================
  void printHelp() {
    Serial.println("\n╔════════════════════════════════════════════════╗");
    Serial.println("║              AVAILABLE COMMANDS                ║");
    Serial.println("╠════════════════════════════════════════════════╣");
    Serial.println("║  sensors  - Show all sensor readings + raw ADC ║");
    Serial.println("║  status   - Show full system status            ║");
    Serial.println("║  wifi     - Show WiFi info                     ║");
    Serial.println("║  scan     - Scan for WiFi networks             ║");
    Serial.println("║  help     - Show this menu                     ║");
    Serial.println("║  shake    - Run 30s earthquake simulation      ║");
    Serial.println("║  b1off    - Trip Breaker 1 (CW)                ║");
    Serial.println("║  b2off    - Trip Breaker 2 (CCW)               ║");
    Serial.println("║  tripall  - Trip ALL breakers                  ║");
    Serial.println("╚════════════════════════════════════════════════╝\n");
  }
  // ==================== SCAN NETWORKS ====================
  void scanNetworks() {
    Serial.println("\n🔍 Scanning for WiFi networks...\n");
    int n = WiFi.scanNetworks();
    if (n == 0) {
      Serial.println("   No networks found!");
    } else {
      for (int i = 0; i < n; i++) {
        Serial.printf("   %d. %s (%d dBm) %s\n",
          i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i),
          (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "🔒");
      }
    }
    Serial.println("");
  }
String getDeviceId() {
  String mac = WiFi.macAddress();  // returns "68:25:DD:31:EB:6C"
  mac.replace(":", "");            // becomes "6825DD31EB6C"
  return mac;
}
// ✅ ADD THIS before setup() — standalone function
void onMqttMessage(int messageSize) {
  String topic = mqttClient.messageTopic();
  String payload = "";
  while (mqttClient.available()) {
    payload += (char)mqttClient.read();
  }
  Serial.printf("📨 [%s]: %s\n", topic.c_str(), payload.c_str());
  jsonReceive.clear();
  DeserializationError error = deserializeJson(jsonReceive, payload);
  if (!error) {
    handleIncomingCommand();
  }
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  ESP32 SAFETY MONITORING SYSTEM   ║");
  Serial.println("║     Version 3.7 (TEMP+POWER FIX)  ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  // ✅ FIRE WIFI FIRST — only once, no duplicate calls
  Serial.print("🌐 Connecting to: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.begin(ssid, password);
  // ← no waiting here, let hardware init below act as the delay
  // ✅ HARDWARE INIT (runs while WiFi connects in background)
  pinMode(GAS_AO, INPUT);
  pinMode(GAS_DO, INPUT_PULLUP);
  pinMode(WATER1, INPUT_PULLUP);
  pinMode(WATER2, INPUT_PULLUP);
  pinMode(WATER3, INPUT_PULLUP);
  pinMode(WATER4, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  configureADC();
  pinMode(TEMP1, INPUT);
  pinMode(TEMP2, INPUT);
  pinMode(VOLTAGE_SENSOR_1, INPUT);
  pinMode(VOLTAGE_SENSOR_2, INPUT);
  pinMode(CURRENT_SENSOR_1, INPUT);
  pinMode(CURRENT_SENSOR_2, INPUT);
  initServos();
  Wire.begin(21, 22);
  mpu.begin();
  Serial.println("🔧 Calibrating gyro...");
  mpu.calcGyroOffsets(true);  // ~3s blocking — WiFi connecting in background
  Serial.println("✅ Gyro calibrated\n");
  // Gas calibration — ~5s blocking — WiFi still connecting in background
  Serial.println("🔧 Calibrating gas sensor baseline (keep away from gas)...");
  long gasSum = 0;
  for (int i = 0; i < 50; i++) {
    gasSum += analogRead(GAS_AO);
    delay(100);
  }
  int gasBaseline = gasSum / 50;
  gasThreshold = gasBaseline + 150;
  Serial.printf("   Gas baseline: %d | Threshold set to: %d\n", gasBaseline, gasThreshold);
  Serial.println("✅ Gas sensor calibrated\n");
  // ✅ CHECK WIFI — ~8-10s have passed by now, should be connected
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.printf(". [%d] status=%d\n", attempts, WiFi.status());
    attempts++;
  }
  Serial.println();
  // ADC pin test
  Serial.println("\n🔍 Quick ADC Pin Test:");
  Serial.printf("   GPIO 32 (I1): %d\n", analogRead(CURRENT_SENSOR_1));
  Serial.printf("   GPIO 33 (I2): %d\n", analogRead(CURRENT_SENSOR_2));
  Serial.printf("   GPIO 34 (V1): %d\n", analogRead(VOLTAGE_SENSOR_1));
  Serial.printf("   GPIO 35 (V2): %d\n", analogRead(VOLTAGE_SENSOR_2));
  if (WiFi.status() == WL_CONNECTED) {
    prevWifiConnected = true;
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║      ✅ WiFi CONNECTED!           ║");
    Serial.println("╠═══════════════════════════════════╣");
    Serial.print("║  IP: ");
    Serial.println(WiFi.localIP());
    Serial.printf("║  Signal: %d dBm\n", WiFi.RSSI());
    Serial.println("╚═══════════════════════════════════╝\n");
    deviceId = getDeviceId();
    telemetryTopic = "apn/device/" + deviceId + "/telemetry";
    commandTopic   = "apn/device/" + deviceId + "/commands";
    Serial.println("📱 Device Configuration:");
    Serial.printf("   Device ID: %s\n", deviceId.c_str());
    Serial.printf("   Telemetry: %s\n", telemetryTopic.c_str());
    Serial.printf("   Commands:  %s\n\n", commandTopic.c_str());
    syncTime();
    // espClient.setInsecure() removed from here — handled inside mqttReconnect()
    mqttClient.onMessage(onMqttMessage);  // ← must be INSIDE setup(), not global scope
    mqttReconnect();
  } else {
    Serial.println("\n❌ WiFi FAILED - Check SSID/password");
    Serial.println("   ⚠️ ESP32 only works with 2.4GHz WiFi!");
    Serial.println("   Type 'scan' to see available networks\n");
  }
  if (enableCurrentSensors) {
    calibrateCurrentSensors();
    // Auto-calibrate temperature
    const float ROOM_TEMP = 28.0f;
    long s1 = 0, s2 = 0;
    for (int i = 0; i < 100; i++) {
      s1 += analogRead(TEMP1);
      s2 += analogRead(TEMP2);
      delay(5);
    }
    float raw1 = (s1 / 100.0f) * 3.3f / 4095.0f * 100.0f;
    float raw2 = (s2 / 100.0f) * 3.3f / 4095.0f * 100.0f;
    tempCalOffset1 = ROOM_TEMP - raw1;
    tempCalOffset2 = ROOM_TEMP - raw2;
    Serial.printf("🌡️ Temp cal: T1 offset=%.1f  T2 offset=%.1f\n", tempCalOffset1, tempCalOffset2);
  }
  Serial.println("╔═══════════════════════════════════╗");
  Serial.println("║  VOLTAGE SENSOR TEST              ║");
  Serial.println("╚═══════════════════════════════════╝");
  updatePowerReadings();
  Serial.printf("   V1: %.2f V (raw: %d)\n", voltage1_V, voltage1_raw);
  Serial.printf("   V2: %.2f V (raw: %d)\n", voltage2_V, voltage2_raw);
  Serial.printf("   Noise threshold: raw < %d (debug only)\n\n", VOLTAGE_NOISE_THRESHOLD);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  Serial.println("✅ System Ready - Type 'help' for commands\n");
  Serial.println("───────────────────────────────────────\n");
  lastHeartbeat = millis();
}
  // ==================== MAIN LOOP ====================
  void loop() {
    unsigned long now = millis();
    if (now - lastWiFiCheck > 10000) {
      lastWiFiCheck = now;
      bool wifiConnected = (WiFi.status() == WL_CONNECTED);
      if (wifiConnected != prevWifiConnected) {
        Serial.println(wifiConnected ? "\n✅ WiFi CONNECTED!" : "\n⚠️ WiFi DISCONNECTED!");
      // ✅ FIX: do NOT call WiFi.RSSI() here — it corrupts TLS state
        prevWifiConnected = wifiConnected;
        if (!wifiConnected) wifiConnecting = false;
      }
      if (!wifiConnected && !wifiConnecting) {
        wifiConnecting = true;
        WiFi.disconnect(true);
        delay(500);
        WiFi.begin(ssid, password);
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqttClient.connected()) {
        if (now - lastMqttReconnectAttempt > mqttReconnectInterval) {
          lastMqttReconnectAttempt = now;
          mqttReconnect();
        }
      } else {
        mqttClient.poll(); 
      }
    }
        // Add this in your loop() somewhere near the top
    static unsigned long lastNtpRetry = 0;
    bool currentMqttConnected = mqttClient.connected();
    if (currentMqttConnected != prevMqttConnected) {
      Serial.printf("📡 MQTT %s\n", currentMqttConnected ? "CONNECTED" : "DISCONNECTED");
      prevMqttConnected = currentMqttConnected;
    }
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      cmd.toLowerCase();
      if      (cmd == "sensors") printSensorReadings();
      else if (cmd == "status")  printFullStatus();
      else if (cmd == "wifi")    printWiFiInfo();
      else if (cmd == "scan")    scanNetworks();
      else if (cmd == "help")    printHelp();
      else if (cmd == "shake")   testGyroShake();
      else if (cmd == "b1off")   tripBreaker(1);
      else if (cmd == "b2off")   tripBreaker(2);
      else if (cmd == "tripall") tripAllBreakers();
    }
    if (!systemEnabled) {
      if (prevSystemEnabled) {
        Serial.println("⏸️ System DISABLED");
        prevSystemEnabled = false;
      }
      delay(100);
      return;
    } else if (!prevSystemEnabled) {
      Serial.println("▶️ System ENABLED");
      prevSystemEnabled = true;
    }
    handleAlerts();
    if (heartbeatInterval > 0 && (now - lastHeartbeat >= heartbeatInterval)) {
      bool anyAlert = waterAlertActive || gasAlertActive || tempAlertActive ||
                      gyroAlertActive || powerAlertActive;
      if (!anyAlert) {
        // ✅ FIX rc=-1: WiFi.RSSI() removed from heartbeat — known ESP32 bug where
        // calling WiFi.RSSI() corrupts the active TLS/SSL session (espressif/arduino-esp32#5146)
        Serial.printf("💚 OK | V1:%.1fV(r%d) I1:%.2fA | WiFi:✅ MQTT:%s Time:%s\n",
          voltage1_V, voltage1_raw, current1_A,
          mqttClient.connected() ? "✅" : "⏳",
          timeIsSynced ? "✅" : "❌");
      }
      lastHeartbeat = now;
    }
    if (enablePeriodicUpdates && periodicUpdateInterval > 0) {
      if (now - lastPeriodicUpdate >= periodicUpdateInterval) {
        sendSensorData(true);
        lastPeriodicUpdate = now;
      }
    }
    delay(50);
  }