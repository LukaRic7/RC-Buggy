#pragma once

#include <Arduino.h>
#include <Servo.h>
#include "TimeScheduler.h"

/**
 * @brief Class that wraps the servo library and uses non-blocking time scheduling.
 * 
 * Converts a direct analog read value into a target angle that respects max steering angles, then
 * converts that to a pulse widths to drive the servo.
 * 
 * @param pin \c uint8_t - The pin the servo is connected to.
 * @param maxSteerAngle \c uint8_t - The max steering angle to either direction. Defaults to 45 deg.
 */
class ServoSteering {
  public:
    static constexpr uint8_t DEADZONE = 15;

    ServoSteering(uint8_t pin, uint8_t maxSteerAngle=45)
      : pin(pin), maxSteerAngle(maxSteerAngle), timer(10000)
    {}

    /**
     * @brief Attach servo pin to the servo driver.
     */
    void begin() {
      servo.attach(pin);
      servo.writeMicroseconds(1500); // Point the servo straight ahead
    }

    /**
     * @brief Call every loop iteration. Updates the servo angle.
     */
    void update() {
      if (!timer.ready()) return;

      uint16_t input = rawValue;
      if (abs(rawValue - 512) < DEADZONE) {
        input = 512;
      }

      int16_t angleOffset = map(input, 0, 1023, -maxSteerAngle, maxSteerAngle);
      int16_t angle = 90 + angleOffset; // Center is 90 degrees

      uint16_t pulse = map(angle, 0, 180, 1000, 2000);
      servo.writeMicroseconds(pulse);
    }

    /**
     * @brief Converts a value between 0-1023 to a servo angle, with 512 being center.
     * 
     * @param value \c uint16_t - Analog read value between 0 and 1023.
     */
    void setServoAngle(uint16_t value) {
      rawValue = constrain(value, 0, 1023);
    }
  
  private:
    uint8_t pin;
    uint8_t maxSteerAngle;
    uint16_t rawValue;

    TimeScheduler timer;

    Servo servo;
};