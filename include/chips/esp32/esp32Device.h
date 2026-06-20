#pragma once
#include <chips/esp32/esp32SysClock.h>
#include <chips/esp32/esp32Gpio.h>
#include <chips/esp32/esp32Twi.h>
#include <chips/esp32/esp32Spi.h>

namespace hw::esp32 {

  // 'ESP32' is a preprocessor macro in the Arduino ESP32 framework — avoid that name.
  struct Esp32Dev {
    Esp32Dev() = delete;

    // Convenience: single GPIO as an output or input pin.
    template<int N> using OutPin = Esp32OutPin<N>;
    template<int N> using InPin  = Esp32InPin<N>;

    // Board — wraps Device<Boot, Peripherals...> with ESP32-aware run().
    // Board::run(fn) yields to FreeRTOS each iteration (feeds WDT on ESP32).
    template<typename Boot, typename... Peripherals>
    struct Board : onePin::Device<Boot, Peripherals...> {
      Board() = delete;

      static void begin() {
        onePin::Device<Boot, Peripherals...>::begin();
      }

      // run() yields between iterations — required on ESP32 to avoid WDT reset.
      template<typename F>
      static void run(F fn) {
        while (true) {
          fn();
#ifdef ARDUINO
          yield();   // FreeRTOS scheduler yield + WDT feed
#else
          taskYIELD();
#endif
        }
      }
    };
  };

  // chip:: alias — all esp32 headers expose their types in hw::esp32::esp32 namespace.
  namespace chip = esp32;

} // hw::esp32
