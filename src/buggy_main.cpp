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
#include "PiezoelectricVibration.h"
#include "TimeScheduler.h"
#include "Accelerometer.h"
#include "ServoSteering.h"
#include "NTCTermistor.h"
#include "Transceiver.h"
#include "Packets.h"
//#include "Motor.h"
#include "LED.h"

// ============================================================================================== //
// CONFIGURATION                                                                                  //
// ============================================================================================== //

// Debugging
constexpr boolean SERIAL_DEBUG_MODE = true;

// Radio communication
const byte RADIO_ADDRESS[6]   = "RCBUG"; 
constexpr uint8_t TRANSMIT_HZ = 50;

// Mechanical
constexpr float GEARBOX_RATIO   = 1.0f;
constexpr float WHEEL_RADIUS_CM = 5.0f;

// Steering
constexpr uint8_t MAX_STEERING_ANGLE_DEG = 45;

// ============================================================================================== //
// PIN DEFINITIONS                                                                                //
// ============================================================================================== //

// Transceiver
constexpr uint8_t TRANS_CE_PIN   = 7;
constexpr uint8_t TRANS_CSN_PIN  = 8;
constexpr uint8_t TRANS_MOSI_PIN = 11;
constexpr uint8_t TRANS_MISO_PIN = 12;
constexpr uint8_t TRANS_SCK_PIN  = 13;

// Accelerometer
constexpr uint8_t ACCEL_SCL = A5;
constexpr uint8_t ACCEL_SDA = A4;

// NTCtermistor
constexpr uint8_t NTC_TERMISTOR_PIN = A0;

// Piezoelectric vibration
constexpr uint8_t PIEZO_VIBRATION_FRONT_PIN = A1;
constexpr uint8_t PIEZO_VIBRATION_BACK_PIN  = A2;

// Motor
constexpr uint8_t MOTOR_FORWARD_PWM_PIN       = 5;
constexpr uint8_t MOTOR_BACKWARD_PWM_PIN      = 6;
constexpr uint8_t MOTOR_ENCODER_CHANNEL_A_PIN = 2;
constexpr uint8_t MOTOR_ENCODER_CHANNEL_B_PIN = 3;

// Servo
constexpr uint8_t SERVO_PIN = 9;

// Lights
constexpr uint8_t HEADLIGHTS_PIN    = 4;
constexpr uint8_t HAZARD_LIGHTS_PIN = 10;

// ============================================================================================== //
// LIFECYCLE                                                                                      //
// ============================================================================================== //

TimeScheduler transmitTimer(1000000 / (uint32_t)TRANSMIT_HZ);

Transceiver transceiver(TRANS_CE_PIN, TRANS_CSN_PIN);

PiezoelectricVibration frontVibration(PIEZO_VIBRATION_FRONT_PIN);
PiezoelectricVibration backVibration(PIEZO_VIBRATION_BACK_PIN);

Accelerometer accelerometer(10);
NTCTermistor ntcTermistor(NTC_TERMISTOR_PIN);

ServoSteering servo(SERVO_PIN, MAX_STEERING_ANGLE_DEG);
/*
Motor motor(
  MOTOR_FORWARD_PWM_PIN, MOTOR_BACKWARD_PWM_PIN, MOTOR_ENCODER_CHANNEL_A_PIN,
  MOTOR_ENCODER_CHANNEL_B_PIN, GEARBOX_RATIO, WHEEL_RADIUS_CM
);
*/

LED headlights(HEADLIGHTS_PIN);
LED hazards(HAZARD_LIGHTS_PIN);

TelemetryPacket telemetry;

/**
 * @brief Called by system at the startup once.
 */
void setup() {
  if (SERIAL_DEBUG_MODE) {
    Serial.begin(115200);
  }

  pinMode(HEADLIGHTS_PIN, OUTPUT);
  pinMode(HAZARD_LIGHTS_PIN, OUTPUT);
  
  transceiver.begin(transceiver.BUGGY, RADIO_ADDRESS);
  accelerometer.begin();
  //motor.begin();
  servo.begin();
}

/**
 * @brief Called by system every time possible.
 */
void loop() {
  accelerometer.update();
  ntcTermistor.update();
  frontVibration.update();
  backVibration.update();
  //motor.update();
  
  telemetry.rpm          = 0; //motor.getRPM();
  telemetry.kmh          = 0; //motor.getKMH() * 100;
  telemetry.temperature  = ntcTermistor.getCelcius() * 10;
  telemetry.corner       = accelerometer.getLatAccel() * 100;
  telemetry.acceleration = accelerometer.getLongAccel() * 100;
  telemetry.pitch        = accelerometer.getPitch() * 10;
  telemetry.roll         = accelerometer.getRoll() * 10;
  telemetry.wattage      = 0 * 10;
  telemetry.batteryPct   = 0 * 10;
  telemetry.frontVib     = frontVibration.getAverage();
  telemetry.backVib      = backVibration.getAverage();

  telemetry.checksum = telemetry.rpm ^ telemetry.wattage;

  ControlPacket control;
  if (transceiver.receive(control)) {
    boolean result = transceiver.sendTelemetry(telemetry);

    // Control headlights
    if (control.headlightsOn) {
      headlights.on();
    } else {
      headlights.off();
    }

    // Control hazards
    if (control.hazardsOn) {
      hazards.blink();
    } else {
      hazards.off();
    }

    // Control motor
    //motor.setMotorPwm(control.throttle);

    // Control servo
    servo.setServoAngle(control.steering);
    servo.update();
  }

  headlights.update();
  hazards.update();
}