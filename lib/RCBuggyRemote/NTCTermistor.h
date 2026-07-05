#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief 
 * 
 * @param sampleRateMs \c uint16_t - 
 */
class NTCTermistor {
  public:
    NTCTermistor(uint16_t sampleRateMs=500)
      : timer((uint32_t)sampleRateMs * 1000)
    {}
    
    /**
     * @brief Call every loop iteration.
     */
    void update() {

    }
    
    /**
     * @brief 
     * 
     * @return \c float - 
     */
    float getCelcius() const {
      
    }

  private:
    TimeScheduler timer;
};