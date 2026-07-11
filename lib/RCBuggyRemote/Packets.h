#pragma once

#include <Arduino.h>

/**
 * @brief Control packet sent from REMOTE to BUGGY.
 * 
 * @param header \c uint8_t - Used for validation / sanity check.
 * @param hazardsOn \c boolean - Hazard lights state.
 * @param headlightsOn \c boolean - Headlights state.
 * @param throttle \c uint16_t - Throttle level 0-1023.
 * @param steering \c uint16_t - Steering angle 0-1023.
 * @param uint16_t \c checksum - Integrity validation (XOR-based).
 */
struct __attribute__((packed)) ControlPacket {
  uint8_t header = 0xAB;
  
  boolean hazardsOn;
  boolean headlightsOn;
  uint16_t throttle;
  uint16_t steering;

  uint16_t checksum;

  ControlPacket() = default;

  /**
   * @brief Construct a valid control packet and compute checksum.
   * 
   * @param rightBlinker \c boolean - Right blinker state.
   * @param leftBlinker \c boolean - Left blinker state.
   * @param throttle \c uint16_t - Throttle level 0-1023.
   * @param steering \c uint16_t - Steering angle 0-1023.
   */
  ControlPacket(boolean hazardsOn, boolean headlightsOn, uint16_t throttle, uint16_t steering)
    : hazardsOn(hazardsOn), headlightsOn(headlightsOn), throttle(throttle), steering(steering)
  {
    checksum = throttle ^ steering;
  }
};

/**
 * @brief Telemetry packet sent from BUGGY to REMOTE via ACK payload.
 * 
 * This is not sent explicitly using radio.write(). Instead it is attacked to the ACK response
 * of a received control packet.
 * 
 * @param header \c uint8_t - Used for validation / sanity check.
 * @param rpm \c uint16_t - Rotations/Minute of the motor.
 * @param kmh \c uint16_t - Kilometers/Hour of the buggy. Multiply by 100.
 * @param temperature \c int16_t - The gearbox temperature. Multiply by 10.
 * @param corner \c int16_t - Lateral g-force experienced. Multiply by 100.
 * @param acceleration \c int16_t - Longitutional g-force experienced. Multiply by 100.
 * @param pitch \c int16_t - Pitch of the buggy. Multiply by 10.
 * @param roll \c int16_t - Roll of the buggy. Multiply by 10.
 * @param wattage \c uint16_t - Current wattage used by the motor. Multiply by 10.
 * @param batteryPct \c uint16_t - Current onboard battery percentage. Multiply by 10.
 * @param frontVib \c uint16_t - Front vibration units.
 * @param backVib \c uint16_t - Back vibration units.
 * @param uint16_t \c checksum - Integrity validation (XOR-based).
 */
struct __attribute((packed)) TelemetryPacket {
  uint8_t header = 0xBA;

  uint16_t rpm;
  uint16_t kmh;          // km/h * 100
  int16_t temperature;   // °C * 10
  int16_t corner;        // m/s² * 100
  int16_t acceleration;  // m/s² * 100
  int16_t pitch;         // degrees * 10
  int16_t roll;          // degrees * 10
  uint16_t wattage;      // Watts * 10
  uint16_t batteryPct;   // percentage * 10
  uint16_t frontVib;
  uint16_t backVib;

  uint16_t checksum;
};

/**
 * @brief Used for serial data logging, combines telemetry and controls into one struct.
 * 
 * @param telemetry \c TelemetryPacket - The telemetry packet.
 * @param throttle \c uint16_t - The throttle value 0-1023.
 * @param steering \c uint16_t - The steering value 0-1023.
 * @param hazards \c uint8_t - Boolean value, true if the hazards are on.
 * @param headlights \c uint8_t - Boolean value, true if the headlights are on.
 */
struct SerialPacket {
  TelemetryPacket telemetry;

  uint16_t throttle;
  uint16_t steering;
  uint8_t hazards;
  uint8_t headlights;
};