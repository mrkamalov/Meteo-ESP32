#include "PowerMonitor.h"
#include "deviceConfig.h"
#include "secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

WiFiClientSecure secured_client;

void PowerMonitor::handle() {
  
  unsigned long now = millis();
  if (now - lastCheckTime >= 1000) {  // раз в секунду    
    lastCheckTime = now;
    if (WiFi.status() != WL_CONNECTED) {
      SerialMon.println("Power monitor: WiFi не подключен. Отправка сообщений невозможна");
      return;
    }  
    checkExternalPower();
  }
}

void PowerMonitor::checkExternalPower() {
  int raw = analogRead(EXTERNAL_POWER_PIN);
  float voltage = (raw / 4095.0) * 3300; // мВ
  SerialMon.printf("Напряжение на IO%d: %.2f мВ\n", EXTERNAL_POWER_PIN, voltage);

  if (powerLost) {
    SerialMon.println("Время работы без питания (мс): " + String(millis() - powerLossTime));
  }
  
  prevPowerLost = powerLost;
  powerLost = voltage < POWER_LOSS_THRESHOLD;
  
  if (powerLost && !prevPowerLost) {
    SerialMon.println("⚠️ Обнаружено пропадание внешнего питания!");
    powerLossTime = millis();
    sendTelegramMessage("⚠️ Обнаружено пропадание внешнего питания устройства!");
  } else if (!powerLost && prevPowerLost) {
    SerialMon.println("✅ Внешнее питание восстановлено.");
    sendTelegramMessage("✅ Внешнее питание восстановлено.");
  }
}

void PowerMonitor::sendTelegramMessage(const String& message) {
  secured_client.setCACert(TELEGRAM_CERT);

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + telegramBotToken + "/sendMessage";

  http.begin(url);//secured_client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "chat_id=" + chatId + "&text=" + message;
  int httpResponseCode = http.POST(body);

  if (httpResponseCode > 0) {
    String response = http.getString();
    SerialMon.println("Сообщение отправлено:");
    SerialMon.println(response);
  } else {
    SerialMon.printf("Ошибка при отправке: %d\n", httpResponseCode);
  }

  http.end();
}
