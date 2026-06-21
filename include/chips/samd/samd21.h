#pragma once
#include <hapi/hapi.h>
#include <stdint.h>

// ATSAMD21G18 chip catalog — Crowduino M0-SD / Arduino Zero compatible.
//
// Architecture: ARM Cortex-M0+ @ 48 MHz, 256 KB Flash, 32 KB SRAM.
// Peripherals: 6× SERCOM (configurable as UART/SPI/I2C), 3× TCC, 3× TC (16-bit),
//              48 GPIO pins in 2 groups (PA/PB), 12-bit ADC, USB FS native.
//
// Unlike AVR, SAMD21 has no fixed peripheral-to-pin assignment — SERCOM pads
// are routed via PMUX multiplexer at runtime. The Board<> chain configures PMUX
// and GCLK feeds. Actual register programming is deferred to SERCOM/TCC/TC wrappers.
//
// This catalog provides:
//   - SERCOM0..SERCOM5 type tags (pin mux configuration lives in board.h)
//   - TCC0/TCC1/TCC2 type tags (4/2/2 channels × 24/16-bit)
//   - TC3/TC4/TC5 type tags (16-bit, 2 channels each)
//   - PA / PB group types (32 pins each, 26+10 physically bonded on QFN48)
//   - Memory specs

namespace hw::samd {

  // ── GPIO groups ──────────────────────────────────────────────────────────
  // SAMD21 uses PORT->Group[0] (PA) and PORT->Group[1] (PB).
  // Pins are addressed as group+offset: PA0..PA31, PB0..PB31.
  // Not all physical pins are bonded — QFN48 has PA0..PA28, PB[2,3,8,9,10,11].

  struct PortA { static constexpr uint8_t group = 0; };
  struct PortB { static constexpr uint8_t group = 1; };

  // Helper: encodes a pin as group/offset for use in PMUX and PORT tables
  template<typename Group, uint8_t Pin>
  struct SamdPin {
    static constexpr uint8_t group = Group::group;
    static constexpr uint8_t pin   = Pin;
    // Arduino pin number depends on board variant — defined in board.h
  };

  // ── SERCOM instances ──────────────────────────────────────────────────────
  // SERCOM0..5 can each be configured as UART / SPI master or slave / I2C master or slave.
  // Pad assignment (which pads become TX/RX/SDA/SCL/MOSI/MISO/SCK) is
  // set in the CTRLA.DOPO / CTRLA.DIPO fields — handled in board.h.

  struct Sercom0 { static constexpr uint8_t idx = 0; };
  struct Sercom1 { static constexpr uint8_t idx = 1; };
  struct Sercom2 { static constexpr uint8_t idx = 2; };
  struct Sercom3 { static constexpr uint8_t idx = 3; };
  struct Sercom4 { static constexpr uint8_t idx = 4; };
  struct Sercom5 { static constexpr uint8_t idx = 5; };

  // ── Timer/Counter for Control (TCC) ──────────────────────────────────────
  // TCC0: 4 channels (WO0..WO3), 24-bit counter — highest resolution
  // TCC1: 2 channels (WO0..WO1), 24-bit counter
  // TCC2: 2 channels (WO0..WO1), 16-bit counter

  struct TCC0 { static constexpr uint8_t idx = 0; static constexpr uint8_t channels = 4; };
  struct TCC1 { static constexpr uint8_t idx = 1; static constexpr uint8_t channels = 2; };
  struct TCC2 { static constexpr uint8_t idx = 2; static constexpr uint8_t channels = 2; };

  // ── Timer/Counter (TC) ───────────────────────────────────────────────────
  // TC3/TC4/TC5: 16-bit, 2 channels (MC0/MC1) each.
  // TC3 is often used by Arduino millis() — avoid if framework is active.

  struct TC3  { static constexpr uint8_t idx = 3; };
  struct TC4  { static constexpr uint8_t idx = 4; };
  struct TC5  { static constexpr uint8_t idx = 5; };

  // ── Memory ───────────────────────────────────────────────────────────────

  static constexpr uint32_t flashSize = 262144;  // 256 KB
  static constexpr uint32_t ramSize   = 32768;   // 32 KB
  // No internal EEPROM — use SD card or emulated Flash (via FlashStorage lib)

  // ── Chip descriptor ──────────────────────────────────────────────────────

  struct ATSAMD21G18 {
    using SercomI2C  = Sercom3;  // Arduino Zero / Crowduino default I2C SERCOM
    using SercomSPI  = Sercom4;  // default SPI SERCOM
    using SercomUART = Sercom0;  // default UART (Serial1 on D0/D1)

    struct BoardDef { BoardDef() = delete; };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

  // Crowduino M0-SD is pin-compatible with Arduino Zero
  using CrowduinoM0SD = ATSAMD21G18;
  using ArduinoZero   = ATSAMD21G18;

} // hw::samd
