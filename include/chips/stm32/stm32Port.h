/**
 * @file stm32Port.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief STM32 GPIO port HAPI components.
 *        Same pattern as avrPort.h: register layout struct, STM32Port<> component,
 *        family namespaces (f4:: / l4:: / h7::), chip:: alias.
 *        No ST HAL dependency — addresses from Reference Manuals.
 *
 * Pin API contract (matches oneBit::Mask<> expectations):
 *   pin()          — input state  (IDR, lower 16 bits)
 *   port()         — output latch (ODR, lower 16 bits)
 *   port(Unit v)   — whole-port write (ODR)
 *   bsrr_set(mask) — atomic set via BSRR (bits 0-15)
 *   bsrr_clr(mask) — atomic reset via BSRR (bits 16-31)
 *   dir_out(mask)  — set pins in mask as output (MODER bits = 01)
 *   dir_in(mask)   — set pins in mask as input  (MODER bits = 00)
 *   ddr()          — virtual 1-bit-per-pin direction (1=output, from MODER)
 *   dir(Unit m)    — whole-port direction write (replaces all MODER I/O bits)
 *   clockEnable()  — enable port clock via RCC
 *
 * Peripheral config (pin-number based, separate from pin API):
 *   pull(pin, PinPull)   — PUPDR
 *   speed(pin, PinSpeed) — OSPEEDR
 *   otype(pin, PinType)  — OTYPER
 *   mode(pin, PinMode)   — MODER (for Alt/Analog, use alongside dir_out/dir_in)
 *   altFunc(pin, af)     — AFR low/high
 */

#pragma once
#include <hapi/hapi.h>
#include <cstdint>

namespace hw {
namespace stm32 {

  using Addr = uintptr_t;
  using Unit = uint32_t;   // GPIO pin masks are 16-bit; registers are 32-bit

  // ============================================================
  // GPIO register block — identical layout across all STM32 families.
  // ============================================================
  struct gpio_regs {
    volatile uint32_t moder;    // 0x00 — mode (2 bits/pin): 00=in 01=out 10=alt 11=analog
    volatile uint32_t otyper;   // 0x04 — output type (1 bit/pin): 0=push-pull 1=open-drain
    volatile uint32_t ospeedr;  // 0x08 — output speed (2 bits/pin)
    volatile uint32_t pupdr;    // 0x0C — pull (2 bits/pin): 00=none 01=up 10=down
    volatile uint32_t idr;      // 0x10 — input data (read-only, bits 0-15)
    volatile uint32_t odr;      // 0x14 — output data (bits 0-15)
    volatile uint32_t bsrr;     // 0x18 — atomic: bits 0-15 set, bits 16-31 reset
    volatile uint32_t lckr;     // 0x1C — configuration lock
    volatile uint32_t afrl;     // 0x20 — alternate function pins 0-7  (4 bits/pin)
    volatile uint32_t afrh;     // 0x24 — alternate function pins 8-15 (4 bits/pin)
    volatile uint32_t brr;      // 0x28 — bit reset (not on F4/H7)
  };

  enum class PinMode  : uint8_t { Input=0, Output=1, Alt=2, Analog=3 };
  enum class PinPull  : uint8_t { None=0, Up=1, Down=2 };
  enum class PinSpeed : uint8_t { Low=0, Medium=1, High=2, VeryHigh=3 };
  enum class PinType  : uint8_t { PushPull=0, OpenDrain=1 };

  // ============================================================
  // MODER helpers — expand 16-bit pin mask to 32-bit MODER mask
  // Constexpr so mask-based calls compile to single instructions.
  // ============================================================
  static constexpr uint32_t moder_mask(uint32_t pins) {
    uint32_t m = 0;
    for (int i = 0; i < 16; i++)
      if (pins & (1u << i)) m |= (3u << (i*2));
    return m;
  }
  static constexpr uint32_t moder_out_bits(uint32_t pins) {
    uint32_t m = 0;
    for (int i = 0; i < 16; i++)
      if (pins & (1u << i)) m |= (1u << (i*2));  // 01 = output
    return m;
  }

  // ============================================================
  // STM32Port<BASE, RCC_ENR_ADDR, RCC_EN_BIT, AllowedMask>
  // ============================================================
  template<Addr BASE, Addr RCC_ENR_ADDR, uint8_t RCC_EN_BIT, Unit AllowedMask = 0xFFFF>
  struct STM32Port {
    using is_stm32_port = std::true_type;
    using Unit = ::hw::stm32::Unit;
    static constexpr Unit allowedMask = AllowedMask;
    template<Unit NewMask>
    using rebind = STM32Port<BASE, RCC_ENR_ADDR, RCC_EN_BIT, NewMask>;

    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static gpio_regs& regs() { return *reinterpret_cast<gpio_regs*>(BASE); }

      static void clockEnable() {
        *reinterpret_cast<volatile uint32_t*>(RCC_ENR_ADDR) |= (1u << RCC_EN_BIT);
      }

      // ---- standard pin API ----
      static Unit pin()        { return Unit(regs().idr); }   // input state
      static Unit port()       { return Unit(regs().odr); }   // output latch
      static void port(Unit v) { regs().odr = v; }            // whole-port write

      // ---- atomic set/clear via BSRR (used by Mask<>::on()/off()) ----
      static void bsrr_set(Unit mask) { regs().bsrr = mask; }          // set pins
      static void bsrr_clr(Unit mask) { regs().bsrr = mask << 16; }   // reset pins

      // ---- masked direction (used by Mask<>::dir_out()/dir_in()) ----
      static void dir_out(Unit mask) {
        regs().moder = (regs().moder & ~moder_mask(mask)) | moder_out_bits(mask);
      }
      static void dir_in(Unit mask) {
        regs().moder &= ~moder_mask(mask);   // 00 = input
      }

      // ---- virtual 1-bit DDR from MODER (informational) ----
      static Unit ddr() {
        Unit result = 0;
        uint32_t m = regs().moder;
        for (int i = 0; i < 16; i++)
          if (((m >> (i*2)) & 3u) == 1u) result |= (Unit(1) << i);
        return result;
      }

      // ---- whole-port direction write from 1-bit mask ----
      static void dir(Unit mask) {
        regs().moder = (regs().moder & ~moder_mask(0xFFFF))
                     | moder_out_bits(mask);
      }

      // ---- peripheral configuration (pin-number based) ----
      static void mode(uint8_t pin, PinMode m) {
        regs().moder = (regs().moder & ~(3u << (pin*2)))
                     | (static_cast<uint32_t>(m) << (pin*2));
      }
      static void pull(uint8_t pin, PinPull p) {
        regs().pupdr = (regs().pupdr & ~(3u << (pin*2)))
                     | (static_cast<uint32_t>(p) << (pin*2));
      }
      static void speed(uint8_t pin, PinSpeed s) {
        regs().ospeedr = (regs().ospeedr & ~(3u << (pin*2)))
                       | (static_cast<uint32_t>(s) << (pin*2));
      }
      static void otype(uint8_t pin, PinType t) {
        if (t == PinType::OpenDrain) regs().otyper |=  (1u << pin);
        else                         regs().otyper &= ~(1u << pin);
      }
      static void altFunc(uint8_t pin, uint8_t af) {
        if (pin < 8)
          regs().afrl = (regs().afrl & ~(0xFu << (pin*4)))     | (uint32_t(af) << (pin*4));
        else
          regs().afrh = (regs().afrh & ~(0xFu << ((pin-8)*4))) | (uint32_t(af) << ((pin-8)*4));
      }
    };
  };

  // ============================================================
  // Family namespaces
  // ============================================================

  // STM32F4 — GPIO on AHB1, RCC AHB1ENR @ 0x40023830
  namespace f4 {
    static constexpr Addr RCC_AHB1ENR = 0x40023830u;
    template<Addr BASE, uint8_t BIT>
    using Port_ = STM32Port<BASE, RCC_AHB1ENR, BIT>;
    using PortA = Port_<0x40020000u, 0>;
    using PortB = Port_<0x40020400u, 1>;
    using PortC = Port_<0x40020800u, 2>;
    using PortD = Port_<0x40020C00u, 3>;
    using PortE = Port_<0x40021000u, 4>;
    using PortF = Port_<0x40021400u, 5>;
    using PortG = Port_<0x40021800u, 6>;
    using PortH = Port_<0x40021C00u, 7>;
    using PortI = Port_<0x40022000u, 8>;
  }

  // STM32L4/G4/WB/WL — GPIO on AHB2, RCC AHB2ENR @ 0x4002104C
  namespace l4 {
    static constexpr Addr RCC_AHB2ENR = 0x4002104Cu;
    template<Addr BASE, uint8_t BIT>
    using Port_ = STM32Port<BASE, RCC_AHB2ENR, BIT>;
    using PortA = Port_<0x48000000u, 0>;
    using PortB = Port_<0x48000400u, 1>;
    using PortC = Port_<0x48000800u, 2>;
    using PortD = Port_<0x48000C00u, 3>;
    using PortE = Port_<0x48001000u, 4>;
    using PortF = Port_<0x48001400u, 5>;
    using PortG = Port_<0x48001800u, 6>;
    using PortH = Port_<0x48001C00u, 7>;
  }

  namespace g4 = l4;

  // STM32H7 — GPIO on AHB4, RCC AHB4ENR @ 0x580244E0
  namespace h7 {
    static constexpr Addr RCC_AHB4ENR = 0x580244E0u;
    template<Addr BASE, uint8_t BIT>
    using Port_ = STM32Port<BASE, RCC_AHB4ENR, BIT>;
    using PortA = Port_<0x58020000u, 0>;
    using PortB = Port_<0x58020400u, 1>;
    using PortC = Port_<0x58020800u, 2>;
    using PortD = Port_<0x58020C00u, 3>;
    using PortE = Port_<0x58021000u, 4>;
    using PortF = Port_<0x58021400u, 5>;
    using PortG = Port_<0x58021800u, 6>;
    using PortH = Port_<0x58021C00u, 7>;
    using PortI = Port_<0x58022000u, 8>;
    using PortJ = Port_<0x58022400u, 9>;
    using PortK = Port_<0x58022800u, 10>;
  }

}} // hw::stm32

// ============================================================
// hw::stm32::chip — macro-selected family alias
// Define STM32F4, STM32L4, STM32H7 etc. before including.
// ============================================================
#if !defined(HW_STM32_CHIP_ALIAS_DEFINED)
  #define HW_STM32_CHIP_ALIAS_DEFINED
  #if defined(STM32H7)
    namespace hw { namespace stm32 { namespace chip = h7; }}
  #elif defined(STM32L4) || defined(STM32G4) || defined(STM32WB)
    namespace hw { namespace stm32 { namespace chip = l4; }}
  #else
    namespace hw { namespace stm32 { namespace chip = f4; }}
  #endif
#endif

// ============================================================
// Usage:
//
//   #define STM32L4
//   #include <onePin/onePin.h>
//   #include <chips/stm32/stm32Port.h>
//   #include <OneBit.h>
//   using namespace onePin; using namespace hw::stm32; using namespace oneBit;
//
//   using Led  = APIOf<Stm32OutPin, Mask<Pins<5>>, chip::PortB>;
//   using Btn  = APIOf<Stm32InPin,  Mask<Pins<3>>, chip::PortC>;
//   using Sda  = APIOf<Stm32IOPin,  Mask<Pins<8>>, chip::PortB>;
//
//   using Board = DeviceClass<Led, Btn, Sda>;  // compile-time collision check
//
//   Led led;
//   led.clockEnable();
//   led.dir_out();    // MODER bit 5 = output (via Mask<>)
//   led.on();         // BSRR set bit 5 (atomic)
//   led.off();        // BSRR reset bit 5 (atomic)
//   led.port(0xFFFF); // raw ODR write (bypasses Mask<>)
//
//   Sda sda;
//   sda.pull(8, PinPull::Up);   // pin-level config
//   sda.dir_out();               // switch to output
//   sda.on();
//   sda.dir_in();                // switch to input
//   auto v = sda.get();          // masked IDR read
// ============================================================
