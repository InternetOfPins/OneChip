#pragma once
#include <stdint.h>
#include <avr/io.h>
#include <hapi/hapi.h>
#include <oneBus/i2c.h>

namespace hw::avr {

  // HAPI hardware core — maps AVR TWI registers, no protocol logic.
  // twi_init/twi_start/twi_stop/twi_write/twi_read are the primitive API;
  // I2cMaster<Freq> (in OneBus/i2c.h) sits above and calls Base::twi_*.
  template<uint32_t CpuHz = 16000000UL>
  struct AvrTwiCore {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void twi_init(uint32_t freq) {
        TWSR = 0;                                        // prescaler = 1
        TWBR = uint8_t((CpuHz / freq - 16) / 2);
      }
      static void twi_start() {
        TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
        while (!(TWCR & (1<<TWINT)));
      }
      static void twi_stop() {
        TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
      }
      static void twi_write(uint8_t b) {
        TWDR = b;
        TWCR = (1<<TWINT)|(1<<TWEN);
        while (!(TWCR & (1<<TWINT)));
      }
      static uint8_t twi_read(bool ack) {
        TWCR = ack
          ? uint8_t((1<<TWINT)|(1<<TWEN)|(1<<TWEA))
          : uint8_t((1<<TWINT)|(1<<TWEN));
        while (!(TWCR & (1<<TWINT)));
        return TWDR;
      }
      static void begin() { Base::begin(); }
    };
  };



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

    template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                            AvrTwiCore<CpuHz>>;
  }

  namespace mega2560 {
    template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                            AvrTwiCore<CpuHz>>;
  }

  namespace mega1284 {
    template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI, oneBus::I2cMaster<SclHz>,
                            AvrTwiCore<CpuHz>>;
  }

} // hw::avr
