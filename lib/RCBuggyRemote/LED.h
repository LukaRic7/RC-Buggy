#pragma once

#include <Arduino.h>
#include "TimeScheduler.h"

/**
 * @brief Control LED using boolean output or PWM smooth blinking with exponential fading.
 * 
 * @param pin \c uint8_t - LED pin, use PWM pin for smooth blinking.
 */
class LED {
  public:
    LED(uint8_t pin)
      : pin(pin), blinking(false), state(false), blinkRateMs(1000), blinkStartMs(0), timer(10000)
    {
      pinMode(pin, OUTPUT);
      analogWrite(pin, 0);
    }
    
    /**
     * @brief Call every loop iteration. Handles exponential blinking.
     */
    void update() {
      if (!blinking) return;      // Not in blinker mode
      if (!timer.ready()) return; // Timer is not ready

      // Compute the position inside the blink cycle and normalize
      uint16_t elapsed = (millis() - blinkStartMs) % blinkRateMs;
      float phase = (float)elapsed / (float)blinkRateMs;

      // Calculate the triangle wave brightness (linear fade in/out), then square it
      float intensity = phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
      float exponentialIntensity = intensity * intensity;
      
      // Scale to a byte and write out
      analogWrite(pin, (uint8_t)(exponentialIntensity * 255.0f));
    }

    /**
     * @brief Turn on the LED at full brightness. Disabling blinking if already enabled.
     */
    void on() {
      blinking = false;
      state = true;

      analogWrite(pin, 255);
    }

    /**
     * @brief Turn off the LED. Disabling blinking if already enabled.
     */
    void off() {
      blinking = false;
      state = false;

      analogWrite(pin, 0);
    }

    /**
     * @brief Start blinking the LED at a given interval.
     * 
     * @param rateMs \c uint16_t - Blinking rate in milliseconds. Defaults to 1000ms.
     */
    void blink(uint16_t rateMs=1000) {
      if (blinking && blinkRateMs == rateMs) return; // Already in blinking mode at the same rate

      blinking = true;
      blinkRateMs = rateMs;
      blinkStartMs = millis();
      
      timer.reset();
    }

    /**
     * @brief Get the LED state.
     * 
     * 0 = Off, 1 = On, 2 = Blinking.
     * 
     * @return \c uint8_t - Read only, the state of the LED.
     */
    uint8_t getState() const {
      return blinking ? 2 : state;
    }

    /**
     * @brief Get the LED blink rate in milliseconds.
     * 
     * @return \c uint16_t - Read only, the blink rate in milliseconds.
     */
    uint16_t getBlinkRate() const {
      return blinkRateMs;
    }

  private:
    uint8_t pin;

    boolean blinking, state;
    
    uint16_t blinkRateMs;
    uint32_t blinkStartMs;
    
    TimeScheduler timer;
};