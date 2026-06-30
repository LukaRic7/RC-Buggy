/**
 * @author  Luka Jacobsen
 * @brief   Source code for the handheld control device for controlling the RC buggy.
 * Designed for ATmega328p Arduino Nano v3.0.
 * @date    2026-06-30
 * @details This file handles the controlling of the RC buggy. Specifically controlling the,
 * blinkers, turning direction, throttle and the displaying of various metrics the buggy is sending
 * over. The remote primarely uses a 20x4 LCD display and 2.4GHz tranceiver amongst other smaller
 * but just as important components.
 * 
 * @note    The pricing of the parts, excluding the casing comes to a cost of around 215 DKK,-
 */

// ============================================================================================== //
// INCLUDE LIBRARIES                                                                              //
// ============================================================================================== //

// Core Arduino operations
#include <Arduino.h>

// Liquid crystal display
#include <LiquidCrystal.h>

// ============================================================================================== //
// CONFIGURATION                                                                                  //
// ============================================================================================== //



// ============================================================================================== //
// PIN DEFINITIONS                                                                                //
// ============================================================================================== //

// Tranceiver
constexpr uint8_t TRANS_CE_PIN   = 9;
constexpr uint8_t TRANS_CSN_PIN  = 10;
constexpr uint8_t TRANS_MOSI_PIN = 11;
constexpr uint8_t TRANS_MISO_PIN = 12;
constexpr uint8_t TRANS_SCK_PIN  = 13;

// Liquid Crystal Display
constexpr uint8_t LCD_RS_PIN = 6;
constexpr uint8_t LCD_EN_PIN = 5;
constexpr uint8_t LCD_D4_PIN = 2;
constexpr uint8_t LCD_D5_PIN = 4;
constexpr uint8_t LCD_D6_PIN = 7;
constexpr uint8_t LCD_D7_PIN = 8;

// Switches
constexpr uint8_t L_BLINKER_SWITCH_PIN = A1;
constexpr uint8_t R_BLINKER_SWITCH_PIN = A2;

// Potentiometers
constexpr uint8_t THROTTLE_POT_PIN = A3;
constexpr uint8_t STEERING_POT_PIN = A0;

// Light Emitting Diodes
constexpr uint8_t ERROR_LED_PIN = 3;

// ============================================================================================== //
// CLASSES / STRUCTS / ENUMS                                                                      //
// ============================================================================================== //

/**
 * @brief Non-blocking time scheduler. Uses nanoseconds as primary unit.
 * 
 * @param intervalUs \c uint32_t - Interval between ready signals in nanoseconds.
 */
class TimeScheduler {
  public:
    TimeScheduler(uint32_t intervalUs) : intervalUs(intervalUs), lastReadyUs(0) {}

    /**
     * @brief Call every loop iteration. Handles checking if timer is ready.
     */
    bool ready() {
      uint32_t nowUs = micros();
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
     * @param newIntervalUs \c uint32_t - The new interval in nanoseconds.
     */
    void setInterval(uint32_t newIntervalUs) {
      intervalUs = newIntervalUs;
    }

    /**
     * @brief Get the timer interval in nanoseconds.
     * 
     * @return \c uint32_t - Read only, the timer interval in nanoseconds.
     */
    uint32_t getInterval() const {
      return intervalUs;
    }

  private:
    uint32_t intervalUs;
    uint32_t lastReadyUs;
};

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
      if (!blinking) return;
      if (!timer.ready()) return;

      uint32_t now = millis();
      uint16_t elapsed = (now - blinkStartMs) % blinkRateMs;

      float phase = (float)elapsed / (float)blinkRateMs;
      float intensity = phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
      float exponentialIntensity = intensity * intensity;
      
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
      if (blinking && blinkRateMs == rateMs) return;

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

class LCD {
  public:
    LCD(
      uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin,
      uint8_t d6Pin, uint8_t d7Pin, uint16_t updateIntervalMs=1000
    )
      : lcd(rsPin, enPin, d4Pin, d5Pin, d6Pin, d7Pin), timer(updateIntervalMs)
    {
      lcd.begin(20, 4);
    }

    void update(
      uint16_t rpm, float kmh, float corner, float acceleration,
      float pitch, float roll, uint16_t wattage, float batteryPct
    ) {
      if (!timer.ready()) return;

      formatTelemetry(rpm, kmh, corner, acceleration, pitch, roll, wattage, batteryPct, text);

      lcd.setCursor(0, 0);
      lcd.print("RPM: ");
      lcd.print(text.rpm);
      lcd.setCursor(11, 0);
      lcd.print("KMH: ");
      lcd.print(text.kmh);

      lcd.setCursor(0, 1);
      lcd.print("CNR: ");
      lcd.print(text.corner);
      lcd.setCursor(11, 1);
      lcd.print("ACC: ");
      lcd.print(text.acceleration);

      lcd.setCursor(0, 2);
      lcd.print("PIT: ");
      lcd.print(text.pitch);
      lcd.setCursor(11, 2);
      lcd.print("ROL: ");
      lcd.print(text.roll);

      lcd.setCursor(0, 3);
      lcd.print("WTT: ");
      lcd.print(text.wattage);
      lcd.setCursor(11, 3);
      lcd.print("BTT: ");
      lcd.print(text.battery);
    }
  
  private:
    LiquidCrystal lcd;

    TimeScheduler timer;
    
    struct TelemetryText {
      char rpm[8];
      char kmh[8];
      char corner[8];
      char acceleration[8];
      char pitch[8];
      char roll[8];
      char wattage[6];
      char battery[6];
    };

    TelemetryText text;

    void formatIntWithComma(uint16_t value, char *out) {
      if (value < 1000) {
        sprintf(out, "%u", value);
      } else {
        sprintf(out, "%u,%03u", value / 1000, value % 1000);
      }
    }

    void formatKmh(float value, char *out) {
      value = constrain(value, 0.0f, 99.9f);
    
      int scaled = (int)(value * 100.0f + 0.5f);
    
      if (value < 10.0f) {
        int intPart = scaled / 100;
        int decPart = scaled % 100;
    
        sprintf(out, "%d.%02d", intPart, decPart);
      } else {
        int scaled10 = (int)(value * 10.0f + 0.5f);
    
        int intPart = scaled10 / 10;
        int decPart = scaled10 % 10;
    
        sprintf(out, "%d.%d", intPart, decPart);
      }
    }

    void formatSmartFloat(float value, char *out, bool forceOneDecimalUnder10, bool signedMode) {
      char sign = 0;

      if (signedMode) {
        sign = value >= 0 ? '+' : '-';
        value = fabs(value);
      }

      char buffer[10];

      dtostrf(value, 0, value < 10.0f && forceOneDecimalUnder10 ? 1 : 2, buffer);
      
      if (signedMode) {
        sprintf(out, "%c%s", sign, buffer);
      } else {
        sprintf(out, "%s", buffer);
      }
    }

    void formatPitchRoll(float value, char *out) {
      char sign = value >= 0 ? '+' : '-';
      value = fabs(value);

      char buffer[10];

      dtostrf(value, 0, value < 10.0f ? 1 : 0, buffer);

      sprintf(out, "%c%s", sign, buffer);
    }

    void formatTelemetry(
      uint16_t rpm, float kmh, float corner, float acceleration, float pitch,
      float roll, uint16_t wattage, float batteryPct, TelemetryText &text
    ) {
      formatIntWithComma(rpm, text.rpm);
      formatKmh(kmh, text.kmh);
      formatSmartFloat(corner, text.corner, true, false);
      formatSmartFloat(acceleration, text.acceleration, true, false);
      formatPitchRoll(pitch, text.pitch);
      formatPitchRoll(roll, text.roll);
      sprintf(text.wattage, "%u", wattage);
      dtostrf(batteryPct, 0, 1, text.battery);
    }
};

class Tranceiver {

};

// ============================================================================================== //
// LIFECYCLE                                                                                      //
// ============================================================================================== //

LCD lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);
LED errorLed(ERROR_LED_PIN);

TimeScheduler timer(100000);

/**
 * @brief Called by system at the startup once.
 */
void setup() {
  Serial.begin(9600);

  pinMode(L_BLINKER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(R_BLINKER_SWITCH_PIN, INPUT_PULLUP);

  errorLed.blink();
}

/**
 * @brief Called by system every time possible.
 */
void loop() {
  uint16_t throttlePot = 1023 - analogRead(THROTTLE_POT_PIN);
  uint16_t steeringPot = analogRead(STEERING_POT_PIN);
  boolean leftBlinker = !analogRead(L_BLINKER_SWITCH_PIN);
  boolean rightBlinker = !analogRead(R_BLINKER_SWITCH_PIN);

  if (timer.ready()) {
    Serial.print("Throttle: " + String(throttlePot) + " | Steering: " + String(steeringPot));
    Serial.println(" | Left: " + String(leftBlinker) + " | Right: " + String(rightBlinker));
  }

  errorLed.update();
  lcd.update(3456, 45.34f, 2.4f, 3.2f, 12.3f, -7.8f, 78, 87.6f);
}