#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <stdint.h>
#else
  #include <cstdint>
#endif

namespace hw::avr {

  // AvrOC<OCR, TCCRA, COM_HI, PORT, DDR, PIN_BIT>
  //
  // Satisfies the PWMChan::PwmPin contract for a single AVR Output Compare channel.
  // All registers referenced by compile-time address — zero RAM, zero virtual.
  //
  // COM_HI: bit position of the upper COM bit (COMnx1) in TCCRnA.
  //   OC_A channels: COM_HI = 7  (bits [7:6] of TCCRnA)
  //   OC_B channels: COM_HI = 5  (bits [5:4] of TCCRnA)
  //
  // enable()  → COM = 10 (non-inverting, clear on match)
  // disable() → COM = 00 (timer disconnected, pin reverts to GPIO)
  // on/off    → direct GPIO write (used by PWMChan at Lo/Hi boundaries)
  template<uintptr_t OCR_ADDR,
           uintptr_t TCCRA_ADDR,
           uint8_t   COM_HI,
           uintptr_t PORT_ADDR,
           uintptr_t DDR_ADDR,
           uint8_t   PIN_BIT>
  struct AvrOC {
    static volatile uint8_t& ocr()   { return *reinterpret_cast<volatile uint8_t*>(OCR_ADDR);   }
    static volatile uint8_t& tccra() { return *reinterpret_cast<volatile uint8_t*>(TCCRA_ADDR); }
    static volatile uint8_t& port()  { return *reinterpret_cast<volatile uint8_t*>(PORT_ADDR);  }
    static volatile uint8_t& ddr()   { return *reinterpret_cast<volatile uint8_t*>(DDR_ADDR);   }

    static constexpr uint8_t mask = uint8_t(1 << PIN_BIT);

    static void begin()        { ddr() |= mask; }
    static void set(uint8_t v) { ocr() = v; }
    static void enable()       { tccra() = (tccra() |  (1 << COM_HI)) & ~uint8_t(1 << (COM_HI - 1)); }
    static void disable()      { tccra() &= ~uint8_t((1 << COM_HI) | (1 << (COM_HI - 1))); }
    static void on()           { port() |= mask; }
    static void off()          { port() &= ~mask; }
  };

} // hw::avr
