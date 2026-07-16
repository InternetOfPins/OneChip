#pragma once
#include <stdint.h>
#ifdef __AVR__
#include <avr/interrupt.h>

namespace hw::avr {

  // AvrEeprom<Size, PageSize> — internal AVR EEPROM driver.
  //
  // Provides the same read(addr, buf, len) / write(addr, buf, len) interface
  // as AT24C, so EepromStore<> / EepromBlock<> work with either backing store.
  //
  // Chip       Size  PageSize
  // ATmega88    512     4
  // ATmega168   512     4
  // ATmega328  1024     4
  // ATmega1280 4096     8
  // ATmega2560 4096     8
  // ATmega1284P 4096    8
  // ATtiny85    512     4
  // ATtiny45    256     4
  //
  // Write timing: ~3.4ms per byte (erase+write mode). The write loop polls
  // EEPE; it blocks until the hardware finishes each byte.
  // Interrupts are briefly disabled (4 cycles) during EEMPE→EEPE strobe.
  //
  // Page granularity: AVR EEPROM is byte-addressable — PageSize here means
  // the natural write unit for BlockRecycle alignment, not a hardware limit.
  // We default to 4 to match the minimum meaningful block boundary.

  // Register addresses (memory-mapped, same across ATmega88/168/328/1280/2560):
  // EECR = 0x3F, EEDR = 0x40, EEARL = 0x41, EEARH = 0x42
  // ATtiny85: EECR = 0x3F, EEDR = 0x40, EEAR = 0x41 (8-bit addr only)

  namespace detail {
    static inline volatile uint8_t& eecr() {
      return *reinterpret_cast<volatile uint8_t*>(0x3F);
    }
    static inline volatile uint8_t& eedr() {
      return *reinterpret_cast<volatile uint8_t*>(0x40);
    }
    static inline volatile uint16_t& eear() {
      return *reinterpret_cast<volatile uint16_t*>(0x41);
    }

    static inline void _wait()  { while (eecr() & (1<<1)); }  // wait EEPE=0
    static inline uint8_t _read_byte(uint16_t addr) {
      _wait();
      eear() = addr;
      eecr() |= (1<<0);        // EERE strobe
      return eedr();
    }
    static inline void _write_byte(uint16_t addr, uint8_t val) {
      _wait();
      eear() = addr;
      eedr() = val;
      uint8_t sreg = SREG;
      cli();
      eecr() |= (1<<2);        // EEMPE
      eecr() |= (1<<1);        // EEPE — must follow within 4 cycles
      SREG = sreg;
    }
  }

  template<uint16_t Size_, uint8_t PageSize_ = 4>
  struct AvrEeprom {
    static constexpr uint16_t size     = Size_;
    static constexpr uint8_t  pageSize = PageSize_;

    static void begin() {}  // no initialisation needed; EEPROM is always on

    static void read(uint16_t addr, uint8_t* buf, uint16_t len) {
      for (uint16_t i = 0; i < len; i++)
        buf[i] = detail::_read_byte(addr + i);
    }

    static void write(uint16_t addr, const uint8_t* buf, uint16_t len) {
      for (uint16_t i = 0; i < len; i++)
        detail::_write_byte(addr + i, buf[i]);
      detail::_wait();  // ensure last write completes before returning
    }
  };

  // Convenience aliases matching ATmega memory specs
  using Mega328Eeprom  = AvrEeprom<1024, 4>;
  using Mega2560Eeprom = AvrEeprom<4096, 8>;
  using Tiny85Eeprom   = AvrEeprom< 512, 4>;
  using Tiny45Eeprom   = AvrEeprom< 256, 4>;
  using Tiny13Eeprom   = AvrEeprom<  64, 4>;

} // hw::avr
#endif
