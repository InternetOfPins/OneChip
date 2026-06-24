#pragma once
#include <hapi/hapi.h>
#include <onePin/onePin.h>
#include <stdint.h>
#ifdef ARDUINO
  #include <Arduino.h>
#endif

// ESP8266 system clock — wraps Arduino millis()/micros().
// No ISR needed. Provides the same Period<> / Blink<> interface as
// AvrSysClock and Esp32Clock for portability.

namespace hw::esp8266 {

  /// @brief ESP8266 system clock component; provides millis()-based Period and Blink helpers
  struct Esp8266Clock {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static void begin()      { Base::begin(); }
      static void onOverflow() {}

      static uint32_t millis() { return ::millis(); }
      static uint32_t micros() { return ::micros(); }

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

  namespace esp8266 {
    using SysClock = hapi::APIOf<onePin::BootDef, Esp8266Clock>;
  }

} // hw::esp8266
