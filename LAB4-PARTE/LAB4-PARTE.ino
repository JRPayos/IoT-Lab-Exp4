#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

const int MOIST_PIN = 34;       // ADC1, input only
// Measured for this probe in Part A
const int RAW_DRY = 4095;       // in air
const int RAW_WET = 2700;       // in water

const int RAIN_PIN = 36;        // input only
const int OVERRIDE_PIN = 19;    // Override switch
const int DHT_PIN = 32;         // DHT22
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// KEYPAD
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {13, 14, 27, 26};
byte colPins[COLS] = {25, 33, 18};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// OVERRIDE
volatile bool overrideFlag = false;
volatile unsigned long lastIsr = 0;
volatile unsigned long overrideTime = 0;

bool manualOverride = false;

// SETTINGS
struct Settings {
  int runMinutes;
  int setpointPct;
  int startHour;
};

Settings cfg = {5, 40, 6};

// MENU VARIABLES
int menuItem = 0;

bool menuActive = false;
bool enteringValue = false;

String inputValue = "";

// OVERRIDE INTERRUPT

void IRAM_ATTR onOverride() {
  unsigned long now = millis();
  if (now - lastIsr < 200) {
    return;
  }
  lastIsr = now;
  overrideFlag = true;
  overrideTime = now;
}

// SOIL MOISTURE
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

// APPLY SETTING
bool applySetting(int which, int value) {

  switch (which) {
    case 0:
      if (value < 1 || value > 30) {
        return false;
      }
      cfg.runMinutes = value;
      break;
    case 1:
      if (value < 10 || value > 90) {
        return false;
      }
      cfg.setpointPct = value;
      break;
    case 2:
      if (value < 0 || value > 23) {
        return false;
      }
      cfg.startHour = value;
      break;
  }
  return true;
}

// DISPLAY MENU
void showMenu() {

  lcd.clear();

  if (menuItem == 0) {
    lcd.setCursor(0, 0);
    lcd.print("1:Pump Runtime");

    lcd.setCursor(0, 1);
    lcd.print(cfg.runMinutes);
    lcd.print(" min");
  }

  else if (menuItem == 1) {
    lcd.setCursor(0, 0);
    lcd.print("2:Moisture Set");

    lcd.setCursor(0, 1);
    lcd.print(cfg.setpointPct);
    lcd.print("%");
  }

  else if (menuItem == 2) {
    lcd.setCursor(0, 0);
    lcd.print("3:Start Hour");

    lcd.setCursor(0, 1);
    lcd.print(cfg.startHour);
    lcd.print(":00");
  }
}

// START NUMERIC ENTRY
void startEntry() {

  enteringValue = true;
  inputValue = "";

  lcd.clear();

  if (menuItem == 0) {
    lcd.print("Pump Time:");
  }

  else if (menuItem == 1) {
    lcd.print("Setpoint:");
  }

  else if (menuItem == 2) {
    lcd.print("Start Hour:");
  }

  lcd.setCursor(0, 1);
}

// HANDLE ENTRY
void handleEntry(char key) {

  if (key >= '0' && key <= '9') {

    if (inputValue.length() < 2) {

      inputValue += key;

      lcd.setCursor(0, 1);
      lcd.print("                ");

      lcd.setCursor(0, 1);
      lcd.print(inputValue);
    }
  }

  else if (key == '#') {
    if (inputValue.length() == 0) {
      return;
    }
    int value = inputValue.toInt();
    if (applySetting(menuItem, value)) {
      lcd.clear();
      lcd.print("Value accepted!");

      delay(1000);

      enteringValue = false;
      inputValue = "";

      showMenu();
    }

    else {

      lcd.clear();
      lcd.print("INVALID VALUE");

      lcd.setCursor(0, 1);

      if (menuItem == 0) {
        lcd.print("Range: 1-30");
      }

      else if (menuItem == 1) {
        lcd.print("Range: 10-90");
      }

      else if (menuItem == 2) {
        lcd.print("Range: 0-23");
      }

      delay(2000);

      enteringValue = false;
      inputValue = "";

      showMenu();
    }
  }

  else if (key == '*') {
    enteringValue = false;
    inputValue = "";

    showMenu();
  }
}

// HANDLE MENU
void handleMenu(char key) {

  if (key == '1') {
    menuItem = 0;
    startEntry();
  }

  else if (key == '2') {
    menuItem = 1;
    startEntry();
  }

  else if (key == '3') {
    menuItem = 2;
    startEntry();
  }

  else if (key == '4') {
    menuItem = 0;
    showMenu();
  }

  else if (key == '5') {
    menuItem = 1;
    showMenu();
  }

  else if (key == '6') {
    menuItem = 2;
    showMenu();
  }

  else if (key == '#') {
    menuActive = false;
    lcd.clear();
  }
}

// SETUP
void setup() {

  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(RAIN_PIN, INPUT);
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(OVERRIDE_PIN),
    onOverride,
    FALLING
  );

  dht.begin();

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Farm");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");

  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Edit config:");
  lcd.setCursor(0, 1);
  lcd.print("Press *");
}

// MAIN LOOP
void loop() {

  // OVERRIDE FLAG
  if (overrideFlag) {

    overrideFlag = false;

    manualOverride = !manualOverride;

    Serial.print("OVERRIDE INTERRUPT CAPTURED at ");
    Serial.print(overrideTime);
    Serial.println(" ms");

    Serial.print("OVERRIDE SWITCH PRESSED -> ");

    if (manualOverride) {
      Serial.println("MANUAL OVERRIDE ON");
    }
    else {
      Serial.println("MANUAL OVERRIDE OFF");
    }
  }

  // KEYPAD
  char key = keypad.getKey();

  if (key) {
    if (key == '*') {
      if (!menuActive) {
        menuActive = true;
        enteringValue = false;
        showMenu();
        return;  // Keep checking keypad while menu is active
      }
      else if (!enteringValue) {
        menuActive = false;
        lcd.clear();
      }
      else {
        handleEntry(key);
      }
    }
    else if (menuActive) {
      if (enteringValue) {
        handleEntry(key);
      }
      else {
        handleMenu(key);
      }
    }
  }

  if (menuActive) {
    return;  // Prevent sensor delay while using configuration menu
  }

  // SENSOR READINGS
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


  // OVERRIDE STATE
  Serial.print("Override State: ");
  if (manualOverride) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }

  // SETTINGS

  Serial.print("Pump Runtime: ");
  Serial.print(cfg.runMinutes);
  Serial.println(" min");

  Serial.print("Moisture Setpoint: ");
  Serial.print(cfg.setpointPct);
  Serial.println("%");

  Serial.print("Irrigation Start: ");
  Serial.print(cfg.startHour);
  Serial.println(":00");

  Serial.println("-----------------------------");


  delay(2000);
}