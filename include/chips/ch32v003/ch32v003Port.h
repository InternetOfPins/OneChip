/**
 * @file ch32v003Port.h
 * @brief CH32V003 GPIO port HAPI components — hardware implementation of the OnePin API.
 *        Use with onePin::InPin / OutPin / IOPin as the terminal (see onePin.h's API
 *        contract comment: pin()/port()/port(Unit)/dir(Unit)).
 *
 * Unlike AVR's simple 1-bit-per-pin DDR register, CH32V003's CFGLR packs 4 bits/pin
 * (CNF[1:0]:MODE[1:0], STM32F1-shaped) — direction setup needs real per-pin nibble writes,
 * not a plain mask OR/AND. Values used (from WCH's Standard Peripheral Library, same
 * encoding as STM32F1's GPIO CRL/CRH):
 *   output, push-pull, 10MHz : MODE=01 CNF=00 -> nibble 0x1
 *   input,  floating         : MODE=00 CNF=01 -> nibble 0x4
 *
 * Register addresses used directly (no vendor struct dependency), matching avrPort.h's
 * style of reinterpreting raw compile-time addresses rather than depending on the SDK's
 * GPIO_TypeDef/RCC_TypeDef layout — confirmed against ch32v00x.h's own #defines:
 *   GPIOA_BASE=0x40010800 GPIOC_BASE=0x40011000 GPIOD_BASE=0x40011400
 *   RCC_BASE=0x40021000, APB2PCENR at RCC_BASE+0x18 (IOPAEN=1<<2 IOPCEN=1<<4 IOPDEN=1<<5)
 *   GPIO block: CFGLR=+0x00 INDR=+0x08 OUTDR=+0x0C BSHR=+0x10 BCR=+0x14
 */
#pragma once
#include <hapi/hapi.h>

#ifdef __riscv
  #include <stdint.h>
#else
  #include <cstdint>
#endif

namespace hw {
namespace ch32v003 {

  using Addr = uintptr_t;
  using Unit = unsigned char;  // ports have at most 8 pins (PA1/PA2, PC0-7, PD0-7)

  inline volatile uint32_t& reg(Addr a) { return *reinterpret_cast<volatile uint32_t*>(a); }

  static constexpr Addr RCC_APB2PCENR = 0x40021018;

  // Ch32v003Port<GpioBase, RccEnableBit, AllowedMask>: one GPIOx block + its RCC enable bit.
  template<Addr GpioBase, uint32_t RccEnableBit, Unit AllowedMask = 0xFF>
  struct Ch32v003Port {
    using is_ch32v003_port = std::true_type;
    using Unit = ::hw::ch32v003::Unit;
    static constexpr Unit allowedMask = AllowedMask;
    static constexpr Addr base = GpioBase;
    template<Unit NewMask>
    using rebind = Ch32v003Port<GpioBase, RccEnableBit, NewMask>;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static void begin() { enableClock(); Base::begin(); }
      static void enableClock() { reg(RCC_APB2PCENR) |= RccEnableBit; }

      static Unit pin()        { return (Unit)reg(base + 0x08); }              // INDR
      static Unit port()       { return (Unit)reg(base + 0x0C); }              // OUTDR
      static void port(Unit v) { reg(base + 0x0C) = v; }                       // OUTDR whole write

      static Unit ddr() { return 0; }  // no single-register direction readback (4 bits/pin); unused by Mask<>

      static void dir_out(Unit m) { setCfg(m, 0x1); }  // output, push-pull, 10MHz
      static void dir_in(Unit m)  { setCfg(m, 0x4); }  // input, floating
      static void dir(Unit m)     { dir_out(m); }       // whole-port direction write: AVRPort's dir() sets
                                                          // all bits as output — CH32V003 has no cheaper
                                                          // single-register equivalent, so per-pin loop it too

    private:
      // Write a 4-bit CFGLR nibble (mode|cnf) to every pin set in mask; leaves other pins untouched.
      static void setCfg(Unit m, uint32_t nibble) {
        uint32_t cfg = reg(base);
        for (uint8_t i = 0; i < 8; i++) {
          if (m & (1u << i)) {
            cfg &= ~(0xFu << (i * 4));
            cfg |= nibble << (i * 4);
          }
        }
        reg(base) = cfg;
      }
    };
  };

  // ============================================================
  // Concrete port descriptors — CH32V003 has PA (only pins 1/2 bonded out), PC, PD.
  // ============================================================
  namespace ch32v003f4 {
    using PortA = Ch32v003Port<0x40010800, (1u << 2)>;  // GPIOA_BASE, RCC IOPAEN
    using PortC = Ch32v003Port<0x40011000, (1u << 4)>;  // GPIOC_BASE, RCC IOPCEN
    using PortD = Ch32v003Port<0x40011400, (1u << 5)>;  // GPIOD_BASE, RCC IOPDEN
  }

}} // hw::ch32v003

// chip:: alias, mirrors avrPort.h's convention
namespace hw { namespace ch32v003 { namespace chip = ch32v003f4; }}

// ============================================================
// Usage:
//
//   using namespace onePin;
//   using namespace hw::ch32v003;
//
//   using Led = APIOf<OutPin<Unit>, Out, oneBit::Mask<oneBit::Pins<0>>, chip::PortC>;
//   Led::begin();  // dir_out() called via Out, RCC clock enabled via Part::begin()
//   Led::on();
// ============================================================
