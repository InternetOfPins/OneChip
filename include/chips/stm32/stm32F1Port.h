#pragma once
#include <hapi/hapi.h>
#include <cstdint>

namespace hw {
namespace stm32 {

  using Addr = uintptr_t;
  using Unit = uint32_t;

  // STM32F1 GPIO registers — CRL/CRH config layout (not MODER like F4/L4/H7)
  struct f1_gpio_regs {
    volatile uint32_t crl;   // 0x00 — pins 0-7:  [CNF1:CNF0:MODE1:MODE0] per pin
    volatile uint32_t crh;   // 0x04 — pins 8-15: same layout
    volatile uint32_t idr;   // 0x08 — input data
    volatile uint32_t odr;   // 0x0C — output latch
    volatile uint32_t bsrr;  // 0x10 — set (bits 0-15) / reset (bits 16-31)
    volatile uint32_t brr;   // 0x14 — reset only
    volatile uint32_t lckr;  // 0x18 — lock
  };

  // Constexpr helpers — operate on one CRL or CRH half (pins start..start+7)
  // MODE=01 (10 MHz output), CNF=00 (push-pull) → field value 0x1
  static constexpr uint32_t f1_crx_clear(uint32_t pins, int start) {
    uint32_t m = 0;
    for (int i = start; i < start + 8; i++)
      if (pins & (1u << i)) m |= (0xFu << ((i - start) * 4));
    return m;
  }
  static constexpr uint32_t f1_crx_out(uint32_t pins, int start) {
    uint32_t m = 0;
    for (int i = start; i < start + 8; i++)
      if (pins & (1u << i)) m |= (0x1u << ((i - start) * 4));
    return m;
  }
  static constexpr uint32_t f1_crx_in(uint32_t pins, int start) {
    uint32_t m = 0;
    for (int i = start; i < start + 8; i++)
      if (pins & (1u << i)) m |= (0x4u << ((i - start) * 4));  // floating input
    return m;
  }

  // ============================================================
  // STM32F1Port<BASE, RCC_APB2ENR, RCC_BIT, AllowedMask>
  // ============================================================
  template<Addr BASE, Addr RCC_APB2ENR_ADDR, uint8_t RCC_BIT, Unit AllowedMask = 0xFFFF>
  struct STM32F1Port {
    using is_stm32_port = std::true_type;
    using Unit = ::hw::stm32::Unit;
    static constexpr Unit allowedMask = AllowedMask;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static f1_gpio_regs& regs() { return *reinterpret_cast<f1_gpio_regs*>(BASE); }

      static void clockEnable() {
        *reinterpret_cast<volatile uint32_t*>(RCC_APB2ENR_ADDR) |= (1u << RCC_BIT);
      }

      static Unit pin()        { return Unit(regs().idr); }
      static Unit port()       { return Unit(regs().odr); }
      static void port(Unit v) { regs().odr = v; }

      static void bsrr_set(Unit mask) { regs().bsrr = mask; }
      static void bsrr_clr(Unit mask) { regs().bsrr = mask << 16; }

      static void dir_out(Unit mask) {
        clockEnable();
        regs().crl = (regs().crl & ~f1_crx_clear(mask,0)) | f1_crx_out(mask,0);
        regs().crh = (regs().crh & ~f1_crx_clear(mask,8)) | f1_crx_out(mask,8);
      }
      static void dir_in(Unit mask) {
        clockEnable();
        regs().crl = (regs().crl & ~f1_crx_clear(mask,0)) | f1_crx_in(mask,0);
        regs().crh = (regs().crh & ~f1_crx_clear(mask,8)) | f1_crx_in(mask,8);
      }

      static Unit ddr() { return 0; }
      static void dir(Unit mask) { dir_out(mask); }
    };
  };

  // ============================================================
  // STM32F1 family namespace — GPIO on APB2, RCC APB2ENR @ 0x40021018
  // APB2ENR bits: IOPAEN=2, IOPBEN=3, IOPCEN=4, IOPDEN=5, IOPEEN=6
  // ============================================================
  namespace f1 {
    static constexpr Addr RCC_APB2ENR = 0x40021018u;
    template<Addr BASE, uint8_t BIT>
    using Port_ = STM32F1Port<BASE, RCC_APB2ENR, BIT>;
    using PortA = Port_<0x40010800u, 2>;
    using PortB = Port_<0x40010C00u, 3>;
    using PortC = Port_<0x40011000u, 4>;
    using PortD = Port_<0x40011400u, 5>;
    using PortE = Port_<0x40011800u, 6>;
  }

}} // hw::stm32

// ============================================================
// hw::stm32::chip — set to f1 when STM32F1xx is detected.
// Include stm32Port.h for F4/L4/H7 (uses MODER).
// ============================================================
#if !defined(HW_STM32_CHIP_ALIAS_DEFINED)
  #define HW_STM32_CHIP_ALIAS_DEFINED
  namespace hw { namespace stm32 { namespace chip = f1; }}
#endif
