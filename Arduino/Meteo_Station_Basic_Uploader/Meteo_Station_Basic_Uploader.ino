
// how many seconds should try to connect to the wifi network
#define WIFI_TIMEOUT 5                  // seconds
#define NETWORK_CFG_FILE_VERSION "3.0"  // network config file version
#define NETWORK_CONFIG_FILE "/network.cfg"
#define ENABLE_HOTSPOT_PSW 0  // 0 -> password disabled 1 -> password enabled


// network defaults
#define DEFAULT_DELAY "15"
#define DEFAULT_SSID ""
#define DEFAULT_PSWD ""
#define DEFAULT_HOTSPOT_SSID "AUGCMyStation"
#define DEFAULT_HOTSPOT_PSWD ""
#define DEFAULT_BLYNK_SERVER "blynk.augc.it"
#define DEFAULT_BLYNK_PORT "8080"
#define DEFAULT_BLYNK_TOKEN ""
#define DEFAULT_THING_CHANNEL ""
#define DEFAULT_THING_APIKEY ""


// tags for network configuration file
#define DELAY_TAG "DeepSleepDelay = "
#define VERSION_TAG "Version = "
#define WIFI_SSID_TAG "WiFiSSID = "
#define WIFI_PSWD_TAG "WiFiPassword = "
#define HS_SSID_TAG "HotspotSSID = "
#define HS_PSWD_TAG "HotspotPassword = "
#define BLYNK_SERVER_TAG "BlynkServer = "
#define BLYNK_PORT_TAG "BlynkPort = "
#define BLYNK_TOKEN_TAG "BlynkToken = "
#define THING_CHANNEL_TAG "ThingChannel = "
#define THING_APIKEY_TAG "ThingApiKey = "


#include "utility.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiManager.h>

const int FW_VERSION = 0;
const char* fwServerBase = "raw.githubusercontent.com";
const char* fwDirBase = "/arduinousergroupcagliari/augc_meteo_esp8266/dev/bin/";
const char* fwNameBase = "latest.version";

// WifiManager callbacks and variables ------------------------------------------------------------------------
bool shouldSaveConfig;
String m_delay, m_wifiSSID, m_wifiPSW, m_hotspotSSID, m_hotspotPSW, m_blynkServer, m_blynkPort, m_blynkToken, m_thingChannel, m_thingApiKey;


// Crea un'istanza di Debug
Debug debug;


// Setup --------------------------------------------------------------------------------------------------------
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  debug.printspc();
  debug.println(F("METEO STATION BASIC UPLOADER!!"));
  debug.println("Firmware version: " + String(FW_VERSION));
  debug.printspc();
  initFS(true);
  if (!readNetworkConfigFile()) setNetworkConfigDefaults();
  wifiConnect(true);
  checkFirmwareUpgrade();
}

void loop() {}


//callback notifying us of the need to save config. Only when the connection is established
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


// ------------------------------------------------------------------------------------------------------------
// enter in config mode
void configModeCallback(WiFiManager* myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}


bool initFS(bool formatFS) {
  debug.println("Trying to initialize the SPIFFS file system...");

  if (!SPIFFS.begin()) {
    debug.println("SPIFFS mount failed, attempting to format...");

    if (!SPIFFS.format() || !SPIFFS.begin()) {
      debug.println("SPIFFS format or re-mount failed, check your setup.");
      return false;
    }

    debug.println("SPIFFS format and re-mount successful.");
    formatFS = false;
  } else {
    debug.println("SPIFFS initialization successful.");
  }

  debug.printf("Searching for %s file...\n", NETWORK_CONFIG_FILE);

  if (!SPIFFS.exists(NETWORK_CONFIG_FILE)) {
    debug.printf("%s file not found.\n", NETWORK_CONFIG_FILE);

    if (formatFS) {
      debug.println("Formatting SPIFFS due to missing config file...");

      if (!SPIFFS.format()) {
        debug.println("SPIFFS format failed.");
        return false;
      }

      debug.println("SPIFFS format successful.");
    }

    if (!writeNetworkConfigFile(true)) {
      debug.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
      return false;
    }

    debug.printf("Created %s file.\n", NETWORK_CONFIG_FILE);
  } else {
    debug.printf("%s file found.\n", NETWORK_CONFIG_FILE);
  }

  return true;
}


bool readNetworkConfigFile(void) {
  File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "r");
  if (!configFile) {
    debug.printf("Unable to open %s file.\n", NETWORK_CONFIG_FILE);
    return (false);
  }
  debug.printf("Open %s file.\n", NETWORK_CONFIG_FILE);

  // read configuration data
  while (configFile.available()) {
    String data = configFile.readStringUntil('\n');
    if (data.startsWith(VERSION_TAG)) {
      data.replace(VERSION_TAG, "");
      if (data != NETWORK_CFG_FILE_VERSION) {
        if (data == "2.0.0") {
          debug.println("Old firmware version, need update");
        } else {
          debug.println("Wrong firmware version, loading defaults.");
        }
        // different firmware version -> generate a new default one
        configFile.close();
        writeNetworkConfigFile(true);
        setNetworkConfigDefaults();
        return (true);
      }
      debug.printf("Firmware Version: %s\n", data.c_str());
    } else if (data.startsWith(WIFI_SSID_TAG)) {
      data.replace(WIFI_SSID_TAG, "");
      m_wifiSSID = data;
      debug.printf("WiFi SSID: %s\n", m_wifiSSID.c_str());
    } else if (data.startsWith(WIFI_PSWD_TAG)) {
      data.replace(WIFI_PSWD_TAG, "");
      m_wifiPSW = data;
      debug.printf("WiFi Password: %s\n", m_wifiPSW.c_str());
    } else if (data.startsWith(HS_SSID_TAG)) {
      data.replace(HS_SSID_TAG, "");
      m_hotspotSSID = data;
      debug.printf("Hotspot SSID: %s\n", m_hotspotSSID.c_str());
    } else if (data.startsWith(HS_PSWD_TAG)) {
      data.replace(HS_PSWD_TAG, "");
      m_hotspotPSW = data;
      debug.printf("Hotspot Password: %s\n", m_hotspotPSW.c_str());
    } else if (data.startsWith(BLYNK_SERVER_TAG)) {
      data.replace(BLYNK_SERVER_TAG, "");
      m_blynkServer = data;
      debug.printf("Blynk Server: %s\n", m_blynkServer.c_str());
    } else if (data.startsWith(BLYNK_PORT_TAG)) {
      data.replace(BLYNK_PORT_TAG, "");
      m_blynkPort = data;
      debug.printf("Blynk Port: %s\n", m_blynkPort.c_str());
    } else if (data.startsWith(BLYNK_TOKEN_TAG)) {
      data.replace(BLYNK_TOKEN_TAG, "");
      m_blynkToken = data;
      debug.printf("Blynk Token: %s\n", m_blynkToken.c_str());
    } else if (data.startsWith(THING_CHANNEL_TAG)) {
      data.replace(THING_CHANNEL_TAG, "");
      m_thingChannel = data;
      debug.printf("ThingSpeak Channel: %s\n", m_thingChannel.c_str());
    } else if (data.startsWith(THING_APIKEY_TAG)) {
      data.replace(THING_APIKEY_TAG, "");
      m_thingApiKey = data;
      debug.printf("ThingSpeak API Key: %s\n", m_thingApiKey.c_str());
    } else if (data.startsWith(DELAY_TAG)) {
      data.replace(DELAY_TAG, "");
      m_delay = data;
      debug.printf("Delay: %s\n", m_delay.c_str());
    }
  }
  configFile.close();
  return (true);
}


bool writeNetworkConfigFile(bool useDefault) {
  File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "w");
  if (!configFile) {
    debug.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
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
    configFile.printf("%s%s\n", THING_APIKEY_TAG, DEFAULT_THING_APIKEY);
    configFile.printf("%s%s\n", DELAY_TAG, DEFAULT_DELAY);
  } else {
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
  debug.printf("Write %s file.\n", NETWORK_CONFIG_FILE);
  return (true);
}


void setNetworkConfigDefaults(void) {
  m_wifiSSID = DEFAULT_SSID;
  m_wifiPSW = DEFAULT_PSWD;
  m_hotspotSSID = DEFAULT_HOTSPOT_SSID;
  m_hotspotPSW = DEFAULT_HOTSPOT_PSWD;
  m_blynkServer = DEFAULT_BLYNK_SERVER;
  m_blynkPort = DEFAULT_BLYNK_PORT;
  m_blynkToken = DEFAULT_BLYNK_TOKEN;
  m_thingChannel = DEFAULT_THING_CHANNEL;
  m_thingApiKey = DEFAULT_THING_APIKEY;
  m_delay = DEFAULT_DELAY;
}


bool wifiConnect(bool autoStartHotspot) {
  debug.println("Start connection");
  WiFi.begin(m_wifiSSID, m_wifiPSW);  // Connect to the network
  debug.printf("Connecting to %s ", m_wifiSSID.c_str());
  int i = 0;
  while ((WiFi.status() != WL_CONNECTED) && (i <= WIFI_TIMEOUT)) {
    delay(1000);
    debug.printp(".");  // Print a dot for each second of connection attempt
    i++;
  }
  debug.printspc();  // Move to the next line
  if (i <= WIFI_TIMEOUT) {
    // Connection established
    debug.println("Connection established!");
    debug.print("IP address: ");
    debug.printp(WiFi.localIP().toString().c_str());
    debug.printspc();
  } else {
    debug.printf("Unable to connect to %s\n", m_wifiSSID.c_str());
    if (autoStartHotspot) {
      debug.println("Launching hotspot...\n");
      startHotspot();
    } else {
      return false;
    }
  }

  return true;
}


void startHotspot(void) {
  WiFiManager wifiManager;
  shouldSaveConfig = false;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter customDelay("DS_DELAY", "DeepSleep Delay", m_delay.c_str(), 5);
  wifiManager.addParameter(&customDelay);
  WiFiManagerParameter customHotspotSSID("HS_SSID", "Hotspot SSID", m_hotspotSSID.c_str(), 40);
  wifiManager.addParameter(&customHotspotSSID);
  WiFiManagerParameter customHotspotPSW("HS_PSWD", "Hotspot password", m_hotspotPSW.c_str(), 40);
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
    m_wifiSSID = WiFi.SSID();
    m_wifiPSW = WiFi.psk();
    m_hotspotSSID = customHotspotSSID.getValue();
    m_hotspotPSW = customHotspotPSW.getValue();
    m_blynkServer = customBlynkServer.getValue();
    m_blynkPort = customBlynkPort.getValue();
    m_blynkToken = customBlynkToken.getValue();
    m_thingChannel = customThingChannel.getValue();
    m_thingApiKey = customThingApiKey.getValue();
    m_delay = customDelay.getValue();

    if (!writeNetworkConfigFile(false)) {
      debug.println("Unable to write config file");
    } else {
      debug.println("Config file written.");
    }
  }
}


void checkFirmwareUpgrade() {
  String newFWVersion = "0000";
  String fwURL = "https://" + String(fwServerBase) + String(fwDirBase);
  String fwVersionURL = fwURL + fwNameBase;

  debug.println("Checking for firmware updates.");
  debug.println("Firmware version URL: " + String(fwVersionURL));

  HTTPClient clientHttp;
  if (!clientHttp.begin(fwVersionURL)) {
    debug.println("Error initializing HTTPS connection.");
    return;
  }

  int httpCode = clientHttp.GET();
  if (httpCode == -1) {
    debug.println("Connection error: check your network connection.");
  } else if (httpCode == HTTP_CODE_OK) {
    newFWVersion = clientHttp.getString();
    debug.println("Current firmware version: " + String(FW_VERSION));
    debug.println("Available firmware version: " + String(newFWVersion));
    int newVersion = newFWVersion.toInt();

    if (newVersion > FW_VERSION) {
      OTAupgrade(clientHttp, fwURL, newVersion);
    } else {
      debug.println("Already on the latest version");
    }
  } else {
    debug.println("Firmware version check failed, got HTTP response code " + String(httpCode));
  }
  clientHttp.end();
}

void OTAupgrade(HTTPClient& clientHttp, String URL, int Version) {
  debug.println("Preparing to upgrade");
  String fwImageURL = URL + String(Version) + ".bin";
  debug.println("Firmware image URL: " + String(fwImageURL));

  if (clientHttp.begin(fwImageURL)) {
    int httpCode = clientHttp.GET();

    if (httpCode == HTTP_CODE_OK) {
      debug.println("Connection OK!");
      int contentLength = clientHttp.getSize();
      debug.println("Firmware size: " + String(contentLength) + " byte");
      if (contentLength > 0) {
        bool canBegin = Update.begin(contentLength);
        if (canBegin) {
          debug.println("Starting upgrade...");
          WiFiClient* clientWifi = clientHttp.getStreamPtr();
          size_t written = Update.writeStream(*clientWifi);
          if (written == contentLength) {
            debug.println("Written : " + String(written) + " successfully");
          } else {
            debug.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
          }
          if (Update.end()) {
            debug.println("OTA done!");
            if (Update.isFinished()) {
              debug.println("Update successfully completed. Rebooting.");
              ESP.restart();
            } else {
              debug.println("Update not finished? Something went wrong!");
            }
          } else {
            debug.println("Error Occurred. Error #: " + String(Update.getError()));
          }
        } else {
          debug.println("Not enough space to begin OTA");
        }
      } else {
        debug.println("Content-Length is not available or invalid");
      }
    } else {
      debug.println("Firmware download failed, got HTTP response code " + String(httpCode));
    }
    clientHttp.end();
  } else {
    debug.println("Error initializing HTTPS connection for firmware download.");
  }
}
