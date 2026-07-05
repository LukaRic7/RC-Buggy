#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

class NTCTermistor {
  public:
    NTCTermistor(uint16_t sampleRateMs=500)
      : timer((uint32_t)sampleRateMs * 1000)
    {}

    void update() {

    }

    int8_t getCelcius() const {
      
    }
    
  private:
    TimeScheduler timer;
};