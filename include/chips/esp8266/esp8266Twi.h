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
#ifdef ARDUINO
  #include <Wire.h>
#endif

namespace hw::esp8266 {

  // SDA / SCL are GPIO numbers (bare integers, not pin objects).
  // SclHz: Wire.setClock() value; typical = 100000 or 400000.
  template<int SDA = 4, int SCL = 5, uint32_t SclHz = 400000UL>
  struct Esp8266TwiMaster {

    static void begin() {
      Wire.begin(SDA, SCL);
      Wire.setClock(SclHz);
    }

    // ── Write streaming ────────────────────────────────────────────────
    static void begin_write(uint8_t addr) { Wire.beginTransmission(addr); }
    static void write_byte(uint8_t b)     { Wire.write(b); }
    static void end_write()               { Wire.endTransmission(); }

    static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
      Wire.beginTransmission(addr);
      while (len--) Wire.write(*data++);
      Wire.endTransmission();
    }

    // ── Read streaming ─────────────────────────────────────────────────
    // Wire.requestFrom() sends START+SLA+R, clocks n bytes, sends STOP.
    // Subsequent read_byte() calls drain the Wire receive buffer.
    static uint8_t request_from(uint8_t addr, uint8_t n) {
      return Wire.requestFrom(static_cast<uint8_t>(addr),
                              static_cast<uint8_t>(n));
    }

    static uint8_t read_byte() { return Wire.read(); }

    static bool available() { return Wire.available() > 0; }
  };

  namespace esp8266 {
    // chip::TwiMaster<SDA,SCL,SclHz> — matches ESP32 alias pattern
    template<int SDA = 4, int SCL = 5, uint32_t SclHz = 400000UL>
    using TwiMaster = Esp8266TwiMaster<SDA, SCL, SclHz>;

    // MicroTC360 / MicroTC361 — I2C bus at 400 kHz on SDA=4/SCL=5
    using MicroTC360_Twi = TwiMaster<4, 5, 400000UL>;
  }

} // hw::esp8266
