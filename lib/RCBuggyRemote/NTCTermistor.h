#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief Class for reading temperature from an NTC thermistor.
 * 
 * @param pin \c uint8_t - Analog pin connected to the thermistor.
 * @param sampleRateMs \c uint16_t - Sampling rate in milliseconds. Default is 500ms.
 */
class NTCTermistor {
  public:
    NTCTermistor(uint8_t pin, uint16_t sampleRateMs=500)
      : pin(pin), timer((uint32_t)sampleRateMs * 1000), celcius(0)
    {
      pinMode(pin, INPUT);
    }
    
    /**
     * @brief Call every loop iteration.
     */
    void update() {
      if (!timer.ready()) return;

      uint16_t rawValue = analogRead(pin);
      float voltage = rawValue * 5.0f / 1023.0f;
      float resistance = (voltage / 5.0f) * 10000.0f
        / (1.0f - (voltage / 5.0f));
      
      celcius = (1.0f / ((1.0f / 298.15f) + (1.0f / 3950.0f)
        * log(resistance / 10000.0f))) - 273.15f;
    }
    
    /**
     * @brief Returns the last measured temperature in degrees Celsius.
     * 
     * @return \c float - Last measured temperature in degrees Celsius.
     */
    float getCelcius() const {
      return celcius;
    }

  private:
    uint8_t pin;
  
    TimeScheduler timer;

    float celcius;
};