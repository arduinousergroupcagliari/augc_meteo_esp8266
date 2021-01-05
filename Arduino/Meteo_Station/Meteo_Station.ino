/* Comment this out to disable prints and save space */
#define USE_DEBUG

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266httpUpdate.h>
#include <time.h>

#ifndef USE_DEBUG
#define BLYNK_PRINT Serial
#endif
#include <BlynkSimpleEsp8266.h>
#define BLYNK_TIMEOUT           5000 // milliseconds
#ifndef USE_DEBUG
#define PRINT_DEBUG_MESSAGES
#endif
#include <ThingSpeak.h>

#include "CStation.h"
#include <hp_BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

CStation myStation;
hp_BH1750 myBH1750;
Adafruit_BME280 myBME280;
WiFiClient client;

const int FW_VERSION = 1216;
bool DEV_VERSION = true;
const char* fwServerBase = "raw.githubusercontent.com";
const char* fwDirBase = "/arduinousergroupcagliari/augc_meteo_esp8266/LittleFS/bin/";
const char* fwNameBase = "latest.version";
const uint8_t fingerprint[20] = { 0x70, 0x94, 0xDE, 0xDD, 0xE6, 0xC4, 0x69, 0x48, 0x3A, 0x92, 0x70, 0xA1, 0x48, 0x56, 0x78, 0x2D, 0x18, 0x64, 0xE0, 0xB7 };

float temperature, humidity, pressure, batteryVoltage;
int batteryLevel, adcValue;
unsigned int lux;
bool useLuxSensor, useBlynk, useThingSpeak;

void preinit() {
  ESP8266WiFiClass::preinitWiFiOff();
}


// Setup --------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(74880);
  DEBUGSPC();
  DEBUGLN(F("METEO STATION!!"));
  DEBUGLN(DEV_VERSION ? "Development version" : "Master version");
  DEBUGLN("Firmware number: " + String(FW_VERSION));
  DEBUGSPC();

  initStation();
  readSensorData();

  DEBUGLN(F("Waking WiFi up!!"));
  WiFi.forceSleepWake();
  if (!myStation.wifiConnect(false)) goSleep();

  if (useBlynk) dataToBlynk();
  if (useThingSpeak) dataToThingSpeak();

  checkupdate();
  goSleep();
}


void loop() {}


void initStation(void) {
  myStation.initStation(false);
  DEBUGLN(F("Check Battery voltage..."));
  int mVolt = myStation.getBatteryVoltage();
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
  if (myBH1750.begin(BH1750_TO_GROUND) == true)                //BH1750_TO_GROUND = 0x23
  {
    myBH1750.calibrateTiming();
    useLuxSensor = true;
    DEBUGLN(F("ROHM BH1750FVI inizialized"));
  }
  else {
    DEBUGLN(F("ROHM BH1750FVI is not present"));
    useLuxSensor = false;
  }

  DEBUG(F("Blynk configuration: "));
  if (myStation.getBlynkToken() != "") {
    useBlynk = true;
    DEBUGP(F("present."));
  }
  else
  { useBlynk = false;
    DEBUGP(F("not present."));
  }
  DEBUGSPC();

  DEBUG(F("ThingSpeak configuration: "));
  if (myStation.getThingApiKey() != "") {
    useThingSpeak = true;
    DEBUGP(F("present."));
  }
  else {
    useThingSpeak = false;
    DEBUGP(F("not present."));
  }
  DEBUGSPC();
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
  {
    myBH1750.start(BH1750_QUALITY_LOW, 31);   //  starts a measurement at low quality
    float val = myBH1750.getLux();            //  do a blocking read and waits until a result receives
    myBH1750.start(BH1750_QUALITY_HIGH2, 254);//  starts a measurement with high quality but restricted range in brightness
    val = myBH1750.getLux();
    if (myBH1750.saturated() == true) {
      val = val * myBH1750.getMtregTime() / myBH1750.getTime();  //  here we calculate from a saturated sensor the brightness!
    }
    lux = (unsigned int)val;
  }

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

void dataToThingSpeak() {
  DEBUGLN(F("Inizialize ThingSpeak..."));
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  ThingSpeak.setStatus((DEV_VERSION ? "Running... Dev_" : "Running... Master_") + String(FW_VERSION));
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
  DEBUGLN("ESP8266 in sleep mode: " + (String)myStation.getDelay() + " minutes");
  ESP.deepSleep(myStation.getDelay() * 60 * 1000000);
  DEBUGSPC();
}

void DEBUG(String toPrint) {
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.print(toPrint);
#endif
}

void DEBUGP(String toPrint) {
#ifdef USE_DEBUG
  Serial.print(toPrint);
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


void checkupdate() {
  DEBUGLN( "Checking for firmware updates." );
  checkNtpClock();
  BearSSL::WiFiClientSecure secureclient;
  secureclient.setFingerprint(fingerprint);
  bool mfln = secureclient.probeMaxFragmentLength(fwServerBase, 443, 1024);  // server must be the same as in ESPhttpUpdate.update()
  DEBUGLN("MFLN supported: " + String(mfln ? "yes" : "no"));
  if (mfln) {
    secureclient.setBufferSizes(1024, 1024);
  }

  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  String newFWVersion = "0000";
  String fwURL = "https://" + String(fwServerBase) + String(fwDirBase);
  String fwVersionURL = fwURL + fwNameBase;
  DEBUGLN( "Firmware version URL: " + String( fwVersionURL ));

  HTTPClient httpClient;
  httpClient.begin(secureclient, fwVersionURL );
  int httpCode = httpClient.GET();
  if ( httpCode == 200 )
    newFWVersion = httpClient.getString();
  else {
    DEBUGLN( "Firmware version check failed, got HTTP response code " + String( httpCode ));
  }
  DEBUGLN( "Current firmware version: " + String( FW_VERSION ));
  DEBUGLN( "Available firmware version: " + String( newFWVersion ));

  int newVersion = newFWVersion.toInt();

  if ( newVersion > FW_VERSION ) {
    DEBUGLN( "Preparing to update" );
    String fwImageURL = fwURL + String(newVersion) + ".bin";
    DEBUGLN( "Firmware image URL: " + String( fwImageURL ));
    t_httpUpdate_return ret = ESPhttpUpdate.update(secureclient, fwImageURL);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        DEBUGLN("HTTP_UPDATE_FAILED Error " + String(ESPhttpUpdate.getLastError()) + ": " + ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        DEBUGLN("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        DEBUGLN("HTTP_UPDATE_OK");
        break;
    }
  }
  else
    DEBUGLN( "Already on latest version" );
}

void checkNtpClock() {
  configTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov", "time.windows.com");  // TZ Rome

  DEBUG(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    DEBUGP(F("."));
    now = time(nullptr);
  }

  DEBUGSPC();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  DEBUGLN("Current time: " + String(asctime(&timeinfo)));
}
