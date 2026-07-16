/**
 * @file avrSysClock.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief millis()/micros() via any 8-bit AVR timer — IOP/HAPI Boot component.
 *
 * SysClock<CSPolicy, PrescaleValue, CpuHz>
 *   HAPI component — layers above TimerCore<tc8_regs,...> in the chain.
 *   Provides begin(), onOverflow(), millis().
 *
 * OverflowCounter
 *   Optional HAPI component — when composed, enables micros() precision.
 *   Omit to save 4 bytes RAM (millis-only mode).
 *
 * Typical chains:
 *   Full (with micros):  APIOf<BootDef, SysClock<...>, OverflowCounter, TimerCore<...>>
 *   Millis-only:         APIOf<BootDef, SysClock<...>, TimerCore<...>>
 *
 * User platform file owns the ISR:
 *   using SysTick = chip::SysTick0<>;
 *   ISR(TIMER0_OVF_vect) { SysTick::onOverflow(); }
 *
 * Device integration:
 *   using Board = Device<Boot<SysTick>, Led, Btn>;
 *   // Board::begin() calls SysTick::begin() before GPIO peripherals
 */

#pragma once
#include <chips/avr/avrTC.h>
#include <onePin/onePin.h>
#include <hapi/hapi.h>
#ifdef __AVR__
  #include <avr/interrupt.h>
#endif

#ifndef IOP
extern "C" unsigned long millis();
extern "C" unsigned long micros();
extern "C" void init();
#endif

namespace hw {
namespace avr {

  // ============================================================
  // OverflowCounter — optional HAPI component for microsecond precision
  // Compose into your SysTick chain to enable micros().
  // ============================================================
  struct OverflowCounter {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      inline static volatile uint32_t _overflow_count = 0;

      // Hook: called by SysClock::onOverflow() if present in chain
      static void recordOverflow() {
        _overflow_count++;
      }
    };
  };

  // ============================================================
  // SysClock<CSPolicy, PrescaleValue, CpuHz>
  // HAPI component — provides millis tracking + conditional micros.
  // Detects OverflowCounter in chain; adapts behavior accordingly.
  // ============================================================
  template<typename CSPolicy  = CS1Policy,
           uint8_t  PrescaleValue = 64,
           uint32_t CpuHz         = 16000000UL>
  struct SysClock {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      // timing constants visible through the chain type
      static constexpr uint8_t  PRESCALE_CODE = CSPolicy::prescaleCode(PrescaleValue);
      static constexpr uint32_t US_PER_OVF    =
        static_cast<uint32_t>(uint64_t(PrescaleValue) * 256 * 1000000 / CpuHz);
      static constexpr uint32_t MS_PER_OVF    = US_PER_OVF / 1000;
      static constexpr uint32_t FRAC_INC      = US_PER_OVF % 1000;
      static constexpr uint32_t US_PER_TICK   = PrescaleValue / (CpuHz / 1000000UL);

      inline static volatile uint32_t _ms             = 0;
      inline static volatile uint16_t _fract          = 0;

      // HAPI trait: detect if OverflowCounter is in the chain
      template<typename T, typename = void>
      struct has_recordOverflow : std::false_type {};

      template<typename T>
      struct has_recordOverflow<T, std::void_t<decltype(std::declval<T>().recordOverflow())>>
        : std::true_type {};

      static constexpr bool HAS_OVERFLOW_COUNTER = has_recordOverflow<Base>::value;

#ifndef IOP
      static uint32_t millis() { return ::millis(); }
      static uint32_t micros() { return ::micros(); }
      static void begin()      { ::init(); }
      static void onOverflow() {}
#else
      static void begin() {
        Base::setWaveMode(0);
        Base::setClockSource(PRESCALE_CODE);
        Base::timsk().toie = 1;
        Base::begin();
      }

      static void onOverflow() {
        // Conditionally update overflow counter if component is in chain
        if constexpr (HAS_OVERFLOW_COUNTER) {
          Base::recordOverflow();
        }

        uint32_t m = _ms;
        uint16_t f = _fract;
        m += MS_PER_OVF;
        f += FRAC_INC;
        if (f >= 1000) { f -= 1000; m++; }
        _ms    = m;
        _fract = f;
      }

      static uint32_t millis() {
        uint32_t m;
#  ifdef __AVR__
        uint8_t sreg = SREG; cli();
        m = _ms;
        SREG = sreg;
#  else
        m = _ms;
#  endif
        return m;
      }

      static uint32_t micros() {
        if constexpr (HAS_OVERFLOW_COUNTER) {
          // Full precision via overflow counter
          uint32_t ov;
          uint8_t  t;
#  ifdef __AVR__
          uint8_t sreg = SREG; cli();
          ov = Base::_overflow_count;
          t  = Base::regs().cnt;
          if (Base::tifr().tov && t < 255) ov++;
          SREG = sreg;
#  else
          ov = Base::_overflow_count;
          t  = 0;
#  endif
          return (ov * 256UL + t) * US_PER_TICK;
        } else {
          // Millis-only: return millis * 1000 (no sub-millisecond precision)
          return millis() * 1000UL;
        }
      }
#endif

      // Period<ms> — fires every ms milliseconds, uses this clock's millis().
      // Wraps at ~49 days (uint32_t), same limit as millis().
      template<uint32_t ms>
      struct Period {
        uint32_t last = 0;
        bool operator()() {
          uint32_t now = millis();
          if (now - last < ms) return false;
          last = now;
          return true;
        }
        explicit operator bool() { return operator()(); }
        void     reset() { last = millis(); }
        uint32_t when()  const { return last + ms; }
      };

      template<uint32_t timeOn, uint32_t timeOff = timeOn>
      struct Blink {
        bool operator()() const {
          return millis() % (timeOn + timeOff) < timeOn;
        }
      };
    };
  };

  // ============================================================
  // Chip-family SysTick aliases — users compose as needed
  // ============================================================
  namespace mega {
    // TC0 — 8-bit, CS1Policy, addresses: TCCR0A=0x44 TIFR0=0x35 TIMSK0=0x6E
    // Default: with OverflowCounter (full precision)
    template<uint32_t CpuHz = 16000000UL>
    using SysTick0 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS1Policy, 64, CpuHz>,
                                  OverflowCounter,
                                  TimerCore<tc8_regs, 0x44, 0x35, 0x6E>>;
    // TC2 — 8-bit, CS2Policy (different prescaler table), addresses: TCCR2A=0xB0 TIFR2=0x37 TIMSK2=0x70
    template<uint32_t CpuHz = 16000000UL>
    using SysTick2 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS2Policy, 64, CpuHz>,
                                  OverflowCounter,
                                  TimerCore<tc8_regs, 0xB0, 0x37, 0x70>>;
  }

  namespace mega2560 {
    template<uint32_t CpuHz = 16000000UL> using SysTick0 = mega::SysTick0<CpuHz>;
    template<uint32_t CpuHz = 16000000UL> using SysTick2 = mega::SysTick2<CpuHz>;
  }

  namespace mega1284 {
    template<uint32_t CpuHz = 16000000UL> using SysTick0 = mega::SysTick0<CpuHz>;
    template<uint32_t CpuHz = 16000000UL> using SysTick2 = mega::SysTick2<CpuHz>;
  }

  namespace tiny85 {
    // TC0 — 8-bit, CS1Policy
    // Addresses differ from mega: TCCR0A=0x4A TIFR0=0x35 TIMSK0=0x6E
    // Default: millis-only (no OverflowCounter, saves 4 bytes RAM)
    template<uint32_t CpuHz = 8000000UL>
    using SysTick0 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS1Policy, 64, CpuHz>,
                                  TimerCore<tc8_regs, 0x4A, 0x35, 0x6E>>;
    // Opt-in to full precision
    template<uint32_t CpuHz = 8000000UL>
    using SysTick0Full = hapi::APIOf<onePin::BootDef,
                                      SysClock<CS1Policy, 64, CpuHz>,
                                      OverflowCounter,
                                      TimerCore<tc8_regs, 0x4A, 0x35, 0x6E>>;
  }

  namespace tiny45 {
    // Identical register layout to tiny85 (same die family)
    // Default: millis-only (saves 4 bytes RAM)
    template<uint32_t CpuHz = 8000000UL>
    using SysTick0 = tiny85::SysTick0<CpuHz>;
    // Opt-in to full precision
    template<uint32_t CpuHz = 8000000UL>
    using SysTick0Full = tiny85::SysTick0Full<CpuHz>;
  }

  namespace tiny13 {
    // Identical register layout to tiny85 (same Timer0 addresses)
    // Default: millis-only (saves 4 bytes RAM)
    template<uint32_t CpuHz = 9600000UL>
    using SysTick0 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS1Policy, 64, CpuHz>,
                                  TimerCore<tc8_regs, 0x4A, 0x35, 0x6E>>;
    // Opt-in to full precision
    template<uint32_t CpuHz = 9600000UL>
    using SysTick0Full = hapi::APIOf<onePin::BootDef,
                                      SysClock<CS1Policy, 64, CpuHz>,
                                      OverflowCounter,
                                      TimerCore<tc8_regs, 0x4A, 0x35, 0x6E>>;
  }

}} // hw::avr

// Wires a timer overflow ISR to Board::onOverflow().
// Place once in the user's main translation unit:
//   IOP_TIMER0_ISR(Board)
//   IOP_TIMER2_ISR(Board)
#define IOP_TIMER0_ISR(board_t) ISR(TIMER0_OVF_vect) { board_t::onOverflow(); }
#define IOP_TIMER2_ISR(board_t) ISR(TIMER2_OVF_vect) { board_t::onOverflow(); }

// chip::SysTick0<> / SysTick2<> resolve via the existing namespace alias
// (chip = mega / mega2560 / mega1284 / tiny45 / tiny85 / tiny13)
