/**
 * @author  Luka Jacobsen
 * @brief   Source code for the buggy microcontroller responsible for driving the RC buggy.
 * Designed for ATmega328p Arduino Nano v3.0.
 * @date    2026-07-02
 * @details This file handles the controlling of the RC buggy. Specifically driving the DC motor,
 * sending metrics data, driving the steering servo, amongst other smaller things like driving the
 * telemetry gathering inter-intergrated circuits.
 * 
 * @note    The pricing of the parts, excluding the chassis/framing, casing and battery comes to a
 * cost of around 670 DKK,-
 */

// ============================================================================================== //
// INCLUDE LIBRARIES                                                                              //
// ============================================================================================== //

// Core Arduino operations
#include <Arduino.h>

// Custom libraries
#include "TimeScheduler.h"
#include "Accelerometer.h"
#include "NTCTermistor.h"
#include "Transceiver.h"
#include "Packets.h"

// ============================================================================================== //
// CONFIGURATION                                                                                  //
// ============================================================================================== //

// Debugging
constexpr boolean SERIAL_DEBUG_MODE = true;

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

// Accelerometer
constexpr uint8_t ACCEL_SCL = A5;
constexpr uint8_t ACCEL_SDA = A4;

// NTCtermistor
constexpr uint8_t NTC_TERMISTOR_PIN = A0;

// ============================================================================================== //
// LIFECYCLE                                                                                      //
// ============================================================================================== //

Transceiver transceiver(TRANS_CE_PIN, TRANS_CSN_PIN);
Accelerometer accelerometer(10);
TimeScheduler transmitTimer(1000000 / (uint32_t)TRANSMIT_HZ);
NTCTermistor ntcTermistor(NTC_TERMISTOR_PIN);

/**
 * @brief Called by system at the startup once.
 */
void setup() {
  if (SERIAL_DEBUG_MODE) {
    Serial.begin(9600);
  }
  
  transceiver.begin(transceiver.BUGGY, RADIO_ADDRESS);
  accelerometer.begin();
}

/**
 * @brief Called by system every time possible.
 */
void loop() {
  ControlPacket control;

  accelerometer.update();
  ntcTermistor.update();

  if (transceiver.receive(control)) {
    TelemetryPacket telemetry;
    telemetry.rpm = 0;
    telemetry.kmh = 0.0f;
    telemetry.corner = accelerometer.getLatAccel();
    telemetry.acceleration = accelerometer.getLongAccel();;
    telemetry.pitch = accelerometer.getPitch();
    telemetry.roll = accelerometer.getRoll();
    telemetry.wattage = 0;
    telemetry.batteryPct = 0.0f;
  
    telemetry.checksum = telemetry.rpm ^ telemetry.wattage;
  
    transceiver.sendTelemetry(telemetry);
  }
}