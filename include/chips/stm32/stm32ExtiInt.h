/**
 * @file stm32ExtiInt.h
 * @brief STM32 EXTI (External Interrupt) — implements onChange abstraction.
 *
 * Maps EXTI to onePin::OnChange/OnRise/OnFall contract.
 * Supports max 3 pins from different ports (PA0, PB1, PC2 etc).
 *
 * Usage:
 *   using MyInt = chip::OnChange<PA0, PB1, PC2>;
 *   MyInt::begin();
 *   uint8_t state = MyInt::read();
 *   if (MyInt::changed()) { handle_edge(); }
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>

namespace hw::stm32 {

  // GPIO port identifiers (encode as PORT_ID=port*16+pin)
  // PA0=0x00, PA1=0x01, ..., PA15=0x0F, PB0=0x10, ..., PB15=0x1F, etc.
  // Extract: port = ID >> 4, pin = ID & 0x0F

  /// EXTI adapter for STM32 (up to 3 pins)
  template<int PIN_ID0, int PIN_ID1 = -1, int PIN_ID2 = -1>
  struct ExtiInt {
    static_assert(PIN_ID0 >= 0 && PIN_ID0 < 112, "STM32: invalid pin ID");
    static_assert(PIN_ID1 < 0 || (PIN_ID1 >= 0 && PIN_ID1 < 112), "STM32: invalid pin ID");
    static_assert(PIN_ID2 < 0 || (PIN_ID2 >= 0 && PIN_ID2 < 112), "STM32: invalid pin ID");

    inline static volatile uint32_t _last = 0;

    // Map pin ID to GPIO register addresses (stub — real impl uses CMSIS)
    static constexpr uint32_t gpio_base(int port) {
      return 0x40020000u + (port * 0x400u);  // GPIOA_BASE + (port offset)
    }

    static constexpr uint32_t gpio_idr(int port) {
      return gpio_base(port) + 0x10u;  // IDR register offset
    }

    template<typename EdgeMode>
    static void begin() {
      // Framework-dependent: CMSIS vs HAL vs bare-metal
      // Sketch: mimics AVR PCINT pattern (per-port enable)
      // Real impl requires:
      // - GPIO clock enable per port (RCC AHBENR)
      // - SYSCFG clock enable (RCC APB2ENR)
      // - EXTI line config (SYSCFG_EXTICR1-4) — maps port+pin → EXTI line
      // - EXTI edge trigger (CR1=rising, CR2=falling)
      // - NVIC enable per line (NVIC_EnableIRQ + priority)
      // Similar to: PCICR |= (1<<PCIE1); PCMSK1 |= mask;
      _last = read();
    }

    template<typename EdgeMode>
    static uint8_t read() {
      uint8_t state = 0;
      // Read GPIO IDR for each pin (stub)
      return state;
    }

    template<typename EdgeMode>
    static bool changed() {
      uint8_t now = read();
      uint8_t mask = ((PIN_ID0 >= 0) ? 0x01 : 0) |
                     ((PIN_ID1 >= 0) ? 0x02 : 0) |
                     ((PIN_ID2 >= 0) ? 0x04 : 0);
      bool diff = ((now ^ _last) & mask) != 0;
      if (diff) _last = now;
      return diff;
    }
  };

} // hw::stm32
