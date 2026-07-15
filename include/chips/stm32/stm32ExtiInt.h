/**
 * @file stm32ExtiInt.h
 * @brief STM32 EXTI (External Interrupt) — implements onChange abstraction.
 *
 * Maps EXTI to onePin::OnChange/OnRise/OnFall contract.
 *
 * Note: Stub implementation. Full implementation requires:
 * - GPIO clock enable
 * - EXTI pin/line configuration
 * - Edge trigger selection (rising/falling/both)
 * - NVIC enable
 * - ISR registration
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>

namespace hw::stm32 {

  /// EXTI (External Interrupt) adapter (stub for now)
  template<int PIN_ID, int PIN_ID2 = -1, int PIN_ID3 = -1>
  struct ExtiInt {
    static_assert(PIN_ID >= 0 && PIN_ID < 16, "STM32: invalid pin line");

    inline static volatile uint32_t _last = 0;

    template<typename EdgeMode>
    static void begin() {
      // TODO: GPIO clock enable for affected ports
      // TODO: EXTI line configuration
      // TODO: Edge trigger setup (OnChange=both, OnRise=rising, OnFall=falling)
      // TODO: NVIC enable
    }

    template<typename EdgeMode>
    static uint8_t read() {
      // TODO: Read GPIO states
      return 0;
    }

    template<typename EdgeMode>
    static bool changed() {
      // TODO: Check if any pins changed
      return false;
    }
  };

} // hw::stm32
