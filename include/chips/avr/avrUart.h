/**
 * @file avrUart.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief AVR USART hardware core — register map only, no protocol layer.
 *
 * ATmega328/2560/1284 register layout (identical across all USARTs):
 *   BASE+0  UCSRnA  (RXCn=7, TXCn=6, UDREn=5, ...)
 *   BASE+1  UCSRnB  (RXENn=4, TXENn=3, ...)
 *   BASE+2  UCSRnC  (UCSZn1=2, UCSZn0=1 → 8-bit frame)
 *   BASE+3  (reserved)
 *   BASE+4  UBRRnL
 *   BASE+5  UBRRnH
 *   BASE+6  UDRn
 *
 * Serial0<>/Serial1<> aliases live in OneBus/uart.h — it wires
 * AvrUsartCore<> with the Uart<BaudRate> protocol component.
 */

#pragma once
#include <stdint.h>

namespace hw::avr {

  struct avr_usart_regs {
    volatile uint8_t ucsra;
    volatile uint8_t ucsrb;
    volatile uint8_t ucsrc;
    volatile uint8_t _pad;
    volatile uint8_t ubrrl;
    volatile uint8_t ubrrh;
    volatile uint8_t udr;
  };

  // Hardware core — maps USART registers at BASE, computes UBRR at compile time.
  template<uint16_t BASE, uint32_t CpuHz = 16000000UL>
  struct AvrUsartCore {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static avr_usart_regs& regs() {
        return *reinterpret_cast<avr_usart_regs*>(BASE);
      }

      static bool uart_available() { return regs().ucsra & (1u << 7); }  // RXCn

      static void uart_putch(uint8_t c) {
        while (!(regs().ucsra & (1u << 5)));  // wait UDREn
        regs().udr = c;
      }

      static uint8_t uart_getch() {
        while (!(regs().ucsra & (1u << 7)));  // wait RXCn
        return regs().udr;
      }

      // baud is a compile-time constant when called from Uart<BaudRate>::begin()
      static void uart_init(uint32_t baud) {
        const uint16_t ubrr = CpuHz / 16 / baud - 1;
        regs().ubrrh = ubrr >> 8;
        regs().ubrrl = ubrr;
        regs().ucsrb = (1u << 4) | (1u << 3);  // RXENn | TXENn
        regs().ucsrc = (1u << 2) | (1u << 1);  // UCSZn1:0 = 11 → 8-bit
      }

      static void begin() { Base::begin(); }
    };
  };

} // hw::avr
