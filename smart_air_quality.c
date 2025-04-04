// Define WiFi Credentials
#define WIFI_SSID "realme"
#define WIFI_PASS "helloworld"
// Pin Definitions
#define MQ135_PIN 34
#define DHT_PIN 18
#define BUZZER_PIN 32
#define DHT_TYPE DHT22

// Thresholds
#define AQI_THRESHOLD 800
#define CO2_THRESHOLD 4000

// MQ135 Calibration Constants
#define RLOAD 10.0
#define R0 37.73
#define CO2_BASE_RATIO 3.6

// Objects
BlynkTimer timer;
DHT dht(DHT_PIN, DHT_TYPE);

// Function to calculate CO₂
float getCO2Level(int sensorValue) {
  float sensorVoltage = sensorValue * (3.3 / 4095.0); // Use 5.0 if sensor is powered via 5V
  if (sensorVoltage < 0.01) sensorVoltage = 0.01; // Avoid division by zero

  float sensorResistance = (RLOAD * (3.3 - sensorVoltage)) / sensorVoltage;
  float ratio = sensorResistance / R0;

  Serial.print("Rs/R0 Ratio: ");
  Serial.println(ratio);

  float co2ppm = 400 * pow((ratio / CO2_BASE_RATIO), -1.5); // Adjust exponent if needed

  // Optional clamping
  if (co2ppm < 0) co2ppm = 0;
  if (co2ppm > 10000) co2ppm = 10000;

  return co2ppm;
}

// Send sensor data to Serial + Blynk
void sendSensorData() {
  int mq135_value = analogRead(MQ135_PIN);
  float co2ppm = getCO2Level(mq135_value);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("⚠ Failed to read from DHT sensor!");
    return;
  }

  // Air Quality Index (Simplified)
  int AQI = mq135_value / 10;

  // Serial Output
  Serial.println("===== Sensor Readings =====");
  Serial.print("Air Quality (PPM): "); Serial.println(mq135_value);
  Serial.print("CO2 Level: "); Serial.print(co2ppm); Serial.println(" ppm");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("AQI: "); Serial.println(AQI);
  Serial.println("===========================\n");

  // Send to Blynk
  Blynk.virtualWrite(V0, temperature);     // Temperature
  Blynk.virtualWrite(V1, humidity);        // Humidity
  Blynk.virtualWrite(V2, mq135_value);     // Air Quality PPM
  Blynk.virtualWrite(V3, co2ppm);          // CO2 Level
  Blynk.virtualWrite(V4, AQI);             // AQI

  // Alerts
  if (co2ppm > CO2_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("🚨 High CO2 Detected!");
    Blynk.logEvent("high_co2", "ALERT! High CO2 levels!");
  } else if (mq135_value > AQI_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("🚨 Poor Air Quality Detected!");
    Blynk.logEvent("poor_air_quality", "ALERT! Bad air detected!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(MQ135_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  dht.begin();

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n✅ Connected to WiFi");

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

  // Call sensor function every 2 seconds
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}
