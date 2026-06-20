#pragma once
// ESP32 GPIO — single-pin types with the IOP on/off/begin/get interface.
// ESP32's GPIO matrix allows any pin for any function, so no parallel port concept.
// Use these directly in place of AVR::OutPin<Pins<N>, PortX>.
//
// #ifdef ARDUINO: wraps pinMode/digitalWrite/digitalRead.
// otherwise: wraps ESP-IDF gpio_set_direction/gpio_set_level/gpio_get_level.
#include <stdint.h>

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <driver/gpio.h>
#endif

namespace hw::esp32 {

  template<int N>
  struct Esp32OutPin {
    static void begin() {
#ifdef ARDUINO
      pinMode(N, OUTPUT);
#else
      gpio_reset_pin((gpio_num_t)N);
      gpio_set_direction((gpio_num_t)N, GPIO_MODE_OUTPUT);
#endif
    }
    static void on()          { set(true); }
    static void off()         { set(false); }
    static void set(bool v)   {
#ifdef ARDUINO
      digitalWrite(N, v ? HIGH : LOW);
#else
      gpio_set_level((gpio_num_t)N, v ? 1 : 0);
#endif
    }
    static bool get() {
#ifdef ARDUINO
      return digitalRead(N) != LOW;
#else
      return gpio_get_level((gpio_num_t)N) != 0;
#endif
    }
    static void toggle() { set(!get()); }
  };

  template<int N>
  struct Esp32InPin {
    static void begin() {
#ifdef ARDUINO
      pinMode(N, INPUT);
#else
      gpio_reset_pin((gpio_num_t)N);
      gpio_set_direction((gpio_num_t)N, GPIO_MODE_INPUT);
#endif
    }
    static bool get() {
#ifdef ARDUINO
      return digitalRead(N) != LOW;
#else
      return gpio_get_level((gpio_num_t)N) != 0;
#endif
    }
  };

  // Convenience: pull-up input
  template<int N>
  struct Esp32InPullUpPin {
    static void begin() {
#ifdef ARDUINO
      pinMode(N, INPUT_PULLUP);
#else
      gpio_reset_pin((gpio_num_t)N);
      gpio_set_direction((gpio_num_t)N, GPIO_MODE_INPUT);
      gpio_set_pull_mode((gpio_num_t)N, GPIO_PULLUP_ONLY);
#endif
    }
    static bool get() { return Esp32InPin<N>::get(); }
  };

} // hw::esp32
