#pragma once

// Select your modem:
#define TINY_GSM_MODEM_SIM800
// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
#define SerialAT Serial1
// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// set GSM PIN, if any
#define GSM_PIN ""
#define LED_PIN 39
#define MODEM_POWER_PIN 8
// GPRS
#define APN         "internet.altel.kz"
#define GPRS_USER   ""
#define GPRS_PASS   ""
// MQTT
#define MQTT_USERNAME "CjQvGwkeDxwLOC8FKCMcACA"
#define MQTT_CLIENT_ID "CjQvGwkeDxwLOC8FKCMcACA"
#define MQTT_PASSWORD "VUgJOQcuoIl+A4OQXmr4Gs01"
#define MQTT_BROKER   "mqtt3.thingspeak.com"
#define MQTT_PORT     1883
#define MQTT_CHANNEL_ID 2385945
#define SUBSCRIBE_TO_CHANNEL 0
