#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "TimeScheduler.h"

/**
 * @brief Class for reading and processing data from the MPU-6050 accelerometer.
 * 
 * @param sampleRateMs \c uint16_t - The sample rate in milliseconds for reading data from the
 * accelerometer.
 * @param address \c uint8_t - The I2C address of the MPU-6050.
 */
class Accelerometer {
  public:
    Accelerometer(uint16_t sampleRateMs=10, uint8_t address=0x68)
      : timer((uint32_t)sampleRateMs * 1000), address(address),
        roll(0), pitch(0), yaw(0), long_acc(0), lat_acc(0),
        dt(sampleRateMs * 1000.0f * 1e-6f)
    {}

  /**
   * @brief Initializes the I2C communication and configures the MPU-6050.
   */
  void begin() {
    Wire.begin();

    // Wake MPU-6050
    Wire.beginTransmission(address);
    Wire.write(0x6B); // Power management address
    Wire.write(0x00); // Sleep
    Wire.endTransmission(true);

    // Set accel to +/- 16 gravitational units range
    Wire.beginTransmission(address);
    Wire.write(0x1C); // Config address
    Wire.write(0x18); // Set full scale
    Wire.endTransmission(true);
  }

  /**
   * @brief Call every loop iteration. Reads the current acceleration and rotation values.
   */
  void update() {
    if (!timer.ready()) return;

    // Burst read 14 bytes
    Wire.beginTransmission(address);
    Wire.write(0x3B);
    Wire.endTransmission(false); // Dont release bus
    
    Wire.requestFrom(address, (uint8_t)14);

    // Read values
    int16_t ax = read16();
    int16_t ay = read16();
    int16_t az = read16();
    read16();
    int16_t gx = read16();
    int16_t gy = read16();
    int16_t gz = read16();

    // Convert gyro
    float gxf = gx / 131.07f;
    float gyf = gy / 131.07f;
    float gzf = gz / 131.07f;

    // Accel angles
    float axf = ax / 16384.0f;
    float ayf = ay / 16384.0f;
    float azf = az / 16384.0f;

    long_acc = axf;
    lat_acc = ayf;

    float roll_acc = atan2(ayf, azf) * RAD_TO_DEG;
    float pitch_acc = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * RAD_TO_DEG;

    // Correct Complementary Filter math
    roll = 0.98f * (roll + gxf * dt) + 0.02f * roll_acc;
    pitch = 0.98f * (pitch + gyf * dt) + 0.02f * pitch_acc;
    yaw += gzf * dt; // Drifts with no magnotometer
  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getPitch() const {
    return pitch;
  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getRoll() const {
    return roll;
  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getLatAccel() const {
    return lat_acc;
  }

  /**
   * @brief 
   * 
   * @return \c float - 
   */
  float getLongAccel() const {
    return long_acc;
  }

  private:
    TimeScheduler timer;

    uint8_t address;

    float roll, pitch, yaw;
    float long_acc, lat_acc;

    float dt;

    /**
     * @brief Internal function for reading 16 bits from wire.
     * 
     * @return int16_t. 2 Byte result from the read.
     */
    inline int16_t read16() {
      return (Wire.read() << 8) | Wire.read();
    }
};