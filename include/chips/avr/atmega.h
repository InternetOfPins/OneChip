/**
 * @file atmega.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief ATmega family chip definitions — values from Microchip ATmega_DFP atdf XML.
 *        One file, all classic ATmega devices. Include only this, pay only for what you use.
 */

#pragma once
#include <chips/avr/watchdog.h>
#include <chips/avr/flashMem.h>
#include <chips/avr/avrPort.h>
#include <chips/avr/avrTC.h>
#include <chips/avr/avrOC.h>
#include <hapi/hapi.h>

namespace hw {
namespace avr {

  // ============================================================
  // Memory primitives — parameterized, reusable across chips
  // ============================================================

  template<uint16_t Start, uint16_t Size>
  struct Ram {
    static constexpr uint16_t start = Start;
    static constexpr uint16_t size  = Size;
    static constexpr uint16_t end   = Start + Size - 1;
  };

  template<uint16_t Size, uint8_t PageSize>
  struct Eeprom {
    static constexpr uint16_t size     = Size;
    static constexpr uint8_t  pageSize = PageSize;
  };

  // ============================================================
  // Family base — shared by all classic ATmega (WDCE unlock)
  // ============================================================

  using ATmegaWatchdog = WatchdogClassic;

  // ============================================================
  // Chip definitions — values from atdf XML
  // Flash: size / pageSize from <memory-segment name="FLASH">
  // Ram:   start / size from <memory-segment name="IRAM">
  // Eeprom: size / pageSize from <memory-segment name="EEPROM">
  // ============================================================

  // ── ATmega8 family peripheral catalog ────────────────────────────────────
  // ATmega88 / 168 / 328 share identical port/timer/OC layout.
  // Each chip struct inherits this and adds its own memory specs.
  struct ATmega8Periph {
    struct PortB : mega::PortB {};
    struct PortC : mega::PortC {};
    struct PortD : mega::PortD {};

    template<void(*fn)() = nullptr> struct TC0 : mega::TC0<fn> {};
    template<void(*fn)() = nullptr> struct TC1 : mega::TC1<fn> {};
    template<void(*fn)() = nullptr> struct TC2 : mega::TC2<fn> {};

    struct OC0A : AvrOC<0x47, 0x44, 7, 0x2B, 0x2A, 6> {};  // PD6
    struct OC0B : AvrOC<0x48, 0x44, 5, 0x2B, 0x2A, 5> {};  // PD5
    struct OC1A : AvrOC<0x88, 0x80, 7, 0x25, 0x24, 1> {};  // PB1
    struct OC1B : AvrOC<0x8A, 0x80, 5, 0x25, 0x24, 2> {};  // PB2
    struct OC2A : AvrOC<0xB3, 0xB0, 7, 0x25, 0x24, 3> {};  // PB3
    struct OC2B : AvrOC<0xB4, 0xB0, 5, 0x2B, 0x2A, 3> {};  // PD3

    static constexpr uint8_t SDA_bit  = 4;   // PC4
    static constexpr uint8_t SCL_bit  = 5;   // PC5
    static constexpr uint8_t TX_bit   = 1;   // PD1
    static constexpr uint8_t RX_bit   = 0;   // PD0
    static constexpr uint8_t INT0_bit = 2;   // PD2
    static constexpr uint8_t INT1_bit = 3;   // PD3
    static constexpr uint8_t SS_bit   = 2;   // PB2
    static constexpr uint8_t MOSI_bit = 3;   // PB3
    static constexpr uint8_t MISO_bit = 4;   // PB4
    static constexpr uint8_t SCK_bit  = 5;   // PB5

    struct BoardDef {
      BoardDef() = delete;
      static void begin() {}
    };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

  struct ATmega88 : ATmega8Periph {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x2000, 0x40>;   // 8k / 64B page
    using Ram_     = Ram<0x0100, 0x0400>;       // 1k SRAM
    using Eeprom_  = Eeprom<0x0200, 0x04>;      // 512B / 4B page
  };

  struct ATmega168 : ATmega8Periph {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x4000, 0x80>;   // 16k / 128B page
    using Ram_     = Ram<0x0100, 0x0400>;       // 1k SRAM
    using Eeprom_  = Eeprom<0x0200, 0x04>;      // 512B / 4B page
  };

  struct ATmega328 : ATmega8Periph {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x8000, 0x80>;   // 32k / 128B page
    using Ram_     = Ram<0x0100, 0x0800>;       // 2k SRAM
    using Eeprom_  = Eeprom<0x0400, 0x04>;      // 1k / 4B page
  };

  using ATmega328P  = ATmega328;
  using ATmega328PB = ATmega328;

  // ── ATmega2560 family peripheral catalog ─────────────────────────────────
  struct ATmega2560Periph {
    struct PortA : mega2560::PortA {};
    struct PortB : mega2560::PortB {};
    struct PortC : mega2560::PortC {};
    struct PortD : mega2560::PortD {};
    struct PortE : mega2560::PortE {};
    struct PortF : mega2560::PortF {};
    struct PortG : mega2560::PortG {};
    struct PortH : mega2560::PortH {};
    struct PortJ : mega2560::PortJ {};
    struct PortK : mega2560::PortK {};
    struct PortL : mega2560::PortL {};

    template<void(*fn)() = nullptr> struct TC0 : mega2560::TC0<fn> {};
    template<void(*fn)() = nullptr> struct TC1 : mega2560::TC1<fn> {};
    template<void(*fn)() = nullptr> struct TC2 : mega2560::TC2<fn> {};
    template<void(*fn)() = nullptr> struct TC3 : mega2560::TC3<fn> {};
    template<void(*fn)() = nullptr> struct TC4 : mega2560::TC4<fn> {};
    template<void(*fn)() = nullptr> struct TC5 : mega2560::TC5<fn> {};

    // OC channels — ATmega2560 pinout (differs from ATmega328)
    struct OC0A : AvrOC<0x47, 0x44, 7, 0x25, 0x24, 7> {};  // PB7  (Arduino D13)
    struct OC0B : AvrOC<0x48, 0x44, 5, 0x34, 0x33, 5> {};  // PG5  (Arduino D4)
    struct OC1A : AvrOC<0x88, 0x80, 7, 0x25, 0x24, 5> {};  // PB5  (Arduino D11)
    struct OC1B : AvrOC<0x8A, 0x80, 5, 0x25, 0x24, 6> {};  // PB6  (Arduino D12)
    struct OC2A : AvrOC<0xB3, 0xB0, 7, 0x25, 0x24, 4> {};  // PB4  (Arduino D10)
    struct OC2B : AvrOC<0xB4, 0xB0, 5, 0x102,0x101,6> {};  // PH6  (Arduino D9)
    struct OC3A : AvrOC<0x98, 0x90, 7, 0x2E, 0x2D, 3> {};  // PE3  (Arduino D5)
    struct OC3B : AvrOC<0x9A, 0x90, 5, 0x2E, 0x2D, 4> {};  // PE4  (Arduino D2)
    struct OC3C : AvrOC<0x9C, 0x90, 3, 0x2E, 0x2D, 5> {};  // PE5  (Arduino D3)
    struct OC4A : AvrOC<0xA8, 0xA0, 7, 0x102,0x101,3> {};  // PH3  (Arduino D6)
    struct OC4B : AvrOC<0xAA, 0xA0, 5, 0x102,0x101,4> {};  // PH4  (Arduino D7)
    struct OC4C : AvrOC<0xAC, 0xA0, 3, 0x102,0x101,5> {};  // PH5  (Arduino D8)
    struct OC5A : AvrOC<0x128,0x120,7, 0x10B,0x10A,3> {};  // PL3  (Arduino D46)
    struct OC5B : AvrOC<0x12A,0x120,5, 0x10B,0x10A,4> {};  // PL4  (Arduino D45)
    struct OC5C : AvrOC<0x12C,0x120,3, 0x10B,0x10A,5> {};  // PL5  (Arduino D44)

    static constexpr uint8_t SDA_bit  = 1;   // PD1
    static constexpr uint8_t SCL_bit  = 0;   // PD0
    static constexpr uint8_t TX_bit   = 1;   // PE1 (USART0)
    static constexpr uint8_t RX_bit   = 0;   // PE0
    static constexpr uint8_t INT0_bit = 0;   // PD0
    static constexpr uint8_t INT1_bit = 1;   // PD1
    static constexpr uint8_t SS_bit   = 0;   // PB0
    static constexpr uint8_t MOSI_bit = 2;   // PB2
    static constexpr uint8_t MISO_bit = 3;   // PB3
    static constexpr uint8_t SCK_bit  = 1;   // PB1

    struct BoardDef {
      BoardDef() = delete;
      static void begin() {}
    };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

  struct ATmega1280 : ATmega2560Periph {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x20000, 0x100>; // 128k / 256B page
    using Ram_     = Ram<0x0200,  0x2000>;      // 8k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page
  };

  struct ATmega2560 : ATmega2560Periph {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x40000, 0x100>; // 256k / 256B page
    using Ram_     = Ram<0x0200,  0x2000>;      // 8k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page
  };

  // ── ATmega1284P peripheral catalog ───────────────────────────────────────
  struct ATmega1284P {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x20000, 0x100>; // 128k / 256B page
    using Ram_     = Ram<0x0100,  0x4000>;      // 16k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page

    struct PortA : mega1284::PortA {};
    struct PortB : mega1284::PortB {};
    struct PortC : mega1284::PortC {};
    struct PortD : mega1284::PortD {};

    template<void(*fn)() = nullptr> struct TC0 : mega::TC0<fn> {};
    template<void(*fn)() = nullptr> struct TC1 : mega::TC1<fn> {};
    template<void(*fn)() = nullptr> struct TC2 : mega::TC2<fn> {};
    template<void(*fn)() = nullptr> struct TC3 : mega1284::TC3<fn> {};

    struct OC0A : AvrOC<0x47, 0x44, 7, 0x25, 0x24, 3> {};  // PB3
    struct OC0B : AvrOC<0x48, 0x44, 5, 0x25, 0x24, 4> {};  // PB4
    struct OC1A : AvrOC<0x88, 0x80, 7, 0x2B, 0x2A, 5> {};  // PD5
    struct OC1B : AvrOC<0x8A, 0x80, 5, 0x2B, 0x2A, 4> {};  // PD4
    struct OC2A : AvrOC<0xB3, 0xB0, 7, 0x2B, 0x2A, 7> {};  // PD7
    struct OC2B : AvrOC<0xB4, 0xB0, 5, 0x2B, 0x2A, 6> {};  // PD6
    struct OC3A : AvrOC<0x98, 0x90, 7, 0x25, 0x24, 6> {};  // PB6
    struct OC3B : AvrOC<0x9A, 0x90, 5, 0x25, 0x24, 7> {};  // PB7

    static constexpr uint8_t SDA_bit  = 1;   // PC1
    static constexpr uint8_t SCL_bit  = 0;   // PC0
    static constexpr uint8_t TX_bit   = 1;   // PD1
    static constexpr uint8_t RX_bit   = 0;   // PD0
    static constexpr uint8_t INT0_bit = 2;   // PD2
    static constexpr uint8_t INT1_bit = 3;   // PD3
    static constexpr uint8_t SS_bit   = 4;   // PB4
    static constexpr uint8_t MOSI_bit = 5;   // PB5
    static constexpr uint8_t MISO_bit = 6;   // PB6
    static constexpr uint8_t SCK_bit  = 7;   // PB7

    struct BoardDef {
      BoardDef() = delete;
      static void begin() {}
    };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

}} // hw::avr
