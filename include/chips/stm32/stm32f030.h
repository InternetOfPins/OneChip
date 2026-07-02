/**
 * @file stm32f030.h
 * @brief STM32F030 chip catalog — memory + GPIO + USART1 + I2C1 + SPI1.
 *
 * Covers STM32F030F4 (TSSOP20 — "HW-538 STM32F030 DEMO BOARD"): 16K Flash, 4K RAM.
 * F030x4/x6/x8/xC share one die per RM0360 — only Flash/RAM differ.
 *
 * Usage:
 *   #define STM32F0xx
 *   #include <chips/stm32/stm32f030.h>
 *   namespace chip = hw::stm32::f0;
 *   using MyChip = hw::stm32::STM32F030F4;
 *
 *   using Led = STM32::OutPin<Pins<13>, chip::PortA>;  // adjust pin to the board's actual LED
 *   using Ser = chip::Serial0<115200>;                  // USART1 — PA9 TX / PA10 RX
 *   using Twi = MyChip::Twi<100000>;                    // I2C1  — PA9 SCL / PA10 SDA (shares
 *                                                        // pins with Serial0 — use one at a time)
 *   using Spi = MyChip::Spi<4000000>;                   // SPI1  — PA5 SCK / PA6 MISO / PA7 MOSI
 */

#pragma once
#include <chips/stm32/stm32Device.h>
#include <oneBus/uart.h>
#include <oneBus/i2c.h>
#include <oneBus/spi.h>

namespace hw::stm32 {

  // ============================================================
  // Memory specs
  // ============================================================
  template<uint32_t FlashBytes, uint32_t RamBytes>
  struct Stm32F030Mem {
    static constexpr uint32_t flash_size = FlashBytes;
    static constexpr uint32_t ram_size   = RamBytes;
    static constexpr uint32_t flash_base = 0x08000000u;
    static constexpr uint32_t ram_base   = 0x20000000u;
  };

  using STM32F030F4_Mem = Stm32F030Mem<0x4000, 0x1000>;   // 16K / 4K

  // ============================================================
  // STM32F030F4 peripheral catalog (TSSOP20 pinout — PA0-14 partial, PB1, PF0/PF1 osc)
  // ============================================================
  template<typename Mem = STM32F030F4_Mem>
  struct STM32F030 : Mem {

    // ── GPIO ports ──────────────────────────────────────────────
    struct PortA : f0::PortA {};
    struct PortB : f0::PortB {};

    // ── Pin role bits — from the "HW-538" board silkscreen (SWD/UART header) ────
    static constexpr uint8_t SWDIO_bit = 13;  // PA13 — SWD (board's "DIO")
    static constexpr uint8_t SWCLK_bit = 14;  // PA14 — SWD (board's "CLK")
    static constexpr uint8_t TX_bit    =  9;  // PA9  — USART1 TX (board's "TXD")
    static constexpr uint8_t RX_bit    = 10;  // PA10 — USART1 RX (board's "RXD")
    static constexpr uint8_t BOOT0_bit =  0;  // BOOT0 pin (board header, not a GPIO)

    // ── SysClk (named to avoid ARM CMSIS's #define SysTick, see stm32SysClock.h) ───
    template<uint32_t CpuHz = 8000000UL>
    using SysClk = f0::SysClk<CpuHz>;

    // ── Serial (USART1) ─────────────────────────────────────────
    // Default CpuHz = 8 MHz — HSI reset default, no PLL configured (matches SysClk's
    // default above; F030 can run up to 48MHz via PLL but that's not set up here yet).
    // Stm32UsartV2Core, not Stm32UsartCore — F0's USART register map differs from F1/F4.
    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 8000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartV2Core<0x40013800u, Stm32F0_Usart1_PA9_PA10, CpuHz>>;

    // ── I2C1 (PA9 SCL / PA10 SDA — shares pins with Serial0) ──────
    // Stm32I2cV2Core, not Stm32I2cCore — F0's I2C register/protocol design
    // differs from F1/F4's. Only (8MHz,100kHz) and (48MHz,100kHz) TIMINGR
    // values are verified — pass a different ApbHz/SclHz and it won't compile
    // until a matching TIMINGR constant is added (see stm32Twi.h).
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 8000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cV2Core<0x40005400u, Stm32F0_I2c1_PA9_PA10, ApbHz, SclHz>>;

    // ── SPI1 (PA5 SCK / PA6 MISO / PA7 MOSI) ───────────────────────
    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 8000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                            Stm32SpiCore<0x40013000u, Stm32F0_Spi1_PA5_PA6_PA7,
                                         ApbHz, Mode, MSBFirst>>;
  };

  // Convenience: chip-specific name
  using STM32F030F4 = STM32F030<STM32F030F4_Mem>;

} // hw::stm32
