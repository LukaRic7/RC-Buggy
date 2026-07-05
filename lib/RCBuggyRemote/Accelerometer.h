#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

class Accelerometer {
  public:
    Accelerometer(uint16_t sampleRateMs=10)
      : timer((uint32_t)sampleRateMs * 1000)
    {}
  
  private:
    TimeScheduler timer;
};