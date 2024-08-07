
// how many seconds should try to connect to the wifi network
#define WIFI_TIMEOUT 5                    // seconds
#define NETWORK_CFG_FILE_VERSION "2.2.0"  // network config file version
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
#include <WiFiManager.h>  // on Arduino Library Manager  --> WiFimanager by Tzapu,Tablatronix version 0.15.0
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <SPIFFS.h>


const int FW_VERSION = 0;
const char* fwServerBase = "raw.githubusercontent.com";
const char* fwDirBase = "/arduinousergroupcagliari/augc_meteo_esp8266/dev/bin/";
const char* fwNameBase = "latest.version";
const char* rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIHOTCCBiGgAwIBAgIQBj1JF0BNOeUTyz/uzRsuGzANBgkqhkiG9w0BAQsFADBZ\n"
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMTMwMQYDVQQDEypE\n"
  "aWdpQ2VydCBHbG9iYWwgRzIgVExTIFJTQSBTSEEyNTYgMjAyMCBDQTEwHhcNMjQw\n"
  "MzE1MDAwMDAwWhcNMjUwMzE0MjM1OTU5WjBnMQswCQYDVQQGEwJVUzETMBEGA1UE\n"
  "CBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5jaXNjbzEVMBMGA1UEChMM\n"
  "R2l0SHViLCBJbmMuMRQwEgYDVQQDDAsqLmdpdGh1Yi5pbzCCASIwDQYJKoZIhvcN\n"
  "AQEBBQADggEPADCCAQoCggEBAK0rFKU6TEGvuLCY3ZOuXlG+3jerD6EP1gc1qe35\n"
  "g68FqyGuVPOUddYNZiymjYMZxywoNp3qxlbFFBTf9etsayavT+uW+2UMjqCotAdK\n"
  "KicBEspuExoACFuNgTi7sSUT7A55+k4/+5O+VtpaxQ5dmQk7HxcqvMYx5owBU+fB\n"
  "wYDD+hXeg3YvxLZNeIlN8OlqWL8w9HbG+3ccegVEjOJQbkrcrW7IQMq2Uk92XjxI\n"
  "PmMVIvaefqcC1poGYvS4VvEh3x64vJK1hEM4YLMKBaE/hqFtcMozi+H/8JqTCfzP\n"
  "Qhnu21HIop9rSucxxnZbe9AeHz2LERpUTf3rjgOMg9PB1RUCAwEAAaOCA+0wggPp\n"
  "MB8GA1UdIwQYMBaAFHSFgMBmx9833s+9KTeqAx2+7c0XMB0GA1UdDgQWBBTob1fr\n"
  "hlGY65+lvlPa25SsKC777TB7BgNVHREEdDByggsqLmdpdGh1Yi5pb4IJZ2l0aHVi\n"
  "LmlvghVnaXRodWJ1c2VyY29udGVudC5jb22CDnd3dy5naXRodWIuY29tggwqLmdp\n"
  "dGh1Yi5jb22CFyouZ2l0aHVidXNlcmNvbnRlbnQuY29tggpnaXRodWIuY29tMD4G\n"
  "A1UdIAQ3MDUwMwYGZ4EMAQICMCkwJwYIKwYBBQUHAgEWG2h0dHA6Ly93d3cuZGln\n"
  "aWNlcnQuY29tL0NQUzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUH\n"
  "AwEGCCsGAQUFBwMCMIGfBgNVHR8EgZcwgZQwSKBGoESGQmh0dHA6Ly9jcmwzLmRp\n"
  "Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbEcyVExTUlNBU0hBMjU2MjAyMENBMS0x\n"
  "LmNybDBIoEagRIZCaHR0cDovL2NybDQuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xv\n"
  "YmFsRzJUTFNSU0FTSEEyNTYyMDIwQ0ExLTEuY3JsMIGHBggrBgEFBQcBAQR7MHkw\n"
  "JAYIKwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBRBggrBgEFBQcw\n"
  "AoZFaHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsRzJU\n"
  "TFNSU0FTSEEyNTYyMDIwQ0ExLTEuY3J0MAwGA1UdEwEB/wQCMAAwggF/BgorBgEE\n"
  "AdZ5AgQCBIIBbwSCAWsBaQB2AE51oydcmhDDOFts1N8/Uusd8OCOG41pwLH6ZLFi\n"
  "mjnfAAABjkN89oAAAAQDAEcwRQIgU/M527Wcx0KQ3II7kCuG5WMuOHRSxKkf1xAj\n"
  "JuSkyPACIQCVX0uurcIA2Ug7ipNN2S1ZygukWqJCh7hjIH0XsrXh8QB2AH1ZHhLh\n"
  "eCp7HGFnfF79+NCHXBSgTpWeuQMv2Q6MLnm4AAABjkN89oEAAAQDAEcwRQIgCxpL\n"
  "BDak+TWKarrCHlZn4DlqwEfAN3lvlgSo21HQuU8CIQDicrb72c0lA2suMWPWT92P\n"
  "FLaRvFrFn9HVzI6Vh50YZgB3AObSMWNAd4zBEEEG13G5zsHSQPaWhIb7uocyHf0e\n"
  "N45QAAABjkN89pQAAAQDAEgwRgIhAPJQX4QArFCjM0sKKzsWLmqmmU8lMhKEYR2T\n"
  "ges1AQyQAiEA2Y3VhP5RG+dapcbwYgVbrTlgWzO7KE/lg1x11CVcz3QwDQYJKoZI\n"
  "hvcNAQELBQADggEBAHKlvzObJBxxgyLaUNCEFf37mNFsUtXmaWvkmcfIt9V+TZ7Q\n"
  "mtvjx5bsd5lqAflp/eqk4+JYpnYcKWrZfM/vMdxPQTeh/VQWewY/hYn6X/V1s2JI\n"
  "MtjqEkW4aotVdWjHVvsx4rAjz5vtub/wVYgtrU8jusH3TVpT9/0AoFhKE5m2IS7M\n"
  "Ig7wKR+DDxoNj4fFFluxteVNgbtwuJcb23NkBQqfHXCvQWqxXZZA4Nwl/WoGPoGG\n"
  "dW5qVOc3BlhtITW53ASyhvKC7HArhj7LwQH8C/dRgn1agIHP9vVJ1NaZnPXhK98T\n"
  "ohv++OO0E/F/bVGNWVnLBQ4v5PjQzRQUTGvM2mU=\n"
  "-----END CERTIFICATE-----\n";


// WifiManager callbacks and variables ------------------------------------------------------------------------
bool shouldSaveConfig;
String m_delay, m_wifiSSID, m_wifiPSW, m_hotspotSSID, m_hotspotPSW, m_blynkServer, m_blynkPort, m_blynkToken, m_thingChannel, m_thingApiKey;


// Crea un'istanza di Debug
Debug debug;


// Setup --------------------------------------------------------------------------------------------------------
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  debug.printspc();
  debug.println(F("METEO STATION BASIC UPLOADER!!"));
  debug.println("Firmware version: " + String(FW_VERSION));
  debug.printspc();
  initFS(true);
  if (!readNetworkConfigFile()) setNetworkConfigDefaults();
  wifiConnect(true);
  checkNtpClock();
  checkupdate();
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
  debug.println("Try to initialize the SPI file system...");

  // Try to initialize the SPI file system
  if (!SPIFFS.begin()) {
    debug.println("SPIFFS initialization failed.");
    return false;
  }
  debug.println("SPIFFS initialization success.");

  // Cerca il file di configurazione
  debug.printf("Search for %s file\n", NETWORK_CONFIG_FILE);

  if (!SPIFFS.exists(NETWORK_CONFIG_FILE)) {
    debug.printf("%s file not found.\n", NETWORK_CONFIG_FILE);

    if (formatFS) {  // No config file present -> format the SPI file system
      debug.println("SPIFFS format...");
      if (!SPIFFS.format()) {
        debug.println("SPIFFS format error.");
        return false;
      } else {
        debug.println("SPIFFS format success.");
      }
    }

    // Create the config file
    if (!writeNetworkConfigFile(true)) {
      debug.printf("Unable to create %s file.\n", NETWORK_CONFIG_FILE);
      return false;
    } else {
      debug.printf("Create %s file.\n", NETWORK_CONFIG_FILE);
    }
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
  // read configiguration data
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
    } else if (data.startsWith(WIFI_SSID_TAG)) {
      data.replace(WIFI_SSID_TAG, "");
      m_wifiSSID = data;
    } else if (data.startsWith(WIFI_PSWD_TAG)) {
      data.replace(WIFI_PSWD_TAG, "");
      m_wifiPSW = data;
    } else if (data.startsWith(HS_SSID_TAG)) {
      data.replace(HS_SSID_TAG, "");
      m_hotspotSSID = data;
    } else if (data.startsWith(HS_PSWD_TAG)) {
      data.replace(HS_PSWD_TAG, "");
      m_hotspotPSW = data;
    } else if (data.startsWith(BLYNK_SERVER_TAG)) {
      data.replace(BLYNK_SERVER_TAG, "");
      m_blynkServer = data;
    } else if (data.startsWith(BLYNK_PORT_TAG)) {
      data.replace(BLYNK_PORT_TAG, "");
      m_blynkPort = data;
    } else if (data.startsWith(BLYNK_TOKEN_TAG)) {
      data.replace(BLYNK_TOKEN_TAG, "");
      m_blynkToken = data;
    } else if (data.startsWith(THING_CHANNEL_TAG)) {
      data.replace(THING_CHANNEL_TAG, "");
      m_thingChannel = data;
    } else if (data.startsWith(THING_APIKEY_TAG)) {
      data.replace(THING_APIKEY_TAG, "");
      m_thingApiKey = data;
    } else if (data.startsWith(DELAY_TAG)) {
      data.replace(DELAY_TAG, "");
      m_delay = data;
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
    debug.printf("%s\n", WiFi.localIP().toString().c_str());  // Print IP address
  } else {
    // Unable to connect -> launch WiFi manager
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
      debug.println("Unable to writing config file");
    } else {
      debug.println("Config file written.");
    }
  }
}


void checkupdate() {
  String newFWVersion = "0000";
  String fwURL = "https://" + String(fwServerBase) + String(fwDirBase);
  String fwVersionURL = fwURL + fwNameBase;

  debug.println("Checking for firmware updates.");
  debug.println("Firmware version URL: " + String(fwVersionURL));
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  HTTPClient https;
  if (!https.begin(client, fwVersionURL)) {
    debug.println("Error initializing HTTPS connection.");
    return;
  }
  int httpCode = https.GET();
  if (httpCode == -1) {
    debug.println("Connection error: check your network connection and URL.");
  } else if (httpCode == 200) {
    newFWVersion = https.getString();
    debug.println("Current firmware version: " + String(FW_VERSION));
    debug.println("Available firmware version: " + String(newFWVersion));
    int newVersion = newFWVersion.toInt();
    if (newVersion > FW_VERSION) {
      OTAupgrade(client, fwURL, newVersion);
    } else {
      debug.println("Already on the latest version");
    }
  } else {
    debug.println("Firmware version check failed, got HTTP response code " + String(httpCode));
  }

  https.end();
}


void OTAupgrade(WiFiClientSecure client, String URL, int Version) {
  debug.println("Preparing to update");
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  String fwImageURL = URL + String(Version) + ".bin";
  debug.println("Firmware image URL: " + String(fwImageURL));
  t_httpUpdate_return ret = httpUpdate.update(client, fwImageURL);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      debug.println("HTTP_UPDATE_FAILED Error " + String(httpUpdate.getLastError()) + ": " + httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      debug.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      debug.println("HTTP_UPDATE_OK");
      break;
  }
}


void checkNtpClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC
  debug.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600) {
    yield();
    delay(500);
    debug.printp(F("."));
    now = time(nullptr);
  }
  debug.printspc();
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  debug.println("Current time: " + String(asctime(&timeinfo)));
}
