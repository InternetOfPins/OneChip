#pragma once
#ifndef IOP
#include <onePin/onePin.h>

extern "C" unsigned long millis();
extern "C" unsigned long micros();

namespace hw::avr {

  // FrameworkClock — delegates millis()/micros() to the active framework.
  // Used when IOP is not defined (Arduino, Mbed, Zephyr, etc.).
  // begin() / onOverflow() are no-ops — the framework owns the timer.
  struct FrameworkClock : onePin::BootDef {
    static void begin() {
      extern "C" void init();
      init();  // framework hardware init — configures Timer0, enables interrupts
    }
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

} // hw::avr
#endif
