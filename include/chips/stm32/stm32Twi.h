/**
 * @file stm32Twi.h
 * @brief STM32 I2C hardware core — register map + pin-config policies.
 *
 * STM32 I2C has a hardware state machine that requires ACK/STOP bits to be
 * managed at precise moments (especially for single-byte reads). Unlike AVR
 * bit-bang primitives, Stm32I2cCore owns the full protocol rather than
 * splitting into twi_* primitives + I2cMaster<> protocol layer.
 * Provides the same user API: begin_write/write_byte/end_write/send/
 * request_from/read_byte.
 *
 * I2C register layout (F1 and F4 share identical offsets):
 *   0x00  CR1   — PE=0, START=8, STOP=9, ACK=10, SWRST=15
 *   0x04  CR2   — FREQ[5:0] (peripheral clock in MHz)
 *   0x08  OAR1
 *   0x0C  OAR2
 *   0x10  DR    — data register
 *   0x14  SR1   — SB=0, ADDR=1, BTF=2, RxNE=6, TxE=7, BERR=8, AF=10
 *   0x18  SR2   — MSL=0, BUSY=1 (read clears ADDR when combined with SR1 read)
 *   0x1C  CCR   — CCR[11:0], FS=15 (1=fast mode 400kHz)
 *   0x20  TRISE
 */

#pragma once
#include <cstdint>
#include <hapi/hapi.h>
#include <oneBus/i2c.h>

namespace hw::stm32 {

  struct stm32_i2c_regs {
    volatile uint32_t cr1;    // 0x00
    volatile uint32_t cr2;    // 0x04
    volatile uint32_t oar1;   // 0x08
    volatile uint32_t oar2;   // 0x0C
    volatile uint32_t dr;     // 0x10
    volatile uint32_t sr1;    // 0x14
    volatile uint32_t sr2;    // 0x18
    volatile uint32_t ccr;    // 0x1C
    volatile uint32_t trise;  // 0x20
  };

  // ============================================================
  // Pin-config policies — clock enable + GPIO setup per instance
  // ============================================================

  // STM32F1 ── AF open-drain: CNF=11, MODE=01 (10 MHz) → field 0xD

  // I2C1: PB6(SCL) / PB7(SDA)  APB1, base 0x40005400
  struct Stm32F1_I2c1_PB6_PB7 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 3);   // IOPBEN APB2ENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 21);  // I2C1EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& crl = *reinterpret_cast<volatile uint32_t*>(0x40010C00u); // GPIOB CRL
      crl = (crl & ~0xFF000000u) | 0xDD000000u;  // PB6=[27:24]=D, PB7=[31:28]=D (AF OD)
    }
  };

  // I2C2: PB10(SCL) / PB11(SDA)  APB1, base 0x40005800
  struct Stm32F1_I2c2_PB10_PB11 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021018u) |= (1u << 3);   // IOPBEN APB2ENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 22);  // I2C2EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& crh = *reinterpret_cast<volatile uint32_t*>(0x40010C04u); // GPIOB CRH
      crh = (crh & ~0x0000FF00u) | 0x0000DD00u;  // PB10=[11:8]=D, PB11=[15:12]=D
    }
  };

  // STM32F4 ── AF4 open-drain with pull-up

  // I2C1: PB6(SCL) / PB7(SDA)  APB1, base 0x40005400
  struct Stm32F4_I2c1_PB6_PB7 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 1);   // GPIOBEN AHB1ENR
      *reinterpret_cast<volatile uint32_t*>(0x40023840u) |= (1u << 21);  // I2C1EN APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& moder  = *reinterpret_cast<volatile uint32_t*>(0x40020400u); // GPIOB
      volatile uint32_t& otyper = *reinterpret_cast<volatile uint32_t*>(0x40020404u);
      volatile uint32_t& pupdr  = *reinterpret_cast<volatile uint32_t*>(0x4002040Cu);
      volatile uint32_t& afrl   = *reinterpret_cast<volatile uint32_t*>(0x40020420u);
      moder  = (moder  & ~0xF000u)    | 0xA000u;    // PB6=[13:12]=10, PB7=[15:14]=10 (AF)
      otyper |= (1u<<6)|(1u<<7);                     // open-drain
      pupdr  = (pupdr  & ~0xF000u)    | 0x5000u;    // pull-up on PB6/PB7
      afrl   = (afrl   & ~0xFF000000u)| 0x44000000u; // PB6=[27:24]=4, PB7=[31:28]=4 (AF4)
    }
  };

  // I2C1: PB8(SCL) / PB9(SDA) alternate pinout  APB1, base 0x40005400
  struct Stm32F4_I2c1_PB8_PB9 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 1);
      *reinterpret_cast<volatile uint32_t*>(0x40023840u) |= (1u << 21);
    }
    static void pin_config() {
      volatile uint32_t& moder  = *reinterpret_cast<volatile uint32_t*>(0x40020400u);
      volatile uint32_t& otyper = *reinterpret_cast<volatile uint32_t*>(0x40020404u);
      volatile uint32_t& pupdr  = *reinterpret_cast<volatile uint32_t*>(0x4002040Cu);
      volatile uint32_t& afrh   = *reinterpret_cast<volatile uint32_t*>(0x40020424u);
      moder  = (moder  & ~0xF0000u)    | 0xA0000u;   // PB8/PB9 AF
      otyper |= (1u<<8)|(1u<<9);
      pupdr  = (pupdr  & ~0xF0000u)    | 0x50000u;
      afrh   = (afrh   & ~0xFFu)       | 0x44u;      // PB8=[3:0]=4, PB9=[7:4]=4
    }
  };

  // I2C2: PB10(SCL) / PB11(SDA)  APB1, base 0x40005800
  struct Stm32F4_I2c2_PB10_PB11 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40023830u) |= (1u << 1);
      *reinterpret_cast<volatile uint32_t*>(0x40023840u) |= (1u << 22);  // I2C2EN
    }
    static void pin_config() {
      volatile uint32_t& moder  = *reinterpret_cast<volatile uint32_t*>(0x40020400u);
      volatile uint32_t& otyper = *reinterpret_cast<volatile uint32_t*>(0x40020404u);
      volatile uint32_t& pupdr  = *reinterpret_cast<volatile uint32_t*>(0x4002040Cu);
      volatile uint32_t& afrh   = *reinterpret_cast<volatile uint32_t*>(0x40020424u);
      moder  = (moder  & ~0xF00000u)   | 0xA00000u;
      otyper |= (1u<<10)|(1u<<11);
      pupdr  = (pupdr  & ~0xF00000u)   | 0x500000u;
      afrh   = (afrh   & ~0xFF00u)     | 0x4400u;    // PB10=[11:8]=4, PB11=[15:12]=4
    }
  };

  // ============================================================
  // Stm32I2cCore — owns the full master protocol.
  // STM32 I2C requires ACK/STOP at precise moments (especially
  // single-byte reads), so this core provides begin_write/write_byte/
  // end_write/send/request_from/read_byte directly rather than
  // splitting into raw twi_* primitives.
  //
  // BASE:  peripheral base address
  // PinCfg: clock_enable() + pin_config()
  // ApbHz:  APB clock in Hz (APB1 for I2C; 8 MHz HSI on F1 at reset,
  //         36 MHz APB1 at 72 MHz PLL; 42 MHz on F4 at 168 MHz)
  // Freq:   SCL frequency in Hz (default 100 kHz standard mode)
  // ============================================================
  template<uint32_t BASE, typename PinCfg, uint32_t ApbHz = 36000000UL,
           uint32_t Freq = 100000UL>
  struct Stm32I2cCore {
    template<typename O>
    struct Part : O {
      using Base = O;

      static stm32_i2c_regs& regs() {
        return *reinterpret_cast<stm32_i2c_regs*>(BASE);
      }

      // twi_init — called by begin() below, exposed for TwiAPI contract
      static void twi_init(uint32_t freq) {
        PinCfg::clock_enable();
        PinCfg::pin_config();

        regs().cr1 |= (1u << 15);  // SWRST — reset peripheral
        regs().cr1 &= ~(1u << 15);

        const uint32_t apb_mhz = ApbHz / 1000000u;
        regs().cr2 = apb_mhz & 0x3Fu;

        if (freq <= 100000u) {
          // Standard mode: CCR = ApbHz / (2 × freq)
          uint32_t ccr = ApbHz / (2u * freq);
          if (ccr < 4u) ccr = 4u;
          regs().ccr   = ccr;
          regs().trise = apb_mhz + 1u;  // Trise = 1000ns / (1/ApbHz) + 1
        } else {
          // Fast mode (400 kHz), duty=0: CCR = ApbHz / (3 × freq)
          uint32_t ccr = ApbHz / (3u * freq);
          if (ccr < 1u) ccr = 1u;
          regs().ccr   = (1u << 15) | ccr;  // FS=1
          regs().trise = apb_mhz * 3u / 10u + 1u;  // Trise = 300ns
        }

        regs().cr1 |= (1u << 0);  // PE — enable
      }

      static void begin() {
        twi_init(Freq);
        Base::begin();
      }

      // ── Write streaming ──────────────────────────────────────────────
      static void begin_write(uint8_t addr) {
        regs().cr1 |= (1u << 8);                    // START
        while (!(regs().sr1 & (1u << 0)));           // wait SB
        regs().dr = addr << 1;                        // SLA+W
        while (!(regs().sr1 & (1u << 1)));           // wait ADDR
        (void)regs().sr1; (void)regs().sr2;          // clear ADDR
      }

      static void write_byte(uint8_t b) {
        while (!(regs().sr1 & (1u << 7)));           // wait TxE
        regs().dr = b;
      }

      static void end_write() {
        while (!(regs().sr1 & (1u << 2)));           // wait BTF
        regs().cr1 |= (1u << 9);                    // STOP
      }

      static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
        begin_write(addr);
        while (len--) write_byte(*data++);
        end_write();
      }

      // ── Read streaming ───────────────────────────────────────────────
      // STM32 ACK/STOP timing rules:
      //   n==1: clear ACK before clearing ADDR, then set STOP before reading DR
      //   n==2: POS+ACK before start, clear ACK after ADDR, after BTF: STOP+read both
      //   n>=3: clear ACK when (n-2) bytes remain, then follow n==2 tail
      // We implement a robust approach for n>=1 using an internal state tracker.
      inline static uint8_t _rcount = 0;

      static uint8_t request_from(uint8_t addr, uint8_t n) {
        _rcount = n;
        regs().cr1 |= (1u << 10) | (1u << 8);       // ACK=1, START
        while (!(regs().sr1 & (1u << 0)));            // wait SB
        regs().dr = uint8_t((addr << 1) | 1u);       // SLA+R
        while (!(regs().sr1 & (1u << 1)));            // wait ADDR
        if (n == 1u) {
          regs().cr1 &= ~(1u << 10);                 // clear ACK before SR2 read
          (void)regs().sr1; (void)regs().sr2;         // clear ADDR
          regs().cr1 |= (1u << 9);                   // STOP must be set before reading DR
        } else {
          (void)regs().sr1; (void)regs().sr2;         // clear ADDR
        }
        return n;
      }

      static uint8_t read_byte() {
        if (_rcount == 1u) {
          // STOP already set in request_from (n==1) or by previous read_byte (n>1 last)
          while (!(regs().sr1 & (1u << 6)));          // wait RxNE
          _rcount = 0;
          return uint8_t(regs().dr);
        }
        if (_rcount == 2u) {
          while (!(regs().sr1 & (1u << 2)));          // wait BTF (both shift+DR ready)
          regs().cr1 |= (1u << 9);                   // STOP
          uint8_t b = uint8_t(regs().dr);
          _rcount--;
          return b;
        }
        // n >= 3: normal ACK read
        if (_rcount == 3u) {
          while (!(regs().sr1 & (1u << 2)));          // wait BTF
          regs().cr1 &= ~(1u << 10);                 // clear ACK (NACK next)
        } else {
          while (!(regs().sr1 & (1u << 6)));          // wait RxNE
        }
        uint8_t b = uint8_t(regs().dr);
        _rcount--;
        return b;
      }
    };
  };

  // ============================================================
  // STM32F0 I2C1: PA9(SCL) / PA10(SDA), AF4, open-drain + pull-up.
  // Shares physical pins with USART1 TX/RX (Stm32F0_Usart1_PA9_PA10) — this
  // package (TSSOP20) doesn't expose I2C1's default PB6/PB7 pins at all, and
  // PA9/PA10 is the only alternate mapping available, verified against the
  // real ST-published PeripheralPins.c for this exact board (F030F4P variant,
  // stm32duino/Arduino_Core_STM32). Use one peripheral or the other on these
  // pins, not both simultaneously.
  // APB1, base 0x40005400 (same IP base as F1/F4).
  // ============================================================
  struct Stm32F0_I2c1_PA9_PA10 {
    static void clock_enable() {
      *reinterpret_cast<volatile uint32_t*>(0x40021014u) |= (1u << 17);   // GPIOAEN RCC_AHBENR
      *reinterpret_cast<volatile uint32_t*>(0x4002101Cu) |= (1u << 21);   // I2C1EN RCC_APB1ENR
    }
    static void pin_config() {
      volatile uint32_t& moder = *reinterpret_cast<volatile uint32_t*>(0x48000000u); // GPIOA MODER
      moder = (moder & ~0x3C0000u) | 0x280000u;  // PA9/PA10 = AF (10)
      volatile uint32_t& otyper = *reinterpret_cast<volatile uint32_t*>(0x48000004u); // GPIOA OTYPER
      otyper |= (1u << 9) | (1u << 10);           // open-drain (required for I2C)
      volatile uint32_t& pupdr = *reinterpret_cast<volatile uint32_t*>(0x4800000Cu);  // GPIOA PUPDR
      pupdr = (pupdr & ~0x3C0000u) | 0x140000u;   // pull-up (01) on PA9/PA10
      volatile uint32_t& afrh = *reinterpret_cast<volatile uint32_t*>(0x48000024u);   // GPIOA AFRH
      afrh = (afrh & ~0xFF0u) | 0x440u;           // PA9=[7:4]=4, PA10=[11:8]=4 (AF4)
    }
  };

  // ============================================================
  // F0/F3/L0/L4/G0 "I2C_V2" register map — a different peripheral design from
  // F1/F4's "I2C_V1" above, not just relabeled offsets: transactions are driven
  // by CR2 (SADD/START/STOP/NBYTES/RELOAD/AUTOEND) instead of manually walking
  // SB/ADDR/BTF, and status moves into ISR (TXIS/RXNE/TC/TCR). Verified against
  // ST's CMSIS stm32f030x6.h I2C_TypeDef (CR1@0x00,CR2@0x04,OAR1@0x08,OAR2@0x0C,
  // TIMINGR@0x10,...,ISR@0x18,ICR@0x1C,RXDR@0x24,TXDR@0x28) and the CR2/ISR bit
  // definitions therein — same lesson as stm32Uart.h's Stm32UsartV2Core: don't
  // assume a peripheral's register map carries over across STM32 families just
  // because the name (I2C, USART, ...) is the same.
  // ============================================================
  struct stm32_i2c_v2_regs {
    volatile uint32_t cr1;      // 0x00
    volatile uint32_t cr2;      // 0x04
    volatile uint32_t oar1;     // 0x08
    volatile uint32_t oar2;     // 0x0C
    volatile uint32_t timingr;  // 0x10
    volatile uint32_t timeoutr; // 0x14
    volatile uint32_t isr;      // 0x18
    volatile uint32_t icr;      // 0x1C
    volatile uint32_t pecr;     // 0x20
    volatile uint16_t rxdr;     // 0x24
    uint16_t _res1;
    volatile uint16_t txdr;     // 0x28
    uint16_t _res2;
  };

  // TIMINGR is not a simple clock divider (unlike V1's CCR) — it encodes
  // PRESC/SCLDEL/SDADEL/SCLH/SCLL, whose correct values depend on bus rise/
  // fall time and are normally generated by ST's CubeMX/AN4235 tool, not
  // computed from a formula. Only pre-verified (ApbHz, Freq) pairs are
  // supported; anything else is a compile error rather than a silently-wrong
  // guess. Sources: 0x00201D2B is ST's own widely-shipped "I2C_TIMING_100KHZ,
  // HSI 8MHz" constant (stm32duino Arduino_Core_STM32, ST HAL examples);
  // 0x0070D8FF is AN4235's published 48MHz/Sm100kHz table entry.
  template<uint32_t ApbHz, uint32_t Freq>
  constexpr uint32_t stm32_i2c_v2_timing() {
    static_assert((ApbHz == 8000000UL || ApbHz == 48000000UL) && Freq == 100000UL,
      "Stm32I2cV2Core: no verified TIMINGR constant for this (ApbHz,Freq) pair — "
      "generate one with STM32CubeMX's I2C timing tool (AN4235) and add it here.");
    if constexpr (ApbHz == 8000000UL)  return 0x00201D2Bu;
    else /* 48000000UL */              return 0x0070D8FFu;
  }

  template<uint32_t BASE, typename PinCfg, uint32_t ApbHz = 8000000UL,
           uint32_t Freq = 100000UL>
  struct Stm32I2cV2Core {
    template<typename O>
    struct Part : O {
      using Base = O;

      static stm32_i2c_v2_regs& regs() {
        return *reinterpret_cast<stm32_i2c_v2_regs*>(BASE);
      }

      static void twi_init(uint32_t /*freq — baked into TIMINGR at compile time*/) {
        PinCfg::clock_enable();
        PinCfg::pin_config();
        regs().timingr = stm32_i2c_v2_timing<ApbHz, Freq>();
        regs().cr1 = (1u << 0);  // PE — enable
      }

      static void begin() {
        twi_init(Freq);
        Base::begin();
      }

      // ── Write streaming ──────────────────────────────────────────────
      // NBYTES must be known at START, but this API streams bytes one at a
      // time without a declared total, so each byte is its own RELOAD-mode
      // sub-transfer: NBYTES=1, wait TCR between bytes to re-arm the next one,
      // and finalize with STOP in end_write() (see RM0360 I2C reload mode).
      inline static bool _first = true;

      static void begin_write(uint8_t addr) {
        _first = true;
        regs().cr2 = (uint32_t(addr) << 1) | (1u << 16) /*NBYTES=1*/ | (1u << 24) /*RELOAD*/
                   | (1u << 13) /*START*/;
      }

      static void write_byte(uint8_t b) {
        if (!_first) {
          while (!(regs().isr & (1u << 7)));  // wait TCR
          regs().cr2 = (regs().cr2 & ~0x00FF0000u) | (1u << 16);  // re-arm NBYTES=1
        }
        _first = false;
        while (!(regs().isr & (1u << 1)));    // wait TXIS
        regs().txdr = b;
      }

      static void end_write() {
        while (!(regs().isr & (1u << 7)));    // wait TCR (last byte accepted)
        regs().cr2 |= (1u << 14);             // STOP
        while (!(regs().isr & (1u << 5)));    // wait STOPF
        regs().icr = (1u << 5);               // clear STOPF
      }

      static void send(uint8_t addr, const uint8_t* data, uint8_t len) {
        begin_write(addr);
        while (len--) write_byte(*data++);
        end_write();
      }

      // ── Read streaming ───────────────────────────────────────────────
      // AUTOEND handles STOP automatically once n bytes are read — simpler
      // than V1 since the byte count is known upfront here.
      static uint8_t request_from(uint8_t addr, uint8_t n) {
        regs().cr2 = (uint32_t(addr) << 1) | (1u << 10) /*RD_WRN*/
                   | (uint32_t(n) << 16) /*NBYTES*/ | (1u << 25) /*AUTOEND*/
                   | (1u << 13) /*START*/;
        return n;
      }

      static uint8_t read_byte() {
        while (!(regs().isr & (1u << 2)));    // wait RXNE
        return uint8_t(regs().rxdr);
      }
    };
  };

  // ============================================================
  // Board-specific namespace aliases for oneBus I2C API
  // ============================================================

  namespace f1 {
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 36000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F1_I2c1_PB6_PB7, ApbHz, SclHz>>;
  }

  namespace f4 {
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 42000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cCore<0x40005400u, Stm32F4_I2c1_PB6_PB7, ApbHz, SclHz>>;
  }

  namespace f0 {
    // Stm32I2cV2Core, NOT Stm32I2cCore — F0's I2C is a different peripheral
    // design ("I2C_V2") from F1/F4's. ApbHz default 8MHz matches f0::SysClk's
    // HSI-reset default; only (8MHz,100kHz) and (48MHz,100kHz) have verified
    // TIMINGR constants — anything else is a compile error by design.
    template<uint32_t SclHz = 100000UL, uint32_t ApbHz = 8000000UL>
    using Twi = hapi::APIOf<oneBus::TwiAPI,
                            Stm32I2cV2Core<0x40005400u, Stm32F0_I2c1_PA9_PA10, ApbHz, SclHz>>;
  }

} // hw::stm32
