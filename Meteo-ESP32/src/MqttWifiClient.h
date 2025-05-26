#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>

class MqttWifiClient {
public:
    MqttWifiClient(const char* ssid, const char* password);

    void begin();
    void loop();
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool isConnected();

    void mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]);
    int mqttSubscribe(long subChannelID, int field = 0, int unsubSub = 0);

    static void mqttCallbackStatic(char* topic, byte* payload, unsigned int len);
    void handleMqttMessage(char* topic, byte* payload, unsigned int len);

private:
    void connectToMqtt();
    void loadSettingsFromEEPROM();

    const char* _ssid;
    const char* _password;

    char _mqttServer[64] = {0};
    uint16_t _mqttPort = 1883;
    char _mqttUser[32] = {0};
    char _mqttPass[32] = {0};

    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    static MqttWifiClient* instance;

public:
    int ledFieldNum = 1;
    int MQTT_CHANNEL_ID = 123456; // заменить на свой ID
    int dataToPublish[8] = {0};
    int fieldsToPublish[8] = {0}; // 1 - публикуем, 0 - пропускаем
};
