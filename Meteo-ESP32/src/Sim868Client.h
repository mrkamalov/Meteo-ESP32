#pragma once

#include <Arduino.h>
#include "deviceConfig.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>

struct GnssLocation {
    bool fixStatus;
    float latitude;
    float longitude;
};

class Sim868Client {
public:
  Sim868Client(TinyGsm* modem, TinyGsmClient* client, PubSubClient*  mqtt);

  void begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId, 
            char* apn = nullptr, char* gprsUser = nullptr, char* gprsPass = nullptr);
  void loop();
  bool isModemConnected();
  void transmitMqttData(const SensorData& sensorData, bool publishSensorData = false);
  void modemPowerUp(void);
  void getGNSSLocation(bool isWifiConnected);
  void gnssPowerOn(void);

  static void mqttCallbackStatic(char *topic, byte *payload, unsigned int len);
private:
    void mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]);
    //void mqttCallback(char* topic, byte* payload, unsigned int len);
    int mqttSubscribe(long subChannelID, int field, int unsubSub );
    boolean mqttConnect();
    void handleMqttMessage(char *topic, byte *payload, unsigned int len);
    void checkExternalPower(void);
    void sendPowerLossAlert(void);
    bool initGsmModem(char* apn, char* gprsUser, char* gprsPass);    
    void loadMQTTSettingsFromEEPROM(void);
    void mqttBrokerReinit(void);    
    bool waitForResponse(const String& expectedResponse, int timeout);    
    void parseGNSS(const String &response);

    static Sim868Client *instance;
    TinyGsm* _gsmModem;
    TinyGsmClient* _gsmClient;
    PubSubClient* _mqttClient;

    char _mqttServer[64] = {0};
    uint16_t _mqttPort = 1883;
    char _mqttUser[32] = {0};
    char _mqttPass[32] = {0};
    char _mqttClientId[32] = {0};
    char _apn[21] = {0}; // APN for GPRS connection
    char _gprsUser[21] = {0}; // GPRS username
    char _gprsPass[21] = {0}; // GPRS password

    int ledStatus = LOW;
    int ledFieldNum = 1;
    uint32_t lastReconnectAttempt = 0;
    uint32_t modemReconnectAttempt = 0; // Attempt to reconnect to modem
    int fieldsToPublish[8]={1,1,1,1,1,1,1,1};             // Change to allow multiple fields.
    int dataToPublish[8] = {0,0,0,0,0,0,0,0};
    bool powerLost = false;
    bool prevPowerLost = false;
    bool isGNSSPowered = false;
    GnssLocation gnssLocation = {false, 0.0f, 0.0f};  
    static const unsigned long GNSS_POLL_INTERVAL_MS = 1 * 60 * 1000; // 15 минут
    
};