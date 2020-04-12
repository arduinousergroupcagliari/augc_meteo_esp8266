#pragma once
#ifndef CStation_H
#define CStation_H

#define USE_DEBUG

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// #define FIRMWARE_VERSION    "1.0.0" // firmware version
#define NETWORK_CFG_FILE_VERSION "2.1.0" // network config file version

//  enable hotspot password
#define ENABLE_HOTSPOT_PSW 0 // 0 -> password disabled
// 1 -> password enabled


class CStation
{
  public:
    CStation();
    CStation(bool formatFS);
    ~CStation();
    void          startHotspot(void);
    bool          wifiConnect(bool autoStartHotspot = true);
    IPAddress     getBlynkIP(void);
    String        getBlynkServer(void);
    unsigned int  getBlynkPort(void);
    String        getBlynkToken(void);
    unsigned long getThingChannel(void);
    String        getThingApiKey(void);
    bool          isBlynkKnownByIP(void);
    unsigned int  getBatteryVoltage(void);
    bool          writeStationConfigFile(bool useDefaults = false);
    unsigned int  getDelay(void);

  private:
    String   m_wifiSSID,
             m_wifiPSW,
             m_hotspotSSID,
             m_hotspotPSW,
             m_blynkServer,
             m_blynkPort,
             m_blynkToken,
             m_thingChannel,
             m_thingApiKey,
             m_delay;

    bool initFS(bool formatFS = false);
    bool writeNetworkConfigFile(bool useDefaults = false);
    bool readNetworkConfigFile(void);
    bool readStationConfigFile(void);
    void setStationConfigDefaults(void);
    void setNetworkConfigDefaults(void);
    bool updateNetworkFile;
};

#endif // !CStation
