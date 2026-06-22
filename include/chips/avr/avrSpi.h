#pragma once
#include <stdint.h>
#include <avr/io.h>

namespace hw::avr {

  // HAPI hardware core — AVR SPI master, fixed pins (MOSI=PB3, SCK=PB5, MISO=PB4).
  // SS=PB2 is set as output so the MSTR bit stays armed; per-device chip-select
  // is the layer above (ChipSelect<>). Mode 0-3 (CPOL/CPHA). MSBFirst: DORD bit.
  template<uint32_t CpuHz = 16000000UL, uint8_t Mode = 0, bool MSBFirst = true>
  struct AvrSpiCore {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void spi_init(uint32_t speed) {
        DDRB  |=  (1<<PB3)|(1<<PB5)|(1<<PB2);   // MOSI, SCK, SS outputs
        DDRB  &= ~(1<<PB4);                        // MISO input
        PORTB |=  (1<<PB2);                        // SS idle high

        uint8_t spcr = (1<<SPE)|(1<<MSTR);
        if (!MSBFirst)    spcr |= (1<<DORD);
        if (Mode & 2)     spcr |= (1<<CPOL);
        if (Mode & 1)     spcr |= (1<<CPHA);

        uint8_t spsr = 0;
        // smallest divider that still delivers ≤ speed
        if      (speed >= CpuHz / 2)   { spsr = (1<<SPI2X); }
        else if (speed >= CpuHz / 4)   { }
        else if (speed >= CpuHz / 8)   { spsr = (1<<SPI2X); spcr |= (1<<SPR0); }
        else if (speed >= CpuHz / 16)  { spcr |= (1<<SPR0); }
        else if (speed >= CpuHz / 32)  { spsr = (1<<SPI2X); spcr |= (1<<SPR1); }
        else if (speed >= CpuHz / 64)  { spcr |= (1<<SPR1); }
        else                           { spcr |= (1<<SPR1)|(1<<SPR0); }  // CpuHz/128

        SPCR = spcr;
        SPSR = spsr;
      }

      static uint8_t spi_transfer(uint8_t b) {
        SPDR = b;
        while (!(SPSR & (1<<SPIF)));
        return SPDR;
      }

      static void begin() { Base::begin(); }
    };
  };

  // ── Backward-compat standalone driver ────────────────────────────────────
  // Use hw::avr::mega::Spi<> from OneBus/spi.h for new code.
  struct AvrSpiMaster {
    static void begin() {
      DDRB  |= (1<<PB3)|(1<<PB5)|(1<<PB2);
      PORTB |= (1<<PB2);
      SPCR   = (1<<SPE)|(1<<MSTR);
      SPSR  |= (1<<SPI2X);                    // FOSC/2
    }
    static uint8_t transfer(uint8_t b) {
      SPDR = b;
      while (!(SPSR & (1<<SPIF)));
      return SPDR;
    }
    static void transfer(const uint8_t* buf, uint16_t len) {
      while (len--) { SPDR = *buf++; while (!(SPSR & (1<<SPIF))); }
    }
    static void fill(uint8_t b, uint16_t count) {
      while (count--) { SPDR = b; while (!(SPSR & (1<<SPIF))); }
    }
  };

  namespace mega {
    using SpiMaster = AvrSpiMaster;
  }

} // hw::avr
