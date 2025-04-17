#pragma once

#define TINY_GSM_MODEM_SIM800

#include <HardwareSerial.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

class Sim868Client {
public:
  Sim868Client(Stream &serialAT, Stream &serialMon, int modemPowerPin, int ledPin);

  void begin();
  void loop();
  void publishData(long channelID, int data[], int fields[]);

private:
  Stream &SerialAT;
  Stream &SerialMon;
  TinyGsm modem;
  TinyGsmClient client;
  PubSubClient mqtt;

  const int MODEM_POWER_PIN;
  const int LED_PIN;

  const char *apn = "internet.altel.kz";
  const char *gprsUser = "";
  const char *gprsPass = "";
  const char *broker = "mqtt3.thingspeak.com";

  const char *mqttUserName = "CjQvGwkeDxwLOC8FKCMcACA";
  const char *mqttPass = "CjQvGwkeDxwLOC8FKCMcACA";
  const char *clientID = "VUgJOQcuoIl+A4OQXmr4Gs01";
  long channelID = 2385945;

  int ledFieldNum = 1;
  int ledStatus = LOW;
  int fieldsToPublish[8];
  int dataToPublish[8];

  uint32_t lastReconnectAttempt = 0;

  void mqttCallback(char *topic, byte *payload, unsigned int len);
  bool mqttConnect();
  void reconnectNetwork();
  void reconnectGPRS();
};
