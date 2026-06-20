/**
 * @file avrPort.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR GPIO port HAPI components — hardware implementations of the OnePin API.
 *        Use with onePin::InPin / OutPin / IOPin as the terminal.
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
  // Port components — HAPI building blocks
  // Each wraps a single AVR I/O register by compile-time address.
  // ============================================================

  // AVRPort<pIn, ddr, port, AllowedMask>: maps PIN/DDR/PORT registers for one port.
  // ATmega memory map: PIN=base, DDR=base+1, PORT=base+2.
  // AllowedMask: which pins this port instance owns (default all 8). Used by PortAlloc.
  template<Addr pIn, Addr pDdr = pIn+1, Addr pOut = pIn+2, Unit AllowedMask = 0xFF>
  struct AVRPort {
    using is_avr_port = std::true_type;
    using Unit = ::hw::avr::Unit;
    static constexpr Unit allowedMask = AllowedMask;
    template<Unit NewMask>
    using rebind = AVRPort<pIn, pDdr, pOut, NewMask>;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static volatile Unit& pin_reg()  { return *reinterpret_cast<volatile Unit*>(pIn); }
      static volatile Unit& ddr_reg()  { return *reinterpret_cast<volatile Unit*>(pDdr); }
      static volatile Unit& port_reg() { return *reinterpret_cast<volatile Unit*>(pOut); }

      static Unit pin()        { return pin_reg(); }   // input state (PIN register)
      static Unit port()       { return port_reg(); }  // output latch (PORT register)
      static void port(Unit v) { port_reg() = v; }     // whole-port write
      static Unit ddr()           { return ddr_reg(); }  // direction latch read
      static void dir_out(Unit m) { ddr_reg() |=  m; }  // set pins as output (atomic SBI)
      static void dir_in(Unit m)  { ddr_reg() &= ~m; }  // set pins as input  (atomic CBI)
      static void dir(Unit m)     { ddr_reg()  =  m; }  // whole-port direction write
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
//   using Led = APIOf<AvrOutPin, chip::PortB>;
//   Led led;
//   led.dir(1<<5);   // PB5 as output
//   led.on(1<<5);    // PB5 high
//   led.off(1<<5);   // PB5 low
//   led.put(0xFF);   // whole port high
// ============================================================
