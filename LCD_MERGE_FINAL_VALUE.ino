#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#endif

//variables for wifi
const char* ssid = "UA-Alumnos";
const char* password = "41umn05WLC";
const char* userEmail = "testbalanza@gmail.com";

//variables for stability and final values
const int numValues = 5; // Number of values to check
int weights[numValues]; // Array to store the weights values
int heights[numValues]; // Array to store the heights values
int currentIndex = 0; // Current index in the array
bool stableWeight = false;
bool stableHeight = false;
unsigned long stabilityStartTime = 0; // Start time for stability
unsigned long restartTime = 0; // Time to restart values

//lcd i2c variables
int lcdColumns = 16;
int lcdRows = 2;

//pins
const int HX711_dout = 19; //mcu > HX711 dout pin
const int HX711_sck = 18; //mcu > HX711 sck pin
const int Trigger = 25;   //Pin digital 2 para el Trigger del sensor
const int Echo = 12;   //Pin digital 3 para el Echo del sensor

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
unsigned long t = 0;
bool newDataReady;
float weight;
float height;
float newWeight;
float newHeight;

//LCD i2c constructor:
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);


void setup() {
  Serial.begin(9600); delay(10);
  
  //connect to wifi
  connectWifi();

  //Start distance sensor  (HC-SR04)
  pinMode(Trigger, OUTPUT); 
  pinMode(Echo, INPUT);  
  digitalWrite(Trigger, LOW);

  //Start LCD and backlight
  lcd.init();                      
  lcd.backlight();

  //Prints Starting on console and display
  Serial.println();
  Serial.println("Iniciando...");
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  //Sart scale and sets calibration value manually
  LoadCell.begin();
  float calibrationValue = 43.69; // calibration value

  //Tares scale and starts up. Timeout ocurrs when not working.
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
    
  }
}

void loop() {
  static boolean newDataReady = 0;

  long time; //time it takes to reach eco
  long distance; //distance (cm)

  //Distance sensor signals (10 us)
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);          
  digitalWrite(Trigger, LOW);

  time = pulseIn(Echo, HIGH); //Width of pulse
  distance = time/59;
  height = 27 - distance; //Height from object

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t ) {
      weight = LoadCell.getData();
      checkPrintableWeight(); //checks if weight > 8 so we print and retrieve values.
    }
    delay(200);
  }
}

// ---------------

bool checkWeightStability() {
  // Check if the weights are stable within ±1 difference
  for (int i = 0; i < numValues - 1; i++) {
    int diff = abs(weights[i] - weights[i + 1]);
    if (diff > 2) {
      return false; // Values are not stable
    }
  }
  return true; // Values are stable
}

bool checkHeightStability() {
  // Check if the heights are stable within ±1 difference
  for (int i = 0; i < numValues - 1; i++) {
    int diff = abs(heights[i] - heights[i + 1]);
    if (diff > 2) {
      return false; // Values are not stable
    }
  }
  return true; // Values are stable
}

void checkPrintableWeight(){
  if (weight > 4) {
        //Prints weights and heights constantly on display
        lcd.setCursor(0, 0);
        lcd.print("Calculando...");

        // Get the new weight and height
        newWeight = weight;
        newHeight = height;

        // Update the array
        weights[currentIndex] = newWeight;
        heights[currentIndex] = newHeight;

        // Check stability if enough values have been collected
        checkFull();

        // Increment the current index
        currentIndex++;
        if (currentIndex >= numValues) {
          currentIndex = 0; // Wrap around to the beginning
        }

        //resets weight value
        newDataReady = 0;
        t = millis();
      }
      else{
        lcd.clear();
      }
}

void checkFull(){
  if (currentIndex >= numValues - 1) {
    stableWeight = checkWeightStability();
    stableHeight = checkHeightStability();

    // If both stable, perform actions
    if (stableWeight && stableHeight) {
      stableValues();
    } 
    // If not stable, clear LCD and reset stability timer
    else {
      //lcd.clear();
      stabilityStartTime = 0;
    }

  }
}

void stableValues(){
  if (stabilityStartTime == 0) {
  stabilityStartTime = millis(); // Start the stability timer
  sendData();
  }
  while(millis() - stabilityStartTime < 10000){
    lcd.clear();
    delay(800);
    //Prints stable weights and heights constantly on display
    lcd.setCursor(0, 0);
    lcd.print("Peso: " + String(newWeight) + " g");
    lcd.setCursor(0,1);
    lcd.print("Altura: " + String(newHeight) + " cm");
    delay(800);
  }
  // Check if 10 seconds have passed since stability
  if (millis() - stabilityStartTime >= 10000) {
    resetValues();
    setup();
  }
}

void resetValues() {
  // Reset all values to 0
  for (int i = 0; i < numValues; i++) {
    weights[i] = 0;
    heights[i] = 0;
  }

  // Reset variables
  currentIndex = 0;
  stableWeight = false;
  stableHeight = false;
  stabilityStartTime = 0;
  restartTime = millis(); // Set the restart time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Retire el peso");
  lcd.setCursor(0, 1);
  lcd.print("de la balanza.");
  delay(5000); // Delay for 5 seconds
  lcd.clear();
}

void sendData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://us-central1-balanzasupersonica.cloudfunctions.net/globalWrite");
    http.addHeader("Content-Type", "application/json");
    String jsonPayload = "{\"collection\":\"" + String(userEmail) + "\",\"weight\":\"" + String(newWeight) + "\",\"height\":\"" + String(newHeight) + "\"}";
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void connectWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}