#pragma once

#include <Arduino.h>

/**
 * @brief Control packet sent from REMOTE to BUGGY.
 * 
 * @param header \c uint8_t - Used for validation / sanity check.
 * @param rightBlinker \c boolean - Right blinker state.
 * @param leftBlinker \c boolean - Left blinker state.
 * @param throttle \c uint16_t - Throttle level 0-1023.
 * @param steering \c uint16_t - Steering angle 0-1023.
 * @param uint16_t \c checksum - Integrity validation (XOR-based).
 */
struct __attribute__((packed)) ControlPacket {
  uint8_t header = 0xAB;
  
  boolean rightBlinker;
  boolean leftBlinker;
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
  ControlPacket(bool rightBlinker, bool leftBlinker, uint16_t throttle, uint16_t steering)
    : rightBlinker(rightBlinker), leftBlinker(leftBlinker),
      throttle(throttle), steering(steering)
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
 * @param kmh \c float - Kilometers/Hour of the buggy.
 * @param corner \c float - Lateral g-force experienced.
 * @param acceleration \c float - Longitutional g-force experienced.
 * @param pitch \c float - Pitch of the buggy.
 * @param roll \c float - Roll of the buggy.
 * @param wattage \c uint16_t - Current wattage used by the motor.
 * @param batteryPct \c float - Current onboard battery percentage.
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
  uint8_t frontVib;
  uint8_t backVib;

  uint16_t checksum;
};