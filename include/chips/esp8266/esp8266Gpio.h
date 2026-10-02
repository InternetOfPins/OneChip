#pragma once
// ESP8266 GPIO -- single-pin types with the IOP begin/on/off/get interface (Arduino pinMode/digitalWrite).
// N is the GPIO number (D8 = 15, ...; see Esp8266Dev). GPIO6..11 are the flash: never a pin here.
#include <stdint.h>
#ifdef ARDUINO
  #include <Arduino.h>
#endif

namespace hw::esp8266 {

#ifdef ARDUINO

  /// @brief ESP8266 output pin over pinMode/digitalWrite
  template<int N>
  struct OutPin {
    static_assert(N < 6 || N > 11, "GPIO6..11 drive the flash chip");
    static void begin()       { pinMode(N, OUTPUT); }
    static void on()          { digitalWrite(N, HIGH); }
    static void off()         { digitalWrite(N, LOW); }
    static void set(bool v)   { digitalWrite(N, v ? HIGH : LOW); }
    [[nodiscard]] static bool get() { return digitalRead(N) != LOW; }
    static void toggle()      { set(!get()); }
  };

  /// @brief ESP8266 input pin; Pull selects INPUT_PULLUP
  template<int N, bool Pull = false>
  struct InPin {
    static_assert(N < 6 || N > 11, "GPIO6..11 drive the flash chip");
    static void begin()             { pinMode(N, Pull ? INPUT_PULLUP : INPUT); }
    [[nodiscard]] static bool get() { return digitalRead(N) != LOW; }
  };

#endif

} // hw::esp8266
