#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <HardwareSerial.h>
// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8

// Используем аппаратный последовательный порт Serial1 для модема
HardwareSerial SerialAT(1);

// Создаем объект модема
TinyGsm modem(SerialAT);

void setup() {
  // Настройка последовательного монитора
  Serial.begin(115200);
  delay(10);

  // Настройка модема (Serial1)
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // Включение питания модема
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(3000); // Ожидание готовности модема

  Serial.println("Инициализация модема...");

  // Инициализация модема
  if (!modem.restart()) {
    Serial.println("Не удалось инициализировать модем!");
    while (true) {
      delay(1000);
    }
  }
  
  // Проверка SIM-карты
  SimStatus simStatus = modem.getSimStatus();
  Serial.print("Статус SIM-карты: ");
  Serial.println(simStatus);

  // Получение уровня сигнала
  int signalQuality = modem.getSignalQuality();
  Serial.print("Уровень сигнала: ");
  Serial.println(signalQuality);

  // Подключение к GPRS (опционально)
  const char apn[] = "internet.altel.kz"; // Укажите APN оператора
  const char gprsUser[] = "";    // Имя пользователя (если требуется)
  const char gprsPass[] = "";    // Пароль (если требуется)

  if (modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("GPRS подключен.");
  } else {
    Serial.println("Ошибка подключения к GPRS.");
  }
}

void loop() {
  // Здесь можно добавить другие задачи
}

// Функция отправки SMS
void sendSMS(const char *number, const char *message) {
  Serial.print("Отправка SMS на номер ");
  Serial.println(number);

  if (modem.sendSMS(number, message)) {
    Serial.println("SMS успешно отправлено.");
  } else {
    Serial.println("Ошибка отправки SMS.");
  }
}
