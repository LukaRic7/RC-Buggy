#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief Piezoelectric vibration module driver.
 * 
 * Collects samples of vibration and uses a rolling buffer to calculate the average vibration over
 * a short period of time.
 * 
 * @param pin \c uint8_t - The analog pin connected to the piezoelectric sensor.
 * @param sampleRateMs \c uint16_t - The sample rate in milliseconds. Defaults to 2 ms.
 */
class PiezoelectricVibration {
  public:
    static constexpr uint16_t BUFFER_SIZE = 1000;

    PiezoelectricVibration(uint8_t pin, uint16_t sampleRateMs=2)
      : pin(pin), timer((uint32_t)sampleRateMs * 1000), baseline(512), sum(0), index(0)
    {}

    /**
     * @brief Call every loop iteration. Appends vibration to a rolling buffer.
     */
    void update() {
      if (!timer.ready()) return;

      uint16_t raw = analogRead(pin);
      
      // Slowly follow the zero point (heat compensation)
      baseline = (baseline * 99 + raw) / 100;

      uint16_t vibration = abs((int)raw - (int)baseline);

      // Append to rolling buffer
      sum -= buffer[index];
      buffer[index] = vibration;
      sum += vibration;
      
      index++;
      if (index >= BUFFER_SIZE) {
        index = 0;
      }
    }

    /**
     * @brief Get the current rolling average vibration value.
     * 
     * @return \c uint16_t - Read only, the rolling average vibration value. Default buffer is 1000.
     */
    uint16_t getAverage() const {
      return sum / BUFFER_SIZE;
    }
  
  private:
    uint8_t pin;
    
    TimeScheduler timer;

    uint16_t buffer[BUFFER_SIZE];

    uint16_t baseline;

    uint32_t sum;
    uint16_t index;
};