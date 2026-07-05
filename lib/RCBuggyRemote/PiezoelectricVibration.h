#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief 
 */
class PiezoelectricVibration {
  public:
    PiezoelectricVibration()
      : timer(1000) // Zero clue about this timing, 1kHz for 5-5ms bursts
    {}

    /**
     * @brief Call every loop iteration.
     */
    void update() {

    }
  
  private:
    TimeScheduler timer;
};