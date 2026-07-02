/**
 * @file stm32Spi.h
 * @brief STM32 SPI hardware core — register map + pin-config policies.
 *
 * SPI register layout (same across F1/F4/L4/H7):
 *   0x00  CR1  — BIDIMODE=15, DFF=11, RXONLY=10, SSM=9, SSI=8,
 *                LSBFIRST=7, SPE=6, BR[2:0]=3-5, MSTR=2, CPOL=1, CPHA=0
 *   0x04  CR2  — TXEIE=7, RXNEIE=6, ERRIE=5, SSOE=2
 *   0x08  SR   — BSY=7, TXE=1, RXNE=0
 *   0x0C  DR   — data (8 or 16-bit)
 *
 * BR[2:0] selects fPCLK / 2^(BR+1):
 *   000=÷2  001=÷4  010=÷8  011=÷16  100=÷32  101=÷64  110=÷128  111=÷256
 *
 * Note: SS is managed externally (ChipSelect<> in OneBus/spi.h).
 * SSM=1, SSI=1 here so the hardware master bit stays set without a
 * hardware NSS pin — same approach as the AVR SS-output trick.
 */

#pragma once
#include <cstdint>

namespace hw::stm32 {

  struct stm32_spi_regs {
    volatile uint32_t cr1;   // 0x00
    volatile uint32_t cr2;   // 0x04
    volatile uint32_t sr;    // 0x08
    volatile uint32_t dr;    // 0x0C
    volatile uint32_t crcpr; // 0x10
    volatile uint32_t rxcrc; // 0x14
    volatile uint32_t txcrc; // 0x18
    volatile uint32_t i2scfg;// 0x1C
    volatile uint32_t i2spr; // 0x20
  };

  // ============================================================
  // Pin-config policies
  // ============================================================

  // STM32F1 SPI1: PA5(SCK) / PA6(MISO) / PA7(MOSI)
  // SCK/MOSI: CNF=10 (AF push-pull), MODE=11 (50 MHz) → field 0xB
  // MISO:     CNF=01 (float input),  MODE=00           → field 0x4
  // APB2 clock, base 0x40013000
  struct Stm32F1_Spi1_PA5_PA6_PA7 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 2) | (1u << 12); // IOPAEN|SPI1EN APB2ENR
    }
    static void pin_config() {
      volatile uint32_t& crl = *reinterpret_cast<volatile uint32_t*>(0x40010800u); // GPIOA CRL
      crl = (crl & ~0xFFF00000u) | 0xB4B00000u;  // PA5=[23:20]=B, PA6=[27:24]=4, PA7=[31:28]=B
    }
  };

  // STM32F1 SPI2: PB13(SCK) / PB14(MISO) / PB15(MOSI)  APB1, base 0x40003800
  struct Stm32F1_Spi2_PB13_PB14_PB15 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 3);   // IOPBEN APB2ENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 14);  // SPI2EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(0x40010C04u); // GPIOB CRH
      crh = (crh & ~0xFFF00000u) | 0xB4B00000u;  // PB13=[23:20]=B, PB14=[27:24]=4, PB15=[31:28]=B
    }
  };

  // STM32F4 SPI1: PA5(SCK) / PA6(MISO) / PA7(MOSI)
  // AF5 for SPI1/2/3/4/5/6 on F4.  APB2, base 0x40013000
  struct Stm32F4_Spi1_PA5_PA6_PA7 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 0);   // GPIOAEN AHB1ENR
      *reinterpret_cast<volatile uint32_t*>(0x40023844u) |= (1u << 12);  // SPI1EN APB2ENR
    }
    static void pin_config() {
      volatile uint32_t& moder  = *reinterpret_cast<volatile uint32_t*>(0x40020000u); // GPIOA
      volatile uint32_t& ospeedr= *reinterpret_cast<volatile uint32_t*>(0x40020008u);
      volatile uint32_t& afrl   = *reinterpret_cast<volatile uint32_t*>(0x40020020u);
      moder   = (moder   & ~0xFC00u)   | 0xA800u;  // PA5/PA6/PA7 AF (10)
      ospeedr = (ospeedr & ~0xFC00u)   | 0xFC00u;  // VeryHigh (11) on all three
      afrl    = (afrl    & ~0xFFF00000u)| 0x55500000u; // PA5/6/7 = AF5
    }
  };

  // STM32F4 SPI2: PB13(SCK) / PB14(MISO) / PB15(MOSI)  APB1, base 0x40003800
  struct Stm32F4_Spi2_PB13_PB14_PB15 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 1);   // GPIOBEN AHB1ENR
      *reinterpret_cast<volatile uint32_t*>(0x40023840u) |= (1u << 14);  // SPI2EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& moder  = *reinterpret_cast<volatile uint32_t*>(0x40020400u); // GPIOB
      volatile uint32_t& ospeedr= *reinterpret_cast<volatile uint32_t*>(0x40020408u);
      volatile uint32_t& afrh   = *reinterpret_cast<volatile uint32_t*>(0x40020424u);
      moder   = (moder   & ~0xFC000000u) | 0xA8000000u;   // PB13/14/15 AF
      ospeedr = (ospeedr & ~0xFC000000u) | 0xFC000000u;   // VeryHigh
      afrh    = (afrh    & ~0xFFF00000u) | 0x55500000u;   // PB13/14/15 = AF5
    }
  };

  // STM32F0 SPI1: PA5(SCK) / PA6(MISO) / PA7(MOSI)
  // AF0 for SPI1 on F0 — NOT F4's AF5, verified against the real F030F4 (TSSOP20)
  // ST-published PeripheralPins.c (PinMap_SPI_SCLK/MISO/MOSI all list GPIO_AF0_SPI1).
  // APB2, base 0x40013000 (same IP base as F1/F4).
  struct Stm32F0_Spi1_PA5_PA6_PA7 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021014u) |= (1u << 17);  // GPIOAEN RCC_AHBENR
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 12);  // SPI1EN RCC_APB2ENR
    }
    static void pin_config() {
      volatile uint32_t& moder = *reinterpret_cast<volatile uint32_t*>(0x48000000u); // GPIOA MODER
      moder = (moder & ~0xFC00u) | 0xA800u;  // PA5/PA6/PA7 = AF (10)
      // AFRL: pin5=[23:20], pin6=[27:24], pin7=[31:28], all AF0 (=0) — no bits to set,
      // just clear any reset-default garbage in that field.
      volatile uint32_t& afrl = *reinterpret_cast<volatile uint32_t*>(0x48000020u); // GPIOA AFRL
      afrl &= ~0xFFF00000u;
    }
  };

  // ============================================================
  // Stm32SpiCore — hardware SPI master core.
  // BASE:   peripheral base address
  // PinCfg: clock_enable() + pin_config()
  // ApbHz:  APB clock feeding this SPI peripheral
  // Mode:   0-3 (CPOL[1]/CPHA[0])
  // MSBFirst: LSBFIRST bit (true = MSB first, LSBFIRST=0)
  // ============================================================
  template<uint32_t BASE, typename PinCfg, uint32_t ApbHz = 42000000UL,
           uint8_t Mode = 0, bool MSBFirst = true>
  struct Stm32SpiCore {
    template<typename O>
    struct Part : O {
      using Base = O;

      static stm32_spi_regs& regs() {
        return *reinterpret_cast<stm32_spi_regs*>(BASE);
      }

      // BR = ceil(log2(ApbHz/speed)) - 1, clamped to [0,7]
      static constexpr uint8_t _br(uint32_t speed) {
        uint8_t br = 0;
        uint32_t clk = ApbHz / 2u;
        while (clk > speed && br < 7u) { clk >>= 1; br++; }
        return br;
      }

      static void spi_init(uint32_t speed) {
        PinCfg::clock_enable();
        PinCfg::pin_config();

        uint32_t cr1 = (1u << 2)                    // MSTR
                     | (1u << 9) | (1u << 8)         // SSM=1, SSI=1 (software SS)
                     | (uint32_t(_br(speed)) << 3);   // BR[2:0]
        if (Mode & 2u) cr1 |= (1u << 1);             // CPOL
        if (Mode & 1u) cr1 |= (1u << 0);             // CPHA
        if (!MSBFirst)  cr1 |= (1u << 7);             // LSBFIRST

        regs().cr1 = cr1;
        regs().cr1 |= (1u << 6);  // SPE — enable
      }

      static uint8_t spi_transfer(uint8_t b) {
        while (!(regs().sr & (1u << 1)));   // wait TXE
        regs().dr = b;
        while (!(regs().sr & (1u << 0)));   // wait RXNE
        return uint8_t(regs().dr);
      }

      static void begin() {
        spi_init(1000000u);  // default 1 MHz; override via spi_init() before begin()
        Base::begin();
      }
    };
  };

} // hw::stm32
