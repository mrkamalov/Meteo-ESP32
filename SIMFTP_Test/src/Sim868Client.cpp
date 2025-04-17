#include "Sim868Client.h"

#define TINY_GSM_DEBUG SerialMon

Sim868Client::Sim868Client(Stream &serialAT, Stream &serialMon, int modemPowerPin, int ledPin)
  : SerialAT(serialAT), SerialMon(serialMon),
    MODEM_POWER_PIN(modemPowerPin), LED_PIN(ledPin),
    modem(serialAT), client(modem), mqtt(client) {

  //memset(fieldsToPublish, 0, sizeof(fieldsToPublish));
  //memset(dataToPublish, 0, sizeof(dataToPublish));
}

void Sim868Client::begin() {
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);
  
  pinMode(LED_PIN, OUTPUT);
  SerialMon.println("Wait 6 seconds for modem to power up...");
  delay(6000);
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  if (modem.getSimStatus() != 3) modem.simUnlock("");

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    return;
  }
  SerialMon.println(" success");

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println("GPRS fail");
    return;
  }
  SerialMon.println("GPRS connected");

  mqtt.setServer(broker, 1883);
  mqtt.setCallback([this](char* topic, byte* payload, unsigned int len) {
    this->mqttCallback(topic, payload, len);
  });
}

void Sim868Client::loop() {
  reconnectNetwork();
  if (!mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    return;
  }
  mqtt.loop();
}

void Sim868Client::publishData(long pubChannelID, int data[], int fields[]) {
  String dataString = "";
  for (int i = 0; i < 8; i++) {
    if (fields[i]) {
      dataString += "&field" + String(i + 1) + "=" + String(data[i]);
    }
  }
  String topic = "channels/" + String(pubChannelID) + "/publish";
  mqtt.publish(topic.c_str(), dataString.c_str());
}

void Sim868Client::mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  String expectedTopic = "channels/" + String(channelID) + "/subscribe/fields/field" + String(ledFieldNum);
  if (String(topic) == expectedTopic) {
    char p[len + 1];
    memcpy(p, payload, len);
    p[len] = '\0';
    ledStatus = atoi(p) > 0 ? HIGH : LOW;
    digitalWrite(LED_PIN, ledStatus);
    dataToPublish[ledFieldNum] = ledStatus * 10;
    publishData(channelID, dataToPublish, fieldsToPublish);
  }
}

bool Sim868Client::mqttConnect() {
  SerialMon.print("Connecting to MQTT...");
  bool status = mqtt.connect(clientID, mqttUserName, mqttPass);
  if (!status) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  String topic = "channels/" + String(channelID) + "/subscribe/fields/field" + String(ledFieldNum);
  mqtt.subscribe(topic.c_str(), 0);
  return true;
}

void Sim868Client::reconnectNetwork() {
  if (!modem.isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (modem.waitForNetwork(180000L, true)) {
      SerialMon.println("Network re-connected");
    }
  }
  reconnectGPRS();
}

void Sim868Client::reconnectGPRS() {
  if (!modem.isGprsConnected()) {
    SerialMon.println("GPRS disconnected");
    if (modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println("GPRS reconnected");
    }
  }
}
