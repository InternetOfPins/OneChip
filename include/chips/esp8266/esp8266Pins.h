#pragma once
#include <stdint.h>

// ESP8266 pins as data: the board names (D0..D8 on NodeMCU / Wemos D1 Mini) and what the chip does with each GPIO, as constexpr facts the
// rules read (OneMachine's interrupt delivery and wiring checks; a wiring tool reads them through a host program). No framework headers:
// this compiles on any target and on the host.
//   strap(g)      sampled at reset to choose the boot mode (GPIO0, GPIO2, GPIO15): a line that something else can drive at reset must not be on it
//   bootLevel(g)  the level a strap pin must have at reset for the chip to run the sketch (GPIO0 and GPIO2 high, GPIO15 low)
//   hasIrq(g)     the pin has an interrupt (every GPIO but GPIO16)
//   flash(g)      the pin is the module's flash (GPIO6..GPIO11): never a GPIO
//   gpio(g)       a GPIO this chip has (0..16)

namespace hw::esp8266 {

  struct Esp8266Pins {
    static constexpr uint8_t D0  = 16;  // NodeMCU D0 — no PWM, no interrupt
    static constexpr uint8_t D1  =  5;  // NodeMCU D1 — I2C SCL default
    static constexpr uint8_t D2  =  4;  // NodeMCU D2 — I2C SDA default
    static constexpr uint8_t D3  =  0;  // NodeMCU D3 — boot strapping (10kΩ pull-up)
    static constexpr uint8_t D4  =  2;  // NodeMCU D4 — boot strapping, UART1 TX, LED
    static constexpr uint8_t D5  = 14;  // NodeMCU D5 — SPI CLK
    static constexpr uint8_t D6  = 12;  // NodeMCU D6 — SPI MISO
    static constexpr uint8_t D7  = 13;  // NodeMCU D7 — SPI MOSI
    static constexpr uint8_t D8  = 15;  // NodeMCU D8 — SPI CS, boot strapping (10kΩ pull-down)
    static constexpr uint8_t RX  =  3;  // UART0 RX
    static constexpr uint8_t TX  =  1;  // UART0 TX (also used for Serial monitor)

    static constexpr bool gpio(uint8_t g)      { return g <= 16; }
    static constexpr bool strap(uint8_t g)     { return g == 0 || g == 2 || g == 15; }
    static constexpr bool bootLevel(uint8_t g) { return g != 15; }
    static constexpr bool hasIrq(uint8_t g)    { return g <= 15; }
    static constexpr bool flash(uint8_t g)     { return g >= 6 && g <= 11; }
  };

}
