#include <DHT.h>
const int MOIST_PIN = 34;       // ADC1, input only
// Measured for this probe in Part A
const int RAW_DRY = 3100;       // in air
const int RAW_WET = 1350;       // in water

const int RAIN_PIN = 36;        // input only
const int OVERRIDE_PIN = 19;    // Override switch
const int DHT_PIN = 32;         // DHT22
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

volatile bool overrideFlag = false;

volatile unsigned long lastIsr = 0;

volatile unsigned long overrideTime = 0;  // Stores when interrupt happened

bool manualOverride = false;

void IRAM_ATTR onOverride() {

  unsigned long now = millis();

  // Ignore switch bounce
  if (now - lastIsr < 200) {
    return;
  }

  lastIsr = now;

  // ISR SETS FLAG
  overrideFlag = true;

  overrideTime = now;  // Records interrupt time
}

// SOIL MOISTURE FUNCTIONS
int readMoistureRaw(int n = 10) {

  long sum = 0;

  for (int i = 0; i < n; i++) {

    sum += analogRead(MOIST_PIN);

    delay(5);
  }

  return sum / n;
}

float readMoisturePct() {
  int raw = readMoistureRaw();
  float pct = 100.0 * (RAW_DRY - raw) /
              (float)(RAW_DRY - RAW_WET);
  return constrain(pct, 0.0, 100.0);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Rain sensor
  pinMode(RAIN_PIN, INPUT);

  // Override switch
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(OVERRIDE_PIN),
    onOverride,
    FALLING
  );

  // DHT22
  dht.begin();

  Serial.println("System started.");
  Serial.println("Override switch ready.");
  Serial.println("-----------------------------");
}

void loop() {

  // OVERRIDE FLAG
  if (overrideFlag) {

    // Clear flag
    overrideFlag = false;

    // ACTUAL ACTION HAPPENS IN MAIN LOOP
    manualOverride = !manualOverride;

    Serial.print("OVERRIDE INTERRUPT CAPTURED at ");
    Serial.print(overrideTime);
    Serial.println(" ms");  // Confirms interrupt happened

    Serial.print("OVERRIDE SWITCH PRESSED -> ");

    if (manualOverride) {
      Serial.println("MANUAL OVERRIDE ON");
    }
    else {
      Serial.println("MANUAL OVERRIDE OFF");
    }
  }

  // SOIL MOISTURE
  int rawValue = readMoistureRaw();
  float percentage = readMoisturePct();

  Serial.print("Raw Value: ");
  Serial.print(rawValue);

  Serial.print(" | Moisture: ");
  Serial.print(percentage, 1);

  Serial.println("%");

  // RAIN SENSOR
  int rainStatus = digitalRead(RAIN_PIN);
  if (rainStatus == LOW) {
    Serial.println("Status: RAIN DETECTED");
  }
  else {
    Serial.println("Status: NO RAIN");
  }

  // DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {

    Serial.println("DHT22 Error: Failed to read sensor");

  }
  else {

    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  }

  // SHOW OVERRIDE STATE
  Serial.print("Override State: ");

  if (manualOverride) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }

  Serial.println("-----------------------------");

  delay(2000);
}