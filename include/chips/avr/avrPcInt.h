/**
 * @file avrPcInt.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR Pin Change Interrupt HAPI components.
 *        Follows the same pattern as avrTC.h: register addresses as template params,
 *        concrete chip aliases in mega:: / mega2560:: / mega1284::, chip:: alias at end.
 */

#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <stdint.h>
#else
  #include <cstdint>
#endif

namespace hw {
namespace avr {

  using Addr = uintptr_t;
  using Unit = unsigned char;

  // ============================================================
  // PcIntCore — HAPI component for one PCINT group
  //
  // Template params:
  //   Group      — PCINT group index (0, 1, 2 …)
  //   PCMSK_ADDR — address of this group's Pin Change Mask Register
  //   PCICR_ADDR — address of the Pin Change Interrupt Control Register (shared)
  //   PCIE_BIT   — bit position of this group's enable bit in PCICR
  //   fn         — compile-time function (NTTP, void(*)(), default nullptr)
  //
  // Two binding mechanisms coexist:
  //   Compile-time: fn NTTP — baked into the type, zero overhead.
  //   Runtime:      fn slot — set via attach(), one pointer per group.
  //
  // act() fires fn (if non-null) then fn (if set).
  // ISR calls act() on the bound type:
  //   ISR(PCINT0_vect) { chip::PcInt0<myHandler>::act(); }
  // ============================================================

  template<uint8_t Group, Addr PCMSK_ADDR, Addr PCICR_ADDR, uint8_t PCIE_BIT,
           void(*fn)() = nullptr>
  struct PcIntCore {
    using IsPcInt = std::true_type;
    static constexpr uint8_t group = Group;

    inline static void (*handler_)() = nullptr;

    static void attach(void (*f)()) { handler_ = f; }

    static void act() {
      if constexpr (fn != nullptr) fn();
      if (handler_) handler_();
    }

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static volatile Unit& pcmsk() { return *reinterpret_cast<volatile Unit*>(PCMSK_ADDR); }
      static volatile Unit& pcicr() { return *reinterpret_cast<volatile Unit*>(PCICR_ADDR); }

      static void enable(Unit mask) {
        pcmsk() |= mask;
        pcicr() |= static_cast<Unit>(1 << PCIE_BIT);
      }
      static void disable(Unit mask) {
        pcmsk() &= ~mask;
        if (!pcmsk()) pcicr() &= ~static_cast<Unit>(1 << PCIE_BIT);
      }

      static void act() {
        if constexpr (fn != nullptr) fn();
        if (handler_) handler_();
        O::act();
      }
    };
  };

  // ============================================================
  // Concrete chip descriptors — PCMSK/PCICR addresses from ATmega datasheets.
  // ATmega328/168/88 and compatible: 3 PCINT groups.
  //   PCICR  @ 0x68 (shared)
  //   PCMSK0 @ 0x6B — group 0, PortB, Arduino pins 8–13
  //   PCMSK1 @ 0x6C — group 1, PortC, Arduino A0–A5
  //   PCMSK2 @ 0x6D — group 2, PortD, Arduino pins 0–7
  // ============================================================

  namespace mega {
    template<void(*fn)() = nullptr> using PcInt0 = PcIntCore<0, 0x6B, 0x68, 0, fn>;
    template<void(*fn)() = nullptr> using PcInt1 = PcIntCore<1, 0x6C, 0x68, 1, fn>;
    template<void(*fn)() = nullptr> using PcInt2 = PcIntCore<2, 0x6D, 0x68, 2, fn>;
  }

  namespace mega2560 {
    template<void(*fn)() = nullptr> using PcInt0 = mega::PcInt0<fn>;
    template<void(*fn)() = nullptr> using PcInt1 = mega::PcInt1<fn>;
    template<void(*fn)() = nullptr> using PcInt2 = mega::PcInt2<fn>;
  }

  namespace mega1284 {
    template<void(*fn)() = nullptr> using PcInt0 = mega::PcInt0<fn>;
    template<void(*fn)() = nullptr> using PcInt1 = mega::PcInt1<fn>;
    template<void(*fn)() = nullptr> using PcInt2 = mega::PcInt2<fn>;
  }

}} // hw::avr

// ============================================================
// chip::PcInt0/1/2 — macro-selected aliases, mirrors avrTC.h convention.
// ============================================================

#if !defined(HW_AVR_CHIP_ALIAS_DEFINED)
  #define HW_AVR_CHIP_ALIAS_DEFINED
  #if defined(__AVR_ATmega640__)  || defined(__AVR_ATmega1280__) || \
      defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || \
      defined(__AVR_ATmega2561__)
    namespace hw { namespace avr { namespace chip = mega2560; }}
  #elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
    namespace hw { namespace avr { namespace chip = mega1284; }}
  #else
    namespace hw { namespace avr { namespace chip = mega; }}
  #endif
#endif

// ============================================================
// Usage:
//
//   void myHandler() { btn.act(); }
//
//   // compile-time binding (zero overhead):
//   using MyPcInt0 = chip::PcInt0<myHandler>;
//   ISR(PCINT0_vect) { MyPcInt0::act(); }
//   MyPcInt0 pcint0;
//   pcint0.enable(1 << 5);
//
//   // runtime binding (one pointer per group):
//   chip::PcInt0<> pcint0;
//   pcint0.attach(myHandler);
//   ISR(PCINT0_vect) { chip::PcInt0<>::act(); }
//
//   // both:
//   chip::PcInt0<staticHandler>::attach(dynamicHandler);
// ============================================================
