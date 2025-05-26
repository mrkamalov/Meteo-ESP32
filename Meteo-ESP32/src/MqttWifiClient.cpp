#include "MqttWifiClient.h"

#define EEPROM_SIZE 256
#define MQTT_EEPROM_START 122
#define LED_PIN 2

MqttWifiClient* MqttWifiClient::instance = nullptr;

MqttWifiClient::MqttWifiClient(const char* ssid, const char* password)
    : _ssid(ssid), _password(password), _mqttClient(_wifiClient) {
    instance = this;
}

void MqttWifiClient::begin() {
    EEPROM.begin(EEPROM_SIZE);
    loadSettingsFromEEPROM();    
    _mqttClient.setServer(_mqttServer, _mqttPort);
    _mqttClient.setCallback(mqttCallbackStatic);
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping MQTT connection");
        return;
    }
    connectToMqtt();
}

void MqttWifiClient::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, MQTT loop skipped");
        return;
    }

    if (!_mqttClient.connected()) {
        connectToMqtt();
    }
    _mqttClient.loop();
}

bool MqttWifiClient::publish(const char* topic, const char* payload, bool retained) {
    if (_mqttClient.connected()) {
        return _mqttClient.publish(topic, payload, retained);
    }
    return false;
}

bool MqttWifiClient::isConnected() {
    return _mqttClient.connected();
}

void MqttWifiClient::connectToMqtt() {
    const int maxAttempts = 5;
    int attempts = 0;

    Serial.print("Connecting to MQTT");

    while (!_mqttClient.connected() && attempts < maxAttempts) {
        String clientId = "ESPClient-" + String(random(0xffff), HEX);
        bool connected;

        if (_mqttUser[0] != '\0' && _mqttPass[0] != '\0') {
            connected = _mqttClient.connect(clientId.c_str(), _mqttUser, _mqttPass);
        } else {
            connected = _mqttClient.connect(clientId.c_str());
        }

        if (connected) {
            Serial.println("\nConnected to MQTT broker");
            return;
        } else {
            Serial.print(".");
            delay(1000);
            attempts++;
        }
    }

    Serial.println("\nFailed to connect to MQTT broker.");
}

void MqttWifiClient::loadSettingsFromEEPROM() {
    int addr = MQTT_EEPROM_START;

    for (int i = 0; i < sizeof(_mqttServer) - 1; i++) {
        _mqttServer[i] = EEPROM.read(addr++);
    }
    _mqttServer[sizeof(_mqttServer) - 1] = '\0';

    _mqttPort = EEPROM.read(addr++) << 8;
    _mqttPort |= EEPROM.read(addr++);

    for (int i = 0; i < sizeof(_mqttUser) - 1; i++) {
        _mqttUser[i] = EEPROM.read(addr++);
    }
    _mqttUser[sizeof(_mqttUser) - 1] = '\0';

    for (int i = 0; i < sizeof(_mqttPass) - 1; i++) {
        _mqttPass[i] = EEPROM.read(addr++);
    }
    _mqttPass[sizeof(_mqttPass) - 1] = '\0';

    Serial.println("Loaded MQTT config from EEPROM:");
    Serial.print("Server: "); Serial.println(_mqttServer);
    Serial.print("Port: "); Serial.println(_mqttPort);
    Serial.print("User: "); Serial.println(_mqttUser);
}

void MqttWifiClient::mqttCallbackStatic(char* topic, byte* payload, unsigned int len) {
    if (instance != nullptr) {
        instance->handleMqttMessage(topic, payload, len);
    }
}

void MqttWifiClient::handleMqttMessage(char* topic, byte* payload, unsigned int len) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.write(payload, len);
    Serial.println();

    String expectedTopic = "channels/" + String(MQTT_CHANNEL_ID) + "/subscribe/fields/field" + String(ledFieldNum);
    if (String(topic) == expectedTopic) {
        Serial.println("Received LED control message");

        char p[len + 1];
        memcpy(p, payload, len);
        p[len] = '\0';

        int ledStatus = atoi(p) > 0 ? HIGH : LOW;
        digitalWrite(LED_PIN, ledStatus);
        dataToPublish[ledFieldNum - 1] = ledStatus * 10;

        mqttPublish(MQTT_CHANNEL_ID, dataToPublish, fieldsToPublish);
    }
}

void MqttWifiClient::mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]) {
    int index = 0;
    String dataString = "";

    while (index < 8) {
        if (fieldArray[index] > 0) {
            dataString += "&field" + String(index + 1) + "=" + String(dataArray[index]);
        }
        index++;
    }

    String topicString = "channels/" + String(pubChannelID) + "/publish";
    Serial.println("Publishing: " + dataString);
    _mqttClient.publish(topicString.c_str(), dataString.c_str());
}

int MqttWifiClient::mqttSubscribe(long subChannelID, int field, int unsubSub) {
    String myTopic;

    if (field == 0) {
        myTopic = "channels/" + String(subChannelID) + "/subscribe";
    } else {
        myTopic = "channels/" + String(subChannelID) + "/subscribe/fields/field" + String(field);
    }

    Serial.println("Subscribing to: " + myTopic);
    if (unsubSub == 1) {
        return _mqttClient.unsubscribe(myTopic.c_str());
    } else {
        return _mqttClient.subscribe(myTopic.c_str(), 0);
    }
}
