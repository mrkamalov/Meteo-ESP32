#pragma once

#include <Arduino.h>
#include "deviceConfig.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>

//void sim868Setup();
//void sim868Loop();
class Sim868Client {
public:
  Sim868Client(TinyGsm* modem, TinyGsmClient* client, PubSubClient*  mqtt);

  void begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId, 
            char* apn = nullptr, char* gprsUser = nullptr, char* gprsPass = nullptr);
  void loop();
  bool isModemConnected();

  static void mqttCallbackStatic(char *topic, byte *payload, unsigned int len);
private:
    void mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]);
    //void mqttCallback(char* topic, byte* payload, unsigned int len);
    int mqttSubscribe(long subChannelID, int field, int unsubSub );
    boolean mqttConnect();
    void handleMqttMessage(char *topic, byte *payload, unsigned int len);

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
    int fieldsToPublish[8]={0,1,0,0,0,0,0,0};             // Change to allow multiple fields.
    int dataToPublish[8] = {0,0,0,0,0,0,0,0}; 
};