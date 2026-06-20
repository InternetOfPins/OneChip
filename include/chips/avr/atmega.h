/**
 * @file atmega.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief ATmega family chip definitions — values from Microchip ATmega_DFP atdf XML.
 *        One file, all classic ATmega devices. Include only this, pay only for what you use.
 */

#pragma once
#include <chips/avr/watchdog.h>
#include <chips/avr/flashMem.h>

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

  struct ATmega88 {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x2000, 0x40>;   // 8k / 64B page
    using Ram_     = Ram<0x0100, 0x0400>;       // 1k SRAM
    using Eeprom_  = Eeprom<0x0200, 0x04>;      // 512B / 4B page
  };

  struct ATmega168 {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x4000, 0x80>;   // 16k / 128B page
    using Ram_     = Ram<0x0100, 0x0400>;       // 1k SRAM
    using Eeprom_  = Eeprom<0x0200, 0x04>;      // 512B / 4B page
  };

  struct ATmega328 {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x8000, 0x80>;   // 32k / 128B page
    using Ram_     = Ram<0x0100, 0x0800>;       // 2k SRAM
    using Eeprom_  = Eeprom<0x0400, 0x04>;      // 1k / 4B page
  };

  using ATmega328P  = ATmega328;
  using ATmega328PB = ATmega328;

  struct ATmega1280 {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x20000, 0x100>; // 128k / 256B page
    using Ram_     = Ram<0x0200,  0x2000>;      // 8k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page
  };

  struct ATmega2560 {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x40000, 0x100>; // 256k / 256B page
    using Ram_     = Ram<0x0200,  0x2000>;      // 8k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page
  };

  struct ATmega1284P {
    using Watchdog = ATmegaWatchdog;
    using Flash    = FlashMem<0x20000, 0x100>; // 128k / 256B page
    using Ram_     = Ram<0x0100,  0x4000>;      // 16k SRAM
    using Eeprom_  = Eeprom<0x1000, 0x08>;      // 4k / 8B page
  };

}} // hw::avr
