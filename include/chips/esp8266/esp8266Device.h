#pragma once
#include <hapi/hapi.h>
#include <chips/esp8266/esp8266SysClock.h>
#include <chips/esp8266/esp8266Twi.h>
#include <stdint.h>

// ESP8266 (Xtensa LX106) chip catalog.
//
// Key differences from ESP32:
//   - Single core @ 80/160 MHz (no FreeRTOS dual-core)
//   - No hardware I2C — Wire.h uses software bit-bang on any two GPIO pins
//   - No hardware DAC
//   - 1× 10-bit ADC (A0, 0–1 V on module, 0–3.3 V on NodeMCU via divider)
//   - Software PWM on any GPIO via analogWrite() (Arduino framework)
//   - Hardware SPI: HSPI (GPIO12/13/14/15) or VSPI (overlap, avoid)
//   - UART0: GPIO1(TX)/GPIO3(RX); UART1: GPIO2(TX only, used for debug)
//   - GPIO6..GPIO11 are connected to flash — never use as GPIO
//   - GPIO0/2/15 have boot-mode strapping — avoid driving at boot
//   - WiFi: 802.11 b/g/n 2.4 GHz (same ESP8266WiFi.h / ESPAsyncWebServer API)
//
// 'ESP8266' is defined as a preprocessor macro by the framework — avoid that name.

namespace hw::esp8266 {

  // chip:: alias — all esp8266 headers expose their types in hw::esp8266::esp8266
  namespace chip = esp8266;

  struct Esp8266Dev {
    Esp8266Dev() = delete;

    // GPIO — all pins are software PWM capable via analogWrite()
    // Physically available on NodeMCU/Wemos D1 Mini: GPIO0,1,2,3,4,5,12,13,14,15,16
    // Safe output pins (no boot strapping): GPIO4, GPIO5, GPIO12, GPIO13, GPIO14
    // Safe input pins: GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO16

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

    // Default I2C pins (Wire.begin(SDA, SCL))
    static constexpr uint8_t SDA_pin = 4;   // D2
    static constexpr uint8_t SCL_pin = 5;   // D1

    // Hardware SPI (HSPI)
    static constexpr uint8_t MISO_pin = 12;  // D6
    static constexpr uint8_t MOSI_pin = 13;  // D7
    static constexpr uint8_t SCK_pin  = 14;  // D5
    static constexpr uint8_t SS_pin   = 15;  // D8

    // ADC
    static constexpr uint8_t A0_pin = 17;   // A0 — 10-bit, 0-1V (module) / 0-3.3V (NodeMCU)

    struct BoardDef { BoardDef() = delete; };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

  // Common board variants — same chip, different pin breakout
  struct NodeMCU    : Esp8266Dev {};  // NodeMCU v2/v3 (CP2102 / CH340)
  struct WemosD1Mini: Esp8266Dev {};  // Wemos/LOLIN D1 Mini
  struct ESP01      : Esp8266Dev {    // ESP-01 — only GPIO0 and GPIO2 broken out
    static constexpr uint8_t SDA_pin = 0;
    static constexpr uint8_t SCL_pin = 2;
  };

  // ── AquaGrow MicroTC360 / MicroTC361 ─────────────────────────────────────
  // ESP8266 (Wemos D1 Mini) aquarium LED controller.
  // MicroTC360: with SSD1306 OLED + rotary encoder (full UI)
  // MicroTC361: headless variant (no display, no encoder) — same PCB, HEADLESS build flag
  //
  // I2C bus (400 kHz): SSD1306 + DS3231 + PCA9685 all on SDA=4/SCL=5
  // PCA9685: 16-channel PWM for LED channels (lights, moonlight, UV, etc.)
  // DS3231: RTC for alarm scheduling
  // Encoder: A=12, B=14, BTN=13 (swap A/B to reverse direction)

  struct MicroTC360 : WemosD1Mini {
    static constexpr uint8_t  SDA_pin   = 4;
    static constexpr uint8_t  SCL_pin   = 5;
    static constexpr uint8_t  ENC_A     = 12;
    static constexpr uint8_t  ENC_B     = 14;
    static constexpr uint8_t  ENC_BTN   = 13;
    static constexpr uint32_t I2C_SPEED = 400000UL;
  };

  // Headless variant — same board, no display/encoder populated
  struct MicroTC361 : MicroTC360 {};

} // hw::esp8266
