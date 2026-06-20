#pragma once
#include <stdint.h>
#include <avr/io.h>

namespace hw::avr {

  // AVR hardware SPI master — bare register driver.
  // Uses fixed hardware pins: MOSI=PB3, SCK=PB5, SS=PB2 (must be output).
  // MISO=PB4 configured as input automatically.
  // Default clock: FOSC/2 (8 MHz @ 16 MHz CPU).
  struct AvrSpiMaster {
    static void begin() {
      DDRB  |= (1<<PB3)|(1<<PB5)|(1<<PB2);  // MOSI, SCK, SS as outputs
      PORTB |= (1<<PB2);                      // SS idle high
      SPCR   = (1<<SPE)|(1<<MSTR);            // enable, master, mode 0
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
