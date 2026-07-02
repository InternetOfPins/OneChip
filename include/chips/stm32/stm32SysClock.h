#pragma once
#include <cstdint>
#include <onePin/onePin.h>

#ifndef IOP
extern "C" unsigned long millis();
extern "C" unsigned long micros();
extern "C" void init();
#endif

namespace hw::stm32 {

  // ============================================================
  // STM32F1 PLL helper — called from SysClock<72MHz>::begin()
  // Configures: Flash 2WS → HSE on → PLL ×9 → APB1 /2 → switch
  // Falls back silently to HSI if HSE fails (no crystal).
  // ============================================================
#if defined(STM32F1xx) && defined(IOP)
  inline void stm32f1_pll_72mhz() {
    volatile uint32_t& flash_acr = *reinterpret_cast<volatile uint32_t*>(0x40022000u);
    volatile uint32_t& rcc_cr    = *reinterpret_cast<volatile uint32_t*>(0x40021000u);
    volatile uint32_t& rcc_cfgr  = *reinterpret_cast<volatile uint32_t*>(0x40021004u);

    flash_acr = (flash_acr & ~0x7u) | 0x2u;  // 2 wait states for 72 MHz

    rcc_cr |= (1u << 16);                     // HSEON
    uint32_t t = 100000;
    while (!(rcc_cr & (1u << 17)) && --t);    // wait HSERDY (timeout → stay on HSI)
    if (!t) return;

    // PLL: source = HSE, multiplier = ×9 → 72 MHz
    rcc_cfgr = (rcc_cfgr & ~0x3F0000u) | 0x1D0000u;  // PLLSRC=HSE, PLLMUL=×9
    rcc_cfgr = (rcc_cfgr & ~0x700u)    | 0x400u;     // APB1 = HCLK/2 = 36 MHz

    rcc_cr |= (1u << 24);                     // PLLON
    while (!(rcc_cr & (1u << 25)));           // wait PLLRDY

    rcc_cfgr = (rcc_cfgr & ~0x3u) | 0x2u;   // SW = PLL
    while ((rcc_cfgr & 0xCu) != 0x8u);       // wait SWS = PLL
  }
#endif

  // ARM Cortex-M SysTick registers (fixed address, all Cortex-M cores)
  struct arm_systick_regs {
    volatile uint32_t ctrl;   // 0xE000E010
    volatile uint32_t load;   // 0xE000E014
    volatile uint32_t val;    // 0xE000E018
    volatile uint32_t calib;  // 0xE000E01C
  };
  static arm_systick_regs& systick_hw() {
    return *reinterpret_cast<arm_systick_regs*>(0xE000E010u);
  }

  // ============================================================
  // SysClock<CpuHz> — ARM Cortex-M SysTick component.
  // IOP:         owns the SysTick peripheral, ISR updates _ms.
  // non-IOP:     delegates millis()/micros() to framework.
  // ============================================================
  template<uint32_t CpuHz = 168000000UL>
  struct SysClock {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

#ifdef IOP
      inline static volatile uint32_t _ms = 0;

      static void begin() {
#if defined(STM32F1xx)
        if constexpr (CpuHz == 72000000UL) stm32f1_pll_72mhz();
#endif
        systick_hw().load = CpuHz / 1000 - 1;  // reload for 1ms tick
        systick_hw().val  = 0;
        systick_hw().ctrl = 0x7;  // AHB clock | TICKINT | ENABLE
        Base::begin();
      }
      static void onOverflow() { _ms++; }
      static uint32_t millis() { return _ms; }
      static uint32_t micros() { return _ms * 1000UL; }
#else
      static void begin()      { ::init(); }
      static void onOverflow() {}
      static uint32_t millis() { return ::millis(); }
      static uint32_t micros() { return ::micros(); }
#endif

      template<uint32_t ms>
      struct Period {
        uint32_t last = 0;
        bool operator()() {
          uint32_t now = millis();
          if (now - last < ms) return false;
          last = now;
          return true;
        }
        void reset() { last = millis(); }
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
  // Family SysClk aliases (CPU frequency differs by family)
  // Named SysClk, not SysTick: ARM CMSIS's core_cmX.h (pulled in by every Cortex-M
  // framework, Arduino included) #defines SysTick as ((SysTick_Type*)SysTick_BASE) —
  // a raw textual macro, so `using SysTick = ...` anywhere in this file (namespace-level
  // or as a struct member, e.g. STM32F103::SysTick) gets silently rewritten into that
  // pointer-cast expression and fails to parse as a type alias. Found 2026-07-02 building
  // the first STM32 target actually compiled under Arduino framework (stm32f030.h) — F1/F4/
  // L4/H7 had the identical latent bug, just never previously exercised through a framework
  // that pulls in core_cmX.h (a bare-metal -DIOP build never includes it, so never collided).
  // ============================================================
  namespace f1 {
    template<uint32_t CpuHz = 72000000UL>
    using SysClk = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace f0 {
    // Default 8MHz — HSI reset default, no PLL configured (unlike f1's stm32f1_pll_72mhz()
    // special-case). Fine for initial bring-up: millis() timing scales with whatever CpuHz
    // is declared here, so an unconfigured clock just shifts blink timing, doesn't break it.
    template<uint32_t CpuHz = 8000000UL>
    using SysClk = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace f4 {
    template<uint32_t CpuHz = 168000000UL>
    using SysClk = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace l4 {
    template<uint32_t CpuHz = 80000000UL>
    using SysClk = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace h7 {
    template<uint32_t CpuHz = 480000000UL>
    using SysClk = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }

} // hw::stm32

// Wire SysTick_Handler to Board::onOverflow() — place once in main translation unit.
#define IOP_SYSTICK_ISR(board_t) \
  extern "C" void SysTick_Handler(void) { board_t::onOverflow(); }
