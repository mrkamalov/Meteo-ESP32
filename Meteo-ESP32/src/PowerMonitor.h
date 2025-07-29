#pragma once

#include <Arduino.h>

class PowerMonitor {
public:  
  void handle();

private:
  bool powerLost = false;
  bool prevPowerLost = false;
  unsigned long powerLossTime = 0;
  unsigned long lastCheckTime = 0;

  void checkExternalPower();
  void sendTelegramMessage(const String& message);
};
