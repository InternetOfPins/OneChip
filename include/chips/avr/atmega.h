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

    // ── Port catalog ─────────────────────────────────────────────────────
    using PortB = mega::PortB;   // 0x23 — PB0..PB7
    using PortC = mega::PortC;   // 0x26 — PC0..PC5 (PC6=RESET)
    using PortD = mega::PortD;   // 0x29 — PD0..PD7

    // ── Timer catalog ────────────────────────────────────────────────────
    // TC0: 8-bit, CS1 prescaler. fn = compile-time ISR callback (nullptr = none)
    template<void(*fn)() = nullptr> using TC0 = mega::TC0<fn>;
    // TC1: 16-bit, CS1 prescaler.
    template<void(*fn)() = nullptr> using TC1 = mega::TC1<fn>;
    // TC2: 8-bit, CS2 prescaler (different prescaler set from TC0/TC1).
    template<void(*fn)() = nullptr> using TC2 = mega::TC2<fn>;

    // ── Output Compare channels — satisfy PWMChan::PwmPin contract ───────
    // AvrOC<OCR_ADDR, TCCRA_ADDR, COM_HI, PORT_ADDR, DDR_ADDR, PIN_BIT>
    using OC0A = AvrOC<0x47, 0x44, 7, 0x2B, 0x2A, 6>;  // TC0 chA → PD6 (Arduino D6)
    using OC0B = AvrOC<0x48, 0x44, 5, 0x2B, 0x2A, 5>;  // TC0 chB → PD5 (Arduino D5)
    using OC1A = AvrOC<0x88, 0x80, 7, 0x25, 0x24, 1>;  // TC1 chA → PB1 (Arduino D9)
    using OC1B = AvrOC<0x8A, 0x80, 5, 0x25, 0x24, 2>;  // TC1 chB → PB2 (Arduino D10)
    using OC2A = AvrOC<0xB3, 0xB0, 7, 0x25, 0x24, 3>;  // TC2 chA → PB3 (Arduino D11)
    using OC2B = AvrOC<0xB4, 0xB0, 5, 0x2B, 0x2A, 3>;  // TC2 chB → PD3 (Arduino D3)

    // ── Named function aliases ────────────────────────────────────────────
    // Use with AVR::OutPin<Pins<N>, ATmega328::PortX> at board level.
    // Bit positions within their respective port:
    static constexpr uint8_t SDA_bit  = 4;   // PC4 — TWI data
    static constexpr uint8_t SCL_bit  = 5;   // PC5 — TWI clock
    static constexpr uint8_t TX_bit   = 1;   // PD1 — USART0 TX
    static constexpr uint8_t RX_bit   = 0;   // PD0 — USART0 RX
    static constexpr uint8_t INT0_bit = 2;   // PD2 — external interrupt 0
    static constexpr uint8_t INT1_bit = 3;   // PD3 — external interrupt 1
    static constexpr uint8_t SS_bit   = 2;   // PB2 — SPI SS
    static constexpr uint8_t MOSI_bit = 3;   // PB3 — SPI MOSI
    static constexpr uint8_t MISO_bit = 4;   // PB4 — SPI MISO
    static constexpr uint8_t SCK_bit  = 5;   // PB5 — SPI clock / Arduino LED

    // ── Board<Components...> ─────────────────────────────────────────────
    // HAPI chain for this chip in a specific application.
    // Components select and initialise the peripherals actually used.
    // "Board" emphasises the physical binding — wires don't change at runtime.
    struct BoardDef { BoardDef() = delete; };
    template<typename... Components>
    using Board = hapi::APIOf<BoardDef, Components...>;
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
