/**
 * @file esp32GpioInt.h
 * @brief ESP32 GPIO Interrupt — implements onChange abstraction.
 *
 * Maps GPIO interrupts to onePin::OnChange/OnRise/OnFall contract.
 *
 * Note: Stub implementation. Full implementation requires:
 * - GPIO interrupt attachment (attachInterrupt or isr_register)
 * - State tracking per GPIO
 * - Edge filtering (OnChange/OnRise/OnFall modes)
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>

namespace hw::esp32 {

  /// GPIO Interrupt adapter (stub for now)
  template<int GPIO_Num, int GPIO_Num2 = -1, int GPIO_Num3 = -1>
  struct GpioInt {
    static_assert(GPIO_Num >= 0 && GPIO_Num < 40, "ESP32: invalid GPIO number");

    inline static volatile uint32_t _last = 0;

    template<typename EdgeMode>
    static void begin() {
      // TODO: attachInterrupt setup for GPIO_Num, GPIO_Num2, GPIO_Num3
      // TODO: Configure edge mode (OnChange/OnRise/OnFall)
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

} // hw::esp32
