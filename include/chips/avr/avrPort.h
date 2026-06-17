/**
 * @file avrPort.h
 * @author Rui Azevedo (neu-rah) (ruihfazevedo@gmail.com)
 * @brief AVR GPIO port HAPI components — hardware implementations of the OnePin API.
 *        Use with OnePin's PinAPI as the terminal.
 */

#pragma once
#include <hapi/hapi.h>

#ifdef __AVR__
  #include <Arduino.h>
#else
  #include <cstdint>
  using byte = unsigned char;
#endif

namespace hw {
namespace avr {

  using Addr = uintptr_t;
  using Unit = unsigned char;

  // ============================================================
  // Port components — HAPI building blocks
  // Each wraps a single AVR I/O register by compile-time address.
  // ============================================================

  template<Addr pIn>
  struct AVRPortRead {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static Unit get() { return *reinterpret_cast<volatile Unit*>(pIn); }
    };
  };

  template<Addr ddr>
  struct AVRPortDir {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static void dir(Unit bits) { *reinterpret_cast<volatile Unit*>(ddr) = bits; }
    };
  };

  template<Addr port>
  struct AVRPortWrite {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;
      static void set(Unit val) { *reinterpret_cast<volatile Unit*>(port) = val; }
    };
  };

  // AVRPort<pIn, ddr, port>: composes read/dir/write for one port.
  // ATmega memory map: PIN=base, DDR=base+1, PORT=base+2.
  template<Addr pIn, Addr ddr = pIn+1, Addr port = pIn+2>
  struct AVRPort {
    using Chain_ = hapi::Chain<AVRPortRead<pIn>, AVRPortDir<ddr>, AVRPortWrite<port>>;
    template<typename O>
    struct Part : Chain_::template Part<O> {
      using Base = typename Chain_::template Part<O>;
      using Base::Base;
    };
  };

  // ============================================================
  // Concrete port descriptors per chip family
  // ============================================================

  namespace mega {
    // ATmega328/168/88 and compatible (Uno, Nano, Pro Mini)
    using PortB = AVRPort<0x23>;  // PIN=0x23, DDR=0x24, PORT=0x25
    using PortC = AVRPort<0x26>;  // PIN=0x26, DDR=0x27, PORT=0x28
    using PortD = AVRPort<0x29>;  // PIN=0x29, DDR=0x2A, PORT=0x2B
  }

  namespace mega2560 {
    // ATmega640/1280/2560 — B/C/D same as mega, plus E..L
    using PortB = mega::PortB;
    using PortC = mega::PortC;
    using PortD = mega::PortD;
    using PortE = AVRPort<0x2C>;  // PIN=0x2C, DDR=0x2D, PORT=0x2E
    using PortF = AVRPort<0x2F>;  // PIN=0x2F, DDR=0x30, PORT=0x31
    using PortG = AVRPort<0x32>;  // PIN=0x32, DDR=0x33, PORT=0x34
    using PortH = AVRPort<0x100>; // PIN=0x100, DDR=0x101, PORT=0x102
    using PortJ = AVRPort<0x103>;
    using PortK = AVRPort<0x106>;
    using PortL = AVRPort<0x109>;
  }

  namespace mega1284 {
    // ATmega1284/644 — has PortA in addition to B/C/D
    using PortA = AVRPort<0x20>;  // PIN=0x20, DDR=0x21, PORT=0x22
    using PortB = mega::PortB;
    using PortC = mega::PortC;
    using PortD = mega::PortD;
  }

}} // hw::avr

// ============================================================
// chip::PortB/C/D — mirrors the alias in avrTC.h
// If avrTC.h is included first, no redefinition needed here.
// ============================================================

#if !defined(HW_AVR_CHIP_ALIAS_DEFINED)
  #define HW_AVR_CHIP_ALIAS_DEFINED
  #if defined(__AVR_ATmega640__)  || defined(__AVR_ATmega1280__) || \
      defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || \
      defined(__AVR_ATmega2561__)
    namespace hw { namespace avr { namespace chip = mega2560; }}
  #elif defined(__AVR_ATmega1284__)
    namespace hw { namespace avr { namespace chip = mega1284; }}
  #else
    namespace hw { namespace avr { namespace chip = mega; }}
  #endif
#endif

// ============================================================
// Usage:
//
//   using namespace onePin;
//   using namespace hw::avr;
//
//   using Led = APIOf<PinAPI, chip::PortB>;
//   Led led;
//   led.dir(1<<5);   // PB5 output
//   led.set(1<<5);   // PB5 high
//   led.set(0);      // PB5 low
// ============================================================
