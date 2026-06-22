#pragma once
#include <stdint.h>
#include <avr/io.h>

namespace hw::avr {

  // AVR TWI (Two Wire Interface) master — bare register driver.
  // SclHz: SCL clock frequency. CpuHz: CPU frequency (for TWBR calc).
  template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
  struct AvrTwiMaster {
    static void begin() {
      TWSR = 0;                              // prescaler = 1
      TWBR = (CpuHz / SclHz - 16) / 2;     // bit rate register
    }

  private:
    static void _start() {
      TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
      while (!(TWCR & (1<<TWINT)));
    }
    static void _write(uint8_t b) {
      TWDR = b;
      TWCR = (1<<TWINT)|(1<<TWEN);
      while (!(TWCR & (1<<TWINT)));
    }
    static void _stop() {
      TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
    }

  public:
    static void send(uint8_t addr, uint8_t data) {
      _start(); _write(addr << 1); _write(data); _stop();
    }
    static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
      _start(); _write(addr << 1);
      while (len--) _write(*data++);
      _stop();
    }

    // ── Write streaming ──────────────────────────────────────────────────────
    static void begin_write(uint8_t addr) { _start(); _write(addr << 1); }
    static void write_byte(uint8_t b)     { _write(b); }
    static void end_write()               { _stop(); }

    // ── Read streaming ───────────────────────────────────────────────────────
    // request_from: sends START + SLA+R, primes the byte counter.
    // read_byte: clocks each byte out; ACKs all but the last, then sends STOP.
    inline static uint8_t _rcount = 0;

    static uint8_t request_from(uint8_t addr, uint8_t n) {
      _rcount = n;
      _start();
      _write(uint8_t((addr << 1) | 1));  // SLA+R
      return n;
    }

    static uint8_t read_byte() {
      if (_rcount > 1) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);  // ACK — more to come
      } else {
        TWCR = (1<<TWINT)|(1<<TWEN);             // NACK — last byte
      }
      while (!(TWCR & (1<<TWINT)));
      uint8_t b = TWDR;
      if (--_rcount == 0) _stop();
      return b;
    }
  };

  namespace mega {
    template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
    using TwiMaster = AvrTwiMaster<SclHz, CpuHz>;
  }

} // hw::avr
