/* Comment this out to disable prints and save space */
#define USE_DEBUG

// how many seconds should try to connect to the wifi network
#define WIFI_TIMEOUT       5    // seconds
#define NETWORK_CFG_FILE_VERSION "2.2.0" // network config file version
#define ENABLE_HOTSPOT_PSW 0 // 0 -> password disabled 1 -> password enabled

// network defaults
#define DEFAULT_DELAY        "15"
#define DEFAULT_SSID         ""
#define DEFAULT_PSWD         ""
#define DEFAULT_HOTSPOT_SSID "AUGCMyStation"
#define DEFAULT_HOTSPOT_PSWD ""
#define DEFAULT_BLYNK_SERVER "blynk.augc.it"
#define DEFAULT_BLYNK_PORT   "8080"
#define DEFAULT_BLYNK_TOKEN  ""
#define DEFAULT_THING_CHANNEL ""
#define DEFAULT_THING_APIKEY  ""

#define NETWORK_CONFIG_FILE "/network.cfg"

// tags for network configuration file
#define DELAY_TAG        "DeepSleepDelay = "
#define VERSION_TAG      "Version = "
#define WIFI_SSID_TAG    "WiFiSSID = "
#define WIFI_PSWD_TAG    "WiFiPassword = "
#define HS_SSID_TAG      "HotspotSSID = "
#define HS_PSWD_TAG      "HotspotPassword = "
#define BLYNK_SERVER_TAG "BlynkServer = "
#define BLYNK_PORT_TAG   "BlynkPort = "
#define BLYNK_TOKEN_TAG  "BlynkToken = "
#define THING_CHANNEL_TAG "ThingChannel = "
#define THING_APIKEY_TAG  "ThingApiKey = "

#include <WiFiManager.h>                  // on Arduino Library Manager  --> WiFimanager by Tzapu,Tablatronix version 0.15.0
#include <ESP8266httpUpdate.h>
#include <time.h>
#include <LittleFS.h>

WiFiClient client;

const int FW_VERSION = 0;
const char* fwServerBase = "raw.githubusercontent.com";
const char* fwDirBase = "/arduinousergroupcagliari/augc_meteo_esp8266/LittleFS/bin/";
const char* fwNameBase = "latest.version";
const uint8_t fingerprint[20] = { 0x70, 0x94, 0xDE, 0xDD, 0xE6, 0xC4, 0x69, 0x48, 0x3A, 0x92, 0x70, 0xA1, 0x48, 0x56, 0x78, 0x2D, 0x18, 0x64, 0xE0, 0xB7 };

// WifiManager callbacks and variables ------------------------------------------------------------------------
bool shouldSaveConfig;
String   m_delay, m_wifiSSID, m_wifiPSW, m_hotspotSSID, m_hotspotPSW, m_blynkServer, m_blynkPort, m_blynkToken, m_thingChannel, m_thingApiKey;


// Setup --------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(74880);
  DEBUGSPC();
  DEBUGLN(F("METEO STATION!!"));
  DEBUGLN("Firmware version: " + String(FW_VERSION));
  DEBUGSPC();

  initFS(false);
  if (!readNetworkConfigFile()) setNetworkConfigDefaults();
  wifiConnect(true);
  checkupdate();
}

void loop() {}


//callback notifying us of the need to save config. Only when the connection is established
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// enter in config mode
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

// ------------------------------------------------------------------------------------------------------------

bool initFS(bool formatFS)
{
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Try to initialize the SPI file system...");
#endif
  // try to initialize the SPI file system
  if (!LittleFS.begin()) {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("LittleFS initialization failed.");
#endif
    return false;
  }
  else
  {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("LittleFS initialization success.");
#endif
  }

#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Search for %s file\n", NETWORK_CONFIG_FILE);
#endif
  if (!LittleFS.exists(NETWORK_CONFIG_FILE) || formatFS) {
    // no config file present -> format the SPI file system
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Check LittleFS format...");
#endif
    if (!LittleFS.format()) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("LittleFS Format error.");
#endif
      return (false);
    }
    else
    {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("LittleFS Format success.");
#endif
    }
    // create the config file
    if (!writeNetworkConfigFile(true)) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
#endif
      return (false);
    }
    else
    {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Create %s file.\n", NETWORK_CONFIG_FILE);
#endif
    }
  }
  else
  {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("%s file found.\n", NETWORK_CONFIG_FILE);
#endif
  }
  return (true);
}


bool readNetworkConfigFile(void)
{
  File configFile = LittleFS.open(NETWORK_CONFIG_FILE, "r");
  if (!configFile) {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to open %s file.\n", NETWORK_CONFIG_FILE);
#endif
    return (false);
  }

  // read configiguration data
  while (configFile.available()) {
    String data = configFile.readStringUntil('\n');
    if (data.startsWith(VERSION_TAG)) {
      data.replace(VERSION_TAG, "");
      if (data != NETWORK_CFG_FILE_VERSION) {
#ifdef USE_DEBUG
        if (data == "2.0.0")
        {
          Serial.print("[");Serial.print(millis());Serial.print("] ");Serial.println("Old firmware version, need update");
        }
        else
        {
          Serial.print("[");Serial.print(millis());Serial.print("] ");Serial.println("Wrong firmware version, loading defaults.");
        }
#endif
        // different firmware version -> generate a new default one
        configFile.close();
        writeNetworkConfigFile(true);
        setNetworkConfigDefaults();
        return (true);
      }
    }
    else if (data.startsWith(WIFI_SSID_TAG)) {
      data.replace(WIFI_SSID_TAG, "");
      m_wifiSSID = data;
    }
    else if (data.startsWith(WIFI_PSWD_TAG)) {
      data.replace(WIFI_PSWD_TAG, "");
      m_wifiPSW = data;
    }
    else if (data.startsWith(HS_SSID_TAG)) {
      data.replace(HS_SSID_TAG, "");
      m_hotspotSSID = data;
    }
    else if (data.startsWith(HS_PSWD_TAG)) {
      data.replace(HS_PSWD_TAG, "");
      m_hotspotPSW = data;
    }
    else if (data.startsWith(BLYNK_SERVER_TAG)) {
      data.replace(BLYNK_SERVER_TAG, "");
      m_blynkServer = data;
    }
    else if (data.startsWith(BLYNK_PORT_TAG)) {
      data.replace(BLYNK_PORT_TAG, "");
      m_blynkPort = data;
    }
    else if (data.startsWith(BLYNK_TOKEN_TAG)) {
      data.replace(BLYNK_TOKEN_TAG, "");
      m_blynkToken = data;
    }
    else if (data.startsWith(THING_CHANNEL_TAG)) {
      data.replace(THING_CHANNEL_TAG, "");
      m_thingChannel = data;
    }
    else if (data.startsWith(THING_APIKEY_TAG)) {
      data.replace(THING_APIKEY_TAG, "");
      m_thingApiKey = data;
    }
    else if (data.startsWith(DELAY_TAG)) {
      data.replace(DELAY_TAG, "");
      m_delay = data;
    }
  }
  configFile.close();
  return (true);
}


bool writeNetworkConfigFile(bool useDefault)
{
  File configFile = LittleFS.open(NETWORK_CONFIG_FILE, "w");
  if (!configFile) {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
#endif
    return (false);
  }
  configFile.printf("%s%s\n", VERSION_TAG, NETWORK_CFG_FILE_VERSION);
  if (useDefault) {
    configFile.printf("%s%s\n", WIFI_SSID_TAG, DEFAULT_SSID);
    configFile.printf("%s%s\n", WIFI_PSWD_TAG, DEFAULT_PSWD);
    configFile.printf("%s%s\n", HS_SSID_TAG, DEFAULT_HOTSPOT_SSID);
    configFile.printf("%s%s\n", HS_PSWD_TAG, DEFAULT_HOTSPOT_PSWD);
    configFile.printf("%s%s\n", BLYNK_SERVER_TAG, DEFAULT_BLYNK_SERVER);
    configFile.printf("%s%s\n", BLYNK_PORT_TAG, DEFAULT_BLYNK_PORT);
    configFile.printf("%s%s\n", BLYNK_TOKEN_TAG, DEFAULT_BLYNK_TOKEN);
    configFile.printf("%s%s\n", THING_CHANNEL_TAG, DEFAULT_THING_CHANNEL);
    configFile.printf("%s%s\n", THING_CHANNEL_TAG, DEFAULT_THING_APIKEY);
    configFile.printf("%s%s\n", DELAY_TAG, DEFAULT_DELAY);
  }
  else {
    configFile.printf("%s%s\n", WIFI_SSID_TAG, m_wifiSSID.c_str());
    configFile.printf("%s%s\n", WIFI_PSWD_TAG, m_wifiPSW.c_str());
    configFile.printf("%s%s\n", HS_SSID_TAG, m_hotspotSSID.c_str());
    configFile.printf("%s%s\n", HS_PSWD_TAG, m_hotspotPSW.c_str());
    configFile.printf("%s%s\n", BLYNK_SERVER_TAG, m_blynkServer.c_str());
    configFile.printf("%s%s\n", BLYNK_PORT_TAG, m_blynkPort.c_str());
    configFile.printf("%s%s\n", BLYNK_TOKEN_TAG, m_blynkToken.c_str());
    configFile.printf("%s%s\n", THING_CHANNEL_TAG, m_thingChannel.c_str());
    configFile.printf("%s%s\n", THING_APIKEY_TAG, m_thingApiKey.c_str());
    configFile.printf("%s%s\n", DELAY_TAG, m_delay.c_str());
  }
  configFile.close();
  return (true);
}


void setNetworkConfigDefaults(void)
{
  m_wifiSSID    = DEFAULT_SSID;
  m_wifiPSW     = DEFAULT_PSWD;
  m_hotspotSSID = DEFAULT_HOTSPOT_SSID;
  m_hotspotPSW  = DEFAULT_HOTSPOT_PSWD;
  m_blynkServer = DEFAULT_BLYNK_SERVER;
  m_blynkPort   = DEFAULT_BLYNK_PORT;
  m_blynkToken  = DEFAULT_BLYNK_TOKEN;
  m_thingChannel = DEFAULT_THING_CHANNEL;
  m_thingApiKey = DEFAULT_THING_APIKEY;
  m_delay = DEFAULT_DELAY;
}


bool wifiConnect(bool autoStartHotspot)
{
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Connecting...");
#endif
  WiFi.begin(m_wifiSSID, m_wifiPSW);  // Connect to the network
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Connecting to %s ", m_wifiSSID.c_str());
#endif
  int i = 0;
  while ((WiFi.status() != WL_CONNECTED) && (i <= WIFI_TIMEOUT)) {
    delay(1000);
#ifdef USE_DEBUG
    Serial.print('.');
#endif
    i++;
  }
#ifdef USE_DEBUG
  Serial.println(F(""));
#endif

  if (i <= WIFI_TIMEOUT) {
    // connection established
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Connection established!");
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.print("IP address: "); Serial.println(WiFi.localIP());
#endif
  }
  else {
    // unable to connect -> launch WiFi manager
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to connect to %s\n", m_wifiSSID.c_str());
#endif
    if (autoStartHotspot) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Launching hotspot...\n");
#endif
      startHotspot();
    }
    else
      return false;
  }
  return true;
}


void startHotspot(void)
{
  WiFiManager wifiManager;
  shouldSaveConfig = false;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter customDelay("DS DELAY", "DeepSleep Delay", m_delay.c_str(), 5);
  wifiManager.addParameter(&customDelay);
  WiFiManagerParameter customHotspotSSID("HS SSID", "Hotspot SSID", m_hotspotSSID.c_str(), 40);
  wifiManager.addParameter(&customHotspotSSID);
  WiFiManagerParameter customHotspotPSW("HS PSWD", "Hotspot password", m_hotspotPSW.c_str(), 40);
  wifiManager.addParameter(&customHotspotPSW);
  WiFiManagerParameter customBlynkServer("Server", "Blynk Server", m_blynkServer.c_str(), 40);
  wifiManager.addParameter(&customBlynkServer);
  WiFiManagerParameter customBlynkPort("Port", "Blynk Port", m_blynkPort.c_str(), 5);
  wifiManager.addParameter(&customBlynkPort);
  WiFiManagerParameter customBlynkToken("Token", "Blynk Token", m_blynkToken.c_str(), 40);
  wifiManager.addParameter(&customBlynkToken);
  WiFiManagerParameter customThingChannel("Channel", "ThingSpeak Channel", m_thingChannel.c_str(), 10);
  wifiManager.addParameter(&customThingChannel);
  WiFiManagerParameter customThingApiKey("ApiKey", "ThingSpeak ApiKey", m_thingApiKey.c_str(), 40);
  wifiManager.addParameter(&customThingApiKey);

#if ENABLE_HOTSPOT_PSW == 0
  wifiManager.startConfigPortal(m_hotspotSSID.c_str());
#else
  wifiManager.startConfigPortal(m_hotspotSSID.c_str(), m_hotspotPSW.c_str());
#endif

  if (shouldSaveConfig) {
    m_wifiSSID    = WiFi.SSID();
    m_wifiPSW     = WiFi.psk();
    m_hotspotSSID = customHotspotSSID.getValue();
    m_hotspotPSW  = customHotspotPSW.getValue();
    m_blynkServer = customBlynkServer.getValue();
    m_blynkPort   = customBlynkPort.getValue();
    m_blynkToken  = customBlynkToken.getValue();
    m_thingChannel = customThingChannel.getValue();
    m_thingApiKey = customThingApiKey.getValue();
    m_delay = customDelay.getValue();

    if (!writeNetworkConfigFile(false)) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Unable to writing config file");
#endif
    } else {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Config file written.");
#endif
    }
  }
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
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

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
