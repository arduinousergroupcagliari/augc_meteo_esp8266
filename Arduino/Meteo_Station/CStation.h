#pragma once
#ifndef CStation_H
#define CStation_H

#define USE_DEBUG

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
// #include <Arduino.h>

#//define FIRMWARE_VERSION    "1.0.0" // firmware version
#define NETWORK_CFG_FILE_VERSION "2.0.0" // network config file version

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

  private:
    String   m_wifiSSID,
             m_wifiPSW,
             m_hotspotSSID,
             m_hotspotPSW,
             m_blynkServer,
             m_blynkPort,
             m_blynkToken,
             m_thingChannel,
             m_thingApiKey;

    // The ESP8266 RTC memory is arranged into blocks of 4 bytes. The access methods read and write 4 bytes at a time,
    // so the RTC data structure should be padded to a 4-byte multiple.
    struct {
      uint32_t m_crc32;   // 4 bytes
      uint8_t m_channel;  // 1 byte,   5 in total
      uint8_t m_bssid[6]; // 6 bytes, 11 in total
      uint8_t m_padding;  // 1 byte,  12 in total
    } rtcData;

    bool initFS(bool formatFS = false);
    bool writeNetworkConfigFile(bool useDefaults = false);
    bool readNetworkConfigFile(void);
    bool readStationConfigFile(void);
    void setStationConfigDefaults(void);
    void setNetworkConfigDefaults(void);
    uint32_t calculateCRC32(const uint8_t*, size_t);
};

#endif // !CStation
