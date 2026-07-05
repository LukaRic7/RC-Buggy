#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

class BatteryManager {
  public:
    BatteryManager(uint16_t sampleRateMs=100)
      : timer((uint32_t)sampleRateMs * 1000)
    {}
  
  private:
    TimeScheduler timer;
};