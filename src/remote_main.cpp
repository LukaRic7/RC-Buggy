/**
 * @author  Luka Jacobsen
 * @brief   Source code for the handheld control device for controlling the RC buggy.
 * Designed for ATmega328p Arduino Nano v3.0.
 * @date    2026-06-30
 * @details This file handles the controlling of the RC buggy remote. Specifically controlling the,
 * blinkers, turning direction, throttle and the displaying of various metrics the buggy is sending
 * over. The remote primarely uses a 20x4 LCD display and 2.4GHz transceiver amongst other smaller
 * but just as important components.
 * 
 * @note    The pricing of the parts, excluding the casing comes to a cost of around 215 DKK,-
 */

// ============================================================================================== //
// INCLUDE LIBRARIES                                                                              //
// ============================================================================================== //

// Core Arduino operations
#include <Arduino.h>

// Custom libraries
#include "TimeScheduler.h"
#include "Transceiver.h"
#include "Packets.h"
#include "LED.h"
#include "LCD.h"

// ============================================================================================== //
// CONFIGURATION                                                                                  //
// ============================================================================================== //

// Debugging / Logging
constexpr boolean SERIAL_DEBUG_MODE    = true;
constexpr boolean SERIAL_LOG_TELEMETRY = true;

// Radio communication
const byte RADIO_ADDRESS[6]   = "RCBUG"; 
constexpr uint8_t TRANSMIT_HZ = 50;

// ============================================================================================== //
// PIN DEFINITIONS                                                                                //
// ============================================================================================== //

// Transceiver
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
constexpr uint8_t HAZARDS_SWITCH_PIN    = A1;
constexpr uint8_t HEADLIGHTS_SWITCH_PIN = A2;

// Potentiometers
constexpr uint8_t THROTTLE_POT_PIN = A3;
constexpr uint8_t STEERING_POT_PIN = A0;

// Light Emitting Diodes
constexpr uint8_t NO_SIGNAL_LED_PIN = 3;

// ============================================================================================== //
// LIFECYCLE                                                                                      //
// ============================================================================================== //

TimeScheduler transmitTimer(1000000 / (uint32_t)TRANSMIT_HZ);

Transceiver transceiver(TRANS_CE_PIN, TRANS_CSN_PIN);

LCD lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN, 500);

LED noSignalLed(NO_SIGNAL_LED_PIN);

/**
 * @brief Called by system at the startup once.
 */
void setup() {
  if (SERIAL_DEBUG_MODE || SERIAL_LOG_TELEMETRY) {
    Serial.begin(115200);
  }

  pinMode(HEADLIGHTS_SWITCH_PIN, INPUT_PULLUP);
  pinMode(HAZARDS_SWITCH_PIN, INPUT_PULLUP);

  transceiver.begin(transceiver.REMOTE, RADIO_ADDRESS);
}

/**
 * @brief Called by system every time possible.
 */
void loop() {
  if (transmitTimer.ready()) {
    uint16_t throttlePot = 1023 - analogRead(THROTTLE_POT_PIN);
    uint16_t steeringPot = analogRead(STEERING_POT_PIN);
    boolean hazardsOn    = digitalRead(HAZARDS_SWITCH_PIN);
    boolean headlightsOn = digitalRead(HEADLIGHTS_SWITCH_PIN);

    ControlPacket control(hazardsOn, headlightsOn, throttlePot, steeringPot);
    TelemetryPacket telemetry;
    if (transceiver.send(control, telemetry)) {
      lcd.setData(
        telemetry.rpm,
        telemetry.kmh / 100.0f,
        telemetry.temperature / 10.0f,
        telemetry.corner / 100.0f,
        telemetry.acceleration / 100.0f,
        telemetry.pitch / 10.0f,
        telemetry.roll / 10.0f,
        telemetry.wattage / 10.0f,
        telemetry.batteryPct / 10.0f,
        telemetry.frontVib,
        telemetry.backVib
      );

      if (SERIAL_LOG_TELEMETRY) {
        SerialPacket serialPacket;

        serialPacket.telemetry  = telemetry;
        serialPacket.throttle   = throttlePot;
        serialPacket.steering   = steeringPot;
        serialPacket.hazards    = hazardsOn;
        serialPacket.headlights = headlightsOn;

        Serial.write((uint8_t*)&serialPacket, sizeof(serialPacket));
      }
    }

    if (!transceiver.isLinkAlive()) {
      noSignalLed.blink();
    } else {
      noSignalLed.off();
    }
  }

  noSignalLed.update();
  lcd.update();
}