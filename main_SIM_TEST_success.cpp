#include <HardwareSerial.h>
void test_sim800_module();
void updateSerial();
#define MODEM_POWER_PIN 8 // Пин для включения модема

void setup() {
  Serial.begin(115200);  
  Serial1.begin(9600, SERIAL_8N1, 18, 17);
  pinMode(MODEM_POWER_PIN, OUTPUT); // Устанавливаем пин управления модемом как выход
  // Включение модема
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(3000);
  test_sim800_module();
  //send_SMS();
}
void loop() {
  updateSerial();
}
void test_sim800_module()
{
  Serial1.println("AT");
  updateSerial();
  Serial.println();
  Serial1.println("AT+CSQ");
  updateSerial();
  Serial1.println("AT+CCID");
  updateSerial();
  Serial1.println("AT+CREG?");
  updateSerial();
  Serial1.println("ATI");
  updateSerial();
  Serial1.println("AT+CBC");
  updateSerial();
}
void updateSerial()
{
  delay(500);
  //while (Serial.available())
  //{
  //  Serial1.write(Serial.read());//Forward what Serial received to Software Serial Port
  //}
  while (Serial1.available())
  {
    Serial.write(Serial1.read());//Forward what Software Serial received to Serial Port
  }
}
void send_SMS()
{
  Serial1.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  Serial1.println("AT+CMGS=\"+919804049270\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  Serial1.print("Circuit Digest"); //text content
  updateSerial();
Serial.println();
  Serial.println("Message Sent");
  Serial1.write(26);
}