#pragma once

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "TimeScheduler.h"

/**
 * @brief Construct the LCD controller and initialize the display. Wraps the LiquidCrystal library.
 * 
 * Expects a display size of 20x4.
 * 
 * @param rsPin \c uint8_t - Register Select pin.
 * @param enPin \c uint8_t - Enable pin.
 * @param d4Pin \c uint8_t - Data pin 4
 * @param d5Pin \c uint8_t - Data pin 5.
 * @param d6Pin \c uint8_t - Data pin 6.
 * @param d7Pin \c uint8_t - Data pin 7.
 * @param updateIntervalMs \c uint16_t - Refresh interval in milliseconds for display updates.
 */
class LCD {
  public:
    LCD(
      uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin,
      uint8_t d6Pin, uint8_t d7Pin, uint16_t updateIntervalMs=1000
    )
      : lcd(rsPin, enPin, d4Pin, d5Pin, d6Pin, d7Pin), timer((uint32_t)updateIntervalMs * 1000)
    {
      lcd.begin(20, 4);
    }
    
    /**
     * @brief Call every loop iteration. Handles updating the Liquid Crystal Display.
     */
    void update() {
      if (!timer.ready()) return;

      formatTelemetry(rpm, kmh, corner, acceleration, pitch, roll, wattage, batteryPct, text);

      // Row 1
      lcd.setCursor(0, 0);
      lcd.print("RPM: ");
      lcd.print(text.rpm);
      lcd.setCursor(11, 0);
      lcd.print("KMH: ");
      lcd.print(text.kmh);

      // Row 2
      lcd.setCursor(0, 1);
      lcd.print("CNR: ");
      lcd.print(text.corner);
      lcd.setCursor(11, 1);
      lcd.print("ACC: ");
      lcd.print(text.acceleration);

      // Row 3
      lcd.setCursor(0, 2);
      lcd.print("PIT: ");
      lcd.print(text.pitch);
      lcd.setCursor(11, 2);
      lcd.print("ROL: ");
      lcd.print(text.roll);

      // Row 4
      lcd.setCursor(0, 3);
      lcd.print("WTT: ");
      lcd.print(text.wattage);
      lcd.setCursor(11, 3);
      lcd.print("BTT: ");
      lcd.print(text.battery);
    }

    /**
     * @brief Set new data that is used when updating the Liquid Crystal Display.
     * 
     * @param rpm \c uint16_t - Engine or motor RPM.
     * @param kmh \c float - Speed in kilometers per hour.
     * @param corner \c float - Cornering force or angle metric.
     * @param acceleration \c float - Acceleration value.
     * @param pitch \c float - Vehicle pitch angle.
     * @param roll \c float - Vehicle roll angle.
     * @param wattage \c uint16_t - Power consumption in watts.
     * @param batteryPct \c float - Battery percentage remaining.
     */
    void setData(
      uint16_t newRpm, float newKmh, float newCorner, float newAcceleration,
      float newPitch, float newRoll, uint16_t newWattage, float newBatteryPct
    ) {
      rpm = newRpm;
      kmh = newKmh;
      corner = newCorner;
      acceleration = newAcceleration;
      pitch = newPitch;
      roll = newRoll;
      wattage = newWattage;
      batteryPct = newBatteryPct;
    }
  
  private:
    LiquidCrystal lcd;

    TimeScheduler timer;

    uint16_t rpm;
    float kmh;
    float corner, acceleration;
    float pitch, roll;
    uint16_t wattage;
    float batteryPct;
    
    /**
     * @brief Containing preformatted telemetry strings.
     */
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

    /**
     * @brief Formats an integer with optional thousands seperator.
     * 
     * @param value \c uint16_t - Input integer value.
     * @param out \c char - Output character buffer.
     */
    void formatIntWithComma(uint16_t value, char *out) {
      if (value < 1000) {
        sprintf(out, "%u", value);
      } else {
        // Add a thousand seperator
        sprintf(out, "%u,%03u", value / 1000, value % 1000);
      }
    }

    /**
     * @brief Formats speed in km/h with adaptive decimal precision.
     * 
     * @param value \c float - Speed value in km/h.
     * @param out \c char - Output character buffer.
     */
    void formatKmh(float value, char *out) {
      value = constrain(value, 0.0f, 99.9f);
    
      int scaled = (int)(value * 100.0f + 0.5f);
    
      // Low speed, use two decimals.
      if (value < 10.0f) {
        int intPart = scaled / 100;
        int decPart = scaled % 100;
    
        sprintf(out, "%d.%02d", intPart, decPart);
      // Higher speed, use one decimal.
      } else {
        int scaled10 = (int)(value * 10.0f + 0.5f);
    
        int intPart = scaled10 / 10;
        int decPart = scaled10 % 10;
    
        sprintf(out, "%d.%d", intPart, decPart);
      }
    }

    /**
     * @brief Formats a floating-point value with optional sign and precision rules.
     * 
     * Uses dtostrf for AVR chip compatibility.
     * 
     * @param value \c float - Input float value.
     * @param out \c char - Output buffer.
     * @param forceOneDecimalUnder10 \c boolean - If true, forces 1 decimal for values under 10.
     * @param signedMode \c boolean - If true, prepends + or - sign.
     */
    void formatSmartFloat(
      float value, char *out, boolean forceOneDecimalUnder10, boolean signedMode
    ) {
      char sign = 0;

      if (signedMode) {
        // Store sign seperately and work with the absolute value
        sign = value >= 0 ? '+' : '-';
        value = fabs(value);
      }

      char buffer[10];

      // Format float to string with configurable decimal precision.
      dtostrf(value, 0, value < 10.0f && forceOneDecimalUnder10 ? 1 : 2, buffer);
      
      if (signedMode) {
        sprintf(out, "%c%s", sign, buffer);
      } else {
        sprintf(out, "%s", buffer);
      }
    }

    /**
     * @brief Formats pitch or roll angle with sign and simplified precision.
     * 
     * @param value \c float - Angle value in degrees.
     * @param out \c char - Output buffer.
     */
    void formatPitchRoll(float value, char *out) {
      char sign = value >= 0 ? '+' : '-';
      value = fabs(value);

      char buffer[10];

      // One decimal for small angles, zero for larger
      dtostrf(value, 0, value < 10.0f ? 1 : 0, buffer);

      sprintf(out, "%c%s", sign, buffer);
    }

    /**
     * @brief Converts raw telemetry values into preformatted display strings.
     * 
     * @param rpm \c uint16_t - Engine or motor RPM.
     * @param kmh \c float - Speed in kilometers per hour.
     * @param corner \c float - Cornering force or angle metric.
     * @param acceleration \c float - Acceleration value.
     * @param pitch \c float - Vehicle pitch angle.
     * @param roll \c float - Vehicle roll angle.
     * @param wattage \c uint16_t - Power consumption in watts.
     * @param batteryPct \c float - Battery percentage remaining.
     * @param text \c TelemetryText - Output struct containing formatted strings.
     */
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