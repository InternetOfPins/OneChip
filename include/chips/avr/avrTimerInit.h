#pragma once
#include <hapi/hapi.h>
#include <stdint.h>

// AVR timer initialisation components for use in Chip::Board<> chains.
//
// Each struct is a HAPI Board<> component — its Part<O>::begin() configures
// the timer registers then calls O::begin() to continue the chain.
//
// Register addresses match ATmega88/168/328/1280/2560 (identical layouts).
// WGM and CS are set; COM bits are left at 00 (OC pins disconnected from
// timer) so AvrOC::enable()/disable() controls them per-channel at runtime.
//
// Prescaler / frequency table (F_CPU = 16 MHz, 8-bit timer, TOP=0xFF=256):
//   PS=1    → 62500 Hz   (ultrasonic, inaudible)
//   PS=8    →  7813 Hz   (audible, low flicker)
//   PS=64   →   977 Hz   (default PWM on Arduino)
//   PS=256  →   244 Hz   (visible flicker on LEDs)
//   PS=1024 →    61 Hz   (very slow)
//
// 16-bit timers (TC1, TC3..TC5) in 8-bit fast PWM mode (WGM=5, TOP=0xFF):
//   Same frequencies as above (same prescaler table CS1Policy).

namespace hw::avr {

  namespace detail {
    static inline volatile uint8_t& reg8(uintptr_t addr) {
      return *reinterpret_cast<volatile uint8_t*>(addr);
    }
  }

  // ── 8-bit timers (TC0, TC2) ───────────────────────────────────────────────
  // Fast PWM, TOP=0xFF, WGM=3 (TCCR0A: WGM01|WGM00, TCCR0B: WGM02=0)

  template<uintptr_t TCCRA, uintptr_t TCCRB, uint8_t CS>
  struct AvrTimer8FastPwm {
    template<typename O> struct Part : O {
      static void begin() {
        detail::reg8(TCCRA) = (1<<1)|(1<<0);  // WGM01|WGM00 — Fast PWM
        detail::reg8(TCCRB) = CS;              // prescaler, WGM02=0
        O::begin();
      }
    };
  };

  // TC0 variants (TCCR0A=0x44, TCCR0B=0x45) — ATmega88/168/328/2560
  struct Timer0Pwm1    : AvrTimer8FastPwm<0x44,0x45, 0x01> {};  // PS=1    ~62500 Hz
  struct Timer0Pwm8    : AvrTimer8FastPwm<0x44,0x45, 0x02> {};  // PS=8    ~7813  Hz
  struct Timer0Pwm64   : AvrTimer8FastPwm<0x44,0x45, 0x03> {};  // PS=64   ~977   Hz (default)
  struct Timer0Pwm256  : AvrTimer8FastPwm<0x44,0x45, 0x04> {};  // PS=256  ~244   Hz
  struct Timer0Pwm1024 : AvrTimer8FastPwm<0x44,0x45, 0x05> {};  // PS=1024 ~61    Hz

  // TC2 variants (TCCR2A=0xB0, TCCR2B=0xB1) — CS2Policy (different table)
  // PS=1,8,32,64,128,256,1024 → codes 1..7
  struct Timer2Pwm1    : AvrTimer8FastPwm<0xB0,0xB1, 0x01> {};
  struct Timer2Pwm8    : AvrTimer8FastPwm<0xB0,0xB1, 0x02> {};
  struct Timer2Pwm32   : AvrTimer8FastPwm<0xB0,0xB1, 0x03> {};
  struct Timer2Pwm64   : AvrTimer8FastPwm<0xB0,0xB1, 0x04> {};
  struct Timer2Pwm128  : AvrTimer8FastPwm<0xB0,0xB1, 0x05> {};
  struct Timer2Pwm256  : AvrTimer8FastPwm<0xB0,0xB1, 0x06> {};
  struct Timer2Pwm1024 : AvrTimer8FastPwm<0xB0,0xB1, 0x07> {};

  // ── 16-bit timers in 8-bit fast PWM mode (WGM=5) ─────────────────────────
  // TCCR1A: WGM10=1 (bit 0); TCCR1B: WGM12=1 (bit 3) | CS

  template<uintptr_t TCCRA, uintptr_t TCCRB, uint8_t CS>
  struct AvrTimer16FastPwm8 {
    template<typename O> struct Part : O {
      static void begin() {
        detail::reg8(TCCRA) = (1<<0);          // WGM10 — 8-bit fast PWM
        detail::reg8(TCCRB) = (1<<3) | CS;    // WGM12 | prescaler
        O::begin();
      }
    };
  };

  // TC1 variants (TCCR1A=0x80, TCCR1B=0x81)
  struct Timer1Pwm1    : AvrTimer16FastPwm8<0x80,0x81, 0x01> {};
  struct Timer1Pwm8    : AvrTimer16FastPwm8<0x80,0x81, 0x02> {};
  struct Timer1Pwm64   : AvrTimer16FastPwm8<0x80,0x81, 0x03> {};
  struct Timer1Pwm256  : AvrTimer16FastPwm8<0x80,0x81, 0x04> {};
  struct Timer1Pwm1024 : AvrTimer16FastPwm8<0x80,0x81, 0x05> {};

  // TC3 variants (TCCR3A=0x90, TCCR3B=0x91) — ATmega2560/1280/1284P
  struct Timer3Pwm1    : AvrTimer16FastPwm8<0x90,0x91, 0x01> {};
  struct Timer3Pwm8    : AvrTimer16FastPwm8<0x90,0x91, 0x02> {};
  struct Timer3Pwm64   : AvrTimer16FastPwm8<0x90,0x91, 0x03> {};
  struct Timer3Pwm256  : AvrTimer16FastPwm8<0x90,0x91, 0x04> {};
  struct Timer3Pwm1024 : AvrTimer16FastPwm8<0x90,0x91, 0x05> {};

  // TC4 variants (TCCR4A=0xA0, TCCR4B=0xA1) — ATmega2560/1280
  struct Timer4Pwm1    : AvrTimer16FastPwm8<0xA0,0xA1, 0x01> {};
  struct Timer4Pwm8    : AvrTimer16FastPwm8<0xA0,0xA1, 0x02> {};
  struct Timer4Pwm64   : AvrTimer16FastPwm8<0xA0,0xA1, 0x03> {};
  struct Timer4Pwm256  : AvrTimer16FastPwm8<0xA0,0xA1, 0x04> {};
  struct Timer4Pwm1024 : AvrTimer16FastPwm8<0xA0,0xA1, 0x05> {};

  // TC5 variants (TCCR5A=0x120, TCCR5B=0x121) — ATmega2560/1280
  struct Timer5Pwm1    : AvrTimer16FastPwm8<0x120,0x121, 0x01> {};
  struct Timer5Pwm8    : AvrTimer16FastPwm8<0x120,0x121, 0x02> {};
  struct Timer5Pwm64   : AvrTimer16FastPwm8<0x120,0x121, 0x03> {};
  struct Timer5Pwm256  : AvrTimer16FastPwm8<0x120,0x121, 0x04> {};
  struct Timer5Pwm1024 : AvrTimer16FastPwm8<0x120,0x121, 0x05> {};

} // hw::avr
