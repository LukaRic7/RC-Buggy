#pragma once

#include <RF24.h>
#include "Packets.h"

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
      radio.enableAckPayload();
      radio.enableDynamicPayloads();
      radio.setRetries(3, 5);

      if (role == REMOTE) {
        radio.openWritingPipe(address);
        radio.stopListening();
      } else {
        radio.openReadingPipe(1, address);
        radio.startListening();
      }

      lastGoodLinkMs = millis();
      linkAlive      = true;

      radio.printDetails();

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
          gotTelemetry      = telemetryOut.checksum == expected;
        }
      }

      // Update link state ONLY on successful full transaction
      if (gotTelemetry) {
        lastGoodLinkMs = millis();
        linkAlive      = true;
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
     * 
     * @return \c boolean - True if ACK payload was successfully attached.
     */
    boolean sendTelemetry(const TelemetryPacket& telemetry) {
      if (role != BUGGY) {
        return false;
      }

      if (radio.writeAckPayload(1, &telemetry, sizeof(telemetry))) {
        return true;
      }

      return false;
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