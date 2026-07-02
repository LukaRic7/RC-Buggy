#pragma once

#include <Arduino.h>

/**
 * @brief Non-blocking time scheduler. Uses microseconds as primary unit.
 * 
 * @param intervalUs \c uint32_t - Interval between ready signals in microseconds.
 */
class TimeScheduler {
  public:
    TimeScheduler(uint32_t intervalUs) : intervalUs(intervalUs), lastReadyUs(0) {}

    /**
     * @brief Call every loop iteration. Handles checking if timer is ready.
     * 
     * @return \c boolean
     */
    boolean ready() {
      uint32_t nowUs = micros();

      // Check if enough time have passed by
      if ((uint32_t)(nowUs - lastReadyUs) >= intervalUs) {
        lastReadyUs = nowUs;
        return true;
      }

      return false;
    }

    /**
     * @brief Reset the timer, setting the last ready state to now.
     */
    void reset() {
      lastReadyUs = micros();
    }

    /**
     * @brief Set the timer interval. Does not reset after setting new interval.
     * 
     * @param newIntervalUs \c uint32_t - The new interval in microseconds.
     */
    void setInterval(uint32_t newIntervalUs) {
      intervalUs = newIntervalUs;
    }

    /**
     * @brief Get the timer interval in microseconds.
     * 
     * @return \c uint32_t - Read only, the timer interval in microseconds.
     */
    uint32_t getInterval() const {
      return intervalUs;
    }

  private:
    uint32_t intervalUs;
    uint32_t lastReadyUs;
};