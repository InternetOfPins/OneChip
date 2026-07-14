/**
 * @file stm32Uart.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief STM32 USART hardware core — register map + pin-config policies.
 *
 * IOP (bare-metal) only. In IOP mode the chip starts at 8MHz HSI by default
 * (no PLL). Pass CpuHz = 8000000UL unless you configure the PLL first.
 *
 * Pin-config policies encode the GPIO setup for a specific USART/pinout combo.
 * Serial aliases live in OneBus/uart.h to avoid circular deps (OneBus→OneChip).
 *
 * USART register layout (F1 and F4 share same functional offsets):
 *   0x00  SR   — status (TXE=7, TC=6, RXNE=5)
 *   0x04  DR   — data
 *   0x08  BRR  — baud: CpuHz / baud  (OVER8=0)
 *   0x0C  CR1  — UE=13, TE=3, RE=2
 *   0x10  CR2
 *   0x14  CR3
 *   0x18  GTPR
 */

#pragma once
#include <cstdint>
#include <hapi/hapi.h>
#include <oneBus/uart.h>

namespace hw::stm32 {

  struct stm32_usart_regs {
    volatile uint32_t sr;    // 0x00
    volatile uint32_t dr;    // 0x04
    volatile uint32_t brr;   // 0x08
    volatile uint32_t cr1;   // 0x0C
    volatile uint32_t cr2;   // 0x10
    volatile uint32_t cr3;   // 0x14
    volatile uint32_t gtpr;  // 0x18
  };

  // ============================================================
  // Pin-config policies — clock enable + GPIO setup per instance
  // ============================================================

  // STM32F1 ── CRL/CRH layout (4 bits per pin: CNF[1:0]:MODE[1:0])
  // TX → CNF=10 (AF push-pull), MODE=01 (10 MHz output) → field 0x9
  // RX → CNF=01 (floating input), MODE=00                → field 0x4

  // USART1: PA9 (TX) / PA10 (RX)   APB2, base 0x40013800
  struct Stm32F1_Usart1_PA9_PA10 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 14) | (1u << 2); // USART1EN|IOPAEN
    }
    static void pin_config() {
      volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(0x40010804u); // GPIOA CRH
      crh = (crh & ~0xFF0u) | 0x490u;  // PA9=[7:4]=9 (AF PP 10MHz), PA10=[11:8]=4 (float in)
    }
  };

  // USART2: PA2 (TX) / PA3 (RX)   APB1, base 0x40004400
  struct Stm32F1_Usart2_PA2_PA3 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 2);   // IOPAEN APB2ENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 17);  // USART2EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& crl = *reinterpret_cast<volatile uint32_t*>(0x40010800u); // GPIOA CRL
      crl = (crl & ~0xFF00u) | 0x4900u; // PA2=[11:8]=9, PA3=[15:12]=4
    }
  };

  // USART3: PB10 (TX) / PB11 (RX)   APB1, base 0x40004800
  struct Stm32F1_Usart3_PB10_PB11 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 3);   // IOPBEN APB2ENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 18);  // USART3EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(0x40010C04u); // GPIOB CRH
      crh = (crh & ~0xFF00u) | 0x4900u; // PB10=[11:8]=9, PB11=[15:12]=4
    }
  };

  // STM32F0 ── MODER (2 bits: AF=10) + AFRH/AFRL (4 bits: AF1 for USART1 — NOT F4's AF7;
  // verified against RM0360/datasheet, F0's AF numbering for USART1 on PA9/PA10 differs from
  // F4's). Clock-enable also splits across two different registers, unlike F1 (single APB2ENR)
  // or F4 (AHB1ENR+APB2ENR pair with different bit meanings): F0's GPIO clock lives in AHBENR
  // (not APB2ENR — see stm32Port.h's f0 namespace), while USART1's own enable bit happens to
  // share F1's exact APB2ENR address and bit position.

  // USART1: PA9 (TX) / PA10 (RX)   APB2, base 0x40013800 (same as F1 — shared IP block)
  struct Stm32F0_Usart1_PA9_PA10 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021014u) |= (1u << 17); // GPIOAEN, RCC_AHBENR
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 14); // USART1EN, RCC_APB2ENR
    }
    static void pin_config() {
      volatile uint32_t& moder = *reinterpret_cast<volatile uint32_t*>(0x48000000u); // GPIOA MODER
      moder = (moder & ~0x3C0000u) | 0x280000u; // PA9=[19:18]=10, PA10=[21:20]=10 (AF)
      volatile uint32_t& afrh  = *reinterpret_cast<volatile uint32_t*>(0x48000024u); // GPIOA AFRH
      afrh  = (afrh  & ~0xFF0u) | 0x110u;       // PA9=[7:4]=1 (AF1), PA10=[11:8]=1 (AF1)
    }
  };

  // STM32F4 ── MODER (2 bits: AF=10) + AFRH/AFRL (4 bits: AF7 for USART)

  // USART1: PA9 (TX) / PA10 (RX)   APB2, base 0x40011000
  struct Stm32F4_Usart1_PA9_PA10 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 0);  // GPIOAEN AHB1ENR
      *reinterpret_cast<volatile uint32_t*>(0x40023844u) |= (1u << 4);  // USART1EN APB2ENR
    }
    static void pin_config() {
      volatile uint32_t& moder = *reinterpret_cast<volatile uint32_t*>(0x40020000u);
      moder = (moder & ~0x3C0000u) | 0x280000u; // PA9=[19:18]=10, PA10=[21:20]=10 (AF)
      volatile uint32_t& afrh = *reinterpret_cast<volatile uint32_t*>(0x40020024u);
      afrh  = (afrh  & ~0xFF0u)    | 0x770u;    // PA9=[7:4]=7,   PA10=[11:8]=7   (AF7)
    }
  };

  // USART2: PA2 (TX) / PA3 (RX)   APB1, base 0x40004400
  struct Stm32F4_Usart2_PA2_PA3 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 0);   // GPIOAEN AHB1ENR
      *reinterpret_cast<volatile uint32_t*>(0x40023840u) |= (1u << 17);  // USART2EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& moder = *reinterpret_cast<volatile uint32_t*>(0x40020000u);
      moder = (moder & ~0xF0u) | 0xA0u;   // PA2=[5:4]=10, PA3=[7:6]=10 (AF)
      volatile uint32_t& afrl  = *reinterpret_cast<volatile uint32_t*>(0x40020020u);
      afrl  = (afrl  & ~0xFF00u) | 0x7700u; // PA2=[11:8]=7, PA3=[15:12]=7 (AF7)
    }
  };

  // ============================================================
  // Generic USART core — hardware register access + init
  // PinCfg: provides clock_enable() and pin_config()
  // CpuHz: APB clock fed to USART (=CPU clock at reset, 8MHz HSI on F1)
  // ============================================================
  template<uint32_t BASE, typename PinCfg, uint32_t CpuHz = 8000000UL>
  struct Stm32UsartCore {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static stm32_usart_regs& regs() {
        return *reinterpret_cast<stm32_usart_regs*>(BASE);
      }

      static bool available() { return regs().sr & (1u << 5); }  // RXNE

      static void putch(uint8_t c) {
        while (!(regs().sr & (1u << 7)));  // wait TXE
        regs().dr = c;
      }

      static uint8_t getch() {
        while (!(regs().sr & (1u << 5)));  // wait RXNE
        return regs().dr;
      }

      // baud is compile-time constant from Uart<BaudRate>::begin()
      static void uart_init(uint32_t baud) {
        PinCfg::clock_enable();
        PinCfg::pin_config();
        regs().brr = CpuHz / baud;
        regs().cr1 = (1u << 13) | (1u << 3) | (1u << 2);  // UE | TE | RE
      }

      static void begin() { Base::begin(); }
    };
  };

  // ============================================================
  // F0/F3/L0/L4/G0 "USART_V2" register map — NOT layout-compatible with the
  // classic F1/F4 "USART_V1" map above (offsets differ entirely: CR1 is at
  // 0x00 not SR, BRR at 0x0C not 0x08; UE moved from CR1 bit13 to bit0; TXE/
  // TC/RXNE moved from SR into ISR but kept the same bit positions [7:6:5];
  // TDR/RDR replace the single shared DR). Verified against ST's CMSIS
  // stm32f030x6.h USART_TypeDef — using the V1 struct here silently writes
  // UE into what's actually BRR and never enables the peripheral, hanging
  // putch() forever waiting on a TXE bit that's actually reading CR1.
  // ============================================================
  struct stm32_usart_v2_regs {
    volatile uint32_t cr1;   // 0x00
    volatile uint32_t cr2;   // 0x04
    volatile uint32_t cr3;   // 0x08
    volatile uint32_t brr;   // 0x0C
    volatile uint32_t gtpr;  // 0x10
    volatile uint32_t rtor;  // 0x14
    volatile uint32_t rqr;   // 0x18
    volatile uint32_t isr;   // 0x1C
    volatile uint32_t icr;   // 0x20
    volatile uint16_t rdr;   // 0x24
    uint16_t _res1;
    volatile uint16_t tdr;   // 0x28
    uint16_t _res2;
  };

  template<uint32_t BASE, typename PinCfg, uint32_t CpuHz = 8000000UL>
  struct Stm32UsartV2Core {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static stm32_usart_v2_regs& regs() {
        return *reinterpret_cast<stm32_usart_v2_regs*>(BASE);
      }

      static bool available() { return regs().isr & (1u << 5); }  // RXNE

      static void putch(uint8_t c) {
        while (!(regs().isr & (1u << 7)));  // wait TXE
        regs().tdr = c;
      }

      static uint8_t getch() {
        while (!(regs().isr & (1u << 5)));  // wait RXNE
        return uint8_t(regs().rdr);
      }

      static void uart_init(uint32_t baud) {
        PinCfg::clock_enable();
        PinCfg::pin_config();
        regs().brr = CpuHz / baud;
        regs().cr1 = (1u << 3) | (1u << 2) | (1u << 0);  // TE | RE | UE
      }

      static void begin() { Base::begin(); }
    };
  };

  // ============================================================
  // Board-specific namespace aliases for oneBus UART API
  // ============================================================

  namespace f1 {
    template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40013800u, Stm32F1_Usart1_PA9_PA10, CpuHz>>;
    template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
    using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004400u, Stm32F1_Usart2_PA2_PA3, CpuHz>>;
    template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
    using Serial2 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004800u, Stm32F1_Usart3_PB10_PB11, CpuHz>>;
  }

  namespace f4 {
    template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40011000u, Stm32F4_Usart1_PA9_PA10, CpuHz>>;
    template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
    using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartCore<0x40004400u, Stm32F4_Usart2_PA2_PA3, CpuHz>>;
  }

  namespace f0 {
    // Default CpuHz=8MHz — HSI reset default (F0 has no PLL configured by f0::SysClk yet).
    // Uses Stm32UsartV2Core, NOT Stm32UsartCore — F0's USART is a different peripheral
    // design ("USART_V2") with a different register map than F1/F4.
    template<uint32_t BaudRate, uint32_t CpuHz = 8000000UL>
    using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                Stm32UsartV2Core<0x40013800u, Stm32F0_Usart1_PA9_PA10, CpuHz>>;
  }

} // hw::stm32
