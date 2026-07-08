#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief Class that drives the motor H-Bridge and reads the RPM from the encoder using pin
 * interrupts.
 * 
 * @param pwmFwdPin \c uint8_t - The forward PWM pin on the H-Bridge.
 * @param pwmBwdPin \c uint8_t - The backward PWM pin on the H-Bridge.
 * @param encoderAPin \c uint8_t - Channel A encoder pin.
 * @param encoderBPin \c uint8_t - Channel B encoder pin.
 */
class Motor {
  public:
    Motor(
      uint8_t pwmFwdPin, uint8_t pwmBwdPin, uint8_t encoderAPin, uint8_t encoderBPin,
      float externalGearboxRatio, float wheelRadiusCm
    )
      : pwmFwdPin(pwmFwdPin), pwmBwdPin(pwmBwdPin), encoderAPin(encoderAPin),
        encoderBPin(encoderBPin), timer(100000), rpm(0), kmh(0), encoderCount(0),
        externalSpeedUpRatio(externalGearboxRatio), wheelRadiusCm(wheelRadiusCm)
    {
      pinMode(pwmFwdPin, OUTPUT);
      pinMode(pwmBwdPin, OUTPUT);

      pinMode(encoderAPin, INPUT_PULLUP);
      pinMode(encoderBPin, INPUT_PULLUP);
    }

    /**
     * @brief Attach interrupt to encoder pin.
     */
    void begin() {
      instance = this;

      attachInterrupt(digitalPinToInterrupt(encoderAPin), encoderISR, RISING);
    }

    /**
     * @brief Call every loop iteration. Calculates RPM.
     */
    void update() {
      if (!timer.ready()) return;

      noInterrupts();

      long counts = encoderCount++;
      encoderCount = 0;

      interrupts();

      // 700 Counts/Rev pr. 100 ms
      rpm = (counts / 700.0f) * 600.0f;

      // Calculate KmH
      float wheelRPM = rpm * externalSpeedUpRatio;
      float wheelCircumference = 2.0f * PI * (wheelRadiusCm / 100.0f);
      kmh = wheelRPM * wheelCircumference * 60.0f / 1000.0f;
    }

    /**
     * @brief Converts 0-1203 from an analog read into pwm signals for the H-Bridge.
     * 
     * Assumes the center is at 512.
     * 
     * @param input \c uint16_t - Input value, expects 0-1023 range. Where 0 = Full backward
     * and 1023 = Full forwards.
     */
    void setMotorPwm(uint16_t input) {
      constexpr uint16_t CENTER   = 512;
      constexpr uint16_t DEADZONE = 10;

      int16_t pwm;

      // Respect deadzone and translate signal to PWM
      if (input > CENTER - DEADZONE && input < CENTER + DEADZONE) {
        pwm = 0;
      } else if (input > CENTER) {
        pwm = map(input, CENTER + DEADZONE, 1023, 0, 255);
      } else {
        pwm = map(input, 0, CENTER - DEADZONE, -255, 0);
      }

      // Write PWM value to pins
      if (pwm > 0) {
        analogWrite(pwmFwdPin, pwm);
        analogWrite(pwmBwdPin, 0);
      } else {
        analogWrite(pwmFwdPin, 0);
        analogWrite(pwmBwdPin, -pwm);
      }
    }

    /**
     * @brief Get the motor RPM after the internal gearing.
     * 
     * @return \c float - Read only, the motor RPM after internal gearing.
     */
    float getRPM() const {
      return rpm;
    }

    /**
     * @brief Get the speed in kilometers per hour.
     * 
     * @return \c float - Read only, the speed in kmh.
     */
    float getKMH() const {
      return kmh;
    }

  private:
    uint8_t pwmFwdPin, pwmBwdPin;
    uint8_t encoderAPin, encoderBPin;
    
    TimeScheduler timer;

    float rpm, kmh;
    volatile long encoderCount;
    
    float externalSpeedUpRatio, wheelRadiusCm;

    inline static Motor* instance = nullptr;

    /**
     * @brief Interrupt service routine function for the encoder interrupt.
     */
    static void encoderISR() {
      if (digitalRead(instance->encoderBPin)) {
        instance->encoderCount++;
      } else {
        instance->encoderCount--;
      }
    }
};