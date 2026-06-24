/**
 * @file avrSysClock.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief millis()/micros() via any 8-bit AVR timer — IOP/HAPI Boot component.
 *
 * SysClock<CSPolicy, PrescaleValue, CpuHz>
 *   HAPI component — layers above TimerCore<tc8_regs,...> in the chain.
 *   Provides begin(), onOverflow(), millis(), micros().
 *
 * Typical chain:
 *   chip::SysTick0<>  ==  APIOf<BootDef, SysClock<CS1Policy,64,16MHz>, TimerCore<tc8_regs,0x44,0x35,0x6E>>
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
  // SysClock<CSPolicy, PrescaleValue, CpuHz>
  // HAPI component — hardware access via O::regs() / O::timsk() from TimerCore below.
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
      inline static volatile uint32_t _overflow_count = 0;  // raw count for micros()

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
        _overflow_count++;
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
        uint32_t ov;
        uint8_t  t;
#  ifdef __AVR__
        uint8_t sreg = SREG; cli();
        ov = _overflow_count;
        t  = Base::regs().cnt;
        if (Base::tifr().tov && t < 255) ov++;
        SREG = sreg;
#  else
        ov = _overflow_count;
        t  = 0;
#  endif
        return (ov * 256UL + t) * US_PER_TICK;
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
  // Chip-family SysTick aliases
  // Addresses match mega/mega2560/mega1284 layouts in avrTC.h.
  // ============================================================
  namespace mega {
    // TC0 — 8-bit, CS1Policy, addresses: TCCR0A=0x44 TIFR0=0x35 TIMSK0=0x6E
    template<uint32_t CpuHz = 16000000UL>
    using SysTick0 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS1Policy, 64, CpuHz>,
                                  TimerCore<tc8_regs, 0x44, 0x35, 0x6E>>;
    // TC2 — 8-bit, CS2Policy (different prescaler table), addresses: TCCR2A=0xB0 TIFR2=0x37 TIMSK2=0x70
    template<uint32_t CpuHz = 16000000UL>
    using SysTick2 = hapi::APIOf<onePin::BootDef,
                                  SysClock<CS2Policy, 64, CpuHz>,
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

}} // hw::avr

// Wires a timer overflow ISR to Board::onOverflow().
// Place once in the user's main translation unit:
//   IOP_TIMER0_ISR(Board)
//   IOP_TIMER2_ISR(Board)
#define IOP_TIMER0_ISR(board_t) ISR(TIMER0_OVF_vect) { board_t::onOverflow(); }
#define IOP_TIMER2_ISR(board_t) ISR(TIMER2_OVF_vect) { board_t::onOverflow(); }

// chip::SysTick0<> / SysTick2<> resolve via the existing namespace alias
// (chip = mega / mega2560 / mega1284) — no extra declarations needed.

// ============================================================
// Usage (Arduino Uno / ATmega328P, Timer0):
//
//   #include <chips/avr/avrSysClock.h>
//   using namespace hw::avr; using namespace onePin;
//
//   using SysTick = chip::SysTick0<>;
//   using Led     = APIOf<AvrOutPin, Mask<Pins<5>>, chip::PortB>;
//   using Board   = Device<Boot<SysTick>, Led>;
//
//   ISR(TIMER0_OVF_vect) { SysTick::onOverflow(); }
//
//   void setup() { Board::begin(); sei(); }
//   void loop()  { if (SysTick::millis() - t0 >= 500) { led.on(); ... } }
//
// To use Timer2 instead (frees Timer0 for PWM):
//   using SysTick = chip::SysTick2<>;
//   ISR(TIMER2_OVF_vect) { SysTick::onOverflow(); }
// ============================================================
