/**
 * @author  Luka Jacobsen
 * @brief   Source code for the handheld control device for controlling the RC buggy.
 * Designed for ATmega328p Arduino Nano v3.0.
 * @date    2026-06-30
 * @details This file handles the controlling of the RC buggy. Specifically controlling the,
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

// Liquid crystal display
#include <LiquidCrystal.h>

// Transceiver
#include <nRF24L01.h>
#include <RF24.h>

// Serial peripheral interface
#include <SPI.h>

// ============================================================================================== //
// CONFIGURATION                                                                                  //
// ============================================================================================== //

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
 * @brief Non-blocking time scheduler. Uses microseconds as primary unit.
 * 
 * @param intervalUs \c uint32_t - Interval between ready signals in microseconds.
 */
class TimeScheduler {
  public:
    TimeScheduler(uint32_t intervalUs) : intervalUs(intervalUs), lastReadyUs(0) {}

    /**
     * @brief Call every loop iteration. Handles checking if timer is ready.
     * 
     * @return \c boolean
     */
    boolean ready() {
      uint32_t nowUs = micros();

      // Check if enough time have passed by
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
     * @param newIntervalUs \c uint32_t - The new interval in microseconds.
     */
    void setInterval(uint32_t newIntervalUs) {
      intervalUs = newIntervalUs;
    }

    /**
     * @brief Get the timer interval in microseconds.
     * 
     * @return \c uint32_t - Read only, the timer interval in microseconds.
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

/**
 * @brief Sets up communication class for the nRF24L01 transceiver.
 * 
 * Tailored for ACK communication between remote and buggy. Sends controls, receives ACK response.
 * 
 * Requires SPI pins on the Arduino Nano 11, 12 & 13.
 * 
 * @param chipEnablePin \c uint8_t - CE pin for RF24 module.
 * @param chipSelectNotPin \c uint8_t - CSN pin for Rf24 module.
 */
class Transceiver {
  public:
    Transceiver(uint8_t chipEnablePin, uint8_t chipSelectNotPin)
      : radio(chipEnablePin, chipSelectNotPin), lastGoodLinkMs(0), linkAlive(false)
    {}
  
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
      float kmh;
      float corner;
      float acceleration;
      float pitch;
      float roll;
      uint16_t wattage;
      float batteryPct;

      uint16_t checksum;
    };

    /**
     * @brief Device role definition.
     * 
     * REMOTE:
     * - Sends ControlPacket.
     * - Receives TelemetryPacket via ACK payload.
     * 
     * BUGGY:
     * - Receives ControlPacket
     * - Sends TelemetryPacket via ACK payload.
     */
    enum Role { BUGGY, REMOTE };

    /**
     * @brief Initialize RF24 ratio and configure role-based behavior.
     * 
     * @param role \c Role - Device role (REMOTE or BUGGY).
     * @param address \c byte - 5-byte RF24 pipe address.
     * 
     * @return \c boolean - True if RF module initialized successfully.
     */
    boolean begin(Role role, const byte address[6]) {
      if (!radio.begin()) {
        return false;
      }

      this->role = role;

      radio.setPALevel(RF24_PA_MIN); // Lower power = Less noise + better stability at short range
      radio.setDataRate(RF24_1MBPS);
      radio.setAutoAck(true);
      radio.setRetries(3, 5);

      radio.openWritingPipe(address);
      radio.openReadingPipe(1, address);

      radio.startListening();

      lastGoodLinkMs = millis();
      linkAlive = true;

      return true;
    }

    /**
     * @brief Send control packet and optionally receive telemetry via ACK payload.
     * 
     * Only used on the REMOTE role.
     * 
     * @param packet \c ControlPacket - Control input data.
     * @param telemetryOut \c TelemetryPacket - Output telemetry received from the buggy.
     * 
     * @return \c boolean - True if valid telemetry was received.
     */
    boolean send(const ControlPacket& packet, TelemetryPacket& telemetryOut) {
      if (role != REMOTE) {
        return false;
      }

      // Switch to TX mode, send the packet, and return to RX mode to capture the ACK response
      radio.stopListening();
      boolean ok = radio.write(&packet, sizeof(packet));
      radio.startListening();

      // Check if the buggy attached telemetry to ACK response
      boolean gotTelemetry = false;
      if (ok && radio.isAckPayloadAvailable()) {
        radio.read(&telemetryOut, sizeof(telemetryOut));

        // Validate the packet identity
        if (telemetryOut.header == 0xBA) {
          uint16_t expected = telemetryOut.rpm ^ telemetryOut.wattage;
          gotTelemetry = telemetryOut.checksum == expected;
        }
      }

      // Update link state ONLY on successful full transaction
      if (gotTelemetry) {
        lastGoodLinkMs = millis();
        linkAlive = true;
      }

      return gotTelemetry;
    }

    /**
     * @brief Receive control packet from the remote.
     * 
     * Only used on the BUGGY.
     * 
     * @param packet \c ControlPacket - Output control packet.
     * 
     * @return \c boolean - True if a valid packet was received.
     */
    boolean receive(ControlPacket& packet) {
      if (role != BUGGY) {
        return false;
      }

      if (!radio.available()) {
        return false;
      }

      radio.read(&packet, sizeof(packet));

      // Check the header
      if (packet.header != 0xAB) {
        return false;
      }

      // Checksum check
      uint16_t expected = packet.throttle ^ packet.steering;
      if (packet.checksum != expected) {
        return false;
      }

      return true;
    }

    /**
     * @brief Attach telemetry data to ACK response.
     * 
     * @param telemetry \c TelemetryPacket - Telemetry snapshot to send back to the remote.
     */
    void sendTelemtry(const TelemetryPacket& telemetry) {
      if (role != BUGGY) return;

      radio.writeAckPayload(1, &telemetry, sizeof(telemetry));
    }

    /**
     * @brief Check if link between remote and buggy is considered alive.
     * 
     * @param timeoutMs \c uint16_t - Time without successful telemetry before link is considered
     * lost. Defaults to 200 ms.
     * 
     * @return \c boolean - True if link is still healthy.
     */
    boolean isLinkAlive(uint16_t timeoutMs=200) {
      if (millis() - lastGoodLinkMs > timeoutMs) {
        linkAlive = false;
      }

      return linkAlive;
    }

  private:  
    RF24 radio;
    Role role;

    uint32_t lastGoodLinkMs;
    boolean linkAlive;
};

// ============================================================================================== //
// LIFECYCLE                                                                                      //
// ============================================================================================== //

LCD lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);
LED noSignalLed(ERROR_LED_PIN);
Transceiver transceiver(TRANS_CE_PIN, TRANS_CSN_PIN);

TimeScheduler transmitTimer(1000000 / (uint32_t)TRANSMIT_HZ);

/**
 * @brief Called by system at the startup once.
 */
void setup() {
  Serial.begin(9600);

  pinMode(L_BLINKER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(R_BLINKER_SWITCH_PIN, INPUT_PULLUP);
}

/**
 * @brief Called by system every time possible.
 */
void loop() {
  if (transmitTimer.ready()) {
    uint16_t throttlePot = 1023 - analogRead(THROTTLE_POT_PIN);
    uint16_t steeringPot = analogRead(STEERING_POT_PIN);
    boolean leftBlinker = !analogRead(L_BLINKER_SWITCH_PIN);
    boolean rightBlinker = !analogRead(R_BLINKER_SWITCH_PIN);

    Transceiver::ControlPacket control(leftBlinker, rightBlinker, throttlePot, steeringPot);
    Transceiver::TelemetryPacket telemetry;
    if (transceiver.send(control, telemetry)) {
     lcd.setData(
      telemetry.rpm, telemetry.kmh, telemetry.corner, telemetry.acceleration, telemetry.pitch,
      telemetry.roll, telemetry.wattage, telemetry.batteryPct
     );
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

/*
Transceiver::ControlPacket control;

if (transceiver.receive(control)) {
  // apply throttle + steering here

  Transceiver::TelemetryPacket telemetry;
  telemetry.rpm = 3456;
  telemetry.kmh = 45.2f;
  telemetry.corner = 1.2f;
  telemetry.acceleration = 3.4f;
  telemetry.pitch = 0.5f;
  telemetry.roll = -0.2f;
  telemetry.wattage = 78;
  telemetry.batteryPct = 87.6f;

  telemetry.checksum = telemetry.rpm ^ telemetry.wattage;

  transceiver.sendTelemetry(telemetry);
}
*/
