/**
 * @file flashMem.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR flash self-programming component — SPM page erase/write, LPM read.
 */

#pragma once
#include <avr/boot.h>
#include <avr/pgmspace.h>

namespace hw {
namespace avr {

  template<uint32_t Size, uint16_t PageSize>
  struct FlashMem {
    static constexpr uint32_t size     = Size;
    static constexpr uint16_t pageSize = PageSize;
    static constexpr uint32_t end      = Size - 1;

    struct Part {
      static void erase(uint32_t addr) {
        boot_page_erase(addr);
        boot_spm_busy_wait();
      }

      static void fill(uint32_t addr, uint16_t word) {
        boot_page_fill(addr, word);
      }

      static void write(uint32_t addr) {
        boot_page_write(addr);
        boot_spm_busy_wait();
        boot_rww_enable();
      }

      static uint8_t read(uint32_t addr) {
        return pgm_read_byte(addr);
      }

      static uint16_t readWord(uint32_t addr) {
        return pgm_read_word(addr);
      }
    };
  };

}} // hw::avr
