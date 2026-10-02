/**
 * @file esp8266Spi.h
 * @brief ESP8266 hardware SPI (HSPI) core — wraps Arduino SPI.h.
 *
 * Fixed pins: SCK=GPIO14 (D5), MISO=GPIO12 (D6), MOSI=GPIO13 (D7). Chip select is the layer above
 * (oneBus::CsPin<> or oneBus::SpiSlots<>), never the hardware CS: one bus, many devices.
 * spi_setup(hz, mode) changes clock and mode at runtime, so devices with different needs share the bus.
 *
 * Usage:
 *   #include <chips/esp8266/esp8266Spi.h>
 *   #include <chips/esp8266/esp8266Gpio.h>
 *   using Bus = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiSlots<hw::esp8266::OutPin<15>>,
 *                           oneBus::SpiMaster<4000000>, hw::esp8266::Esp8266SpiCore>;
 */

#pragma once
#include <stdint.h>
#include <hapi/hapi.h>
#include <oneBus/spi.h>
#ifdef ARDUINO
  #include <SPI.h>
#endif

namespace hw::esp8266 {

#ifdef ARDUINO

  /// @brief ESP8266 HSPI core over Arduino SPI: spi_init / spi_setup(hz, mode) / spi_transfer
  struct Esp8266SpiCore {
    template<typename O>
    struct Part : O {
      static void spi_init(uint32_t hz) { SPI.begin(); SPI.setFrequency(hz); SPI.setDataMode(SPI_MODE0); }
      static void spi_setup(uint32_t hz, uint8_t mode) {
        SPI.setFrequency(hz);
        SPI.setDataMode(mode == 1 ? SPI_MODE1 : mode == 2 ? SPI_MODE2 : mode == 3 ? SPI_MODE3 : SPI_MODE0);
      }
      [[nodiscard]] static uint8_t spi_transfer(uint8_t b) { return SPI.transfer(b); }
      static void begin() { O::begin(); }
    };
  };

  namespace esp8266 {
    template<uint32_t Speed = 4000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>, Esp8266SpiCore>;
  }

#endif

} // hw::esp8266
