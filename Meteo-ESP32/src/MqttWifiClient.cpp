#include "MqttWifiClient.h"
#include "deviceConfig.h"

MqttWifiClient* MqttWifiClient::instance = nullptr;

MqttWifiClient::MqttWifiClient()
    : _mqttClient(_wifiClient) {
    instance = this;    
}

void MqttWifiClient::begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId) {
    strncpy(_mqttServer, broker, sizeof(_mqttServer) - 1);
    _mqttServer[sizeof(_mqttServer) - 1] = '\0'; // Ensure null termination
    _mqttPort = port;
    strncpy(_mqttUser, user, sizeof(_mqttUser) - 1);
    _mqttUser[sizeof(_mqttUser) - 1] = '\0'; // Ensure null termination
    strncpy(_mqttPass, pass, sizeof(_mqttPass) - 1);
    _mqttPass[sizeof(_mqttPass) - 1] = '\0'; // Ensure null termination
    strncpy(_mqttClientId, clientId, sizeof(_mqttClientId) - 1);
    _mqttClientId[sizeof(_mqttClientId) - 1] = '\0'; // Ensure null termination
    SerialMon.printf("MQTT Server: %s, Port: %d, User: %s, Client ID: %s\n", 
                _mqttServer, _mqttPort, _mqttUser, _mqttClientId);
    _mqttClient.setServer(_mqttServer, _mqttPort);
    _mqttClient.setCallback(mqttCallbackStatic);
    if (WiFi.status() != WL_CONNECTED) {
        SerialMon.println("WiFi not connected, skipping MQTT connection");
        return;
    }
    connectToMqtt();
}

void MqttWifiClient::loop(const SensorData& sensorData, bool publishSensorData) {    
    if (WiFi.status() != WL_CONNECTED) {        
        return;
    }

    if (_mqttClient.connected()) {
        _mqttClient.loop();
        if (publishSensorData) {
            dataToPublish[0] = sensorData.gas1;
            dataToPublish[1] = sensorData.gas2;
            dataToPublish[2] = sensorData.gas3;
            dataToPublish[3] = sensorData.gas4;
            dataToPublish[4] = sensorData.pm25;
            dataToPublish[5] = sensorData.externalTemp;
            dataToPublish[6] = sensorData.humidity;
            dataToPublish[7] = millis() / 1000.0; // Current time in seconds
            mqttPublish(MQTT_CHANNEL_ID, dataToPublish, fieldsToPublish);
        }        
    } else {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > _reconnectInterval) {
            Serial.println("Attempting MQTT reconnect...");
            _lastReconnectAttempt = now;
            connectToMqtt();
        }
    }
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

    SerialMon.print("Connecting to MQTT");

    while (!_mqttClient.connected() && attempts < maxAttempts) {
        //String clientId = "ESPClient-" + String(random(0xffff), HEX);        
        bool connected;

        if (_mqttUser[0] != '\0' && _mqttPass[0] != '\0') {
            connected = _mqttClient.connect(_mqttClientId, _mqttUser, _mqttPass);
            SerialMon.print(" with user: ");
            SerialMon.print(_mqttUser);
        } else {
            connected = _mqttClient.connect(_mqttClientId);
        }

        if (connected) {
            SerialMon.println("\nConnected to MQTT broker");
			if(mqttSubscribe(MQTT_CHANNEL_ID,ledFieldNum,SUBSCRIBE_TO_CHANNEL )==1 ){
				SerialMon.println( " Subscribed " );
			}
            return;
        } else {
            SerialMon.print(".");
            delay(1000);
            attempts++;
        }
    }

    SerialMon.println("\nFailed to connect to MQTT broker.");
}

/*void MqttWifiClient::loadSettingsFromEEPROM() {
    int addr = MQTT_EEPROM_START;

    bool isEmpty = true;
    for (int i = 0; i < 10; ++i) {
        if (EEPROM.read(MQTT_EEPROM_START + i) != 0xFF) {
            isEmpty = false;
            break;
        }
    }

    for (int i = 0; i < sizeof(_mqttServer); i++) {
        _mqttServer[i] = EEPROM.read(addr++);
    }
    _mqttServer[sizeof(_mqttServer) - 1] = '\0';

    _mqttPort = EEPROM.read(addr++) << 8;
    _mqttPort |= EEPROM.read(addr++);

    for (int i = 0; i < sizeof(_mqttUser); i++) {
        _mqttUser[i] = EEPROM.read(addr++);
    }
    _mqttUser[sizeof(_mqttUser) - 1] = '\0';

    for (int i = 0; i < sizeof(_mqttPass); i++) {
        _mqttPass[i] = EEPROM.read(addr++);
    }
    _mqttPass[sizeof(_mqttPass) - 1] = '\0';

    for (int i = 0; i < sizeof(_mqttClientId); i++) {
        _mqttClientId[i] = EEPROM.read(addr++);
    }
    _mqttClientId[sizeof(_mqttClientId) - 1] = '\0';

    if (isEmpty || _mqttServer[0] == '\0' || _mqttPort == 0 || _mqttUser[0] == '\0' || _mqttPass[0] == '\0') {
        SerialMon.println("EEPROM is empty, loading default MQTT config");

        strncpy(_mqttServer, MQTT_BROKER, sizeof(_mqttServer));
        _mqttPort = MQTT_PORT;
        strncpy(_mqttUser, MQTT_USERNAME, sizeof(_mqttUser));
        strncpy(_mqttPass, MQTT_PASSWORD, sizeof(_mqttPass));
        strncpy(_mqttClientId, MQTT_CLIENT_ID, sizeof(_mqttClientId));
    }
    else SerialMon.println("Loaded MQTT config from EEPROM");    
}*/

void MqttWifiClient::mqttCallbackStatic(char* topic, byte* payload, unsigned int len) {
    if (instance != nullptr) {
        instance->handleMqttMessage(topic, payload, len);
    }
}

void MqttWifiClient::handleMqttMessage(char* topic, byte* payload, unsigned int len) {
    SerialMon.print("Message arrived [");
    SerialMon.print(topic);
    SerialMon.print("]: ");
    SerialMon.write(payload, len);
    SerialMon.println();
}

void MqttWifiClient::mqttPublish(long pubChannelID, float dataArray[], int fieldArray[]) {
    int index = 0;
    String dataString = "";

    while (index < 8) {
        if (fieldArray[index] > 0) {
            dataString += "&field" + String(index + 1) + "=" + String(dataArray[index]);
        }
        index++;
    }

    String topicString = "channels/" + String(pubChannelID) + "/publish";
    SerialMon.println("Publishing to: " + topicString);
    SerialMon.println("Publishing: " + dataString);
    if(_mqttClient.publish(topicString.c_str(), dataString.c_str())) {
        SerialMon.println("Published successfully to channel " + String(pubChannelID));
    } else {
        SerialMon.println("Failed to publish to channel " + String(pubChannelID));
    }
}

int MqttWifiClient::mqttSubscribe(long subChannelID, int field, int unsubSub) {
    String myTopic;

    if (field == 0) {
        myTopic = "channels/" + String(subChannelID) + "/subscribe";
    } else {
        myTopic = "channels/" + String(subChannelID) + "/subscribe/fields/field" + String(field);
    }

    SerialMon.println("Subscribing to: " + myTopic);
    if (unsubSub == 1) {
        return _mqttClient.unsubscribe(myTopic.c_str());
    } else {
        return _mqttClient.subscribe(myTopic.c_str(), 0);
    }
}
