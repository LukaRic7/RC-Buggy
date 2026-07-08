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
 * Defaults to 1000 ms.
 */
class LCD {
  public:
    LCD(
      uint8_t rsPin, uint8_t enPin, uint8_t d4Pin, uint8_t d5Pin, uint8_t d6Pin, uint8_t d7Pin,
      uint16_t updateIntervalMs=1000
    )
      : lcd(rsPin, enPin, d4Pin, d5Pin, d6Pin, d7Pin), timer((uint32_t)updateIntervalMs * 1000)
    {
      lcd.begin(20, 4);
      loadCustomChars();
    }

    /**
     * @brief Call every loop iteration. Handles updating the Liquid Crystal Display.
     */
    void update() {
      if (!timer.ready()) return;

      formatTelemetry(
        rpm, kmh, temperature, corner, acceleration, pitch, roll, wattage, batteryPct,
        frontVib, backVib, text
      );

      lcd.clear();

      // Row 1
      lcd.setCursor(0, 0);
      lcd.write(0x2A);
      lcd.print(text.rpm);
      lcd.setCursor(7, 0);
      lcd.print(text.kmh);
      lcd.print("kmh");
      lcd.setCursor(15, 0);
      lcd.write(0xDF);
      lcd.print(text.temperature);

      // Row 2
      lcd.setCursor(0, 1);
      lcd.write((byte)0);
      lcd.print(text.corner);
      lcd.setCursor(6, 1);
      lcd.write((byte)1);
      lcd.print(text.acceleration);
      lcd.setCursor(13, 1);
      lcd.write((byte)4);
      lcd.print(text.frontVib);

      // Row 3
      lcd.setCursor(0, 2);
      lcd.write((byte)2);
      lcd.print(text.pitch);
      lcd.setCursor(6, 2);
      lcd.write((byte)3);
      lcd.print(text.roll);
      lcd.setCursor(13, 2);
      lcd.write((byte)5);
      lcd.print(text.backVib);

      // Row 4
      lcd.setCursor(0, 3);
      lcd.write((byte)6);
      lcd.print(text.wattage);
      lcd.print("W");
      lcd.setCursor(14, 3);
      lcd.write((byte)7);
      lcd.print(text.battery);
      lcd.print("%");
    }

    /**
     * @brief Set new data that is used when updating the Liquid Crystal Display.
     *
     * @param newRpm \c uint16_t - Engine or motor RPM.
     * @param newKmh \c float - Speed in kilometers per hour.
     * @param newTemperature \c float - Gearbox temperature in celcius.
     * @param newCorner \c float - Cornering force or angle metric.
     * @param newAcceleration \c float - Acceleration value.
     * @param newPitch \c float - Vehicle pitch angle.
     * @param newRoll \c float - Vehicle roll angle.
     * @param newWattage \c uint16_t - Power consumption in watts.
     * @param newBatteryPct \c float - Battery percentage remaining.
     * @param newFrontVib \c uint16_t - Front vibration.
     * @param newBackVib \c uint16_t - Back vibration.
     */
    void setData(
        uint16_t newRpm, float newKmh, float newTemperature, float newCorner,
        float newAcceleration, float newPitch, float newRoll, uint16_t newWattage,
        float newBatteryPct, uint16_t newFrontVib, uint16_t newBackVib
    ) {
      rpm          = newRpm;
      kmh          = newKmh;
      temperature  = newTemperature;
      corner       = newCorner;
      acceleration = newAcceleration;
      pitch        = newPitch;
      roll         = newRoll;
      wattage      = newWattage;
      batteryPct   = newBatteryPct;
      frontVib     = newFrontVib;
      backVib      = newBackVib;
    }

  private:
    LiquidCrystal lcd;

    TimeScheduler timer;
    
    uint16_t rpm;
    float kmh;
    float temperature;
    float corner, acceleration;
    float pitch, roll;
    float wattage;
    float batteryPct;
    uint16_t frontVib;
    uint16_t backVib;

    /**
     * @brief Containing preformatted telemetry strings.
     */
    struct TelemetryText
    {
      char rpm[8];
      char kmh[8];
      char temperature[8];
      char corner[8];
      char acceleration[8];
      char pitch[8];
      char roll[8];
      char wattage[8];
      char battery[8];
      char frontVib[4];
      char backVib[4];
    };

    /**
     * @brief Loads all the custom characters.
     */
    void loadCustomChars() {
      // Create a bitmap off all the custom characters
      static const byte bitmaps[8][8] = {
        { // CORNER_G
          0b00010,
          0b00110,
          0b01110,
          0b11110,
          0b01110,
          0b00110,
          0b00010,
          0b00000
        }, { // STRAIGHT_G
          0b00100,
          0b01110,
          0b10101,
          0b00100,
          0b00100,
          0b00100,
          0b00100,
          0b00100
        }, { // PITCH
          0b00100,
          0b01110,
          0b10101,
          0b00100,
          0b00100,
          0b10101,
          0b01110,
          0b00100
        }, { // ROLL
          0b00100,
          0b01000,
          0b10000,
          0b01100,
          0b00110,
          0b00001,
          0b00010,
          0b00100
        }, { // FRONT_VIBRATION
          0b10001,
          0b11111,
          0b10001,
          0b00000,
          0b00000,
          0b00000,
          0b00000,
          0b00000
        }, { // BACK_VIBRATION
          0b00000,
          0b00000,
          0b00000,
          0b00000,
          0b00000,
          0b10001,
          0b11111,
          0b10001
        }, { // WATTAGE
          0b00011,
          0b00110,
          0b01100,
          0b11111,
          0b00110,
          0b01100,
          0b11000,
          0b00000
        }, { // BATTERY
          0b01110,
          0b11111,
          0b10001,
          0b10101,
          0b10101,
          0b10101,
          0b10001,
          0b11111
        }
      };

      // Load the custom characters into the LCD
      for (uint8_t i = 0; i < 8; i++) {
        lcd.createChar(i, const_cast<uint8_t*>(bitmaps[i]));
      }
    }

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
        sign  = value >= 0 ? '+' : '-';
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
      value     = fabs(value);

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
     * @param frontVib \c uint16_t - Front vibration.
     * @param backVib \c uint16_t - Back vibration.
     * @param text \c TelemetryText - Output struct containing formatted strings.
     */
    void formatTelemetry(
      uint16_t rpm, float kmh, float temperature, float corner, float acceleration,
      float pitch, float roll, float wattage, float batteryPct, uint16_t frontVib,
      uint16_t backVib, TelemetryText &text
    ) {
      formatIntWithComma(rpm, text.rpm);
      formatKmh(kmh, text.kmh);
      formatSmartFloat(temperature, text.temperature, true, false);
      formatSmartFloat(corner, text.corner, false, false);
      formatSmartFloat(acceleration, text.acceleration, false, false);
      formatPitchRoll(pitch, text.pitch);
      formatPitchRoll(roll, text.roll);
      dtostrf(wattage, 0, 1, text.wattage);
      dtostrf(batteryPct, 0, 1, text.battery);
      sprintf(text.frontVib, "%u", frontVib);
      sprintf(text.backVib, "%u", backVib);
    }
};