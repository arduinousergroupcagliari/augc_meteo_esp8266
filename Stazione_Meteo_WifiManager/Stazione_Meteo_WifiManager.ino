/* Comment this out to disable prints and save space */
#define DELAY 5 // minutes
#define USE_DEBUG
#define USE_BLYNK
#define USE_THINGSPEAK

#ifdef USE_BLYNK
#ifndef USE_DEBUG
#else
#define BLYNK_PRINT Serial
#endif
#include <BlynkSimpleEsp8266.h>
#define BLYNK_TIMEOUT           5000 // milliseconds
#endif
#ifdef USE_THINGSPEAK
#ifndef USE_DEBUG
#else
#define PRINT_DEBUG_MESSAGES
#endif
#include <ThingSpeak.h>
#endif

#include <BH1750FVI.h>        //https://github.com/enjoyneering/BH1750FVI
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "CStation.h"

BH1750FVI myBH1750(BH1750_DEFAULT_I2CADDR, BH1750_CONTINUOUS_HIGH_RES_MODE_2, BH1750_SENSITIVITY_DEFAULT, BH1750_ACCURACY_DEFAULT);
CStation myStation;
Adafruit_BME280 myBME280; // I2C
WiFiClient client;

// preinit() is called before system startup
// from nonos-sdk's user entry point user_init()

void preinit() {
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

float temperature, humidity, pressure, batteryVoltage;
int batteryLevel, adcValue;
unsigned int lux;
bool useLuxSensor;

// Setup --------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  DEBUGSPC();
  DEBUGLN(F("METEO STATION!!"));

  initStation();
  readSensorData();

  DEBUGLN(F("Waking WiFi up!!"));
  WiFi.forceSleepWake();
  if (!myStation.wifiConnect(false)) goSleep();

#ifdef USE_BLYNK
  dataToBlynk();
#endif

#ifdef USE_THINGSPEAK
  dataToThongSpeak();
#endif

  // checkForUpdates();
  goSleep();
}


void loop() {}


void initStation(void) {
  int mVolt;

  mVolt = myStation.getBatteryVoltage();
  if (mVolt < 3000) {
    DEBUGLN("Battery LOW: " + String(mVolt) + " mV");
    goSleep();
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  DEBUGLN(F("Initialize BME280..."));
  while (!myBME280.begin(0x76)) {
    DEBUGLN(F("Could not find a valid BME280 sensor, check wiring!"));
    for (int x = 0; x < 10; x++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
    }
    DEBUGLN(F("Waking WiFi up and start HotSpot!"));
    WiFi.forceSleepWake();
    myStation.startHotspot();
    delay(5000);
  }
  DEBUGLN(F("BME280 sensor inizialized"));

  DEBUGLN(F("Initialize BH1750FVI..."));
  if (myBH1750.begin(D2, D1) == true)                //SDA - D2, SCL - D1
  {
    myBH1750.setResolution(BH1750_CONTINUOUS_HIGH_RES_MODE);
    myBH1750.setSensitivity(1.61);
    useLuxSensor = true;
    DEBUGLN(F("ROHM BH1750FVI inizialized"));
  }
  else {
    DEBUGLN(F("ROHM BH1750FVI is not present")); //(F()) saves string to flash & keeps dynamic memory free
    useLuxSensor = false;
  }
}


void readSensorData(void) {
  DEBUGLN(F("Read Sensor Data..."));
  DEBUGSPC();
  temperature = NAN;
  pressure    = NAN;
  humidity    = NAN;

  temperature    = myBME280.readTemperature();
  pressure       = myBME280.readPressure() / 100.0F;
  humidity       = myBME280.readHumidity();
  batteryVoltage = myStation.getBatteryVoltage();
  adcValue       = analogRead(A0);
  batteryLevel   = constrain(map(batteryVoltage, 0, 4200, 0, 100), 0, 100);
  if (useLuxSensor)
    lux = myBH1750.readLightLevel();

  DEBUGLN(F("Data readed"));
  DEBUGLN("temperature:    " + String(temperature) + " °C");
  DEBUGLN("humidity:       " + String(humidity) + "%");
  DEBUGLN("pressure :      " + String(pressure) + " mbar");
  DEBUGLN("analog Read:    " + String(adcValue));
  DEBUGLN("batteryVoltage: " + String(batteryVoltage / 1000) + " V");
  DEBUGLN("batteryLevel :  " + String(batteryLevel) + "%");
  if (useLuxSensor)
    DEBUGLN("lux :  " + String(lux) + "lux");
  DEBUGSPC();
}

void dataToBlynk(void) {
  DEBUGLN(F("Initialize Blynk..."));
  char server[40];
  char token[40];
  strcpy(server, myStation.getBlynkServer().c_str());
  strcpy(token, myStation.getBlynkToken().c_str());
  if (myStation.isBlynkKnownByIP()) {
    DEBUGLN("Connecting using IP: " + myStation.getBlynkIP().toString());
    Blynk.config(token, myStation.getBlynkIP(), myStation.getBlynkPort());
  }
  else {
    DEBUGLN("Connecting using server: " + myStation.getBlynkServer());
    Blynk.config(token, server, myStation.getBlynkPort());
  }
  Blynk.connect(BLYNK_TIMEOUT);
  if (!Blynk.connected())
    DEBUGLN("Unable to connect to " + myStation.getBlynkServer() + " server");
  else
    Blynk.run();
  DEBUGSPC();

  if (!Blynk.connected())
    DEBUGLN("Unable to connect to " + myStation.getBlynkServer() + " server");
  else
  {
    DEBUGLN(F("Send data to Blynk:"));
    Blynk.run();
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, pressure);
    Blynk.virtualWrite(V2, humidity);
    Blynk.virtualWrite(V3, adcValue);
    Blynk.virtualWrite(V4, batteryVoltage / 1000);
    if (useLuxSensor)
      Blynk.virtualWrite(V5, lux);
    DEBUGLN(F("Blynk data send."));
    DEBUGSPC();
  }
}

void dataToThongSpeak() {
  DEBUGLN(F("Inizialize ThingSpeak..."));
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  ThingSpeak.setStatus("Running...");
  DEBUGSPC();
  DEBUGLN(F("Send data to ThingSpeak:"));
  ThingSpeak.setField(1, String(temperature));
  ThingSpeak.setField(2, String(humidity));
  ThingSpeak.setField(3, String(pressure));
  ThingSpeak.setField(4, String(batteryVoltage / 1000));
  ThingSpeak.setField(5, String(adcValue));
  if (useLuxSensor)
    ThingSpeak.setField(6, String(lux));
  // write to the ThingSpeak channel
  char myapikey[40];
  strcpy(myapikey, myStation.getThingApiKey().c_str());
  int error = ThingSpeak.writeFields(myStation.getThingChannel(), myapikey);
  if (error == 200)
    DEBUGLN(F("ThingSpeak data send"));
  else
    DEBUGLN("Problem updating channel. HTTP error code " + String(error));
  DEBUGSPC();
}

void goSleep(void) {
  delay(1000);
  DEBUGLN(F("ESP8266 in sleep mode"));
  ESP.deepSleep(DELAY * 60 * 1000000);
  DEBUGSPC();
}

void DEBUG(String toPrint) {
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.print(toPrint);
#endif
}

void DEBUGLN(String toPrint) {
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println(toPrint);
#endif
}

void DEBUGSPC(void) {
#ifdef USE_DEBUG
  Serial.println(F(""));
#endif
}
