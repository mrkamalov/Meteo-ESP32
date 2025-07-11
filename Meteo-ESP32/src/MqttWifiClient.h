#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "deviceConfig.h"

class MqttWifiClient {
public:
    MqttWifiClient();

    void begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId);
    void loop(const SensorData& sensorData, bool publishSensorData);
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool isConnected();

    void mqttPublish(long pubChannelID, float dataArray[], int fieldArray[]);
    int mqttSubscribe(long subChannelID, int field = 0, int unsubSub = 0);

    static void mqttCallbackStatic(char* topic, byte* payload, unsigned int len);
    void handleMqttMessage(char* topic, byte* payload, unsigned int len);

private:
    void connectToMqtt();    

    const char* _ssid;
    const char* _password;

    char _mqttServer[64] = {0};
    uint16_t _mqttPort = 1883;
    char _mqttUser[32] = {0};
    char _mqttPass[32] = {0};
    char _mqttClientId[32] = {0};

    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    static MqttWifiClient* instance;
    unsigned long _lastReconnectAttempt = 0;
    const unsigned long _reconnectInterval = 10000; // 10 секунд
    int ledStatus = LOW;
    int ledFieldNum = 1;    
    int fieldsToPublish[8]={1,1,1,1,1,1,1,1};             // Change to allow multiple fields.
    float dataToPublish[8] = {0,0,0,0,0,0,0,0};
};
