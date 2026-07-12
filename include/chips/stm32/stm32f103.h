/**
 * @file stm32f103.h
 * @brief STM32F103 chip catalog (Blue Pill / Maple Mini class).
 *
 * Covers STM32F103C6/C8/CB (LQFP48, 48-pin):
 *   F103C6: 32K Flash, 10K RAM
 *   F103C8: 64K Flash, 20K RAM  ← Blue Pill
 *   F103CB: 128K Flash, 20K RAM
 *
 * Usage:
 *   #define STM32F1xx
 *   #include <chips/stm32/stm32f103.h>
 *   namespace chip = hw::stm32::f1;   // or set via avrDevice.h-style macro
 *
 *   using Led = STM32::OutPin<Pins<13>, chip::PortC>;
 *   using Ser = chip::Serial0<115200>;
 *   using Twi = chip::Twi<100000>;
 *   using Spi = chip::Spi<4000000>;
 */

#pragma once
#include <chips/stm32/stm32Device.h>
#include <oneBus/uart.h>
#include <oneBus/i2c.h>
#include <oneBus/spi.h>

namespace hw::stm32 {

  // Memory specs
  template<uint32_t FlashBytes, uint32_t RamBytes>
  struct Stm32F103Mem {
    static constexpr uint32_t flash_size = FlashBytes;
    static constexpr uint32_t ram_size   = RamBytes;
    static constexpr uint32_t flash_base = 0x08000000u;
    static constexpr uint32_t ram_base   = 0x20000000u;
  };

  using STM32F103C6_Mem = Stm32F103Mem<0x8000,  0x2800>;   // 32K / 10K
  using STM32F103C8_Mem = Stm32F103Mem<0x10000, 0x5000>;   // 64K / 20K
  using STM32F103CB_Mem = Stm32F103Mem<0x20000, 0x5000>;   // 128K / 20K

  // STM32F103 peripheral catalog (LQFP48 pinout)
  // All F103Cx share the same peripherals; only Flash/RAM differ.
  template<typename Mem = STM32F103C8_Mem>
  struct STM32F103 : Mem {

    // ── GPIO ports ──────────────────────────────────────────────
    struct PortA : f1::PortA {};
    struct PortB : f1::PortB {};
    struct PortC : f1::PortC {};

    // ── Pin role bits (for documentation / static_assert use) ───
    static constexpr uint8_t LED_bit   = 13;  // PC13 — Blue Pill on-board LED (active-low)
    static constexpr uint8_t TX_bit    =  9;  // PA9  — USART1 TX
    static constexpr uint8_t RX_bit    = 10;  // PA10 — USART1 RX
    static constexpr uint8_t SCL_bit   =  6;  // PB6  — I2C1 SCL
    static constexpr uint8_t SDA_bit   =  7;  // PB7  — I2C1 SDA
    static constexpr uint8_t SCK_bit   =  5;  // PA5  — SPI1 SCK
    static constexpr uint8_t MISO_bit  =  6;  // PA6  — SPI1 MISO
    static constexpr uint8_t MOSI_bit  =  7;  // PA7  — SPI1 MOSI
    static constexpr uint8_t USB_DP    = 12;  // PA12 — USB D+
    static constexpr uint8_t USB_DM    = 11;  // PA11 — USB D-
    static constexpr uint8_t BOOT0_bit =  0;  // BOOT0 pin (board header)

    // ── Serial (USART) ───────────────────────────────────────────
    // Default CpuHz = 72 MHz (PLL ×9 from 8 MHz HSE / HSI)
    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 72000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40013800u, Stm32F1_Usart1_PA9_PA10, CpuHz>>;

    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 72000000UL>
    using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004400u, Stm32F1_Usart2_PA2_PA3, CpuHz>>;

    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 72000000UL>
    using Serial2 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004800u, Stm32F1_Usart3_PB10_PB11, CpuHz>>;

    // ── I2C ──────────────────────────────────────────────────────
    // ApbHz = APB1 clock = CpuHz/2 at 72 MHz PLL
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 36000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F1_I2c1_PB6_PB7, ApbHz, SclHz>>;

    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 36000000UL>
    using Twi2 = hapi::APIOf<oneBus::TwiAPI,
                             Stm32I2cCore<0x40005800u, Stm32F1_I2c2_PB10_PB11, ApbHz, SclHz>>;

    // ── SPI ──────────────────────────────────────────────────────
    // SPI1 on APB2 = CpuHz at 72 MHz; SPI2 on APB1 = CpuHz/2
    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 72000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                            Stm32SpiCore<0x40013000u, Stm32F1_Spi1_PA5_PA6_PA7,
                                         ApbHz, Mode, MSBFirst>>;

    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 36000000UL>
    using Spi2 = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                             Stm32SpiCore<0x40003800u, Stm32F1_Spi2_PB13_PB14_PB15,
                                          ApbHz, Mode, MSBFirst>>;

    // ── SysClk (named to avoid ARM CMSIS's #define SysTick, see stm32SysClock.h) ───
    template<uint32_t CpuHz = 72000000UL>
    using SysClk = f1::SysClk<CpuHz>;
  };

  // Convenience: chip-specific names
  using STM32F103C8 = STM32F103<STM32F103C8_Mem>;
  using STM32F103CB = STM32F103<STM32F103CB_Mem>;
  using STM32F103C6 = STM32F103<STM32F103C6_Mem>;
  using BluePill    = STM32F103C8;

  namespace f1 {
    // chip::Twi<> and chip::Spi<> via STM32F103 catalog
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 36000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F1_I2c1_PB6_PB7, ApbHz, SclHz>>;

    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 72000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                            Stm32SpiCore<0x40013000u, Stm32F1_Spi1_PA5_PA6_PA7,
                                         ApbHz, Mode, MSBFirst>>;
  }

} // hw::stm32
