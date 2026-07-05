#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief Driver for a current sensor.
 * 
 * @param pin \c uint8_t - Current sensor output pin (must be analog).
 * @param sensitivitymVprA \c float - Sensitivity in millivolts per amps. Defaults to 0.066mV/A
 * @param offsetV \c float - Offset in volts, where current drawn is 0. Defaults to 2.5V
 * @param sampleRateMs \c uint16_t - The sample rate in milliseconds. Defaults to 20ms
 */
class CurrentSensor {
  public:
    CurrentSensor(
      uint8_t pin, float sensitivitymVprA=0.066f, float offsetV=2.5, uint16_t sampleRateMs=20
    )
      : pin(pin), sensitivity(sensitivitymVprA), offset(offsetV),
        timer((uint32_t)sampleRateMs * 1000)
    {}
  
    /**
     * @brief Call every loop iteration. Reads the pin value and calculates the current drawn.
     */
    void update() {
      if (!timer.ready()) return;

      uint16_t raw = analogRead(pin);

      // Convert reading to voltage, then to current drawn
      float voltage = raw * (5.0f / 1023.0f);
      current = (voltage - offset) / sensitivity;
    }

    /**
     * @brief Get the current, current being drawn.
     * 
     * @return \c float - Read only, current being drawn in amps.
     */
    float getCurrent() const {
      return current;
    }

  private:
    uint8_t pin;

    float sensitivity, offset;
    
    float current;

    TimeScheduler timer;
};