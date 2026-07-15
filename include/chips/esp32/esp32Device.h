#pragma once
#include <chips/esp32/esp32SysClock.h>
#include <chips/esp32/esp32Gpio.h>
#include <chips/esp32/esp32Twi.h>
#include <chips/esp32/esp32Spi.h>
#include <chips/esp32/esp32GpioInt.h>

namespace hw::esp32 {

  /// @brief ESP32 chip descriptor: GPIO catalog, Twi/Spi aliases, FreeRTOS-aware Board wrapper
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

  // ── Interrupt source aliases (OnChange/OnRise/OnFall) ────────────────
  namespace interrupt_sources {
    template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
    using OnChange = GpioInt<GPIO0, GPIO1, GPIO2>;

    template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
    using OnRise = GpioInt<GPIO0, GPIO1, GPIO2>;

    template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
    using OnFall = GpioInt<GPIO0, GPIO1, GPIO2>;
  }

  // chip:: alias — all esp32 headers expose their types in hw::esp32::esp32 namespace.
  namespace chip = esp32;

} // hw::esp32

// Platform-agnostic alias
namespace chip {
  template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
  using OnChange = hw::esp32::interrupt_sources::OnChange<GPIO0, GPIO1, GPIO2>;

  template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
  using OnRise = hw::esp32::interrupt_sources::OnRise<GPIO0, GPIO1, GPIO2>;

  template<int GPIO0, int GPIO1 = -1, int GPIO2 = -1>
  using OnFall = hw::esp32::interrupt_sources::OnFall<GPIO0, GPIO1, GPIO2>;
}
