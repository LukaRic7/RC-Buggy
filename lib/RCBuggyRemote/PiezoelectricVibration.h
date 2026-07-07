#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief 
 */
class PiezoelectricVibration {
  public:
    PiezoelectricVibration(uint8_t pin, uint16_t sampleRateMs=4)
      : pin(pin), timer((uint32_t)sampleRateMs * 1000), baseline(512), current(0),
        accumulator(0), sampleCount(0)
    {}

    /**
     * @brief Call every loop iteration.
     */
    void update() {
      if (!timer.ready()) return;

      uint16_t raw = analogRead(pin);
      uint16_t vibration = abs((int)raw - baseline);

      accumulator += vibration;
      sampleCount++;
      
      if (sampleCount >= 25) {
        current = accumulator / sampleCount;

        accumulator = 0;
        sampleCount = 0;
      }
    }

    /**
     * @brief
     */
    uint16_t getCurrent() const {
      return current;
    }
  
  private:
    uint8_t pin;
    
    TimeScheduler timer;

    uint16_t baseline;

    uint16_t current;

    uint32_t accumulator;
    uint16_t sampleCount;
};