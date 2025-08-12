#include "Sim868Client.h"
#include <HardwareSerial.h>
#include "SerialMon.h"
#include <EEPROM.h>

// MQTT details
// const char* topicLed       = "GsmClientTest/led";
// const char* topicInit      = "GsmClientTest/init";
// const char* topicLedStatus = "GsmClientTest/ledStatus";

Sim868Client* Sim868Client::instance = nullptr;

Sim868Client::Sim868Client(TinyGsm* modem, TinyGsmClient* client, PubSubClient*  mqtt)
  : _gsmModem(modem), _gsmClient(client), _mqttClient(mqtt) {
    instance = this; 
}

void Sim868Client::mqttCallbackStatic(char* topic, byte* payload, unsigned int len) {
  if (instance != nullptr) {
    instance->handleMqttMessage(topic, payload, len);
  }
}

/**
 * Publish to a channel
 *   pubChannelID - Channel to publish to.
 *   pubWriteAPIKey - Write API key for the channel to publish to.
 *   dataArray - Binary array indicating which fields to publish to, starting with field 1.
 *   fieldArray - Array of values to publish, starting with field 1.
 */

void Sim868Client::mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]) {
    int index=0;
    String dataString="";
    
    while (index<8){        
        // Look at the field array to build the posting string to send to ThingSpeak.
        if (fieldArray[ index ]>0){          
            dataString+="&field" + String( index+1 ) + "="+String( dataArray [ index ] );
        }
        index++;
    }
    
    SerialMon.println( dataString );    
    // Create a topic string and publish data to ThingSpeak channel feed.
     String topicString ="channels/" + String( pubChannelID ) + "/publish";
    if(_mqttClient->publish(topicString.c_str(), dataString.c_str())) {
        SerialMon.println("Published successfully to channel " + String(pubChannelID));
    } else {
        SerialMon.println("Failed to publish to channel " + String(pubChannelID));
    }
}

void Sim868Client::handleMqttMessage(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();  
}

/**
 * Subscribe to fields of a channel.
 *   subChannelID - Channel to subscribe to.
 *   field - Field to subscribe to. Value 0 means subscribe to all fields.
 *   readKey - Read API key for the subscribe channel.
 *   unSub - Set to 1 for unsubscribe.
 */
 
int Sim868Client::mqttSubscribe( long subChannelID, int field, int unsubSub ){
    String myTopic;
    
    // There is no field zero, so if field 0 is sent to subscribe to, then subscribe to the whole channel feed.
    if (field==0){
        myTopic="channels/"+String( subChannelID )+"/subscribe";
    }
    else{
        myTopic="channels/"+String( subChannelID )+"/subscribe/fields/field"+String( field );
    }
    
    SerialMon.println( "Subscribing to " +myTopic );
    SerialMon.println( "State= " + String(_mqttClient->state() ) );

    if ( unsubSub==1 ){
        return _mqttClient->unsubscribe(myTopic.c_str());
    }
    return _mqttClient->subscribe( myTopic.c_str() ,0 );
}

boolean Sim868Client::mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(_mqttServer);

  // Connect to MQTT Broker
  //boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  boolean status = _mqttClient->connect(_mqttClientId, _mqttUser, _mqttPass);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  if(mqttSubscribe(MQTT_CHANNEL_ID,ledFieldNum,SUBSCRIBE_TO_CHANNEL )==1 ){
    SerialMon.println( " Subscribed " );
  }
  return _mqttClient->connected();
}

void Sim868Client::modemPowerUp(void){
 // Включение питания модема
  SerialMon.println("Modem power up delay");  
  pinMode(MODEM_GSM_EN_PIN, OUTPUT);
  digitalWrite(MODEM_GSM_EN_PIN, LOW);
  delay(3200);
  digitalWrite(MODEM_GSM_EN_PIN, HIGH);
  SerialMon.println("Waiting for modem to power up...");
  delay(500);
  pinMode(MODEM_PWRKEY_PIN, OUTPUT);
  digitalWrite(MODEM_PWRKEY_PIN, HIGH);
  SerialMon.println("PWR KEY LOW");
  delay(1500);  
  digitalWrite(MODEM_PWRKEY_PIN, LOW);  // Включаем питание модема
  SerialMon.println("PWR KEY HIGH");
  SerialMon.println("Wait...");
  delay(2000);
  pinMode(MODEM_STATUS_PIN, INPUT); // Set status pin as input
  //read status pin
  if (digitalRead(MODEM_STATUS_PIN) != HIGH) delay(2000);
}

bool Sim868Client::initGsmModem(char* apn, char* gprsUser, char* gprsPass){
  modemPowerUp();

  strncpy(_apn, apn ? apn : APN_DEFAULT, sizeof(_apn) - 1);
  _apn[sizeof(_apn) - 1] = '\0'; // Ensure null termination
  strncpy(_gprsUser, gprsUser ? gprsUser : GPRS_USER_DEFAULT, sizeof(_gprsUser) - 1);
  _gprsUser[sizeof(_gprsUser) - 1] = '\0'; // Ensure null termination
  strncpy(_gprsPass, gprsPass ? gprsPass : GPRS_PASS_DEFAULT, sizeof(_gprsPass) - 1);
  _gprsPass[sizeof(_gprsPass) - 1] = '\0'; // Ensure null termination 
   
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  _gsmModem->restart();
  // modem.init();

  String modemInfo = _gsmModem->getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && _gsmModem->getSimStatus() != 3) { _gsmModem->simUnlock(GSM_PIN); }

  SerialMon.print("Waiting for network...");
  if (!_gsmModem->waitForNetwork(10000L, true)) {
    SerialMon.println(" fail");    
    return false ;
  }
  SerialMon.println(" success");

  if (_gsmModem->isNetworkConnected()) { SerialMon.println("Network connected"); }
  return true;
}

void Sim868Client::loadMQTTSettingsFromEEPROM(void){
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

    if (isEmpty || _mqttServer[0] == '\0' || _mqttPort == 0 || _mqttUser[0] == '\0' || _mqttPass[0] == '\0' 
        || _mqttClientId[0] == '\0') {
        SerialMon.println("EEPROM is empty, loading default MQTT config");

        strncpy(_mqttServer, MQTT_BROKER_DEFAULT, sizeof(_mqttServer));
        _mqttPort = MQTT_PORT_DEFAULT;
        strncpy(_mqttUser, MQTT_USERNAME_DEFAULT, sizeof(_mqttUser));
        strncpy(_mqttPass, MQTT_PASSWORD_DEFAULT, sizeof(_mqttPass));
        strncpy(_mqttClientId, MQTT_CLIENT_ID_DEFAULT, sizeof(_mqttClientId));
    }
    else SerialMon.println("Loaded MQTT config from EEPROM");
}

void Sim868Client::mqttBrokerReinit(void){
  // Load MQTT settings from EEPROM
  loadMQTTSettingsFromEEPROM();

  // Set MQTT server and port
  _mqttClient->setServer(_mqttServer, _mqttPort);
  _mqttClient->setCallback(mqttCallbackStatic);

  SerialMon.printf("MQTT Server: %s, Port: %d, User: %s, Client ID: %s\n", 
              _mqttServer, _mqttPort, _mqttUser, _mqttClientId);
  if (mqttConnect()) { lastReconnectAttempt = 0; }
  else SerialMon.println("Failed to connect to MQTT broker, will retry later."); 
}      

void Sim868Client::begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId, char* apn, char* gprsUser, char* gprsPass) {
  if(!initGsmModem(apn, gprsUser, gprsPass)) return;  

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(_apn);
  if (!_gsmModem->gprsConnect(_apn, _gprsUser, _gprsPass)) {
    SerialMon.println(" fail");    
    return;
  }
  SerialMon.println(" success");

  if (_gsmModem->isGprsConnected()) { SerialMon.println("GPRS connected"); }
  checkExternalPower();

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
  // MQTT Broker setup
  _mqttClient->setServer(_mqttServer, _mqttPort);
  _mqttClient->setCallback(mqttCallbackStatic);

  // Проверка связи
  int signalQuality = _gsmModem->getSignalQuality();
  SerialMon.print("Уровень сигнала: ");
  SerialMon.print(signalQuality);
  SerialMon.println(" (0-31, где 31 = максимальный сигнал)");

  if (signalQuality <= 5) {
    SerialMon.println("⚠️ Сигнал слабый! Проверьте антенну или покрытие.");
  } else {
    SerialMon.println("✅ Сигнал в порядке.");
  }  
}

void Sim868Client::sendPowerLossAlert() {
  if (!_gsmModem->isGprsConnected()){
    SerialMon.println("Sending power loss alert via SMS...");
    _gsmModem->sendSMS(SMS_TARGET, String("Power off"));
  }
  else
    SerialMon.println("GPRS connected, send alert to Telegram");
  
}

// === Проверка внешнего питания ===
void Sim868Client::checkExternalPower() {
  static unsigned long powerLossTime = 0;
  int raw = analogRead(EXTERNAL_POWER_PIN);
  float voltage = (raw / 4095.0) * 3300; // мВ
  SerialMon.printf("Напряжение на IO16: %.2f мВ\n", voltage);
  if(powerLost){    
    SerialMon.println("Время работы без питания (мс): " + String(millis()-powerLossTime));
  }

  prevPowerLost = powerLost;
  powerLost = voltage < POWER_LOSS_THRESHOLD;

  if (powerLost && !prevPowerLost) {
    SerialMon.println("⚠️ Обнаружено пропадание внешнего питания!");
    powerLossTime = millis(); // Запоминаем время потери питания
    sendPowerLossAlert();
  } else if (!powerLost && prevPowerLost) {
    SerialMon.println("✅ Внешнее питание восстановлено.");
  }
}

void Sim868Client::loop() {
  // Make sure we're still registered on the network
  if (!_gsmModem->isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (!_gsmModem->waitForNetwork(3000L, true)) {
      SerialMon.println(" fail");
      uint32_t t = millis();
      if (t - modemReconnectAttempt > 60000L) {
        modemReconnectAttempt = t;
        digitalWrite(MODEM_GSM_EN_PIN, LOW);
        delay(3200);
        digitalWrite(MODEM_GSM_EN_PIN, HIGH);
        delay(500);
        digitalWrite(MODEM_PWRKEY_PIN, HIGH);
        SerialMon.println("PWR KEY LOW");
        delay(1500);
        digitalWrite(MODEM_PWRKEY_PIN, LOW);  // Включаем питание модема
        SerialMon.println("PWR KEY HIGH");
        SerialMon.println("Wait...");
        delay(2000);        
        if (digitalRead(MODEM_STATUS_PIN) != HIGH) delay(2000);
        SerialMon.println("Reconnecting to network...");
        if (!_gsmModem->restart()) {
          SerialMon.println("Failed to restart modem");
          return;
        }
      }      
      else return;
    }
    if (_gsmModem->isNetworkConnected()) {
      SerialMon.println("Network re-connected");
    }

    // and make sure GPRS/EPS is still connected
    if (_gsmModem->isNetworkConnected()) {
      SerialMon.println("GPRS disconnected!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(_apn);
      if (!_gsmModem->gprsConnect(_apn, _gprsUser, _gprsPass)) {
        SerialMon.println(" fail");        
        return;
      }
      if (_gsmModem->isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }
  }

  checkExternalPower();

  if(_gsmModem->isNetworkConnected() && !_gsmModem->isGprsConnected()) {
    SerialMon.println("GPRS disconnected!");
    SerialMon.print(F("Connecting to "));
    SerialMon.print(_apn);
    if (!_gsmModem->gprsConnect(_apn, _gprsUser, _gprsPass)) {
      SerialMon.println(" fail");      
      return;
    }
    if (_gsmModem->isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
  }

  if (!_mqttClient->connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      SerialMon.println("Attempting to reconnect to MQTT broker...");
      lastReconnectAttempt = t;
      mqttBrokerReinit();
    }
    delay(100);
    return;
  }

  _mqttClient->loop();
}

bool Sim868Client::isModemConnected() {
  return _gsmModem->isNetworkConnected() && _gsmModem->isGprsConnected();
}

void Sim868Client::transmitMqttData(const SensorData& sensorData, bool publishSensorData) {
  if (!isModemConnected()) {
    SerialMon.println("Modem is not connected, cannot transmit data.");
    return;
  }

  if(!_mqttClient->connected()) {
    SerialMon.println("MQTT client is not connected, cannot transmit data.");
    return;
  }

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
}