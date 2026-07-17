/**
 * @file avrTC.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AVR Timer/Counter HAPI components
 * @note Ported from 2014 avrTC — tested register layouts preserved exactly.
 *       Runtime instances replaced by zero-RAM compile-time HAPI components.
 */

#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <stdint.h>
#else
  #include <cstdint>
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
      uint8_t crA;
      struct {
        uint8_t wgm0:1;
        uint8_t wgm1:1;
        uint8_t _r:2;
        uint8_t comB:2;
        uint8_t comA:2;
      };
    };
    union {
      uint8_t crB;
      struct {
        uint8_t cs:3;
        uint8_t wgm2:1;
        uint8_t _r:2;
        uint8_t focB:1;
        uint8_t focA:1;
      };
    };
    volatile uint8_t cnt;
    uint8_t ocrA;
    uint8_t ocrB;
  };

  /// @brief 16-bit timer register layout (TC1, TC3, TC4, TC5)
  struct tc16_regs {
    union {
      uint8_t crA;
      struct { uint8_t wgm0:1, wgm1:1, _r:2, comB:2, comA:2; };
    };
    union {
      uint8_t crB;
      struct { uint8_t cs:3, wgm2:1, wgm3:1, _r:1, ices:1, icnc:1; };
    };
    union {
      uint8_t crC;
      struct { uint8_t _r:6, focB:1, focA:1; };
    };
    uint8_t _reserved;
    volatile uint16_t cnt;
    uint16_t icr;
    uint16_t ocrA;
    uint16_t ocrB;
    uint16_t ocrC;
  };

  /// @brief Interrupt flag register layout
  struct tifr_t {
    uint8_t tov:1;
    uint8_t ocfA:1;
    uint8_t ocfB:1;
    uint8_t ocfC:1;
    uint8_t _r:1;
    uint8_t icf:1;
    uint8_t _r:2;
  };

  /// @brief Interrupt mask register layout
  struct timsk_t {
    uint8_t toie:1;
    uint8_t ocieA:1;
    uint8_t ocieB:1;
    uint8_t ocieC:1;
    uint8_t _r:1;
    uint8_t icie:1;
    uint8_t _r:2;
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

    static void         act()                    {}
    static void         on(uint16_t, uint8_t, uint8_t) {}
    static void         off()                    {}
    static void         intA()                   {}
    static float        play(float, uint8_t)        { return 0; }
    static void         setWaveMode(uint8_t)        {}
    static void         setOutMode_A(uint8_t)       {}
    static void         setOutMode_B(uint8_t)       {}
    static void         setClockSource(uint8_t)     {}
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
  template<typename Regs, uintptr_t BASE, uint8_t TIFR_ADDR, uint8_t TIMSK_ADDR,
           void(*fn)() = nullptr>
  struct TimerCore {
    using IsTimer = std::true_type;

    inline static void (*handler_)() = nullptr;
    static void attach(void(*f)()) { handler_ = f; }
    static void act() {
      if constexpr (fn != nullptr) fn();
      if (handler_) handler_();
    }

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

      static void attach(void(*f)()) { handler_ = f; }
      static void act() {
        if constexpr (fn != nullptr) fn();
        if (handler_) handler_();
        O::act();
      }
      static void on(uint16_t fr, uint8_t scale, uint8_t duty = 50) {
        regs().cnt  = 0;
        regs().ocrA = fr;
        regs().crB  = (regs().crB & ~0b00000111) | scale;
      }
      static void off() {
        regs().crB &= ~0b00000111;
        regs().cnt  = 0;
      }
      static void intA()               { timsk().ocieA = 1; }
      static void setClockSource(uint8_t n) { regs().cs = n; }
      static void setOutMode_A(uint8_t n)   { regs().comA = n; }
      static void setOutMode_B(uint8_t n)   { regs().comB = n; }
      static void setWaveMode(uint8_t n) {
        regs().crA = (regs().crA & ~0b00000011) | (n & 0b11);
        regs().wgm2 = n >> 2;
      }
    };
  };

  // ============================================================
  // TimerCoreI — individually-addressed timer core
  // ============================================================

  /// @brief Zero-state proxy for a single memory-mapped byte register.
  template<uintptr_t Addr>
  struct RegProxy8 {
    operator uint8_t() const { return *reinterpret_cast<volatile uint8_t*>(Addr); }
    RegProxy8& operator=(uint8_t v) { *reinterpret_cast<volatile uint8_t*>(Addr) = v; return *this; }
  };

  /// @brief Zero-state proxy for a single bit within a memory-mapped byte register.
  template<uintptr_t Addr, uint8_t Bit>
  struct BitProxy8 {
    operator bool() const { return (*reinterpret_cast<volatile uint8_t*>(Addr)) & uint8_t(1 << Bit); }
    BitProxy8& operator=(bool v) {
      auto& r = *reinterpret_cast<volatile uint8_t*>(Addr);
      if (v) r |= uint8_t(1 << Bit); else r &= ~uint8_t(1 << Bit);
      return *this;
    }
  };

  /// @brief Individually-addressed 8-bit timer core.
  /// TimerCore (above) maps a single contiguous Regs struct onto one BASE
  /// address — that only works when a chip's TCCRA/TCCRB/TCNT/OCRA/OCRB
  /// registers sit at consecutive addresses in that exact order, as they
  /// do on ATmega. ATtiny TC0 registers are scattered, interleaved with
  /// other peripherals' registers (e.g. ATtiny85/45's Timer1), so each
  /// register needs its own address — hence separate NTTPs here instead
  /// of one BASE.
  /// WGM/COM/CS bit positions within TCCRA/TCCRB are the standard AVR
  /// 8-bit-timer layout (portable across chips); only the addresses and
  /// the TOV/TOIE/OCIEA bit positions vary per chip (ATtiny85/45 share
  /// one TIFR/TIMSK with Timer1; ATtiny13 has dedicated TIFR0/TIMSK0).
  /// Same Part<O> surface as TimerCore, so Prescaler<>/SysClock<> compose
  /// unchanged.
  template<uintptr_t TCCRA_ADDR, uintptr_t TCCRB_ADDR, uintptr_t TCNT_ADDR,
           uintptr_t OCRA_ADDR, uintptr_t OCRB_ADDR,
           uintptr_t TIFR_ADDR, uintptr_t TIMSK_ADDR,
           uint8_t TOV_BIT, uint8_t TOIE_BIT, uint8_t OCIEA_BIT,
           void(*fn)() = nullptr>
  struct TimerCoreI {
    using IsTimer = std::true_type;

    inline static void (*handler_)() = nullptr;
    static void attach(void(*f)()) { handler_ = f; }
    static void act() {
      if constexpr (fn != nullptr) fn();
      if (handler_) handler_();
    }

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static volatile uint8_t& tccra()   { return *reinterpret_cast<volatile uint8_t*>(TCCRA_ADDR); }
      static volatile uint8_t& tccrb()   { return *reinterpret_cast<volatile uint8_t*>(TCCRB_ADDR); }
      static volatile uint8_t& tcnt()    { return *reinterpret_cast<volatile uint8_t*>(TCNT_ADDR); }
      static volatile uint8_t& ocra()    { return *reinterpret_cast<volatile uint8_t*>(OCRA_ADDR); }
      static volatile uint8_t& ocrb()    { return *reinterpret_cast<volatile uint8_t*>(OCRB_ADDR); }
      static volatile uint8_t& timskReg(){ return *reinterpret_cast<volatile uint8_t*>(TIMSK_ADDR); }

      // Minimal facades matching only the fields SysClock<> actually reads —
      // regs().cnt, tifr().tov, timsk().toie.
      struct RegsFacade  { RegProxy8<TCNT_ADDR> cnt; };
      struct TifrFacade  { BitProxy8<TIFR_ADDR,  TOV_BIT>  tov; };
      struct TimskFacade { BitProxy8<TIMSK_ADDR, TOIE_BIT> toie; };
      static RegsFacade  regs()  { return {}; }
      static TifrFacade  tifr()  { return {}; }
      static TimskFacade timsk() { return {}; }

      static void attach(void(*f)()) { handler_ = f; }
      static void act() {
        if constexpr (fn != nullptr) fn();
        if (handler_) handler_();
        O::act();
      }
      static void on(uint16_t fr, uint8_t scale, uint8_t /*duty*/ = 50) {
        tcnt() = 0;
        ocra() = uint8_t(fr);
        tccrb() = (tccrb() & ~uint8_t(0b111)) | scale;
      }
      static void off() {
        tccrb() &= ~uint8_t(0b111);
        tcnt() = 0;
      }
      static void intA()               { timskReg() |= uint8_t(1 << OCIEA_BIT); }
      static void setClockSource(uint8_t n) { tccrb() = (tccrb() & ~uint8_t(0b111)) | (n & 0b111); }
      static void setOutMode_A(uint8_t n)   { tccra() = (tccra() & ~uint8_t(0b11 << 6)) | uint8_t((n & 0b11) << 6); }
      static void setOutMode_B(uint8_t n)   { tccra() = (tccra() & ~uint8_t(0b11 << 4)) | uint8_t((n & 0b11) << 4); }
      static void setWaveMode(uint8_t n) {
        tccra() = (tccra() & ~uint8_t(0b11)) | (n & 0b11);
        tccrb() = (tccrb() & ~uint8_t(1 << 3)) | uint8_t(((n >> 2) & 1) << 3);
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
    static constexpr uint8_t prescaleCode(int p) {
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
    static constexpr uint8_t prescaleCode(int p) {
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

    template<void(*fn)() = nullptr>
    using TC0 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc8_regs,  0x44, 0x35, 0x6E, fn>>;
    template<void(*fn)() = nullptr>
    using TC1 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x80, 0x36, 0x6F, fn>>;
    template<void(*fn)() = nullptr>
    using TC2 = hapi::APIOf<TimerAPI<>, Prescaler<CS2Policy>, TimerCore<tc8_regs,  0xB0, 0x37, 0x70, fn>>;

  } // mega

  namespace mega2560 {

    template<void(*fn)() = nullptr> using TC0 = mega::TC0<fn>;
    template<void(*fn)() = nullptr> using TC1 = mega::TC1<fn>;
    template<void(*fn)() = nullptr> using TC2 = mega::TC2<fn>;
    template<void(*fn)() = nullptr>
    using TC3 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x90, 0x38, 0x71, fn>>;
    template<void(*fn)() = nullptr>
    using TC4 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0xA0, 0x39, 0x72, fn>>;
    template<void(*fn)() = nullptr>
    using TC5 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x120,0x40, 0x73, fn>>;

  } // mega2560

  namespace mega1284 {

    template<void(*fn)() = nullptr> using TC0 = mega::TC0<fn>;
    template<void(*fn)() = nullptr> using TC1 = mega::TC1<fn>;
    template<void(*fn)() = nullptr> using TC2 = mega::TC2<fn>;
    template<void(*fn)() = nullptr>
    using TC3 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>, TimerCore<tc16_regs, 0x90, 0x38, 0x71, fn>>;

  } // mega1284

  namespace tiny85 {
    // ATtiny85/45 TC0 — 8-bit, CS1Policy. Registers are scattered, not
    // contiguous like ATmega's TC0 block (interleaved with Timer1's
    // registers) — hence TimerCoreI, not TimerCore.
    //   TCCR0A=0x4A TCCR0B=0x53 TCNT0=0x52 OCR0A=0x49 OCR0B=0x48
    //   TIFR=0x58 TIMSK=0x59 — shared with Timer1: TOV0/TOIE0=bit1, OCIE0A=bit4
    //
    // No TC1 here: ATtiny85/45's Timer1 is an unrelated 8-bit PLL timer
    // (different register set, GTCCR-based PWM enable) that doesn't fit
    // this model — use attiny.h's dedicated AvrTimer1_Tiny85 / OC1B instead.
    template<void(*fn)() = nullptr>
    using TC0 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>,
                             TimerCoreI<0x4A,0x53,0x52,0x49,0x48, 0x58,0x59, 1,1,4, fn>>;
  } // tiny85

  namespace tiny45 {
    // Register-identical to tiny85 (same die family)
    template<void(*fn)() = nullptr> using TC0 = tiny85::TC0<fn>;
  } // tiny45

  namespace tiny13 {
    // ATtiny13 TC0 — different SFR map from tiny85/45 (older/smaller
    // chip); no Timer1 on this chip at all.
    //   TCCR0A=0x4F TCCR0B=0x53 TCNT0=0x52 OCR0A=0x56 OCR0B=0x49
    //   TIFR0=0x58 TIMSK0=0x59 — dedicated to TC0: TOV0/TOIE0=bit1, OCIE0A=bit2
    template<void(*fn)() = nullptr>
    using TC0 = hapi::APIOf<TimerAPI<>, Prescaler<CS1Policy>,
                             TimerCoreI<0x4F,0x53,0x52,0x56,0x49, 0x58,0x59, 1,1,2, fn>>;
  } // tiny13

}} // hw::avr

// ============================================================
// Macro-selected chip aliases (Arduino-style, optional)
// mirrors the 2014 #ifdef block
// ============================================================

#if !defined(HW_AVR_CHIP_ALIAS_DEFINED)
  #define HW_AVR_CHIP_ALIAS_DEFINED
  #if defined(__AVR_ATmega640__)  || defined(__AVR_ATmega1280__) || \
      defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || \
      defined(__AVR_ATmega2561__)
    namespace hw { namespace avr { namespace chip = mega2560; }}
  #elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
    namespace hw { namespace avr { namespace chip = mega1284; }}
  #elif defined(__AVR_ATtiny85__)
    namespace hw { namespace avr { namespace chip = tiny85; }}
  #elif defined(__AVR_ATtiny45__)
    namespace hw { namespace avr { namespace chip = tiny45; }}
  #elif defined(__AVR_ATtiny13__)
    namespace hw { namespace avr { namespace chip = tiny13; }}
  #else
    namespace hw { namespace avr { namespace chip = mega; }}
  #endif
#endif

// ============================================================
// Usage:
//
//   chip::TC1<>::play(5, 10);            // 5 Hz, 10% duty (no ISR)
//   chip::TC1<>::intA();                 // enable compare-A interrupt
//
//   void myTick() { ... }
//   using MyTC1 = chip::TC1<myTick>;    // compile-time binding
//   ISR(TIMER1_COMPA_vect) { MyTC1::act(); }
//
//   chip::TC1<>::attach(myTick);        // runtime binding
//   ISR(TIMER1_COMPA_vect) { chip::TC1<>::act(); }
//
//   // explicit chip family:
//   hw::avr::mega2560::TC3<>::play(440);
// ============================================================
