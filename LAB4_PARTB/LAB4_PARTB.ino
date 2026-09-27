#include <DHT.h>

const int MOIST_PIN = 34;       // ADC1, input only
// Measured for this probe in Part A
const int RAW_DRY = 3100;       // in air
const int RAW_WET = 1350;       // in water

const int RAIN_PIN = 36;        // input only
const int DHT_PIN = 32;         // input only
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

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

  pinMode(RAIN_PIN, INPUT);
  dht.begin();
}


void loop() {

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
  float temperature = dht.readTemperature();  // Celsius

  // If DHT22 reading failed
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

  Serial.println("-----------------------------");

  delay(2000);
}