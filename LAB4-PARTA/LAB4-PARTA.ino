const int MOIST_PIN = 34;              // ADC1, input only 
// measured for this probe in Part A 
const int RAW_DRY = 3100;              // in air 
const int RAW_WET = 1350;              // in water 

int readMoistureRaw(int n = 10) {      // average to steady the reading 
  long sum = 0; 
  for (int i = 0; i < n; i++) { 
    sum += analogRead(MOIST_PIN); 
    delay(5); 
  } 
  return sum / n; 
}

float readMoisturePct() { 
  int raw = readMoistureRaw(); 
  float pct = 100.0 * (RAW_DRY - raw) / (float)(RAW_DRY - RAW_WET); 
  return constrain(pct, 0.0, 100.0); 
}

void setup() {
  Serial.begin(115200);                 // Initialize serial communication at 115200 baud rate
  while (!Serial) { delay(10); }        // Wait for serial port to connect
}

void loop() {
  int rawValue = readMoistureRaw();
  float percentage = readMoisturePct();

  Serial.print("Raw Value: ");
  Serial.print(rawValue);
  Serial.print(" | Moisture: ");
  Serial.print(percentage, 1);          // Print percentage to 1 decimal place
  Serial.println("%");

  delay(1000);                          // Read every second
}