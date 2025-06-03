#include "Sim868Client.h"
#include <HardwareSerial.h>

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
    
    //updateRSSIValue();  // Make sure the stored value is updated.
    
    // 
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
    _mqttClient->publish( topicString.c_str(), dataString.c_str() );
    SerialMon.println( "Published to channel " + String( pubChannelID ) );
}

void Sim868Client::handleMqttMessage(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  //Only proceed if incoming message's topic matches
  if (String(topic) == "channels/"+String(MQTT_CHANNEL_ID)+"/subscribe/fields/field"+String(ledFieldNum)) {
    SerialMon.println("receive ledField topic");
    char p[len + 1];
    memcpy(p, payload, len);
    p[len] = NULL;
    ledStatus =atoi(p)>0?HIGH:LOW;
    digitalWrite(LED_PIN, ledStatus);
    dataToPublish[ledFieldNum] = ledStatus*10;
    mqttPublish(MQTT_CHANNEL_ID, dataToPublish, fieldsToPublish);
  }
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


void Sim868Client::begin(char* broker, uint16_t& port, char* user, char* pass, char* clientId) {
  // Включение питания модема
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);  
  delay(10);
  
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

  pinMode(LED_PIN, OUTPUT);
  SerialMon.println("Wait...");
  delay(6000);

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
  if (!_gsmModem->waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (_gsmModem->isNetworkConnected()) { SerialMon.println("Network connected"); }

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(APN);
  if (!_gsmModem->gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (_gsmModem->isGprsConnected()) { SerialMon.println("GPRS connected"); }

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

void Sim868Client::loop() {
  // Make sure we're still registered on the network
  if (!_gsmModem->isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (!_gsmModem->waitForNetwork(180000L, true)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (_gsmModem->isNetworkConnected()) {
      SerialMon.println("Network re-connected");
    }

    // and make sure GPRS/EPS is still connected
    if (!_gsmModem->isGprsConnected()) {
      SerialMon.println("GPRS disconnected!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(APN);
      if (!_gsmModem->gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (_gsmModem->isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }
  }

  if (!_mqttClient->connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) { lastReconnectAttempt = 0; }
    }
    delay(100);
    return;
  }

  _mqttClient->loop();
}

bool Sim868Client::isModemConnected() {
  return _gsmModem->isNetworkConnected() && _gsmModem->isGprsConnected();
}