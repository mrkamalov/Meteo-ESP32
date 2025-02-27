/**************************************************************
 *
 * For this example, you need to install PubSubClient library:
 *   https://github.com/knolleary/pubsubclient
 *   or from http://librarymanager/all#PubSubClient
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 * For more MQTT examples, see PubSubClient library
 *
 **************************************************************
 * This example connects to HiveMQ's showcase broker.
 *
 * You can quickly test sending and receiving messages from the HiveMQ webclient
 * available at http://www.hivemq.com/demos/websocket-client/.
 *
 * Subscribe to the topic GsmClientTest/ledStatus
 * Publish "toggle" to the topic GsmClientTest/led and the LED on your board
 * should toggle and you should see a new message published to
 * GsmClientTest/ledStatus with the newest LED status.
 *
 **************************************************************/
#include <HardwareSerial.h>
#include "mqtt_secrets.h"
// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8
// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
#define SerialAT Serial1

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// MQTT details
const char* broker = "mqtt3.thingspeak.com";
char mqttUserName[] = SECRET_MQTT_USERNAME;  // Change to your MQTT device username.    
char mqttPass[] = SECRET_MQTT_PASSWORD;      // Change to your MQTT device password.
char clientID[] = SECRET_MQTT_CLIENT_ID;     // Change to your MQTT device clientID.
long channelID = 2385945;
// const char* topicLed       = "GsmClientTest/led";
// const char* topicInit      = "GsmClientTest/init";
// const char* topicLedStatus = "GsmClientTest/ledStatus";
int fieldsToPublish[8]={0,1,0,0,0,0,0,0};             // Change to allow multiple fields.
int dataToPublish[8] = {0,0,0,0,0,0,0,0}; 
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

#define LED_PIN 39
int ledStatus = LOW;
int ledFieldNum = 1;
#define SUBSCRIBE_TO_CHANNEL 0
uint32_t lastReconnectAttempt = 0;

/**
 * Publish to a channel
 *   pubChannelID - Channel to publish to.
 *   pubWriteAPIKey - Write API key for the channel to publish to.
 *   dataArray - Binary array indicating which fields to publish to, starting with field 1.
 *   fieldArray - Array of values to publish, starting with field 1.
 */

void mqttPublish(long pubChannelID, int dataArray[], int fieldArray[]) {
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
    
    Serial.println( dataString );
    
    // Create a topic string and publish data to ThingSpeak channel feed.
     String topicString ="channels/" + String( pubChannelID ) + "/publish";
    mqtt.publish( topicString.c_str(), dataString.c_str() );
    Serial.println( "Published to channel " + String( pubChannelID ) );
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  //Only proceed if incoming message's topic matches
  if (String(topic) == "channels/"+String(channelID)+"/subscribe/fields/field"+String(ledFieldNum)) {
    SerialMon.println("receive ledField topic");
    char p[len + 1];
    memcpy(p, payload, len);
    p[len] = NULL;
    ledStatus =atoi(p)>0?HIGH:LOW;
    digitalWrite(LED_PIN, ledStatus);
    dataToPublish[ledFieldNum] = ledStatus*10;
    mqttPublish(channelID, dataToPublish, fieldsToPublish);
  }
}

/**
 * Subscribe to fields of a channel.
 *   subChannelID - Channel to subscribe to.
 *   field - Field to subscribe to. Value 0 means subscribe to all fields.
 *   readKey - Read API key for the subscribe channel.
 *   unSub - Set to 1 for unsubscribe.
 */
 
int mqttSubscribe( long subChannelID, int field, int unsubSub ){
    String myTopic;
    
    // There is no field zero, so if field 0 is sent to subscribe to, then subscribe to the whole channel feed.
    if (field==0){
        myTopic="channels/"+String( subChannelID )+"/subscribe";
    }
    else{
        myTopic="channels/"+String( subChannelID )+"/subscribe/fields/field"+String( field );
    }
    
    Serial.println( "Subscribing to " +myTopic );
    Serial.println( "State= " + String( mqtt.state() ) );

    if ( unsubSub==1 ){
        return mqtt.unsubscribe(myTopic.c_str());
    }
    return mqtt.subscribe( myTopic.c_str() ,0 );
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  //boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  boolean status = mqtt.connect(clientID, mqttUserName, mqttPass);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  if(mqttSubscribe(channelID,ledFieldNum,SUBSCRIBE_TO_CHANNEL )==1 ){
    Serial.println( " Subscribed " );
  }
  return mqtt.connected();
}


void setup() {
  // Включение питания модема
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  pinMode(LED_PIN, OUTPUT);
  SerialMon.println("Wait...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  // Make sure we're still registered on the network
  if (!modem.isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected()) {
      SerialMon.println("Network re-connected");
    }

#if TINY_GSM_USE_GPRS
    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
      SerialMon.println("GPRS disconnected!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }
#endif
  }

  if (!mqtt.connected()) {
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

  mqtt.loop();
}