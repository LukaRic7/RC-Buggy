#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "TimeScheduler.h"

/**
 * @brief 
 * 
 * @param sampleRateMs \c uint16_t - 
 */
class Accelerometer {
  public:
    Accelerometer(uint16_t sampleRateMs=10)
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
  float getPitch() const {

  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getRoll() const {

  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getLatAccel() const {

  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getLongAccel() const {

  }

  private:
    TimeScheduler timer;
};