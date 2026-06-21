#pragma once
#include <chips/avr/avrPort.h>
#include <chips/avr/avrOC.h>
#include <chips/avr/avrEeprom.h>
#include <chips/avr/avrTimerInit.h>
#include <hapi/hapi.h>

// ATtiny family chip catalogs.
// Same named-struct pattern as atmega.h — struct names appear in error messages.

namespace hw::avr {

  // ── ATtiny85 peripheral catalog ──────────────────────────────────────────
  // 8-pin DIP/SOIC: PB0..PB5 (PB5=RESET).
  // Single port (B). Two timers: TC0 (8-bit standard) + TC1 (8-bit PLL).
  //
  // ATtiny85 register addresses (memory-mapped = I/O + 0x20):
  //   PINB=0x36  DDRB=0x37  PORTB=0x38
  //   TCCR0A=0x4A  OCR0A=0x49  OCR0B=0x48  TCCR0B=0x53
  //   GTCCR=0x4C  OCR1B=0x4D  TCCR1=0x4F  OCR1C=0x4E
  //
  // OC pin map:
  //   OC0A → PB0 (pin 5)     OC0B → PB1 (pin 6)
  //   OC1A → PB1 (pin 6, shared with OC0B — avoid using both)
  //   OC1B → PB4 (pin 3)

  struct ATtiny85 {
    // Port B: all 6 usable pins (PB0..PB5)
    struct PortB : AVRPort<0x36, 0x37, 0x38> {};

    // Timer0 — 8-bit, same structure as ATmega TC0 but different addresses
    // Init: use ATtiny85-specific Timer0 init components (below)
    struct OC0A : AvrOC<0x49, 0x4A, 7, 0x38, 0x37, 0> {};  // PB0
    struct OC0B : AvrOC<0x48, 0x4A, 5, 0x38, 0x37, 1> {};  // PB1

    // Timer1 — 8-bit with PLL clock option.
    // OC1B control lives in GTCCR (0x4C): PWM1B(6) COM1B1(5) COM1B0(4).
    // enable() must also set PWM1B; disable() must clear it.
    struct OC1B : AvrOC<0x4D, 0x4C, 5, 0x38, 0x37, 4> {     // PB4
      static void enable() {
        // COM1B=10 (non-inverting) + PWM1B=1
        auto& g = *reinterpret_cast<volatile uint8_t*>(0x4C);
        g = (g | (1<<6)|(1<<5)) & ~uint8_t(1<<4);
      }
      static void disable() {
        auto& g = *reinterpret_cast<volatile uint8_t*>(0x4C);
        g &= ~uint8_t((1<<6)|(1<<5)|(1<<4));
      }
    };

    // Flash / RAM / EEPROM
    static constexpr uint16_t flashSize  = 8192;
    static constexpr uint16_t ramSize    =  512;
    using Eep = Tiny85Eeprom;  // 512B, 4B page

    // Function bit positions on PortB
    static constexpr uint8_t MOSI_bit = 0;  // PB0 (USI DI)
    static constexpr uint8_t MISO_bit = 1;  // PB1 (USI DO)
    static constexpr uint8_t SCK_bit  = 2;  // PB2 (USI SCK / SCL)
    static constexpr uint8_t SDA_bit  = 0;  // PB0 (USI DI — I2C slave only)
    static constexpr uint8_t SCL_bit  = 2;  // PB2 (USI SCK)
    static constexpr uint8_t INT0_bit = 2;  // PB2 (INT0)
    static constexpr uint8_t RESET_bit= 5;  // PB5 (active-low RESET)

    struct BoardDef { BoardDef() = delete; };
    template<typename... CC> using Board = hapi::APIOf<BoardDef, CC...>;
  };

  // ── ATtiny85 timer init Board<> components ───────────────────────────────
  // ATtiny85 TC0 addresses differ from ATmega (0x4A/0x53 vs 0x44/0x45).
  // ATtiny85 TC1: set CTC1=0 in TCCR1 + clock bits; PWM1B set by OC1B::enable().

  // TC0 fast PWM (WGM=3): TCCR0A=0x4A, TCCR0B=0x53
  struct ATtiny85_Timer0Pwm1    : AvrTimer8FastPwm<0x4A,0x53, 0x01> {};  // PS=1   ~31250Hz @8MHz
  struct ATtiny85_Timer0Pwm8    : AvrTimer8FastPwm<0x4A,0x53, 0x02> {};  // PS=8   ~3906 Hz
  struct ATtiny85_Timer0Pwm64   : AvrTimer8FastPwm<0x4A,0x53, 0x03> {};  // PS=64  ~488  Hz
  struct ATtiny85_Timer0Pwm256  : AvrTimer8FastPwm<0x4A,0x53, 0x04> {};  // PS=256 ~122  Hz
  struct ATtiny85_Timer0Pwm1024 : AvrTimer8FastPwm<0x4A,0x53, 0x05> {};  // PS=1024~30   Hz

  // TC1 clock — sets CS1 in TCCR1 (0x4F) bits [3:0], no WGM change
  // (PWM1B mode is activated by OC1B::enable() via GTCCR)
  template<uint8_t CS1>
  struct AvrTimer1_Tiny85 {
    template<typename O> struct Part : O {
      static void begin() {
        auto& t = *reinterpret_cast<volatile uint8_t*>(0x4F);  // TCCR1
        t = (t & 0xF0) | (CS1 & 0x0F);
        O::begin();
      }
    };
  };
  struct ATtiny85_Timer1Clk1    : AvrTimer1_Tiny85<0x01> {};  // clk/1
  struct ATtiny85_Timer1Clk2    : AvrTimer1_Tiny85<0x02> {};  // clk/2
  struct ATtiny85_Timer1Clk4    : AvrTimer1_Tiny85<0x03> {};  // clk/4
  struct ATtiny85_Timer1Clk8    : AvrTimer1_Tiny85<0x04> {};  // clk/8
  struct ATtiny85_Timer1Clk16   : AvrTimer1_Tiny85<0x05> {};  // clk/16
  struct ATtiny85_Timer1Clk32   : AvrTimer1_Tiny85<0x06> {};  // clk/32
  struct ATtiny85_Timer1Clk64   : AvrTimer1_Tiny85<0x07> {};  // clk/64

} // hw::avr
