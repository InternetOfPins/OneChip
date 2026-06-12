/**
 * @file avrTC.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AVR Timer/Counter HAPI components
 * @note Ported from 2014 avrTC — tested register layouts preserved exactly.
 *       Runtime instances replaced by zero-RAM compile-time HAPI components.
 *
 * creative commons license 3.0: Attribution-ShareAlike CC BY-SA
 */

#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <Arduino.h>
#else
  #include <cstdint>
  using byte = uint8_t;
#endif

// this macro allows for usage of '_r' as a reserved bit or bitfield (only 1 per line)
#define _CONCAT_(x,y) x ## y
#define CONCAT(x,y) _CONCAT_(x,y)
#define _r CONCAT(_R,__LINE__)

namespace hw {
namespace avr {

  // ============================================================
  // Register layout structs
  // Pure memory map — no logic, no methods.
  // Match hardware exactly; do not reorder fields.
  // ============================================================

  /// @brief 8-bit timer register layout (TC0, TC2)
  struct tc8_regs {
    union {
      byte crA;
      struct {
        byte wgm0:1;
        byte wgm1:1;
        byte _r:2;
        byte comB:2;
        byte comA:2;
      };
    };
    union {
      byte crB;
      struct {
        byte cs:3;
        byte wgm2:1;
        byte _r:2;
        byte focB:1;
        byte focA:1;
      };
    };
    volatile byte cnt;
    byte ocrA;
    byte ocrB;
  };

  /// @brief 16-bit timer register layout (TC1, TC3, TC4, TC5)
  struct tc16_regs {
    union {
      byte crA;
      struct { byte wgm0:1, wgm1:1, _r:2, comB:2, comA:2; };
    };
    union {
      byte crB;
      struct { byte cs:3, wgm2:1, wgm3:1, _r:1, ices:1, icnc:1; };
    };
    union {
      byte crC;
      struct { byte _r:6, focB:1, focA:1; };
    };
    byte _reserved;
    volatile uint16_t cnt;
    uint16_t icr;
    uint16_t ocrA;
    uint16_t ocrB;
    uint16_t ocrC;
  };

  /// @brief Interrupt flag register layout
  struct tifr_t {
    byte tov:1;
    byte ocfA:1;
    byte ocfB:1;
    byte ocfC:1;
    byte _r:1;
    byte icf:1;
    byte _r:2;
  };

  /// @brief Interrupt mask register layout
  struct timsk_t {
    byte toie:1;
    byte ocieA:1;
    byte ocieB:1;
    byte ocieC:1;
    byte _r:1;
    byte icie:1;
    byte _r:2;
  };

  // ============================================================
  // Timer API — terminal/fallback, not a component, no Part<>
  // ============================================================

  /// @brief Timer API terminal.
  /// Fallback implementations — silently do nothing if no layer above overrides.
  /// Cfg carries platform config (e.g. F_CPU override) accessible via TimerAPI::Config.
  template<typename Cfg = hapi::Nil>
  struct TimerAPI {
    using Config = Cfg;

    static void         on(uint16_t, byte, byte) {}
    static void         off()                    {}
    static void         intA()                   {}
    static float        play(float, byte)        { return 0; }
    static void         setWaveMode(byte)        {}
    static void         setOutMode_A(byte)       {}
    static void         setOutMode_B(byte)       {}
    static void         setClockSource(byte)     {}
  };

  // ============================================================
  // TimerCore — HAPI component, hardware register access
  // ============================================================

  /// @brief Hardware register access layer.
  /// Maps compile-time address constants to register operations.
  /// @tparam Regs    register layout struct (tc8_regs or tc16_regs)
  /// @tparam BASE    base address of the timer register block
  /// @tparam TIFR_ADDR  address of the TIFR register
  /// @tparam TIMSK_ADDR address of the TIMSK register
  template<typename Regs, uintptr_t BASE, byte TIFR_ADDR, byte TIMSK_ADDR>
  struct TimerCore {
    /// capability tag — queryable via hapi::query<TimerCore<...>::IsTimer, Chain>
    using IsTimer = std::true_type;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static Regs& regs() {
        return *reinterpret_cast<Regs*>(BASE);
      }
      static tifr_t& tifr() {
        return *reinterpret_cast<tifr_t*>(TIFR_ADDR);
      }
      static timsk_t& timsk() {
        return *reinterpret_cast<timsk_t*>(TIMSK_ADDR);
      }

      static void on(uint16_t fr, byte scale, byte duty = 50) {
        regs().cnt  = 0;
        regs().ocrA = fr;
        regs().crB  = (regs().crB & ~0b00000111) | scale;
      }
      static void off() {
        regs().crB &= ~0b00000111;
        regs().cnt  = 0;
      }
      static void intA()          { timsk().ocieA = 1; }
      static void setClockSource(byte n) { regs().cs = n; }
      static void setOutMode_A(byte n)   { regs().comA = n; }
      static void setOutMode_B(byte n)   { regs().comB = n; }
      static void setWaveMode(byte n) {
        regs().crA = (regs().crA & ~0b00000011) | (n & 0b11);
        regs().wgm2 = n >> 2;
      }
    };
  };

  // ============================================================
  // Prescaler policies — pure static, zero RAM, no state
  // ============================================================

  #define DUTTY_MAX     100
  #define DEFAULT_DUTTY (DUTTY_MAX/2)

  /// @brief CS1 prescaler table: 1, 8, 64, 256, 1024
  /// Used by TC0, TC1, TC3, TC4, TC5
  struct CS1Policy {
    static constexpr int bestPrescale(unsigned long f) {
      unsigned long r = F_CPU / f;
      if (r <= 0xFFFFUL)                          return 1;
      if ((r >> 3)  <= 0xFFFFUL && (r >> 3)  > 0) return 8;
      if ((r >> 6)  <= 0xFFFFUL && (r >> 6)  > 0) return 64;
      if ((r >> 8)  <= 0xFFFFUL && (r >> 8)  > 0) return 256;
      if ((r >> 10) <= 0xFFFFUL && (r >> 10) > 0) return 1024;
      return 0;
    }
    static constexpr byte prescaleCode(int p) {
      if (p == 1)    return 1;
      if (p == 8)    return 2;
      if (p == 64)   return 3;
      if (p == 256)  return 4;
      if (p == 1024) return 5;
      return 0;
    }
  };

  /// @brief CS2 prescaler table: 1, 8, 32, 64, 128, 256, 1024
  /// Used by TC2 only (different prescaler set on ATmega)
  struct CS2Policy {
    static constexpr int bestPrescale(unsigned long f) {
      unsigned long r = F_CPU / f;
      if (r <= 0xFFUL)                           return 1;
      if ((r >> 3)  <= 0xFFUL && (r >> 3)  > 0) return 8;
      if ((r >> 5)  <= 0xFFUL && (r >> 5)  > 0) return 32;
      if ((r >> 6)  <= 0xFFUL && (r >> 6)  > 0) return 64;
      if ((r >> 7)  <= 0xFFUL && (r >> 7)  > 0) return 128;
      if ((r >> 8)  <= 0xFFUL && (r >> 8)  > 0) return 256;
      if ((r >> 10) <= 0xFFUL && (r >> 10) > 0) return 1024;
      return 0;
    }
    static constexpr byte prescaleCode(int p) {
      if (p == 1)    return 1;
      if (p == 8)    return 2;
      if (p == 32)   return 3;
      if (p == 64)   return 4;
      if (p == 128)  return 5;
      if (p == 256)  return 6;
      if (p == 1024) return 7;
      return 0;
    }
  };

  // ============================================================
  // Prescaler — HAPI component
  // ============================================================

  /// @brief Prescaler layer — wraps TimerCore, adds play() with auto prescale selection.
  /// @tparam CSPolicy  CS1Policy or CS2Policy
  template<typename CSPolicy>
  struct Prescaler {
    using IsPrescaler = std::true_type;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      /// @brief Set frequency and duty cycle, returns actual achieved frequency.
      /// @param f     target frequency in Hz
      /// @param duty  duty cycle 0..DUTTY_MAX (default 50)
      /// @return actual frequency achieved, 0 if not achievable
      static float play(float f, int duty = DEFAULT_DUTTY) {
        f *= 2;
        int p = CSPolicy::bestPrescale((unsigned long)f);
        if (!p) return 0;
        unsigned long r = F_CPU / ((unsigned long)p * (unsigned long)f);
        if (r > 0xFFFFUL) return 0;
        Base::on((uint16_t)r, CSPolicy::prescaleCode(p), duty);
        return F_CPU / (2.0f * p * r);
      }
    };
  };

  // ============================================================
  // Concrete chip timer descriptors
  // Chain: Prescaler → TimerCore → TimerAPI<>
  // ============================================================

  namespace mega {

    /// @brief ATmega328/168/88 and compatible
    using TC0 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc8_regs,  0x44, 0x35, 0x6E>>;
    using TC1 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x80, 0x36, 0x6F>>;
    using TC2 = hapi::APIOf<TimerAPI<>, Prescaler<CS2Policy>, TimerCore<tc8_regs,  0xB0, 0x37, 0x70>>;

  } // mega

  namespace mega2560 {

    /// @brief ATmega640/1280/1281/2560/2561 and compatible
    using TC0 = mega::TC0;
    using TC1 = mega::TC1;
    using TC2 = mega::TC2;
    using TC3 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x90, 0x38, 0x71>>;
    using TC4 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0xA0, 0x39, 0x72>>;
    using TC5 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x120,0x40, 0x73>>;

  } // mega2560

  namespace mega1284 {

    /// @brief ATmega1284
    using TC0 = mega::TC0;
    using TC1 = mega::TC1;
    using TC2 = mega::TC2;
    using TC3 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x90, 0x38, 0x71>>;

  } // mega1284

}} // hw::avr

// ============================================================
// Macro-selected chip aliases (Arduino-style, optional)
// mirrors the 2014 #ifdef block
// ============================================================

#if defined(__AVR_ATmega640__)  || defined(__AVR_ATmega1280__) || \
    defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || \
    defined(__AVR_ATmega2561__)
  namespace hw { namespace avr { namespace chip = mega2560; }}
#elif defined(__AVR_ATmega1284__)
  namespace hw { namespace avr { namespace chip = mega1284; }}
#else
  namespace hw { namespace avr { namespace chip = mega; }}
#endif

// ============================================================
// Usage:
//
//   hw::avr::chip::TC1::play(5, 10);     // 5 Hz, 10% duty
//   hw::avr::chip::TC2::setWaveMode(2);  // CTC
//   hw::avr::chip::TC2::setOutMode_A(1); // Toggle
//   hw::avr::chip::TC1::intA();          // enable compare-A interrupt
//
//   // explicit chip family:
//   hw::avr::mega2560::TC3::play(440);
// ============================================================
