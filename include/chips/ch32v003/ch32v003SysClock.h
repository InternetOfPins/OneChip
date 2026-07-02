/**
 * @file ch32v003SysClock.h
 * @brief CH32V003 system clock — free-running SysTick-based millis()/micros().
 *
 * CH32V003 (bare-metal noneos-sdk, no OS/millis() provided). SysTick ticks at
 * SystemCoreClock/8 (confirmed against WCH's own Delay_Init/Delay_Us in debug.c —
 * p_us = SystemCoreClock/8000000, i.e. 8 ticks per microsecond at typical clocks). Only
 * CTLR bit0 (STE, enable) is used here — that's the one bit whose behavior is confirmed
 * against that reference; the compare/interrupt bits WCH's own Delay_Ms uses for a
 * one-shot busy-wait aren't needed for a plain free-running counter and weren't worth
 * guessing at. CNT is a 32-bit free-running counter — wraps every ~715s at 48MHz/8;
 * unsigned subtraction in Period<> handles that correctly without special-casing.
 */
#pragma once
#include <hapi/hapi.h>
#include <onePin/onePin.h>
#include <stdint.h>

extern "C" {
  #include "ch32v00x.h"
}

namespace hw::ch32v003 {

  /// @brief CH32V003 system clock component; provides millis()-based Period and Blink helpers
  struct Ch32v003Clock {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static void begin() {
        SysTick->CTLR = 0;      // stop, plain up-count mode (no compare/interrupt)
        SysTick->CNT  = 0;
        SysTick->CTLR = 1;      // bit0 STE — enable, free-running
        Base::begin();
      }
      static void onOverflow() {}

      static uint32_t ticksPerUs() { return SystemCoreClock / 8000000u; }
      static uint32_t micros() { return SysTick->CNT / ticksPerUs(); }
      static uint32_t millis() { return SysTick->CNT / (ticksPerUs() * 1000u); }

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

  namespace ch32v003 {
    using SysClock = hapi::APIOf<onePin::BootDef, Ch32v003Clock>;
  }

} // hw::ch32v003
