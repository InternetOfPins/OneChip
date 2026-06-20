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

    // Streaming API — for multi-byte payloads without repeated start/stop.
    static void begin_write(uint8_t addr) { _start(); _write(addr << 1); }
    static void write_byte(uint8_t b)     { _write(b); }
    static void end_write()               { _stop(); }
  };

  namespace mega {
    template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
    using TwiMaster = AvrTwiMaster<SclHz, CpuHz>;
  }

} // hw::avr
