/**
 * @file stm32f407.h
 * @brief STM32F407 chip catalog (F4 Discovery / Nucleo-144 class).
 *
 * Covers STM32F407VG (LQFP100, 168 MHz, 1M Flash, 192K RAM).
 * Same peripheral layout applies to F405/F415/F417.
 *
 * Usage:
 *   #include <chips/stm32/stm32f407.h>
 *   using Led = STM32::OutPin<Pins<12>, hw::stm32::f4::PortD>; // F4Discovery green
 *   using Ser = hw::stm32::STM32F407::Serial0<115200>;
 *   using Twi = hw::stm32::STM32F407::Twi<400000>;
 */

#pragma once
#include <chips/stm32/stm32Device.h>
#include <oneBus/uart.h>
#include <oneBus/i2c.h>
#include <oneBus/spi.h>

namespace hw::stm32 {

  // ============================================================
  // STM32F407VG peripheral catalog
  // CpuHz = 168 MHz (PLL from 8 MHz HSE); APB1 = 42 MHz, APB2 = 84 MHz
  // ============================================================
  struct STM32F407 {
    static constexpr uint32_t flash_size = 0x100000u;  // 1M
    static constexpr uint32_t ram_size   = 0x20000u;   // 128K (+ 64K CCM @ 0x10000000)
    static constexpr uint32_t ccm_size   = 0x10000u;   // 64K CCM-SRAM
    static constexpr uint32_t flash_base = 0x08000000u;
    static constexpr uint32_t ram_base   = 0x20000000u;
    static constexpr uint32_t ccm_base   = 0x10000000u;

    // ── GPIO ports ──────────────────────────────────────────────
    struct PortA : f4::PortA {};
    struct PortB : f4::PortB {};
    struct PortC : f4::PortC {};
    struct PortD : f4::PortD {};
    struct PortE : f4::PortE {};

    // ── F4 Discovery on-board LEDs (PD12-PD15) ──────────────────
    static constexpr uint8_t LED_GREEN  = 12;  // PD12
    static constexpr uint8_t LED_ORANGE = 13;  // PD13
    static constexpr uint8_t LED_RED    = 14;  // PD14
    static constexpr uint8_t LED_BLUE   = 15;  // PD15
    static constexpr uint8_t BTN_bit    =  0;  // PA0  — User button

    // ── Pin roles ────────────────────────────────────────────────
    static constexpr uint8_t TX_bit   =  9;  // PA9  — USART1 TX
    static constexpr uint8_t RX_bit   = 10;  // PA10 — USART1 RX
    static constexpr uint8_t SCL_bit  =  6;  // PB6  — I2C1 SCL (default)
    static constexpr uint8_t SDA_bit  =  7;  // PB7  — I2C1 SDA
    static constexpr uint8_t SCK_bit  =  5;  // PA5  — SPI1 SCK
    static constexpr uint8_t MISO_bit =  6;  // PA6  — SPI1 MISO
    static constexpr uint8_t MOSI_bit =  7;  // PA7  — SPI1 MOSI

    // ── Serial (USART) ───────────────────────────────────────────
    // USART1 on APB2 = 84 MHz; USART2/3 on APB1 = 42 MHz
    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 84000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40011000u, Stm32F4_Usart1_PA9_PA10, CpuHz>>;

    template<uint32_t BaudRate = 115200UL, uint32_t CpuHz = 42000000UL>
    using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004400u, Stm32F4_Usart2_PA2_PA3, CpuHz>>;

    // ── I2C ──────────────────────────────────────────────────────
    // APB1 = 42 MHz at 168 MHz CPU
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F4_I2c1_PB6_PB7, ApbHz>>;

    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
    using Twi_PB8_PB9 = hapi::APIOf<oneBus::TwiAPI,
                                    Stm32I2cCore<0x40005400u, Stm32F4_I2c1_PB8_PB9, ApbHz>>;

    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
    using Twi2 = hapi::APIOf<oneBus::TwiAPI,
                             Stm32I2cCore<0x40005800u, Stm32F4_I2c2_PB10_PB11, ApbHz>>;

    // ── SPI ──────────────────────────────────────────────────────
    // SPI1 on APB2 = 84 MHz; SPI2 on APB1 = 42 MHz
    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 84000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                            Stm32SpiCore<0x40013000u, Stm32F4_Spi1_PA5_PA6_PA7,
                                         ApbHz, Mode, MSBFirst>>;

    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 42000000UL>
    using Spi2 = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                             Stm32SpiCore<0x40003800u, Stm32F4_Spi2_PB13_PB14_PB15,
                                          ApbHz, Mode, MSBFirst>>;

    // ── SysTick ──────────────────────────────────────────────────
    template<uint32_t CpuHz = 168000000UL>
    using SysTick = f4::SysTick<CpuHz>;
  };

  namespace f4 {
    // chip::Twi<> and chip::Spi<> via F4 defaults
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F4_I2c1_PB6_PB7, ApbHz>>;

    template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
             bool MSBFirst = true, uint32_t ApbHz = 84000000UL>
    using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                            Stm32SpiCore<0x40013000u, Stm32F4_Spi1_PA5_PA6_PA7,
                                         ApbHz, Mode, MSBFirst>>;
  }

} // hw::stm32
