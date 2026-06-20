#pragma once
#include <cstdint>
#include <onePin/onePin.h>

#ifndef IOP
extern "C" unsigned long millis();
extern "C" unsigned long micros();
extern "C" void init();
#endif

namespace hw::stm32 {

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
  // Family SysTick aliases (CPU frequency differs by family)
  // ============================================================
  namespace f1 {
    template<uint32_t CpuHz = 72000000UL>
    using SysTick = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace f4 {
    template<uint32_t CpuHz = 168000000UL>
    using SysTick = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace l4 {
    template<uint32_t CpuHz = 80000000UL>
    using SysTick = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }
  namespace h7 {
    template<uint32_t CpuHz = 480000000UL>
    using SysTick = hapi::APIOf<onePin::BootDef, SysClock<CpuHz>>;
  }

} // hw::stm32

// Wire SysTick_Handler to Board::onOverflow() — place once in main translation unit.
#define IOP_SYSTICK_ISR(board_t) \
  extern "C" void SysTick_Handler(void) { board_t::onOverflow(); }
