#pragma once
#include <stdint.h>

// ESP32 SPI master.
// ESP32 default VSPI pins: MOSI=23, MISO=19, SCK=18.
// CS is handled by the transport (SpiOled<SpiMaster,CsPin,...>).
//
// #ifdef ARDUINO: wraps Arduino SPI class.
// otherwise: uses ESP-IDF spi_master driver (stub — implement when needed).

#ifdef ARDUINO
  #include <SPI.h>
#endif

namespace hw::esp32 {

#ifdef ARDUINO

  template<int MOSI = 23, int MISO = 19, int SCK = 18>
  struct Esp32SpiMaster {
    static void begin() {
      SPI.begin(SCK, MISO, MOSI, -1);  // -1 = no hardware CS
      SPI.setFrequency(8000000UL);
      SPI.setDataMode(SPI_MODE0);
    }
    static uint8_t transfer(uint8_t b) { return SPI.transfer(b); }
    static void transfer(const uint8_t* buf, uint16_t len) {
      while (len--) SPI.transfer(*buf++);
    }
    static void fill(uint8_t b, uint16_t count) {
      while (count--) SPI.transfer(b);
    }
  };

#else // ESP-IDF stub

  template<int MOSI = 23, int MISO = 19, int SCK = 18>
  struct Esp32SpiMaster {
    static void begin()                               { /* TODO: spi_bus_initialize */ }
    static uint8_t transfer(uint8_t)                  { return 0; }
    static void transfer(const uint8_t*, uint16_t)    {}
    static void fill(uint8_t, uint16_t)               {}
  };

#endif

  namespace esp32 {
    template<int MOSI = 23, int MISO = 19, int SCK = 18>
    using SpiMaster = Esp32SpiMaster<MOSI, MISO, SCK>;
  }

} // hw::esp32
