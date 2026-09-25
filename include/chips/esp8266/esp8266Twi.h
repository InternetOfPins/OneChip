/**
 * @file esp8266Twi.h
 * @brief ESP8266 I2C master — wraps Arduino Wire.h (software bit-bang).
 *
 * ESP8266 has no hardware I2C; Wire.h implements bit-bang on any two GPIO
 * pins. Default pins are GPIO4(SDA=D2) / GPIO5(SCL=D1) — the NodeMCU /
 * Wemos D1 Mini standard, matching MicroTC360 hardware.
 *
 * Provides the same streaming API as AvrTwiCore / Stm32I2cCore:
 *   begin() / begin_write(addr) / write_byte(b) / end_write() / send()
 *   request_from(addr, n) / read_byte()
 *
 * Usage:
 *   #include <chips/esp8266/esp8266Twi.h>
 *   using Twi = hw::esp8266::TwiMaster<4, 5, 400000>;
 *   Twi::begin();
 *   Twi::begin_write(0x27); Twi::write_byte(0xFF); Twi::end_write();
 *   Twi::request_from(0x68, 7);
 *   for (int i=0; i<7; i++) buf[i] = Twi::read_byte();
 */

#pragma once
#include <stdint.h>
#include <oneBus/busAPI.h>
#include <oneBus/twiMaster.h>
#ifdef ARDUINO
  #include <Wire.h>
#endif

namespace hw::esp8266 {

  /// @brief ESP8266 I2C master via Arduino Wire; SDA/SCL are GPIO numbers, SclHz sets clock rate
  template<int SDA = 4, int SCL = 5, uint32_t SclHz = 400000UL>
  struct Esp8266TwiMaster {
    inline static oneBus::TwiCause _cause = oneBus::TwiCause::None;

    static void begin() {
      Wire.begin(SDA, SCL);
      Wire.setClock(SclHz);
    }

    // ── Write streaming ────────────────────────────────────────────────
    // Wire buffers the transaction: the address is only put on the wire, and acknowledged, in end_write().
    // endTransmission(): 0 ok, 2 NACK on address, 3 NACK on data, 4 line busy.
    static bool begin_write(uint8_t addr) { Wire.beginTransmission(addr); return true; }
    static void write_byte(uint8_t b)     { Wire.write(b); }
    static bool end_write() {
      const uint8_t e = Wire.endTransmission();
      _cause = oneBus::twiCauseFromWire(e);
      return e == 0;
    }

    static bool send(uint8_t addr, const uint8_t* data, uint8_t len) {
      Wire.beginTransmission(addr);
      while (len--) Wire.write(*data++);
      return end_write();
    }

    // ── Read streaming ─────────────────────────────────────────────────
    // Wire.requestFrom() sends START+SLA+R, clocks n bytes, sends STOP; it returns 0 on any failure, without a cause.
    // Subsequent read_byte() calls drain the Wire receive buffer.
    [[nodiscard]] static uint8_t request_from(uint8_t addr, uint8_t n) {
      const uint8_t got = Wire.requestFrom(static_cast<uint8_t>(addr), static_cast<uint8_t>(n));
      _cause = got ? oneBus::TwiCause::None : oneBus::TwiCause::Unknown;
      return got;
    }

    [[nodiscard]] static uint8_t read_byte() { return Wire.read(); }

    [[nodiscard]] static bool available() { return Wire.available() > 0; }

    static oneBus::TwiCause cause() { return _cause; }

    // Presence. The read-probe uses the core's twi_readFrom, whose result carries the cause (0 ok, 2 NACK, 4 line busy).
    static bool probe(uint8_t addr, oneBus::ProbeKind kind) {
      if (kind == oneBus::ProbeKind::Write) {
        Wire.beginTransmission(addr);
        return end_write();
      }
      unsigned char b;
      const uint8_t e = twi_readFrom(addr, &b, 1, true);
      _cause = oneBus::twiCauseFromWire(e);
      return e == 0;
    }
  };

  namespace esp8266 {
    // chip::TwiMaster<SDA,SCL,SclHz> — matches ESP32 alias pattern
    template<int SDA = 4, int SCL = 5, uint32_t SclHz = 400000UL>
    using TwiMaster = Esp8266TwiMaster<SDA, SCL, SclHz>;

    // MicroTC360 / MicroTC361 — I2C bus at 400 kHz on SDA=4/SCL=5
    using MicroTC360_Twi = TwiMaster<4, 5, 400000UL>;
  }

} // hw::esp8266
