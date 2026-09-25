#pragma once
#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>
#include <hapi/hapi.h>
#include <oneBus/i2c.h>

namespace hw::avr {

  // TWI primitives with every wait bounded. Each step reads TWSR, so an address NACK, a lost arbitration and a
  // bus error are told apart; the cause is kept until the next start(). kSpins is about 9 ms of loop iterations (7 cycles each)
  // (a busy-wait has no clock): far above a byte at 100 kHz, so a healthy bus never reaches it.
  template<uint32_t CpuHz>
  struct TwiPrims {
    inline static oneBus::TwiCause cause = oneBus::TwiCause::None;
    static constexpr uint16_t kSpins = uint16_t(CpuHz / 800);

    static bool wait() {
      for (uint16_t n = kSpins; n; --n) if (TWCR & (1<<TWINT)) return true;
      return false;
    }
    // give the bus back: after a timeout the interface may be stuck, so re-enable it; otherwise STOP
    static bool fail(oneBus::TwiCause c) {
      cause = c;
      if (c == oneBus::TwiCause::Timeout) { TWCR = 0; TWCR = (1<<TWEN); }
      else TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
      return false;
    }
    // TWCR has just been written: wait for TWINT, then classify TWSR. a/b/c are the acknowledging statuses.
    // A NACK leaves the bus held: the caller sends the STOP.
    static bool step(uint8_t a, uint8_t b, uint8_t c) {
      if (!wait()) return fail(oneBus::TwiCause::Timeout);
      const uint8_t s = TW_STATUS;
      if (s == a || s == b || s == c) return true;
      if (s == TW_MT_SLA_NACK || s == TW_MT_DATA_NACK || s == TW_MR_SLA_NACK) { cause = oneBus::TwiCause::Nack; return false; }
      return fail(s == TW_MT_ARB_LOST ? oneBus::TwiCause::ArbLost : oneBus::TwiCause::BusError);
    }
    static bool start() {
      cause = oneBus::TwiCause::None;
      TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
      return step(TW_START, TW_REP_START, TW_START);
    }
    static void stop() { TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); }
    // true: acknowledged (SLA+W, data, or SLA+R)
    static bool write(uint8_t b) {
      TWDR = b;
      TWCR = (1<<TWINT)|(1<<TWEN);
      return step(TW_MT_SLA_ACK, TW_MT_DATA_ACK, TW_MR_SLA_ACK);
    }
    static uint8_t read(bool ack) {
      TWCR = ack
        ? uint8_t((1<<TWINT)|(1<<TWEN)|(1<<TWEA))
        : uint8_t((1<<TWINT)|(1<<TWEN));
      if (!wait()) { fail(oneBus::TwiCause::Timeout); return 0xFF; }
      return TWDR;
    }
  };

  // HAPI hardware core — maps AVR TWI registers, no protocol logic.
  // twi_init/twi_start/twi_stop/twi_write/twi_read are the primitive API;
  // I2cMaster<Freq> (in OneBus/i2c.h) sits above and calls Base::twi_*.
  template<uint32_t CpuHz = 16000000UL>
  struct AvrTwiCore {
    template<typename O>
    struct Part : O {
      using Base = O;
      using P = TwiPrims<CpuHz>;

      static void twi_init(uint32_t freq) {
        TWSR = 0;                                        // prescaler = 1
        TWBR = uint8_t((CpuHz / freq - 16) / 2);
      }
      static bool    twi_start()            { return P::start(); }
      static void    twi_stop()             { P::stop(); }
      static bool    twi_write(uint8_t b)   { return P::write(b); }
      static uint8_t twi_read(bool ack)     { return P::read(ack); }
      static oneBus::TwiCause twi_cause()   { return P::cause; }
      static void begin() { Base::begin(); }
    };
  };

  // AVR TWI (Two Wire Interface) master: the I2cMaster protocol over AvrTwiCore, plus the single-byte send
  // that OneIO's expander drivers call.
  // SclHz: SCL clock frequency. CpuHz: CPU frequency (for TWBR calc).
  template<uint32_t SclHz = 100000UL, uint32_t CpuHz = 16000000UL>
  struct AvrTwiMaster
    : oneBus::I2cMaster<SclHz>::template Part<typename AvrTwiCore<CpuHz>::template Part<oneBus::TwiAPI>> {
    using Base = typename oneBus::I2cMaster<SclHz>::template Part<typename AvrTwiCore<CpuHz>::template Part<oneBus::TwiAPI>>;
    using Base::send;
    static bool send(uint8_t addr, uint8_t data) { return Base::send(addr, &data, 1); }
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
