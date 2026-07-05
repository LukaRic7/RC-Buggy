#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief 
 * 
 * @param sampleRateMs \c uint16_t - 
 */
class BatteryManager {
  public:
    BatteryManager(uint16_t sampleRateMs=100)
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
    float getBatteryVoltage() const {

    }

    /**
     * @brief 
     * 
     * @param cellIndex \c uint8_t - 
     * 
     * @return \c float - 
     */
    float getCellVoltage(uint8_t cellIndex=0) const {

    }

    /**
     * @brief 
     * 
     * @return \c boolean - 
     */
    boolean isSafe() const {

    }
  
  private:
    TimeScheduler timer;
};