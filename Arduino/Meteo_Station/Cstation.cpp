#include "CStation.h"
#include "FS.h"

ADC_MODE(ADC_TOUT) // NodeMCU ADC initialization: external pin reading values enabled
#define ADC_BATTERY_COEFFICENT 1000.0 // ADC correction factor... depending on the voltage divider
#define BATTERY_VOLTAGE (4.2 / 1024.0) * ADC_BATTERY_COEFFICENT // mv

// how many seconds should try to connect to the wifi network
#define WIFI_TIMEOUT       5    // seconds

//  enable hotspot password
#define ENABLE_HOTSPOT_PSW 0 // 0 -> password disabled
// 1 -> password enabled

// network defaults
#define DEFAULT_SSID         ""
#define DEFAULT_PSWD         ""
#define DEFAULT_HOTSPOT_SSID "AUGCMyStation"
#define DEFAULT_HOTSPOT_PSWD ""
#define DEFAULT_BLYNK_SERVER "shurillu.no-ip.org"
#define DEFAULT_BLYNK_PORT   "8080"
#define DEFAULT_BLYNK_TOKEN  ""
#define DEFAULT_THING_CHANNEL ""
#define DEFAULT_THING_APIKEY  ""

#define NETWORK_CONFIG_FILE "/network.cfg"

// tags for network configuration file
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

// WifiManager callbacks and variables ------------------------------------------------------------------------
bool shouldSaveConfig;

// enter in config mode
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

//callback notifying us of the need to save config. Only when the connection is established
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
// ------------------------------------------------------------------------------------------------------------

CStation::CStation(): CStation(false)
{
}

CStation::CStation(bool formatFS)
{
  // ADC initialization (for battery voltage reading)
  pinMode(A0, INPUT);

  initFS(formatFS);
  if (!readNetworkConfigFile()) setNetworkConfigDefaults();
}

CStation::~CStation()
{
}

bool CStation::wifiConnect(bool autoStartHotspot)
{
#ifdef USE_DEBUG
  Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Connecting...");
#endif
  WiFi.begin(m_wifiSSID, m_wifiPSW);  // Connect to the network
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Quick connecting to %s ", m_wifiSSID.c_str());
#endif
  }
  else {
    // The RTC data was not valid, so make a regular connection
    WiFi.begin(m_wifiSSID, m_wifiPSW);  // Connect to the network
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Normal connecting to %s ", m_wifiSSID.c_str());
#endif
  }

  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries <= WIFI_TIMEOUT)) {
    delay(1000);
#ifdef USE_DEBUG
    Serial.print('.');
#endif
    retries++;
  }

#ifdef USE_DEBUG
  Serial.println(F(""));
#endif

  if ( retries <= WIFI_TIMEOUT && rtcValid ) {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to use quick connection.");
#endif
    // Quick connect is not working, reset WiFi and try regular connection
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.persistent(true);
    delay( 10 );
    WiFi.forceSleepBegin();
    delay( 10 );
    WiFi.forceSleepWake();
    delay( 10 );
    WiFi.begin(m_wifiSSID, m_wifiPSW);  // Connect to the network
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Normal connecting to %s ", m_wifiSSID.c_str());
#endif
    retries = 0;
    while ((WiFi.status() != WL_CONNECTED) && (retries <= WIFI_TIMEOUT)) {
      delay(1000);
#ifdef USE_DEBUG
      Serial.print('.');
#endif
      retries++;
    }
  }

#ifdef USE_DEBUG
  Serial.println(F(""));
#endif

  if (retries <= WIFI_TIMEOUT) {
    // connection established

    // Write current connection info back to RTC
    rtcData.m_channel = WiFi.channel();
    memcpy( rtcData.m_bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
    rtcData.m_crc32 = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
    ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );

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

IPAddress CStation::getBlynkIP(void)
{
  if (!isBlynkKnownByIP())
    return IPAddress(0, 0, 0, 0);
  IPAddress ip;
  ip.fromString(m_blynkServer);
  return (ip);
}

String CStation::getBlynkServer(void)
{
  return m_blynkServer;
}

unsigned int CStation::getBlynkPort(void)
{
  // long port = atol(m_blynkPort.c_str());
  long port = m_blynkPort.toInt();
  if (port < 0)
    port = 0;
  else if (port > 65535)
    port = 0;
  return port;
}

String CStation::getBlynkToken(void)
{
  return m_blynkToken;
}

String CStation::getThingApiKey(void)
{
  return m_thingApiKey;
}

unsigned long CStation::getThingChannel(void)
{
  // long mychannel = atol(m_thingChannel.c_str());
  long mychannel = m_thingChannel.toInt();
  return (mychannel);
}

bool CStation::isBlynkKnownByIP(void)
{
  IPAddress ip;
  return (ip.fromString(m_blynkServer));
}

unsigned int CStation::getBatteryVoltage(void)
{
  uint16_t voltage = analogRead(A0);            // read battery voltage [0..1023]
  voltage = (float)voltage * BATTERY_VOLTAGE;   // convert it in mV
  return voltage;
}


bool CStation::initFS(bool formatFS)
{
  // try to initialize the SPI file system
  if (!SPIFFS.begin()) {
#ifdef USE_DEBUG
    Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("\nSPIFFS initialization failed.");
#endif
    return false;
  }

  if (!SPIFFS.exists(NETWORK_CONFIG_FILE) || formatFS) {
    // no config file present -> format the SPI file system
    if (!SPIFFS.format()) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("SPIFFS Format error.");
#endif
      return (false);
    }
    // create the config file
    if (!writeNetworkConfigFile(true)) {
#ifdef USE_DEBUG
      Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
#endif
      return (false);
    }
  }
  return (true);
}


bool CStation::writeNetworkConfigFile(bool useDefault)
{
  File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "w");
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
  }
  configFile.close();
  return (true);
}

bool CStation::readNetworkConfigFile(void)
{
  File configFile = SPIFFS.open(NETWORK_CONFIG_FILE, "r");
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
        Serial.print("["); Serial.print(millis()); Serial.print("] "); Serial.println("Wrong firmware version, loading defaults.");
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
  }
  configFile.close();
  return (true);
}

void CStation::startHotspot(void)
{
  WiFiManager wifiManager;
  // wifimanager configuration
  shouldSaveConfig = false;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
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

    if (!writeNetworkConfigFile()) {
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

void CStation::setNetworkConfigDefaults(void)
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
}

uint32_t CStation::calculateCRC32( const uint8_t *data, size_t length ) {
  uint32_t crc = 0xffffffff;
  while ( length-- ) {
    uint8_t c = *data++;
    for ( uint32_t i = 0x80; i > 0; i >>= 1 ) {
      bool bit = crc & 0x80000000;
      if ( c & i ) {
        bit = !bit;
      }

      crc <<= 1;
      if ( bit ) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}
