#pragma once
#include <hapi/hapi.h>
#include <onePin/onePin.h>
#include <stdint.h>

// ESP32 system clock — wraps millis()/micros().
// Arduino framework: provided by FreeRTOS tick.
// ESP-IDF: provided by esp_timer_get_time().
//
// No ISR needed — the runtime handles it. Provides the same
// Period<> / Blink<> helpers as avrSysClock for portability.

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <esp_timer.h>
#endif

namespace hw::esp32 {

  /// @brief ESP32 system clock component; provides millis()-based Period and Blink helpers
  struct Esp32Clock {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static void begin()      { Base::begin(); }  // millis already running on ESP32
      static void onOverflow() {}

#ifdef ARDUINO
      static uint32_t millis() { return ::millis(); }
      static uint32_t micros() { return ::micros(); }
#else
      static uint32_t millis() { return (uint32_t)(esp_timer_get_time() / 1000); }
      static uint32_t micros() { return (uint32_t)(esp_timer_get_time()); }
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

  namespace esp32 {
    using SysClock = hapi::APIOf<onePin::BootDef, Esp32Clock>;
  }

} // hw::esp32
