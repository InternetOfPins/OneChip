/**
 * @file watchdog.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR Watchdog HAPI components — two unlock variants, selected by chip definition.
 */

#pragma once

namespace hw {
namespace avr {

  struct WatchdogClassic {
    static void reset() { __asm__ __volatile__ ("wdr\n"); }
    static void config(uint8_t x) {
      WDTCSR = _BV(WDCE) | _BV(WDE);
      WDTCSR = x;
    }
  };

  struct WatchdogCcp {
    static void reset() { __asm__ __volatile__ ("wdr\n"); }
    static void config(uint8_t x) {
      CCP    = 0xD8;
      WDTCSR = x;
    }
  };

}} // hw::avr
