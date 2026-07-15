/**
 * @file esp32GpioInt.h
 * @brief ESP32 GPIO Interrupt — implements onChange abstraction.
 *
 * Maps GPIO interrupts to onePin::OnChange/OnRise/OnFall contract.
 * Supports max 3 pins per instance (encoder: A, B, button).
 *
 * Usage:
 *   using MyInt = chip::OnChange<GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36>;
 *   MyInt::begin();
 *   uint8_t state = MyInt::read();
 *   if (MyInt::changed()) { handle_edge(); }
 */

#pragma once
#include <stdint.h>
#include <onePin/onChange.h>

#ifdef ESP32
  #include <driver/gpio.h>
#endif

namespace hw::esp32 {

  /// GPIO Interrupt adapter for ESP32
  template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
  struct GpioInt {
    static_assert(GPIO0 >= 0 && GPIO0 < 40, "ESP32: GPIO must be 0..39");
    static_assert(GPIO1 < 0 || (GPIO1 >= 0 && GPIO1 < 40), "ESP32: GPIO must be 0..39 or -1");
    static_assert(GPIO2 < 0 || (GPIO2 >= 0 && GPIO2 < 40), "ESP32: GPIO must be 0..39 or -1");

    inline static volatile uint32_t _last = 0;
    inline static volatile bool _changed = false;

    template<typename EdgeMode>
    static void begin() {
#ifdef ESP32
      gpio_config_t cfg = {};
      cfg.pin_bit_mask = (1ULL << GPIO0);
      if (GPIO1 >= 0) cfg.pin_bit_mask |= (1ULL << GPIO1);
      if (GPIO2 >= 0) cfg.pin_bit_mask |= (1ULL << GPIO2);
      cfg.mode = GPIO_MODE_INPUT;
      cfg.pull_up_en = GPIO_PULLUP_ENABLE;
      cfg.intr_type = GPIO_INTR_ANYEDGE;
      gpio_config(&cfg);

      _last = read();
      _changed = false;
#endif
    }

    template<typename EdgeMode>
    static uint8_t read() {
      uint8_t state = 0;
#ifdef ESP32
      if (GPIO0 >= 0) state |= (gpio_get_level((gpio_num_t)GPIO0) << 0);
      if (GPIO1 >= 0) state |= (gpio_get_level((gpio_num_t)GPIO1) << 1);
      if (GPIO2 >= 0) state |= (gpio_get_level((gpio_num_t)GPIO2) << 2);
#endif
      return state;
    }

    template<typename EdgeMode>
    static bool changed() {
      uint8_t now = read();
      uint8_t mask = ((GPIO0 >= 0) ? 0x01 : 0) |
                     ((GPIO1 >= 0) ? 0x02 : 0) |
                     ((GPIO2 >= 0) ? 0x04 : 0);
      bool diff = ((now ^ _last) & mask) != 0;
      if (diff) _last = now;
      return diff;
    }
  };

} // hw::esp32
